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

// A small index with continuous updates (doesn't need explicit Flush
// to persiste) but has more possibility for corruption. It can always
// detect corruption reliably.

#ifndef ICING_INDEX_LITE_INDEX_H_
#define ICING_INDEX_LITE_INDEX_H_

#include <cstdint>
#include <limits>
#include <memory>
#include <string>
#include <vector>

#include "icing/text_classifier/lib3/utils/base/status.h"
#include "icing/text_classifier/lib3/utils/base/statusor.h"
#include "icing/file/filesystem.h"
#include "icing/index/hit/doc-hit-info.h"
#include "icing/index/hit/hit.h"
#include "icing/index/lite/term-id-hit-pair.h"
#include "icing/legacy/index/icing-array-storage.h"
#include "icing/legacy/index/icing-dynamic-trie.h"
#include "icing/legacy/index/icing-filesystem.h"
#include "icing/legacy/index/icing-lite-index-header.h"
#include "icing/legacy/index/icing-lite-index-options.h"
#include "icing/legacy/index/icing-mmapper.h"
#include "icing/proto/storage.pb.h"
#include "icing/proto/term.pb.h"
#include "icing/schema/section.h"
#include "icing/store/document-id.h"
#include "icing/store/namespace-id.h"
#include "icing/util/bit-util.h"
#include "icing/util/crc32.h"

namespace icing {
namespace lib {

class LiteIndex {
 public:
  // An entry in the hit buffer.
  using Options = IcingLiteIndexOptions;

  // Updates checksum of subcomponents.
  ~LiteIndex();

  // Creates lite index from storage. The files will be created if they do not
  // already exist.
  //
  // Returns:
  //  OK on success
  //  DATA_LOSS if the index was corrupted and cleared
  //  INTERNAL on I/O error
  static libtextclassifier3::StatusOr<std::unique_ptr<LiteIndex>> Create(
      const Options& options, const IcingFilesystem* filesystem);

  // Resets all internal members of the index. Returns OK if all operations were
  // successful.
  libtextclassifier3::Status Reset();

  // Advises the OS to cache pages in the index, which will be accessed for a
  // query soon.
  void Warm();

  // Syncs all modified files in the index to disk.
  //
  // Returns:
  //   OK on success
  //   INTERNAL on I/O error
  libtextclassifier3::Status PersistToDisk();

  // Calculate the checksum of all sub-components of the LiteIndex
  Crc32 ComputeChecksum();

  // Returns term_id if term found, NOT_FOUND otherwise.
  libtextclassifier3::StatusOr<uint32_t> GetTermId(
      const std::string& term) const;

  // Returns an iterator for all terms for which 'prefix' is a prefix.
  class PrefixIterator {
   public:
    explicit PrefixIterator(const IcingDynamicTrie::Iterator& delegate)
        : delegate_(delegate) {}
    bool IsValid() const { return delegate_.IsValid(); }

    void Advance() { delegate_.Advance(); }

    const char* GetKey() const { return delegate_.GetKey(); }

    uint32_t GetValueIndex() const { return delegate_.GetValueIndex(); }

   private:
    IcingDynamicTrie::Iterator delegate_;
  };

  PrefixIterator FindTermPrefixes(const std::string& prefix) const {
    return PrefixIterator(IcingDynamicTrie::Iterator(lexicon_, prefix.c_str()));
  }

  // Inserts a term with its properties.
  //
  // Returns:
  //   A value index on success
  //   RESOURCE_EXHAUSTED if lexicon is full or no disk space is available
  libtextclassifier3::StatusOr<uint32_t> InsertTerm(
      const std::string& term, TermMatchType::Code term_match_type,
      NamespaceId namespace_id);

  // Updates term properties by setting hasPrefixHits and namespace id of the
  // term.
  //
  // Returns:
  //   OK on success
  //   RESOURCE_EXHAUSTED if no disk space is available
  libtextclassifier3::Status UpdateTermProperties(uint32_t tvi,
                                                  bool hasPrefixHits,
                                                  NamespaceId namespace_id);

  // Append hit to buffer. term_id must be encoded using the same term_id_codec
  // supplied to the index constructor.
  // RETURNS:
  //  - OK if hit was successfully added
  //  - RESOURCE_EXHAUSTED if hit could not be added (either due to hit buffer
  //    or file system capacity reached).
  libtextclassifier3::Status AddHit(uint32_t term_id, const Hit& hit);

  // Add all hits with term_id from the sections specified in section_id_mask,
  // skipping hits in non-prefix sections if only_from_prefix_sections is true,
  // to hits_out. If hits_out is nullptr, no hits will be added.
  //
  // Returns the number of hits that would be added to hits_out.
  int AppendHits(uint32_t term_id, SectionIdMask section_id_mask,
                 bool only_from_prefix_sections,
                 std::vector<DocHitInfo>* hits_out);

  // Returns the hit count of the term.
  int CountHits(uint32_t term_id);

  // Check if buffer has reached its capacity.
  bool is_full() const;

  bool empty() const { return size() == 0; }

  uint32_t size() const { return header_->cur_size(); }

  bool WantsMerge() const {
    return size() >= (options_.hit_buffer_want_merge_bytes /
                      sizeof(TermIdHitPair::Value));
  }

  class const_iterator {
    friend class LiteIndex;

   public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = TermIdHitPair;
    using reference = const value_type&;
    using pointer = const value_type*;

    const_iterator() : const_iterator(nullptr, -1, -1) {}

    reference operator*() const { return start_[position_]; }

    pointer operator->() const { return start_ + position_; }

    const_iterator& operator++() {
      if (++position_ >= end_position_) {
        start_ = nullptr;
        position_ = -1;
        end_position_ = -1;
      }
      return *this;
    }

    const_iterator operator++(int) {
      auto tmp = *this;
      ++*this;
      return tmp;
    }

    bool operator!=(const const_iterator& rhs) { return !(*this == rhs); }

    bool operator==(const const_iterator& rhs) {
      return start_ == rhs.start_ && position_ == rhs.position_;
    }

   private:
    explicit const_iterator(const TermIdHitPair* start, int position,
                            int end_position)
        : start_(start), position_(position), end_position_(end_position) {}

    const TermIdHitPair* start_;
    int position_;
    int end_position_;
  };

  const_iterator begin() const {
    // If the LiteIndex is empty, just return end().
    return empty() ? end()
                   : const_iterator(hit_buffer_.array_cast<TermIdHitPair>(), 0,
                                    header_->cur_size());
  }

  const_iterator end() const { return const_iterator(); }

  constexpr static uint32_t max_hit_buffer_size() {
    return std::numeric_limits<uint32_t>::max() / sizeof(TermIdHitPair);
  }

  // We keep track of the last added document_id. This is always the largest
  // document_id that has been added because hits can only be added in order of
  // increasing document_id.
  DocumentId last_added_document_id() const {
    return header_->last_added_docid();
  }
  void set_last_added_document_id(DocumentId document_id) const {
    header_->set_last_added_docid(document_id);
  }

  const IcingDynamicTrie& lexicon() const { return lexicon_; }

  // Returns debug information for the index in out.
  // verbosity <= 0, simplest debug information - size of lexicon, hit buffer
  // verbosity > 0, more detailed debug information from the lexicon.
  void GetDebugInfo(int verbosity, std::string* out) const;

  // Returns the byte size of all the elements held in the index. This excludes
  // the size of any internal metadata of the index, e.g. the index's header.
  //
  // Returns:
  //   Byte size on success
  //   INTERNAL_ERROR on IO error
  libtextclassifier3::StatusOr<int64_t> GetElementsSize() const;

  // Takes the provided storage_info, populates the fields related to the lite
  // index and returns that storage_info.
  //
  // If an IO error occurs while trying to calculate the value for a field, then
  // that field will be set to -1.
  IndexStorageInfoProto GetStorageInfo(
      IndexStorageInfoProto storage_info) const;

 private:
  static IcingDynamicTrie::RuntimeOptions MakeTrieRuntimeOptions();

  LiteIndex(const Options& options, const IcingFilesystem* filesystem);

  // Initializes lite index from storage. Must be called exactly once after
  // object construction.
  //
  // Returns:
  //  OK on success
  //  DATA_LOSS if the index was corrupted and cleared
  //  INTERNAL on I/O error
  libtextclassifier3::Status Initialize();

  bool initialized() const { return header_ != nullptr; }

  // Sets the computed checksum in the header
  void UpdateChecksum();

  // Returns the position of the first element with term_id, or the size of the
  // hit buffer if term_id is not present.
  uint32_t Seek(uint32_t term_id);

  // File descriptor that points to where the header and hit buffer are written
  // to.
  ScopedFd hit_buffer_fd_;

  // Mmapped region past the header that stores the hits.
  IcingArrayStorage hit_buffer_;

  // Crc checksum of the hits, excludes the header.
  uint32_t hit_buffer_crc_;

  // Trie that maps indexed terms to their term id
  IcingDynamicTrie lexicon_;

  // TODO(b/140437260): Port over to MemoryMappedFile
  // Memory mapped region of the underlying file that reflects the header.
  IcingMMapper header_mmap_;

  // Wrapper around the mmapped header that contains stats on the lite index.
  std::unique_ptr<IcingLiteIndex_Header> header_;

  // Options used to initialize the LiteIndex.
  const Options options_;

  // TODO(b/139087650) Move to icing::Filesystem
  const IcingFilesystem* const filesystem_;
};

}  // namespace lib
}  // namespace icing

#endif  // ICING_INDEX_LITE_INDEX_H_
