// Copyright (C) 2019 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// File-backed log of protos with append-only writes and position based reads.
//
// There should only be one instance of a FileBackedProtoLog of the same file at
// a time; using multiple instances at the same time may lead to undefined
// behavior.
//
// The entire checksum is computed on initialization to verify the contents are
// valid. On failure, the log will be truncated to the last verified state when
// PersistToDisk() was called. If the log cannot successfully restore the last
// state due to disk corruption or some other inconsistency, then the entire log
// will be lost.
//
// Each proto written to the file will have a metadata written just before it.
// The metadata consists of
//   {
//     1 bytes of kProtoMagic;
//     3 bytes of the proto size
//     n bytes of the proto itself
//   }
//
// Example usage:
//   ICING_ASSERT_OK_AND_ASSIGN(auto create_result,
//       FileBackedProtoLog<DocumentProto>::Create(filesystem, file_path_,
//                                                  options));
//   auto proto_log = create_result.proto_log;
//
//   Document document;
//   document.set_namespace("com.google.android.example");
//   document.set_uri("www.google.com");
//
//   int64_t document_offset = proto_log->WriteProto(document));
//   Document same_document = proto_log->ReadProto(document_offset));
//   proto_log->PersistToDisk();
//
// TODO(b/136514769): Add versioning to the header and a UpgradeToVersion
// migration method.

#ifndef ICING_FILE_FILE_BACKED_PROTO_LOG_H_
#define ICING_FILE_FILE_BACKED_PROTO_LOG_H_

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "icing/text_classifier/lib3/utils/base/status.h"
#include "icing/text_classifier/lib3/utils/base/statusor.h"
#include <google/protobuf/io/gzip_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl_lite.h>
#include "icing/absl_ports/canonical_errors.h"
#include "icing/absl_ports/str_cat.h"
#include "icing/file/filesystem.h"
#include "icing/file/memory-mapped-file.h"
#include "icing/legacy/core/icing-string-util.h"
#include "icing/portable/platform.h"
#include "icing/portable/zlib.h"
#include "icing/util/crc32.h"
#include "icing/util/data-loss.h"
#include "icing/util/logging.h"
#include "icing/util/status-macros.h"

namespace icing {
namespace lib {

template <typename ProtoT>
class FileBackedProtoLog {
 public:
  struct Options {
    // Whether to compress each proto before writing to the proto log.
    bool compress;

    // Byte-size limit for each proto written to the store. This does not
    // include the bytes needed for the metadata of each proto.
    //
    // NOTE: Currently, we only support protos up to 16MiB. We store the proto
    // size in 3 bytes within the metadata.
    //
    // NOTE: This limit is only enforced for future writes. If the store
    // previously had a higher limit, then reading older entries could return
    // larger protos.
    //
    // NOTE: The max_proto_size is the upper limit for input protos into the
    // ProtoLog. Even if the proto is larger than max_proto_size, but compresses
    // to a smaller size, ProtoLog will not accept it. Protos that result in a
    // compressed size larger than max_proto_size are also not accepted.
    const int32_t max_proto_size;

    // Must specify values for options.
    Options() = delete;
    explicit Options(bool compress_in,
                     const int32_t max_proto_size_in = kMaxProtoSize)
        : compress(compress_in), max_proto_size(max_proto_size_in) {}
  };

  // Header stored at the beginning of the file before the rest of the log
  // contents. Stores metadata on the log.
  //
  // TODO(b/139375388): Migrate the Header struct to a proto. This makes
  // migrations easier since we don't need to worry about different size padding
  // (which would affect the checksum) and different endians.
  struct Header {
    static constexpr int32_t kMagic = 0xf4c6f67a;

    // Holds the magic as a quick sanity check against file corruption.
    int32_t magic = kMagic;

    // Whether to compress the protos before writing to the log.
    bool compress = true;

    // The maximum proto size that can be written to the log.
    int32_t max_proto_size = 0;

    // Checksum of the log elements, doesn't include the header fields.
    uint32_t log_checksum = 0;

    // Last known good offset at which the log and its checksum were updated.
    // If we crash between writing to the log and updating the checksum, we can
    // try to rewind the log to this offset and verify the checksum is still
    // valid instead of throwing away the entire log.
    int64_t rewind_offset = sizeof(Header);

    // Must be at the end. Contains the crc checksum of the preceding fields.
    uint32_t header_checksum = 0;

    uint32_t CalculateHeaderChecksum() const {
      Crc32 crc;
      std::string_view header_str(reinterpret_cast<const char*>(this),
                                  offsetof(Header, header_checksum));
      crc.Append(header_str);
      return crc.Get();
    }
  };

  struct CreateResult {
    // A successfully initialized log.
    std::unique_ptr<FileBackedProtoLog<ProtoT>> proto_log;

    // The data status after initializing from a previous state. Data loss can
    // happen if the file is corrupted or some previously added data was
    // unpersisted. This may be used to signal that any derived data off of the
    // proto log may need to be regenerated.
    DataLoss data_loss;

    bool has_data_loss() {
      return data_loss == DataLoss::PARTIAL || data_loss == DataLoss::COMPLETE;
    }
  };

  // Factory method to create, initialize, and return a FileBackedProtoLog. Will
  // create the file if it doesn't exist.
  //
  // If on re-initialization the log detects disk corruption or some previously
  // added data was unpersisted, the log will rewind to the last-good state. The
  // log saves these checkpointed "good" states when PersistToDisk() is called
  // or the log is safely destructed. If the log rewinds successfully to the
  // last-good state, then the returned CreateResult.data_loss indicates
  // whether it has a data loss and what kind of data loss it is (partial or
  // complete) so that any derived data may know that it needs to be updated. If
  // the log re-initializes successfully without any data loss,
  // CreateResult.data_loss will be NONE.
  //
  // Params:
  //   filesystem: Handles system level calls
  //   file_path: Path of the underlying file. Directory of the file should
  //   already exist
  //   options: Configuration options for the proto log
  //
  // Returns:
  //   FileBackedProtoLog::CreateResult on success
  //   INVALID_ARGUMENT on an invalid option
  //   INTERNAL_ERROR on IO error
  static libtextclassifier3::StatusOr<CreateResult> Create(
      const Filesystem* filesystem, const std::string& file_path,
      const Options& options);

  // Not copyable
  FileBackedProtoLog(const FileBackedProtoLog&) = delete;
  FileBackedProtoLog& operator=(const FileBackedProtoLog&) = delete;

  // This will update the checksum of the log as well.
  ~FileBackedProtoLog();

  // Writes the serialized proto to the underlying file. Writes are applied
  // directly to the underlying file. Users do not need to sync the file after
  // writing.
  //
  // Returns:
  //   Offset of the newly appended proto in file on success
  //   INVALID_ARGUMENT if proto is too large, as decided by
  //     Options.max_proto_size
  //   INTERNAL_ERROR on IO error
  libtextclassifier3::StatusOr<int64_t> WriteProto(const ProtoT& proto);

  // Reads out a proto located at file_offset from the file.
  //
  // Returns:
  //   A proto on success
  //   NOT_FOUND if the proto at the given offset has been erased
  //   OUT_OF_RANGE_ERROR if file_offset exceeds file size
  //   INTERNAL_ERROR on IO error
  libtextclassifier3::StatusOr<ProtoT> ReadProto(int64_t file_offset) const;

  // Erases the data of a proto located at file_offset from the file.
  //
  // Returns:
  //   OK on success
  //   OUT_OF_RANGE_ERROR if file_offset exceeds file size
  //   INTERNAL_ERROR on IO error
  libtextclassifier3::Status EraseProto(int64_t file_offset);

  // Calculates and returns the disk usage in bytes. Rounds up to the nearest
  // block size.
  //
  // Returns:
  //   Disk usage on success
  //   INTERNAL_ERROR on IO error
  libtextclassifier3::StatusOr<int64_t> GetDiskUsage() const;

  // Returns the file size of all the elements held in the log. File size is in
  // bytes. This excludes the size of any internal metadata of the log, e.g. the
  // log's header.
  //
  // Returns:
  //   File size on success
  //   INTERNAL_ERROR on IO error
  libtextclassifier3::StatusOr<int64_t> GetElementsFileSize() const;

  // An iterator helping to find offsets of all the protos in file.
  // Example usage:
  //
  // while (iterator.Advance().ok()) {
  //   int64_t offset = iterator.GetOffset();
  //   // Do something
  // }
  class Iterator {
   public:
    Iterator(const Filesystem& filesystem, const std::string& file_path,
             int64_t initial_offset);

    // Advances to the position of next proto whether it has been erased or not.
    //
    // Returns:
    //   OK on success
    //   OUT_OF_RANGE_ERROR if it reaches the end
    //   INTERNAL_ERROR on IO error
    libtextclassifier3::Status Advance();

    // Returns the file offset of current proto.
    int64_t GetOffset();

   private:
    static constexpr int64_t kInvalidOffset = -1;
    // Used to read proto metadata
    MemoryMappedFile mmapped_file_;
    // Offset of first proto
    int64_t initial_offset_;
    int64_t current_offset_;
    int64_t file_size_;
  };

  // Returns an iterator of current proto log. The caller needs to keep the
  // proto log unchanged while using the iterator, otherwise unexpected
  // behaviors could happen.
  Iterator GetIterator();

  // Persists all changes since initialization or the last call to
  // PersistToDisk(). Any changes that aren't persisted may be lost if the
  // system fails to close safely.
  //
  // Example use case:
  //
  //   Document document;
  //   document.set_namespace("com.google.android.example");
  //   document.set_uri("www.google.com");
  //
  //   {
  //     ICING_ASSERT_OK_AND_ASSIGN(auto create_result,
  //         FileBackedProtoLog<DocumentProto>::Create(filesystem, file_path,
  //                                                    options));
  //     auto proto_log = std::move(create_result.proto_log);
  //
  //     int64_t document_offset = proto_log->WriteProto(document));
  //
  //     // We lose the document here since it wasn't persisted.
  //     // *SYSTEM CRASH*
  //   }
  //
  //   {
  //     // Can still successfully create after a crash since the log can
  //     // rewind/truncate to recover into a previously good state
  //     ICING_ASSERT_OK_AND_ASSIGN(auto create_result,
  //         FileBackedProtoLog<DocumentProto>::Create(filesystem, file_path,
  //                                                    options));
  //     auto proto_log = std::move(create_result.proto_log);
  //
  //     // Lost the proto since we didn't PersistToDisk before the crash
  //     proto_log->ReadProto(document_offset)); // INVALID_ARGUMENT error
  //
  //     int64_t document_offset = proto_log->WriteProto(document));
  //
  //     // Persisted this time, so we should be ok.
  //     ICING_ASSERT_OK(proto_log->PersistToDisk());
  //   }
  //
  //   {
  //     ICING_ASSERT_OK_AND_ASSIGN(auto create_result,
  //         FileBackedProtoLog<DocumentProto>::Create(filesystem, file_path,
  //                                                    options));
  //     auto proto_log = std::move(create_result.proto_log);
  //
  //     // SUCCESS
  //     Document same_document = proto_log->ReadProto(document_offset));
  //   }
  //
  // NOTE: Since all protos are already written to the file directly, this
  // just updates the checksum and rewind position. Without these updates,
  // future initializations will truncate the file and discard unpersisted
  // changes.
  //
  // Returns:
  //   OK on success
  //   INTERNAL_ERROR on IO error
  libtextclassifier3::Status PersistToDisk();

  // Calculates the checksum of the log contents. Excludes the header content.
  //
  // Returns:
  //   Crc of the log content
  //   INTERNAL_ERROR on IO error
  libtextclassifier3::StatusOr<Crc32> ComputeChecksum();

 private:
  // Object can only be instantiated via the ::Create factory.
  FileBackedProtoLog(const Filesystem* filesystem, const std::string& file_path,
                     std::unique_ptr<Header> header);

  // Initializes a new proto log.
  //
  // Returns:
  //   std::unique_ptr<CreateResult> on success
  //   INTERNAL_ERROR on IO error
  static libtextclassifier3::StatusOr<CreateResult> InitializeNewFile(
      const Filesystem* filesystem, const std::string& file_path,
      const Options& options);

  // Verifies that the existing proto log is in a good state. If not in a good
  // state, then the proto log may be truncated to the last good state and
  // content will be lost.
  //
  // Returns:
  //   std::unique_ptr<CreateResult> on success
  //   INTERNAL_ERROR on IO error or internal inconsistencies in the file
  //   INVALID_ARGUMENT_ERROR if options aren't consistent with previous
  //     instances
  static libtextclassifier3::StatusOr<CreateResult> InitializeExistingFile(
      const Filesystem* filesystem, const std::string& file_path,
      const Options& options, int64_t file_size);

  // Takes an initial checksum and updates it with the content between `start`
  // and `end` offsets in the file.
  //
  // Returns:
  //   Crc of the content between `start`, inclusive, and `end`, exclusive.
  //   INTERNAL_ERROR on IO error
  //   INVALID_ARGUMENT_ERROR if start and end aren't within the file size
  static libtextclassifier3::StatusOr<Crc32> ComputeChecksum(
      const Filesystem* filesystem, const std::string& file_path,
      Crc32 initial_crc, int64_t start, int64_t end);

  static bool IsEmptyBuffer(const char* buffer, int size) {
    return std::all_of(buffer, buffer + size,
                       [](const char byte) { return byte == 0; });
  }

  // Helper function to get stored proto size from the metadata.
  // Metadata format: 8 bits magic + 24 bits size
  static int GetProtoSize(int metadata) { return metadata & 0x00FFFFFF; }

  // Helper function to get stored proto magic from the metadata.
  // Metadata format: 8 bits magic + 24 bits size
  static uint8_t GetProtoMagic(int metadata) { return metadata >> 24; }

  // Reads out the metadata of a proto located at file_offset from the file.
  //
  // Returns:
  //   Proto's metadata on success
  //   OUT_OF_RANGE_ERROR if file_offset exceeds file_size
  //   INTERNAL_ERROR if the metadata is invalid or any IO errors happen
  static libtextclassifier3::StatusOr<int> ReadProtoMetadata(
      MemoryMappedFile* mmapped_file, int64_t file_offset, int64_t file_size);

  // Magic number added in front of every proto. Used when reading out protos
  // as a first check for corruption in each entry in the file. Even if there is
  // a corruption, the best we can do is roll back to our last recovery point
  // and throw away un-flushed data. We can discard/reuse this byte if needed so
  // that we have 4 bytes to store the size of protos, and increase the size of
  // protos we support.
  static constexpr uint8_t kProtoMagic = 0x5C;

  // Our internal max for protos.
  //
  // WARNING: Changing this to a larger number may invalidate our assumption
  // that that proto size can safely be stored in the last 3 bytes of the proto
  // header.
  static constexpr int kMaxProtoSize = (1 << 24) - 1;  // 16MiB
  static_assert(kMaxProtoSize <= 0x00FFFFFF,
                "kMaxProtoSize doesn't fit in 3 bytes");

  // Level of compression, BEST_SPEED = 1, BEST_COMPRESSION = 9
  static constexpr int kDeflateCompressionLevel = 3;

  // Chunks of the file to mmap at a time, so we don't mmap the entire file.
  // Only used on 32-bit devices
  static constexpr int kMmapChunkSize = 4 * 1024 * 1024;  // 4MiB

  ScopedFd fd_;
  const Filesystem* const filesystem_;
  const std::string file_path_;
  std::unique_ptr<Header> header_;
};

template <typename ProtoT>
constexpr uint8_t FileBackedProtoLog<ProtoT>::kProtoMagic;

template <typename ProtoT>
FileBackedProtoLog<ProtoT>::FileBackedProtoLog(const Filesystem* filesystem,
                                               const std::string& file_path,
                                               std::unique_ptr<Header> header)
    : filesystem_(filesystem),
      file_path_(file_path),
      header_(std::move(header)) {
  fd_.reset(filesystem_->OpenForAppend(file_path.c_str()));
}

template <typename ProtoT>
FileBackedProtoLog<ProtoT>::~FileBackedProtoLog() {
  if (!PersistToDisk().ok()) {
    ICING_LOG(WARNING)
        << "Error persisting to disk during destruction of FileBackedProtoLog: "
        << file_path_;
  }
}

template <typename ProtoT>
libtextclassifier3::StatusOr<typename FileBackedProtoLog<ProtoT>::CreateResult>
FileBackedProtoLog<ProtoT>::Create(const Filesystem* filesystem,
                                   const std::string& file_path,
                                   const Options& options) {
  if (options.max_proto_size <= 0) {
    return absl_ports::InvalidArgumentError(IcingStringUtil::StringPrintf(
        "options.max_proto_size must be greater than 0, was %d",
        options.max_proto_size));
  }

  // Since we store the proto_size in 3 bytes, we can only support protos of up
  // to 16MiB.
  if (options.max_proto_size > kMaxProtoSize) {
    return absl_ports::InvalidArgumentError(IcingStringUtil::StringPrintf(
        "options.max_proto_size must be under 16MiB, was %d",
        options.max_proto_size));
  }

  if (!filesystem->FileExists(file_path.c_str())) {
    return InitializeNewFile(filesystem, file_path, options);
  }

  int64_t file_size = filesystem->GetFileSize(file_path.c_str());
  if (file_size == Filesystem::kBadFileSize) {
    return absl_ports::InternalError(
        absl_ports::StrCat("Bad file size '", file_path, "'"));
  }

  if (file_size == 0) {
    return InitializeNewFile(filesystem, file_path, options);
  }

  return InitializeExistingFile(filesystem, file_path, options, file_size);
}

template <typename ProtoT>
libtextclassifier3::StatusOr<typename FileBackedProtoLog<ProtoT>::CreateResult>
FileBackedProtoLog<ProtoT>::InitializeNewFile(const Filesystem* filesystem,
                                              const std::string& file_path,
                                              const Options& options) {
  // Create the header
  std::unique_ptr<Header> header = std::make_unique<Header>();
  header->compress = options.compress;
  header->max_proto_size = options.max_proto_size;
  header->header_checksum = header->CalculateHeaderChecksum();

  if (!filesystem->Write(file_path.c_str(), header.get(), sizeof(Header))) {
    return absl_ports::InternalError(
        absl_ports::StrCat("Failed to write header for file: ", file_path));
  }

  CreateResult create_result = {
      std::unique_ptr<FileBackedProtoLog<ProtoT>>(
          new FileBackedProtoLog<ProtoT>(filesystem, file_path,
                                         std::move(header))),
      /*data_loss=*/DataLoss::NONE};

  return create_result;
}

template <typename ProtoT>
libtextclassifier3::StatusOr<typename FileBackedProtoLog<ProtoT>::CreateResult>
FileBackedProtoLog<ProtoT>::InitializeExistingFile(const Filesystem* filesystem,
                                                   const std::string& file_path,
                                                   const Options& options,
                                                   int64_t file_size) {
  if (file_size < sizeof(Header)) {
    return absl_ports::InternalError(
        absl_ports::StrCat("File header too short for: ", file_path));
  }

  std::unique_ptr<Header> header = std::make_unique<Header>();
  if (!filesystem->PRead(file_path.c_str(), header.get(), sizeof(Header),
                         /*offset=*/0)) {
    return absl_ports::InternalError(
        absl_ports::StrCat("Failed to read header for file: ", file_path));
  }

  // Make sure the header is still valid before we use any of its values. This
  // is covered by the header_checksum check below, but this is a quick check
  // that can save us from an extra crc computation.
  if (header->magic != Header::kMagic) {
    return absl_ports::InternalError(
        absl_ports::StrCat("Invalid header kMagic for file: ", file_path));
  }

  if (header->header_checksum != header->CalculateHeaderChecksum()) {
    return absl_ports::InternalError(
        absl_ports::StrCat("Invalid header checksum for: ", file_path));
  }

  if (header->compress != options.compress) {
    return absl_ports::InvalidArgumentError(IcingStringUtil::StringPrintf(
        "Inconsistent compress option, expected %d, actual %d",
        header->compress, options.compress));
  }

  if (header->max_proto_size > options.max_proto_size) {
    return absl_ports::InvalidArgumentError(IcingStringUtil::StringPrintf(
        "Max proto size cannot be smaller than previous "
        "instantiations, previous size %d, wanted size %d",
        header->max_proto_size, options.max_proto_size));
  }
  header->max_proto_size = options.max_proto_size;

  DataLoss data_loss = DataLoss::NONE;
  ICING_ASSIGN_OR_RETURN(Crc32 calculated_log_checksum,
                         ComputeChecksum(filesystem, file_path, Crc32(),
                                         sizeof(Header), file_size));

  // Double check that the log checksum is the same as the one that was
  // persisted last time. If not, we start recovery logic.
  if (header->log_checksum != calculated_log_checksum.Get()) {
    // Need to rewind the proto log since the checksums don't match.
    // Worst case, we have to rewind the entire log back to just the header
    int64_t last_known_good = sizeof(Header);

    // Calculate the checksum of the log contents just up to the last rewind
    // offset point. This will be valid if we just appended contents to the log
    // without updating the checksum, and we can rewind back to this point
    // safely.
    ICING_ASSIGN_OR_RETURN(
        calculated_log_checksum,
        ComputeChecksum(filesystem, file_path, Crc32(), sizeof(Header),
                        header->rewind_offset));
    if (header->log_checksum == calculated_log_checksum.Get()) {
      // Check if it matches our last rewind state. If so, this becomes our last
      // good state and we can safely truncate and recover from here.
      last_known_good = header->rewind_offset;
      data_loss = DataLoss::PARTIAL;
    } else {
      // Otherwise, we're going to truncate the entire log and this resets the
      // checksum to an empty log state.
      header->log_checksum = 0;
      data_loss = DataLoss::COMPLETE;
    }

    if (!filesystem->Truncate(file_path.c_str(), last_known_good)) {
      return absl_ports::InternalError(
          absl_ports::StrCat("Error truncating file: ", file_path));
    }

    ICING_LOG(INFO) << "Truncated '" << file_path << "' to size "
                    << last_known_good;
  }

  CreateResult create_result = {
      std::unique_ptr<FileBackedProtoLog<ProtoT>>(
          new FileBackedProtoLog<ProtoT>(filesystem, file_path,
                                         std::move(header))),
      data_loss};

  return create_result;
}

template <typename ProtoT>
libtextclassifier3::StatusOr<Crc32> FileBackedProtoLog<ProtoT>::ComputeChecksum(
    const Filesystem* filesystem, const std::string& file_path,
    Crc32 initial_crc, int64_t start, int64_t end) {
  auto mmapped_file = MemoryMappedFile(*filesystem, file_path,
                                       MemoryMappedFile::Strategy::READ_ONLY);
  Crc32 new_crc(initial_crc.Get());

  if (start < 0) {
    return absl_ports::InvalidArgumentError(IcingStringUtil::StringPrintf(
        "Starting checksum offset of file '%s' must be greater than 0, was "
        "%lld",
        file_path.c_str(), static_cast<long long>(start)));
  }

  if (end < start) {
    return absl_ports::InvalidArgumentError(IcingStringUtil::StringPrintf(
        "Ending checksum offset of file '%s' must be greater than start "
        "'%lld', was '%lld'",
        file_path.c_str(), static_cast<long long>(start),
        static_cast<long long>(end)));
  }

  int64_t file_size = filesystem->GetFileSize(file_path.c_str());
  if (end > file_size) {
    return absl_ports::InvalidArgumentError(IcingStringUtil::StringPrintf(
        "Ending checksum offset of file '%s' must be within "
        "file size of %lld, was %lld",
        file_path.c_str(), static_cast<long long>(file_size),
        static_cast<long long>(end)));
  }

  Architecture architecture = GetArchitecture();
  switch (architecture) {
    case Architecture::BIT_64: {
      // Don't mmap in chunks here since mmapping can be harmful on 64-bit
      // devices where mmap/munmap calls need the mmap write semaphore, which
      // blocks mmap/munmap/mprotect and all page faults from executing while
      // they run. On 64-bit devices, this doesn't actually load into memory, it
      // just makes the file faultable. So the whole file should be ok.
      // b/185822878.
      ICING_RETURN_IF_ERROR(mmapped_file.Remap(start, end - start));
      auto mmap_str = std::string_view(mmapped_file.region(), end - start);
      new_crc.Append(mmap_str);
      break;
    }
    case Architecture::BIT_32:
      [[fallthrough]];
    case Architecture::UNKNOWN: {
      // 32-bit devices only have 4GB of RAM. Mmap in chunks to not use up too
      // much memory at once. If we're unknown, then also chunk it because we're
      // not sure what the device can handle.
      for (int i = start; i < end; i += kMmapChunkSize) {
        // Don't read past the file size.
        int next_chunk_size = kMmapChunkSize;
        if ((i + kMmapChunkSize) >= end) {
          next_chunk_size = end - i;
        }

        ICING_RETURN_IF_ERROR(mmapped_file.Remap(i, next_chunk_size));

        auto mmap_str =
            std::string_view(mmapped_file.region(), next_chunk_size);
        new_crc.Append(mmap_str);
      }
      break;
    }
  }

  return new_crc;
}

template <typename ProtoT>
libtextclassifier3::StatusOr<int64_t> FileBackedProtoLog<ProtoT>::WriteProto(
    const ProtoT& proto) {
  int64_t proto_size = proto.ByteSizeLong();
  int32_t metadata;
  int metadata_size = sizeof(metadata);
  int64_t current_position = filesystem_->GetCurrentPosition(fd_.get());

  if (proto_size > header_->max_proto_size) {
    return absl_ports::InvalidArgumentError(IcingStringUtil::StringPrintf(
        "proto_size, %lld, was too large to write. Max is %d",
        static_cast<long long>(proto_size), header_->max_proto_size));
  }

  // At this point, we've guaranteed that proto_size is under kMaxProtoSize
  // (see
  // ::Create), so we can safely store it in an int.
  int final_size = 0;

  std::string proto_str;
  google::protobuf::io::StringOutputStream proto_stream(&proto_str);

  if (header_->compress) {
    google::protobuf::io::GzipOutputStream::Options options;
    options.format = google::protobuf::io::GzipOutputStream::ZLIB;
    options.compression_level = kDeflateCompressionLevel;

    google::protobuf::io::GzipOutputStream compressing_stream(&proto_stream,
                                                                  options);

    bool success = proto.SerializeToZeroCopyStream(&compressing_stream) &&
                   compressing_stream.Close();

    if (!success) {
      return absl_ports::InternalError("Error compressing proto.");
    }

    final_size = proto_str.size();

    // In case the compressed proto is larger than the original proto, we also
    // can't write it.
    if (final_size > header_->max_proto_size) {
      return absl_ports::InvalidArgumentError(IcingStringUtil::StringPrintf(
          "Compressed proto size, %d, was greater than "
          "max_proto_size, %d",
          final_size, header_->max_proto_size));
    }
  } else {
    // Serialize the proto directly into the write buffer at an offset of the
    // metadata.
    proto.SerializeToZeroCopyStream(&proto_stream);
    final_size = proto_str.size();
  }

  // 1st byte for magic, next 3 bytes for proto size.
  metadata = (kProtoMagic << 24) | final_size;

  // Actually write metadata, has to be done after we know the possibly
  // compressed proto size
  if (!filesystem_->Write(fd_.get(), &metadata, metadata_size)) {
    return absl_ports::InternalError(
        absl_ports::StrCat("Failed to write proto metadata to: ", file_path_));
  }

  // Write the serialized proto
  if (!filesystem_->Write(fd_.get(), proto_str.data(), proto_str.size())) {
    return absl_ports::InternalError(
        absl_ports::StrCat("Failed to write proto to: ", file_path_));
  }

  return current_position;
}

template <typename ProtoT>
libtextclassifier3::StatusOr<ProtoT> FileBackedProtoLog<ProtoT>::ReadProto(
    int64_t file_offset) const {
  int64_t file_size = filesystem_->GetFileSize(fd_.get());
  MemoryMappedFile mmapped_file(*filesystem_, file_path_,
                                MemoryMappedFile::Strategy::READ_ONLY);
  if (file_offset >= file_size) {
    // file_size points to the next byte to write at, so subtract one to get
    // the inclusive, actual size of file.
    return absl_ports::OutOfRangeError(
        IcingStringUtil::StringPrintf("Trying to read from a location, %lld, "
                                      "out of range of the file size, %lld",
                                      static_cast<long long>(file_offset),
                                      static_cast<long long>(file_size - 1)));
  }

  // Read out the metadata
  ICING_ASSIGN_OR_RETURN(
      int metadata, ReadProtoMetadata(&mmapped_file, file_offset, file_size));

  // Copy out however many bytes it says the proto is
  int stored_size = GetProtoSize(metadata);

  ICING_RETURN_IF_ERROR(
      mmapped_file.Remap(file_offset + sizeof(metadata), stored_size));

  if (IsEmptyBuffer(mmapped_file.region(), mmapped_file.region_size())) {
    return absl_ports::NotFoundError("The proto data has been erased.");
  }

  google::protobuf::io::ArrayInputStream proto_stream(
      mmapped_file.mutable_region(), stored_size);

  // Deserialize proto
  ProtoT proto;
  if (header_->compress) {
    google::protobuf::io::GzipInputStream decompress_stream(&proto_stream);
    proto.ParseFromZeroCopyStream(&decompress_stream);
  } else {
    proto.ParseFromZeroCopyStream(&proto_stream);
  }

  return proto;
}

template <typename ProtoT>
libtextclassifier3::Status FileBackedProtoLog<ProtoT>::EraseProto(
    int64_t file_offset) {
  int64_t file_size = filesystem_->GetFileSize(fd_.get());
  if (file_offset >= file_size) {
    // file_size points to the next byte to write at, so subtract one to get
    // the inclusive, actual size of file.
    return absl_ports::OutOfRangeError(IcingStringUtil::StringPrintf(
        "Trying to erase data at a location, %lld, "
        "out of range of the file size, %lld",
        static_cast<long long>(file_offset),
        static_cast<long long>(file_size - 1)));
  }

  MemoryMappedFile mmapped_file(
      *filesystem_, file_path_,
      MemoryMappedFile::Strategy::READ_WRITE_AUTO_SYNC);

  // Read out the metadata
  ICING_ASSIGN_OR_RETURN(
      int metadata, ReadProtoMetadata(&mmapped_file, file_offset, file_size));

  ICING_RETURN_IF_ERROR(mmapped_file.Remap(file_offset + sizeof(metadata),
                                           GetProtoSize(metadata)));

  // We need to update the crc checksum if the erased area is before the
  // rewind position.
  if (file_offset + sizeof(metadata) < header_->rewind_offset) {
    // We need to calculate [original string xor 0s].
    // The xored string is the same as the original string because 0 xor 0 =
    // 0, 1 xor 0 = 1.
    const std::string_view xored_str(mmapped_file.region(),
                                     mmapped_file.region_size());

    Crc32 crc(header_->log_checksum);
    ICING_ASSIGN_OR_RETURN(
        uint32_t new_crc,
        crc.UpdateWithXor(
            xored_str,
            /*full_data_size=*/header_->rewind_offset - sizeof(Header),
            /*position=*/file_offset + sizeof(metadata) - sizeof(Header)));

    header_->log_checksum = new_crc;
    header_->header_checksum = header_->CalculateHeaderChecksum();

    if (!filesystem_->PWrite(fd_.get(), /*offset=*/0, header_.get(),
                             sizeof(Header))) {
      return absl_ports::InternalError(
          absl_ports::StrCat("Failed to update header to: ", file_path_));
    }
  }

  memset(mmapped_file.mutable_region(), '\0', mmapped_file.region_size());
  return libtextclassifier3::Status::OK;
}

template <typename ProtoT>
libtextclassifier3::StatusOr<int64_t> FileBackedProtoLog<ProtoT>::GetDiskUsage()
    const {
  int64_t size = filesystem_->GetDiskUsage(file_path_.c_str());
  if (size == Filesystem::kBadFileSize) {
    return absl_ports::InternalError("Failed to get disk usage of proto log");
  }
  return size;
}

template <typename ProtoT>
libtextclassifier3::StatusOr<int64_t>
FileBackedProtoLog<ProtoT>::GetElementsFileSize() const {
  int64_t total_file_size = filesystem_->GetFileSize(file_path_.c_str());
  if (total_file_size == Filesystem::kBadFileSize) {
    return absl_ports::InternalError(
        "Failed to get file size of elments in the proto log");
  }
  return total_file_size - sizeof(Header);
}

template <typename ProtoT>
FileBackedProtoLog<ProtoT>::Iterator::Iterator(const Filesystem& filesystem,
                                               const std::string& file_path,
                                               int64_t initial_offset)
    : mmapped_file_(filesystem, file_path,
                    MemoryMappedFile::Strategy::READ_ONLY),
      initial_offset_(initial_offset),
      current_offset_(kInvalidOffset),
      file_size_(filesystem.GetFileSize(file_path.c_str())) {
  if (file_size_ == Filesystem::kBadFileSize) {
    // Fails all Advance() calls
    file_size_ = 0;
  }
}

template <typename ProtoT>
libtextclassifier3::Status FileBackedProtoLog<ProtoT>::Iterator::Advance() {
  if (current_offset_ == kInvalidOffset) {
    // First Advance() call
    current_offset_ = initial_offset_;
  } else {
    // Jumps to the next proto position
    ICING_ASSIGN_OR_RETURN(
        int metadata,
        ReadProtoMetadata(&mmapped_file_, current_offset_, file_size_));
    current_offset_ += sizeof(metadata) + GetProtoSize(metadata);
  }

  if (current_offset_ < file_size_) {
    return libtextclassifier3::Status::OK;
  } else {
    return absl_ports::OutOfRangeError(IcingStringUtil::StringPrintf(
        "The next proto offset, %lld, is out of file range [0, %lld)",
        static_cast<long long>(current_offset_),
        static_cast<long long>(file_size_)));
  }
}

template <typename ProtoT>
int64_t FileBackedProtoLog<ProtoT>::Iterator::GetOffset() {
  return current_offset_;
}

template <typename ProtoT>
typename FileBackedProtoLog<ProtoT>::Iterator
FileBackedProtoLog<ProtoT>::GetIterator() {
  return Iterator(*filesystem_, file_path_,
                  /*initial_offset=*/sizeof(Header));
}

template <typename ProtoT>
libtextclassifier3::StatusOr<int> FileBackedProtoLog<ProtoT>::ReadProtoMetadata(
    MemoryMappedFile* mmapped_file, int64_t file_offset, int64_t file_size) {
  // Checks file_offset
  if (file_offset >= file_size) {
    return absl_ports::OutOfRangeError(IcingStringUtil::StringPrintf(
        "offset, %lld, is out of file range [0, %lld)",
        static_cast<long long>(file_offset),
        static_cast<long long>(file_size)));
  }
  int metadata;
  int metadata_size = sizeof(metadata);
  if (file_offset + metadata_size >= file_size) {
    return absl_ports::InternalError(IcingStringUtil::StringPrintf(
        "Wrong metadata offset %lld, metadata doesn't fit in "
        "with file range [0, %lld)",
        static_cast<long long>(file_offset),
        static_cast<long long>(file_size)));
  }
  // Reads metadata
  ICING_RETURN_IF_ERROR(mmapped_file->Remap(file_offset, metadata_size));
  memcpy(&metadata, mmapped_file->region(), metadata_size);
  // Checks magic number
  uint8_t stored_k_proto_magic = GetProtoMagic(metadata);
  if (stored_k_proto_magic != kProtoMagic) {
    return absl_ports::InternalError(IcingStringUtil::StringPrintf(
        "Failed to read kProtoMagic, expected %d, actual %d", kProtoMagic,
        stored_k_proto_magic));
  }
  return metadata;
}

template <typename ProtoT>
libtextclassifier3::Status FileBackedProtoLog<ProtoT>::PersistToDisk() {
  int64_t file_size = filesystem_->GetFileSize(file_path_.c_str());
  if (file_size == header_->rewind_offset) {
    // No new protos appended, don't need to update the checksum.
    return libtextclassifier3::Status::OK;
  }

  int64_t new_content_size = file_size - header_->rewind_offset;
  Crc32 crc;
  if (new_content_size < 0) {
    // File shrunk, recalculate the entire checksum.
    ICING_ASSIGN_OR_RETURN(
        crc, ComputeChecksum(filesystem_, file_path_, Crc32(), sizeof(Header),
                             file_size));
  } else {
    // Append new changes to the existing checksum.
    ICING_ASSIGN_OR_RETURN(
        crc,
        ComputeChecksum(filesystem_, file_path_, Crc32(header_->log_checksum),
                        header_->rewind_offset, file_size));
  }

  header_->log_checksum = crc.Get();
  header_->rewind_offset = file_size;
  header_->header_checksum = header_->CalculateHeaderChecksum();

  if (!filesystem_->PWrite(fd_.get(), /*offset=*/0, header_.get(),
                           sizeof(Header)) ||
      !filesystem_->DataSync(fd_.get())) {
    return absl_ports::InternalError(
        absl_ports::StrCat("Failed to update header to: ", file_path_));
  }

  return libtextclassifier3::Status::OK;
}

template <typename ProtoT>
libtextclassifier3::StatusOr<Crc32>
FileBackedProtoLog<ProtoT>::ComputeChecksum() {
  return FileBackedProtoLog<ProtoT>::ComputeChecksum(
      filesystem_, file_path_, Crc32(), /*start=*/sizeof(Header),
      /*end=*/filesystem_->GetFileSize(file_path_.c_str()));
}

}  // namespace lib
}  // namespace icing

#endif  // ICING_FILE_FILE_BACKED_PROTO_LOG_H_
