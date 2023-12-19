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

#include "icing/file/file-backed-proto-log.h"

#include <cstdint>
#include <cstdlib>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "icing/document-builder.h"
#include "icing/file/filesystem.h"
#include "icing/file/mock-filesystem.h"
#include "icing/portable/equals-proto.h"
#include "icing/proto/document.pb.h"
#include "icing/testing/common-matchers.h"
#include "icing/testing/tmp-directory.h"

namespace icing {
namespace lib {

namespace {

using ::icing::lib::portable_equals_proto::EqualsProto;
using ::testing::A;
using ::testing::Eq;
using ::testing::Gt;
using ::testing::Not;
using ::testing::NotNull;
using ::testing::Pair;
using ::testing::Return;

class FileBackedProtoLogTest : public ::testing::Test {
 protected:
  // Adds a user-defined default construct because a const member variable may
  // make the compiler accidentally delete the default constructor.
  // https://stackoverflow.com/a/47368753
  FileBackedProtoLogTest() {}

  void SetUp() override {
    file_path_ = GetTestTempDir() + "/proto_log";
    filesystem_.DeleteFile(file_path_.c_str());
  }

  void TearDown() override { filesystem_.DeleteFile(file_path_.c_str()); }

  const Filesystem filesystem_;
  std::string file_path_;
  bool compress_ = true;
  int64_t max_proto_size_ = 256 * 1024;  // 256 KiB
};

TEST_F(FileBackedProtoLogTest, Initialize) {
  // max_proto_size must be greater than 0
  int invalid_max_proto_size = 0;
  ASSERT_THAT(FileBackedProtoLog<DocumentProto>::Create(
                  &filesystem_, file_path_,
                  FileBackedProtoLog<DocumentProto>::Options(
                      compress_, invalid_max_proto_size)),
              StatusIs(libtextclassifier3::StatusCode::INVALID_ARGUMENT));

  ICING_ASSERT_OK_AND_ASSIGN(
      FileBackedProtoLog<DocumentProto>::CreateResult create_result,
      FileBackedProtoLog<DocumentProto>::Create(
          &filesystem_, file_path_,
          FileBackedProtoLog<DocumentProto>::Options(compress_,
                                                     max_proto_size_)));
  EXPECT_THAT(create_result.proto_log, NotNull());
  EXPECT_FALSE(create_result.has_data_loss());

  // Can't recreate the same file with different options.
  ASSERT_THAT(FileBackedProtoLog<DocumentProto>::Create(
                  &filesystem_, file_path_,
                  FileBackedProtoLog<DocumentProto>::Options(!compress_,
                                                             max_proto_size_)),
              StatusIs(libtextclassifier3::StatusCode::INVALID_ARGUMENT));
}

TEST_F(FileBackedProtoLogTest, WriteProtoTooLarge) {
  int max_proto_size = 1;
  ICING_ASSERT_OK_AND_ASSIGN(
      FileBackedProtoLog<DocumentProto>::CreateResult create_result,
      FileBackedProtoLog<DocumentProto>::Create(
          &filesystem_, file_path_,
          FileBackedProtoLog<DocumentProto>::Options(compress_,
                                                     max_proto_size)));
  auto proto_log = std::move(create_result.proto_log);
  ASSERT_FALSE(create_result.has_data_loss());

  DocumentProto document = DocumentBuilder().SetKey("namespace", "uri").Build();

  // Proto is too large for the max_proto_size_in
  ASSERT_THAT(proto_log->WriteProto(document),
              StatusIs(libtextclassifier3::StatusCode::INVALID_ARGUMENT));
}

TEST_F(FileBackedProtoLogTest, ReadProtoWrongKProtoMagic) {
  ICING_ASSERT_OK_AND_ASSIGN(
      FileBackedProtoLog<DocumentProto>::CreateResult create_result,
      FileBackedProtoLog<DocumentProto>::Create(
          &filesystem_, file_path_,
          FileBackedProtoLog<DocumentProto>::Options(compress_,
                                                     max_proto_size_)));
  auto proto_log = std::move(create_result.proto_log);
  ASSERT_FALSE(create_result.has_data_loss());

  // Write a proto
  DocumentProto document = DocumentBuilder().SetKey("namespace", "uri").Build();

  ICING_ASSERT_OK_AND_ASSIGN(int64_t file_offset,
                             proto_log->WriteProto(document));

  // The 4 bytes of metadata that just doesn't have the same kProtoMagic
  // specified in file-backed-proto-log.h
  uint32_t wrong_magic = 0x7E000000;

  // Sanity check that we opened the file correctly
  int fd = filesystem_.OpenForWrite(file_path_.c_str());
  ASSERT_GT(fd, 0);

  // Write the wrong kProtoMagic in, kProtoMagics are stored at the beginning of
  // a proto entry.
  filesystem_.PWrite(fd, file_offset, &wrong_magic, sizeof(wrong_magic));

  ASSERT_THAT(proto_log->ReadProto(file_offset),
              StatusIs(libtextclassifier3::StatusCode::INTERNAL));
}

TEST_F(FileBackedProtoLogTest, ReadWriteUncompressedProto) {
  int last_offset;
  {
    ICING_ASSERT_OK_AND_ASSIGN(
        FileBackedProtoLog<DocumentProto>::CreateResult create_result,
        FileBackedProtoLog<DocumentProto>::Create(
            &filesystem_, file_path_,
            FileBackedProtoLog<DocumentProto>::Options(
                /*compress_in=*/false, max_proto_size_)));
    auto proto_log = std::move(create_result.proto_log);
    ASSERT_FALSE(create_result.has_data_loss());

    // Write the first proto
    DocumentProto document1 =
        DocumentBuilder().SetKey("namespace1", "uri1").Build();

    ICING_ASSERT_OK_AND_ASSIGN(int written_position,
                               proto_log->WriteProto(document1));

    int document1_offset = written_position;

    // Check that what we read is what we wrote
    ASSERT_THAT(proto_log->ReadProto(written_position),
                IsOkAndHolds(EqualsProto(document1)));

    // Write a second proto that's close to the max size. Leave some room for
    // the rest of the proto properties.
    std::string long_str(max_proto_size_ - 1024, 'a');
    DocumentProto document2 = DocumentBuilder()
                                  .SetKey("namespace2", "uri2")
                                  .AddStringProperty("long_str", long_str)
                                  .Build();

    ICING_ASSERT_OK_AND_ASSIGN(written_position,
                               proto_log->WriteProto(document2));

    int document2_offset = written_position;
    last_offset = written_position;
    ASSERT_GT(document2_offset, document1_offset);

    // Check the second proto
    ASSERT_THAT(proto_log->ReadProto(written_position),
                IsOkAndHolds(EqualsProto(document2)));

    ICING_ASSERT_OK(proto_log->PersistToDisk());
  }

  {
    // Make a new proto_log with the same file_path, and make sure we
    // can still write to the same underlying file.
    ICING_ASSERT_OK_AND_ASSIGN(
        FileBackedProtoLog<DocumentProto>::CreateResult create_result,
        FileBackedProtoLog<DocumentProto>::Create(
            &filesystem_, file_path_,
            FileBackedProtoLog<DocumentProto>::Options(
                /*compress_in=*/false, max_proto_size_)));
    auto recreated_proto_log = std::move(create_result.proto_log);
    ASSERT_FALSE(create_result.has_data_loss());

    // Write a third proto
    DocumentProto document3 =
        DocumentBuilder().SetKey("namespace3", "uri3").Build();

    ASSERT_THAT(recreated_proto_log->WriteProto(document3),
                IsOkAndHolds(Gt(last_offset)));
  }
}

TEST_F(FileBackedProtoLogTest, ReadWriteCompressedProto) {
  int last_offset;

  {
    ICING_ASSERT_OK_AND_ASSIGN(
        FileBackedProtoLog<DocumentProto>::CreateResult create_result,
        FileBackedProtoLog<DocumentProto>::Create(
            &filesystem_, file_path_,
            FileBackedProtoLog<DocumentProto>::Options(
                /*compress_in=*/true, max_proto_size_)));
    auto proto_log = std::move(create_result.proto_log);
    ASSERT_FALSE(create_result.has_data_loss());

    // Write the first proto
    DocumentProto document1 =
        DocumentBuilder().SetKey("namespace1", "uri1").Build();

    ICING_ASSERT_OK_AND_ASSIGN(int written_position,
                               proto_log->WriteProto(document1));

    int document1_offset = written_position;

    // Check that what we read is what we wrote
    ASSERT_THAT(proto_log->ReadProto(written_position),
                IsOkAndHolds(EqualsProto(document1)));

    // Write a second proto that's close to the max size. Leave some room for
    // the rest of the proto properties.
    std::string long_str(max_proto_size_ - 1024, 'a');
    DocumentProto document2 = DocumentBuilder()
                                  .SetKey("namespace2", "uri2")
                                  .AddStringProperty("long_str", long_str)
                                  .Build();

    ICING_ASSERT_OK_AND_ASSIGN(written_position,
                               proto_log->WriteProto(document2));

    int document2_offset = written_position;
    last_offset = written_position;
    ASSERT_GT(document2_offset, document1_offset);

    // Check the second proto
    ASSERT_THAT(proto_log->ReadProto(written_position),
                IsOkAndHolds(EqualsProto(document2)));

    ICING_ASSERT_OK(proto_log->PersistToDisk());
  }

  {
    // Make a new proto_log with the same file_path, and make sure we
    // can still write to the same underlying file.
    ICING_ASSERT_OK_AND_ASSIGN(
        FileBackedProtoLog<DocumentProto>::CreateResult create_result,
        FileBackedProtoLog<DocumentProto>::Create(
            &filesystem_, file_path_,
            FileBackedProtoLog<DocumentProto>::Options(
                /*compress_in=*/true, max_proto_size_)));
    auto recreated_proto_log = std::move(create_result.proto_log);
    ASSERT_FALSE(create_result.has_data_loss());

    // Write a third proto
    DocumentProto document3 =
        DocumentBuilder().SetKey("namespace3", "uri3").Build();

    ASSERT_THAT(recreated_proto_log->WriteProto(document3),
                IsOkAndHolds(Gt(last_offset)));
  }
}

TEST_F(FileBackedProtoLogTest, CorruptHeader) {
  {
    ICING_ASSERT_OK_AND_ASSIGN(
        FileBackedProtoLog<DocumentProto>::CreateResult create_result,
        FileBackedProtoLog<DocumentProto>::Create(
            &filesystem_, file_path_,
            FileBackedProtoLog<DocumentProto>::Options(compress_,
                                                       max_proto_size_)));
    auto recreated_proto_log = std::move(create_result.proto_log);
    EXPECT_FALSE(create_result.has_data_loss());

    int corrupt_offset =
        offsetof(FileBackedProtoLog<DocumentProto>::Header, rewind_offset);
    // We should never rewind to a negative offset.
    int invalid_rewind_offset = -1;
    filesystem_.PWrite(file_path_.c_str(), corrupt_offset,
                       &invalid_rewind_offset, sizeof(invalid_rewind_offset));
  }

  {
    // Reinitialize the same proto_log
    ASSERT_THAT(FileBackedProtoLog<DocumentProto>::Create(
                    &filesystem_, file_path_,
                    FileBackedProtoLog<DocumentProto>::Options(
                        compress_, max_proto_size_)),
                StatusIs(libtextclassifier3::StatusCode::INTERNAL));
  }
}

TEST_F(FileBackedProtoLogTest, CorruptContent) {
  {
    ICING_ASSERT_OK_AND_ASSIGN(
        FileBackedProtoLog<DocumentProto>::CreateResult create_result,
        FileBackedProtoLog<DocumentProto>::Create(
            &filesystem_, file_path_,
            FileBackedProtoLog<DocumentProto>::Options(compress_,
                                                       max_proto_size_)));
    auto proto_log = std::move(create_result.proto_log);
    EXPECT_FALSE(create_result.has_data_loss());

    DocumentProto document =
        DocumentBuilder().SetKey("namespace1", "uri1").Build();

    // Write and persist an document.
    ICING_ASSERT_OK_AND_ASSIGN(int document_offset,
                               proto_log->WriteProto(document));
    ICING_ASSERT_OK(proto_log->PersistToDisk());

    // "Corrupt" the content written in the log.
    document.set_uri("invalid");
    std::string serialized_document = document.SerializeAsString();
    filesystem_.PWrite(file_path_.c_str(), document_offset,
                       serialized_document.data(), serialized_document.size());
  }

  {
    // We can recover, but we have data loss.
    ICING_ASSERT_OK_AND_ASSIGN(
        FileBackedProtoLog<DocumentProto>::CreateResult create_result,
        FileBackedProtoLog<DocumentProto>::Create(
            &filesystem_, file_path_,
            FileBackedProtoLog<DocumentProto>::Options(compress_,
                                                       max_proto_size_)));
    auto proto_log = std::move(create_result.proto_log);
    ASSERT_TRUE(create_result.has_data_loss());
    ASSERT_THAT(create_result.data_loss, Eq(DataLoss::COMPLETE));

    // Lost everything in the log since the rewind position doesn't help if
    // there's been data corruption within the persisted region
    ASSERT_EQ(filesystem_.GetFileSize(file_path_.c_str()),
              sizeof(FileBackedProtoLog<DocumentProto>::Header));
  }
}

TEST_F(FileBackedProtoLogTest, PersistToDisk) {
  DocumentProto document1 =
      DocumentBuilder().SetKey("namespace1", "uri1").Build();
  DocumentProto document2 =
      DocumentBuilder().SetKey("namespace2", "uri2").Build();
  int document1_offset, document2_offset;
  int log_size;

  {
    ICING_ASSERT_OK_AND_ASSIGN(
        FileBackedProtoLog<DocumentProto>::CreateResult create_result,
        FileBackedProtoLog<DocumentProto>::Create(
            &filesystem_, file_path_,
            FileBackedProtoLog<DocumentProto>::Options(compress_,
                                                       max_proto_size_)));
    auto proto_log = std::move(create_result.proto_log);
    ASSERT_FALSE(create_result.has_data_loss());

    // Write and persist the first proto
    ICING_ASSERT_OK_AND_ASSIGN(document1_offset,
                               proto_log->WriteProto(document1));
    ICING_ASSERT_OK(proto_log->PersistToDisk());

    // Write, but don't explicitly persist the second proto
    ICING_ASSERT_OK_AND_ASSIGN(document2_offset,
                               proto_log->WriteProto(document2));

    // Check that what we read is what we wrote
    ASSERT_THAT(proto_log->ReadProto(document1_offset),
                IsOkAndHolds(EqualsProto(document1)));
    ASSERT_THAT(proto_log->ReadProto(document2_offset),
                IsOkAndHolds(EqualsProto(document2)));

    log_size = filesystem_.GetFileSize(file_path_.c_str());
    ASSERT_GT(log_size, 0);
  }

  {
    // The header rewind position and checksum aren't updated in this "system
    // crash" scenario.

    std::string bad_proto =
        "some incomplete proto that we didn't finish writing before the system "
        "crashed";
    filesystem_.PWrite(file_path_.c_str(), log_size, bad_proto.data(),
                       bad_proto.size());

    // Double check that we actually wrote something to the underlying file
    ASSERT_GT(filesystem_.GetFileSize(file_path_.c_str()), log_size);
  }

  {
    // We can recover, but we have data loss
    ICING_ASSERT_OK_AND_ASSIGN(
        FileBackedProtoLog<DocumentProto>::CreateResult create_result,
        FileBackedProtoLog<DocumentProto>::Create(
            &filesystem_, file_path_,
            FileBackedProtoLog<DocumentProto>::Options(compress_,
                                                       max_proto_size_)));
    auto proto_log = std::move(create_result.proto_log);
    ASSERT_TRUE(create_result.has_data_loss());
    ASSERT_THAT(create_result.data_loss, Eq(DataLoss::PARTIAL));

    // Check that everything was persisted across instances
    ASSERT_THAT(proto_log->ReadProto(document1_offset),
                IsOkAndHolds(EqualsProto(document1)));
    ASSERT_THAT(proto_log->ReadProto(document2_offset),
                IsOkAndHolds(EqualsProto(document2)));

    // We correctly rewound to the last good state.
    ASSERT_EQ(log_size, filesystem_.GetFileSize(file_path_.c_str()));
  }
}

TEST_F(FileBackedProtoLogTest, Iterator) {
  DocumentProto document1 =
      DocumentBuilder().SetKey("namespace", "uri1").Build();
  DocumentProto document2 =
      DocumentBuilder().SetKey("namespace", "uri2").Build();

  ICING_ASSERT_OK_AND_ASSIGN(
      FileBackedProtoLog<DocumentProto>::CreateResult create_result,
      FileBackedProtoLog<DocumentProto>::Create(
          &filesystem_, file_path_,
          FileBackedProtoLog<DocumentProto>::Options(compress_,
                                                     max_proto_size_)));
  auto proto_log = std::move(create_result.proto_log);
  ASSERT_FALSE(create_result.has_data_loss());

  {
    // Empty iterator
    auto iterator = proto_log->GetIterator();
    ASSERT_THAT(iterator.Advance(),
                StatusIs(libtextclassifier3::StatusCode::OUT_OF_RANGE));
  }

  {
    // Iterates through some documents
    ICING_ASSERT_OK(proto_log->WriteProto(document1));
    ICING_ASSERT_OK(proto_log->WriteProto(document2));
    auto iterator = proto_log->GetIterator();
    // 1st proto
    ICING_ASSERT_OK(iterator.Advance());
    ASSERT_THAT(proto_log->ReadProto(iterator.GetOffset()),
                IsOkAndHolds(EqualsProto(document1)));
    // 2nd proto
    ICING_ASSERT_OK(iterator.Advance());
    ASSERT_THAT(proto_log->ReadProto(iterator.GetOffset()),
                IsOkAndHolds(EqualsProto(document2)));
    // Tries to advance
    ASSERT_THAT(iterator.Advance(),
                StatusIs(libtextclassifier3::StatusCode::OUT_OF_RANGE));
  }

  {
    // Iterator with bad filesystem
    MockFilesystem mock_filesystem;
    ON_CALL(mock_filesystem, GetFileSize(A<const char *>()))
        .WillByDefault(Return(Filesystem::kBadFileSize));
    FileBackedProtoLog<DocumentProto>::Iterator bad_iterator(
        mock_filesystem, file_path_, /*initial_offset=*/0);
    ASSERT_THAT(bad_iterator.Advance(),
                StatusIs(libtextclassifier3::StatusCode::OUT_OF_RANGE));
  }
}

TEST_F(FileBackedProtoLogTest, ComputeChecksum) {
  DocumentProto document = DocumentBuilder().SetKey("namespace", "uri").Build();
  Crc32 checksum;

  {
    ICING_ASSERT_OK_AND_ASSIGN(
        FileBackedProtoLog<DocumentProto>::CreateResult create_result,
        FileBackedProtoLog<DocumentProto>::Create(
            &filesystem_, file_path_,
            FileBackedProtoLog<DocumentProto>::Options(compress_,
                                                       max_proto_size_)));
    auto proto_log = std::move(create_result.proto_log);
    ASSERT_FALSE(create_result.has_data_loss());

    ICING_EXPECT_OK(proto_log->WriteProto(document));

    ICING_ASSERT_OK_AND_ASSIGN(checksum, proto_log->ComputeChecksum());

    // Calling it twice with no changes should get us the same checksum
    EXPECT_THAT(proto_log->ComputeChecksum(), IsOkAndHolds(Eq(checksum)));
  }

  {
    ICING_ASSERT_OK_AND_ASSIGN(
        FileBackedProtoLog<DocumentProto>::CreateResult create_result,
        FileBackedProtoLog<DocumentProto>::Create(
            &filesystem_, file_path_,
            FileBackedProtoLog<DocumentProto>::Options(compress_,
                                                       max_proto_size_)));
    auto proto_log = std::move(create_result.proto_log);
    ASSERT_FALSE(create_result.has_data_loss());

    // Checksum should be consistent across instances
    EXPECT_THAT(proto_log->ComputeChecksum(), IsOkAndHolds(Eq(checksum)));

    // PersistToDisk shouldn't affect the checksum value
    ICING_EXPECT_OK(proto_log->PersistToDisk());
    EXPECT_THAT(proto_log->ComputeChecksum(), IsOkAndHolds(Eq(checksum)));

    // Check that modifying the log leads to a different checksum
    ICING_EXPECT_OK(proto_log->WriteProto(document));
    EXPECT_THAT(proto_log->ComputeChecksum(), IsOkAndHolds(Not(Eq(checksum))));
  }
}

TEST_F(FileBackedProtoLogTest, EraseProtoShouldSetZero) {
  DocumentProto document1 =
      DocumentBuilder().SetKey("namespace", "uri1").Build();

  ICING_ASSERT_OK_AND_ASSIGN(
      FileBackedProtoLog<DocumentProto>::CreateResult create_result,
      FileBackedProtoLog<DocumentProto>::Create(
          &filesystem_, file_path_,
          FileBackedProtoLog<DocumentProto>::Options(compress_,
                                                     max_proto_size_)));
  auto proto_log = std::move(create_result.proto_log);
  ASSERT_FALSE(create_result.has_data_loss());

  // Writes and erases proto
  ICING_ASSERT_OK_AND_ASSIGN(int64_t document1_offset,
                             proto_log->WriteProto(document1));
  ICING_ASSERT_OK(proto_log->EraseProto(document1_offset));

  // Checks if the erased area is set to 0.
  int64_t file_size = filesystem_.GetFileSize(file_path_.c_str());
  MemoryMappedFile mmapped_file(filesystem_, file_path_,
                                MemoryMappedFile::Strategy::READ_ONLY);

  // document1_offset + sizeof(int) is the start byte of the proto where
  // sizeof(int) is the size of the proto metadata.
  mmapped_file.Remap(document1_offset + sizeof(int), file_size - 1);
  for (size_t i = 0; i < mmapped_file.region_size(); ++i) {
    ASSERT_THAT(mmapped_file.region()[i], Eq(0));
  }
}

TEST_F(FileBackedProtoLogTest, EraseProtoShouldReturnNotFound) {
  DocumentProto document1 =
      DocumentBuilder().SetKey("namespace", "uri1").Build();
  DocumentProto document2 =
      DocumentBuilder().SetKey("namespace", "uri2").Build();

  ICING_ASSERT_OK_AND_ASSIGN(
      FileBackedProtoLog<DocumentProto>::CreateResult create_result,
      FileBackedProtoLog<DocumentProto>::Create(
          &filesystem_, file_path_,
          FileBackedProtoLog<DocumentProto>::Options(compress_,
                                                     max_proto_size_)));
  auto proto_log = std::move(create_result.proto_log);
  ASSERT_FALSE(create_result.has_data_loss());

  // Writes 2 protos
  ICING_ASSERT_OK_AND_ASSIGN(int64_t document1_offset,
                             proto_log->WriteProto(document1));
  ICING_ASSERT_OK_AND_ASSIGN(int64_t document2_offset,
                             proto_log->WriteProto(document2));

  // Erases the first proto
  ICING_ASSERT_OK(proto_log->EraseProto(document1_offset));

  // The first proto has been erased.
  ASSERT_THAT(proto_log->ReadProto(document1_offset),
              StatusIs(libtextclassifier3::StatusCode::NOT_FOUND));
  // The second proto should be returned.
  ASSERT_THAT(proto_log->ReadProto(document2_offset),
              IsOkAndHolds(EqualsProto(document2)));
}

TEST_F(FileBackedProtoLogTest, ChecksumShouldBeCorrectWithErasedProto) {
  DocumentProto document1 =
      DocumentBuilder().SetKey("namespace", "uri1").Build();
  DocumentProto document2 =
      DocumentBuilder().SetKey("namespace", "uri2").Build();
  DocumentProto document3 =
      DocumentBuilder().SetKey("namespace", "uri3").Build();
  DocumentProto document4 =
      DocumentBuilder().SetKey("namespace", "uri4").Build();

  int64_t document2_offset;
  int64_t document3_offset;

  {
    // Erase data after the rewind position. This won't update the checksum
    // immediately.
    ICING_ASSERT_OK_AND_ASSIGN(
        FileBackedProtoLog<DocumentProto>::CreateResult create_result,
        FileBackedProtoLog<DocumentProto>::Create(
            &filesystem_, file_path_,
            FileBackedProtoLog<DocumentProto>::Options(compress_,
                                                       max_proto_size_)));
    auto proto_log = std::move(create_result.proto_log);
    ASSERT_FALSE(create_result.has_data_loss());

    // Writes 3 protos
    ICING_ASSERT_OK_AND_ASSIGN(int64_t document1_offset,
                               proto_log->WriteProto(document1));
    ICING_ASSERT_OK_AND_ASSIGN(document2_offset,
                               proto_log->WriteProto(document2));
    ICING_ASSERT_OK_AND_ASSIGN(document3_offset,
                               proto_log->WriteProto(document3));

    // Erases the 1st proto, checksum won't be updated immediately because the
    // rewind position is 0.
    ICING_ASSERT_OK(proto_log->EraseProto(document1_offset));

    EXPECT_THAT(proto_log->ComputeChecksum(),
                IsOkAndHolds(Eq(Crc32(2293202502))));
  }  // New checksum is updated in destructor.

  {
    // Erase data before the rewind position. This will update the checksum
    // immediately.
    ICING_ASSERT_OK_AND_ASSIGN(
        FileBackedProtoLog<DocumentProto>::CreateResult create_result,
        FileBackedProtoLog<DocumentProto>::Create(
            &filesystem_, file_path_,
            FileBackedProtoLog<DocumentProto>::Options(compress_,
                                                       max_proto_size_)));
    auto proto_log = std::move(create_result.proto_log);
    ASSERT_FALSE(create_result.has_data_loss());

    // Erases the 2nd proto that is now before the rewind position. Checksum is
    // updated.
    ICING_ASSERT_OK(proto_log->EraseProto(document2_offset));

    EXPECT_THAT(proto_log->ComputeChecksum(),
                IsOkAndHolds(Eq(Crc32(639634028))));
  }

  {
    // Append data and erase data before the rewind position. This will update
    // the checksum twice: in EraseProto() and destructor.
    ICING_ASSERT_OK_AND_ASSIGN(
        FileBackedProtoLog<DocumentProto>::CreateResult create_result,
        FileBackedProtoLog<DocumentProto>::Create(
            &filesystem_, file_path_,
            FileBackedProtoLog<DocumentProto>::Options(compress_,
                                                       max_proto_size_)));
    auto proto_log = std::move(create_result.proto_log);
    ASSERT_FALSE(create_result.has_data_loss());

    // Append a new document which is after the rewind position.
    ICING_ASSERT_OK(proto_log->WriteProto(document4));

    // Erases the 3rd proto that is now before the rewind position. Checksum is
    // updated.
    ICING_ASSERT_OK(proto_log->EraseProto(document3_offset));

    EXPECT_THAT(proto_log->ComputeChecksum(),
                IsOkAndHolds(Eq(Crc32(1990198693))));
  }  // Checksum is updated with the newly appended document.

  {
    // A successful creation means that the checksum matches.
    ICING_ASSERT_OK_AND_ASSIGN(
        FileBackedProtoLog<DocumentProto>::CreateResult create_result,
        FileBackedProtoLog<DocumentProto>::Create(
            &filesystem_, file_path_,
            FileBackedProtoLog<DocumentProto>::Options(compress_,
                                                       max_proto_size_)));
    auto proto_log = std::move(create_result.proto_log);
    EXPECT_FALSE(create_result.has_data_loss());
  }
}

}  // namespace
}  // namespace lib
}  // namespace icing
