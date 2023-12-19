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

#ifndef ICING_STORE_KEY_MAPPER_H_
#define ICING_STORE_KEY_MAPPER_H_

#include <cstdint>
#include <cstring>
#include <memory>
#include <string>
#include <string_view>
#include <type_traits>

#include "icing/text_classifier/lib3/utils/base/status.h"
#include "icing/text_classifier/lib3/utils/base/statusor.h"
#include "icing/absl_ports/canonical_errors.h"
#include "icing/absl_ports/str_cat.h"
#include "icing/file/filesystem.h"
#include "icing/legacy/index/icing-dynamic-trie.h"
#include "icing/legacy/index/icing-filesystem.h"
#include "icing/util/crc32.h"
#include "icing/util/status-macros.h"

namespace icing {
namespace lib {

// File-backed mapping between the string key and a trivially copyable value
// type.
//
// KeyMapper is thread-compatible
template <typename T>
class KeyMapper {
 public:
  // Returns an initialized instance of KeyMapper that can immediately handle
  // read/write operations.
  // Returns any encountered IO errors.
  //
  // base_dir : Base directory used to save all the files required to persist
  //            KeyMapper. If this base_dir was previously used to create a
  //            KeyMapper, then this existing data would be loaded. Otherwise,
  //            an empty KeyMapper would be created.
  // maximum_size_bytes : The maximum allowable size of the key mapper storage.
  static libtextclassifier3::StatusOr<std::unique_ptr<KeyMapper<T>>> Create(
      const Filesystem& filesystem, std::string_view base_dir,
      int maximum_size_bytes);

  // Deletes all the files associated with the KeyMapper. Returns success or any
  // encountered IO errors
  //
  // base_dir : Base directory used to save all the files required to persist
  //            KeyMapper. Should be the same as passed into Create().
  static libtextclassifier3::Status Delete(const Filesystem& filesystem,
                                           std::string_view base_dir);

  ~KeyMapper() = default;

  // Inserts/Updates value for key.
  // Returns any encountered IO errors.
  //
  // NOTE: Put() doesn't automatically flush changes to disk and relies on
  // either explicit calls to PersistToDisk() or a clean shutdown of the class.
  libtextclassifier3::Status Put(std::string_view key, T value);

  // Finds the current value for key and returns it. If key is not present, it
  // is inserted with next_value and next_value is returned.
  //
  // Returns any IO errors that may occur during Put.
  libtextclassifier3::StatusOr<T> GetOrPut(std::string_view key, T next_value);

  // Returns the value corresponding to the key.
  //
  // Returns NOT_FOUND error if the key was missing.
  // Returns any encountered IO errors.
  libtextclassifier3::StatusOr<T> Get(std::string_view key) const;

  // Deletes data related to the given key. Returns true on success.
  bool Delete(std::string_view key);

  // Returns a map of values to keys. Empty map if the mapper is empty.
  std::unordered_map<T, std::string> GetValuesToKeys() const;

  // Count of unique keys stored in the KeyMapper.
  int32_t num_keys() const { return trie_.size(); }

  // Syncs all the changes made to the KeyMapper to disk.
  // Returns any encountered IO errors.
  //
  // NOTE: To control disk-churn, Put() doesn't automatically persist every
  // change to disk. The caller should explicitly call PersistToDisk() to make
  // sure that the data is durable.
  //
  // Returns:
  //   OK on success
  //   INTERNAL on I/O error
  libtextclassifier3::Status PersistToDisk();

  // Calculates and returns the disk usage in bytes. Rounds up to the nearest
  // block size.
  //
  // Returns:
  //   Disk usage on success
  //   INTERNAL_ERROR on IO error
  libtextclassifier3::StatusOr<int64_t> GetDiskUsage() const;

  // Returns the size of the elements held in the key mapper. This excludes the
  // size of any internal metadata of the key mapper, e.g. the key mapper's
  // header.
  //
  // Returns:
  //   File size on success
  //   INTERNAL_ERROR on IO error
  libtextclassifier3::StatusOr<int64_t> GetElementsSize() const;

  // Computes and returns the checksum of the header and contents.
  Crc32 ComputeChecksum();

 private:
  static constexpr char kKeyMapperDir[] = "key_mapper_dir";
  static constexpr char kKeyMapperPrefix[] = "key_mapper";

  // Use KeyMapper::Create() to instantiate.
  explicit KeyMapper(std::string_view key_mapper_dir);

  // Load any existing KeyMapper data from disk, or creates a new instance
  // of KeyMapper on disk and gets ready to process read/write operations.
  //
  // Returns any encountered IO errors.
  libtextclassifier3::Status Initialize(int maximum_size_bytes);

  const std::string file_prefix_;

  // TODO(adorokhine) Filesystem is a forked class that's available both in
  // icing and icing namespaces. We will need icing::Filesystem in order
  // to use IcingDynamicTrie. Filesystem class should be fully refactored
  // to have a single definition across both namespaces. Such a class should
  // use icing (and general google3) coding conventions and behave like
  // a proper C++ class.
  const IcingFilesystem icing_filesystem_;
  IcingDynamicTrie trie_;

  static_assert(std::is_trivially_copyable<T>::value,
                "T must be trivially copyable");
};

template <typename T>
libtextclassifier3::StatusOr<std::unique_ptr<KeyMapper<T>>>
KeyMapper<T>::Create(const Filesystem& filesystem, std::string_view base_dir,
                     int maximum_size_bytes) {
  // We create a subdirectory since the trie creates and stores multiple files.
  // This makes it easier to isolate the trie files away from other files that
  // could potentially be in the same base_dir, and makes it easier to delete.
  const std::string key_mapper_dir =
      absl_ports::StrCat(base_dir, "/", kKeyMapperDir);
  if (!filesystem.CreateDirectoryRecursively(key_mapper_dir.c_str())) {
    return absl_ports::InternalError(absl_ports::StrCat(
        "Failed to create KeyMapper directory: ", key_mapper_dir));
  }
  auto mapper = std::unique_ptr<KeyMapper<T>>(new KeyMapper<T>(key_mapper_dir));
  ICING_RETURN_IF_ERROR(mapper->Initialize(maximum_size_bytes));
  return mapper;
}

template <typename T>
libtextclassifier3::Status KeyMapper<T>::Delete(const Filesystem& filesystem,
                                                std::string_view base_dir) {
  std::string key_mapper_dir = absl_ports::StrCat(base_dir, "/", kKeyMapperDir);
  if (!filesystem.DeleteDirectoryRecursively(key_mapper_dir.c_str())) {
    return absl_ports::InternalError(absl_ports::StrCat(
        "Failed to delete KeyMapper directory: ", key_mapper_dir));
  }
  return libtextclassifier3::Status::OK;
}

template <typename T>
KeyMapper<T>::KeyMapper(std::string_view key_mapper_dir)
    : file_prefix_(absl_ports::StrCat(key_mapper_dir, "/", kKeyMapperPrefix)),
      trie_(file_prefix_,
            IcingDynamicTrie::RuntimeOptions().set_storage_policy(
                IcingDynamicTrie::RuntimeOptions::kMapSharedWithCrc),
            &icing_filesystem_) {}

template <typename T>
libtextclassifier3::Status KeyMapper<T>::Initialize(int maximum_size_bytes) {
  IcingDynamicTrie::Options options;
  // Divide the max space between the three internal arrays: nodes, nexts and
  // suffixes. MaxNodes and MaxNexts are in units of their own data structures.
  // MaxSuffixesSize is in units of bytes.
  options.max_nodes = maximum_size_bytes / (3 * sizeof(IcingDynamicTrie::Node));
  options.max_nexts = options.max_nodes;
  options.max_suffixes_size =
      sizeof(IcingDynamicTrie::Node) * options.max_nodes;
  options.value_size = sizeof(T);

  if (!trie_.CreateIfNotExist(options)) {
    return absl_ports::InternalError(
        absl_ports::StrCat("Failed to create KeyMapper file: ", file_prefix_));
  }
  if (!trie_.Init()) {
    return absl_ports::InternalError(
        absl_ports::StrCat("Failed to init KeyMapper file: ", file_prefix_));
  }
  return libtextclassifier3::Status::OK;
}

template <typename T>
libtextclassifier3::StatusOr<T> KeyMapper<T>::GetOrPut(std::string_view key,
                                                       T next_value) {
  std::string string_key(key);
  uint32_t value_index;
  if (!trie_.Insert(string_key.c_str(), &next_value, &value_index,
                    /*replace=*/false)) {
    return absl_ports::InternalError(absl_ports::StrCat(
        "Unable to insert key ", key, " into KeyMapper ", file_prefix_, "."));
  }
  // This memory address could be unaligned since we're just grabbing the value
  // from somewhere in the trie's suffix array. The suffix array is filled with
  // chars, so the address might not be aligned to T values.
  const T* unaligned_value =
      static_cast<const T*>(trie_.GetValueAtIndex(value_index));

  // memcpy the value to ensure that the returned value here is in a T-aligned
  // address
  T aligned_value;
  memcpy(&aligned_value, unaligned_value, sizeof(T));
  return aligned_value;
}

template <typename T>
libtextclassifier3::Status KeyMapper<T>::Put(std::string_view key, T value) {
  std::string string_key(key);
  if (!trie_.Insert(string_key.c_str(), &value)) {
    return absl_ports::InternalError(absl_ports::StrCat(
        "Unable to insert key ", key, " into KeyMapper ", file_prefix_, "."));
  }
  return libtextclassifier3::Status::OK;
}

template <typename T>
libtextclassifier3::StatusOr<T> KeyMapper<T>::Get(std::string_view key) const {
  std::string string_key(key);
  T value;
  if (!trie_.Find(string_key.c_str(), &value)) {
    return absl_ports::NotFoundError(absl_ports::StrCat(
        "Key not found ", key, " in KeyMapper ", file_prefix_, "."));
  }
  return value;
}

template <typename T>
bool KeyMapper<T>::Delete(std::string_view key) {
  return trie_.Delete(key);
}

template <typename T>
std::unordered_map<T, std::string> KeyMapper<T>::GetValuesToKeys() const {
  std::unordered_map<T, std::string> values_to_keys;
  for (IcingDynamicTrie::Iterator itr(trie_, /*prefix=*/""); itr.IsValid();
       itr.Advance()) {
    if (itr.IsValid()) {
      T value;
      memcpy(&value, itr.GetValue(), sizeof(T));
      values_to_keys.insert({value, itr.GetKey()});
    }
  }

  return values_to_keys;
}

template <typename T>
libtextclassifier3::Status KeyMapper<T>::PersistToDisk() {
  if (!trie_.Sync()) {
    return absl_ports::InternalError(
        absl_ports::StrCat("Failed to sync KeyMapper file: ", file_prefix_));
  }

  return libtextclassifier3::Status::OK;
}

template <typename T>
libtextclassifier3::StatusOr<int64_t> KeyMapper<T>::GetDiskUsage() const {
  int64_t size = trie_.GetDiskUsage();
  if (size == IcingFilesystem::kBadFileSize || size < 0) {
    return absl_ports::InternalError("Failed to get disk usage of key mapper");
  }
  return size;
}

template <typename T>
libtextclassifier3::StatusOr<int64_t> KeyMapper<T>::GetElementsSize() const {
  int64_t size = trie_.GetElementsSize();
  if (size == IcingFilesystem::kBadFileSize || size < 0) {
    return absl_ports::InternalError(
        "Failed to get disk usage of elements in the key mapper");
  }
  return size;
}

template <typename T>
Crc32 KeyMapper<T>::ComputeChecksum() {
  return Crc32(trie_.UpdateCrc());
}

}  // namespace lib
}  // namespace icing

#endif  // ICING_STORE_KEY_MAPPER_H_
