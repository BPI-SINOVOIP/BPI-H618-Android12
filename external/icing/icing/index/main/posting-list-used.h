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

#ifndef ICING_INDEX_MAIN_POSTING_LIST_USED_H_
#define ICING_INDEX_MAIN_POSTING_LIST_USED_H_

#include <string.h>
#include <sys/mman.h>

#include <algorithm>
#include <vector>

#include "icing/text_classifier/lib3/utils/base/status.h"
#include "icing/text_classifier/lib3/utils/base/statusor.h"
#include "icing/index/hit/hit.h"
#include "icing/index/main/posting-list-utils.h"
#include "icing/util/logging.h"

namespace icing {
namespace lib {

// A posting list with hits in it. Layout described in comments in
// posting-list-used.cc.
class PostingListUsed {
 public:
  // Creates a PostingListUsed that points to a buffer of size_in_bytes bytes.
  // 'Preexisting' means that posting_list_buffer was previously modified by
  // another instance of PostingListUsed.
  //
  // Caller owns the hits buffer and must not free it while using a
  // PostingListUsed.
  //
  // RETURNS:
  //   - A valid PostingListUsed if successful
  //   - INVALID_ARGUMENT if size_in_bytes < min_posting_list_size()
  //       || size_in_bytes % sizeof(Hit) != 0.
  //   - FAILED_PRECONDITION if posting_list_buffer is null
  static libtextclassifier3::StatusOr<PostingListUsed>
  CreateFromPreexistingPostingListUsedRegion(void *posting_list_buffer,
                                             uint32_t size_in_bytes);

  // Creates a PostingListUsed that points to a buffer of size_in_bytes bytes
  // and initializes the content of the buffer so that the returned
  // PostingListUsed is empty.
  //
  // Caller owns the posting_list_buffer buffer and must not free it while using
  // a PostingListUsed.
  //
  // RETURNS:
  //   - A valid PostingListUsed if successful
  //   - INVALID_ARGUMENT if size_in_bytes < min_posting_list_size()
  //       || size_in_bytes % sizeof(Hit) != 0.
  //   - FAILED_PRECONDITION if posting_list_buffer is null
  static libtextclassifier3::StatusOr<PostingListUsed>
  CreateFromUnitializedRegion(void *posting_list_buffer,
                              uint32_t size_in_bytes);

  // Move contents from another posting list. Clears other.
  //
  // RETURNS:
  //   - OK, if successful
  //   - INVALID_ARGUMENT if 'other' is not valid or 'other' is too large to fit
  //       in 'this'.
  //   - FAILED_PRECONDITION if 'this' posting list is in a corrupted state.
  libtextclassifier3::Status MoveFrom(PostingListUsed *other);

  // Min size of posting list that can fit these used bytes. (See
  // MoveFrom.)
  uint32_t MinPostingListSizeToFit() const;

  // Prepend a hit to the posting list.
  // RETURNS:
  //   - INVALID_ARGUMENT if !hit.is_valid() or if hit is not less than the
  //   previously added hit.
  //   - RESOURCE_EXHAUSTED if there is no more room to add hit to the posting
  //   list.
  libtextclassifier3::Status PrependHit(const Hit &hit);

  // Prepend hits to the posting list. Hits should be sorted in
  // descending order (as defined by the less than operator for Hit)
  //
  // Returns the number of hits that could be prepended to the posting list. If
  // keep_prepended is true, whatever could be prepended is kept, otherwise the
  // posting list is left in its original state.
  template <class T, Hit (*GetHit)(const T &)>
  uint32_t PrependHitArray(const T *array, uint32_t num_hits,
                           bool keep_prepended);

  // Retrieves the hits stored in the posting list.
  //
  // RETURNS:
  //   - On success, a vector of hits sorted by the reverse order of prepending.
  //   - INTERNAL_ERROR if the posting list has been corrupted somehow.
  libtextclassifier3::StatusOr<std::vector<Hit>> GetHits() const;

  // Same as GetHits but appends hits to hits_out.
  //
  // RETURNS:
  //   - On success, a vector of hits sorted by the reverse order of prepending.
  //   - INTERNAL_ERROR if the posting list has been corrupted somehow.
  libtextclassifier3::Status GetHits(std::vector<Hit> *hits_out) const;

  // Undo the last num_hits hits prepended. If num_hits > number of
  // hits we clear all hits.
  //
  // RETURNS:
  //   - OK on success
  //   - INTERNAL_ERROR if the posting list has been corrupted somehow.
  libtextclassifier3::Status PopFrontHits(uint32_t num_hits);

  // Returns bytes used by actual hits.
  uint32_t BytesUsed() const;

 private:
  // Posting list layout formats:
  //
  // not_full
  //
  // +-----------------+----------------+-------+-----------------+
  // |hits-start-offset|Hit::kInvalidVal|xxxxxxx|(compressed) hits|
  // +-----------------+----------------+-------+-----------------+
  //
  // almost_full
  //
  // +-----------------+----------------+-------+-----------------+
  // |Hit::kInvalidVal |1st hit         |(pad)  |(compressed) hits|
  // +-----------------+----------------+-------+-----------------+
  //
  // full()
  //
  // +-----------------+----------------+-------+-----------------+
  // |1st hit          |2nd hit         |(pad)  |(compressed) hits|
  // +-----------------+----------------+-------+-----------------+
  //
  // The first two uncompressed hits also implicitly encode information about
  // the size of the compressed hits region.
  //
  // 1. If the posting list is NOT_FULL, then
  // posting_list_buffer_[0] contains the byte offset of the start of the
  // compressed hits - and, thus, the size of the compressed hits region is
  // size_in_bytes - posting_list_buffer_[0].
  //
  // 2. If posting list is ALMOST_FULL or FULL, then the compressed hits region
  // starts somewhere between [kSpecialHitsSize, kSpecialHitsSize + sizeof(Hit)
  // - 1] and ends at size_in_bytes - 1.
  //
  // Hit term frequencies are stored after the hit value, compressed or
  // uncompressed. For the first two special hits, we always have a
  // space for the term frequency. For hits in the compressed area, we only have
  // the term frequency following the hit value of hit.has_term_frequency() is
  // true. This allows good compression in the common case where hits don't have
  // a valid term frequency.
  //
  // EXAMPLE
  // Posting list storage. Posting list size: 20 bytes
  // EMPTY!
  // +--bytes 0-4--+----- 5-9 ------+---------------- 10-19 -----------------+
  // |     20      |Hit::kInvalidVal|                 0x000                  |
  // +-------------+----------------+----------------+-----------------------+
  //
  // Add Hit 0x07FFF998 (DocumentId = 12, SectionId = 3, Flags = 0)
  // NOT FULL!
  // +--bytes 0-4--+----- 5-9 ------+----- 10-15 -----+-------- 16-19 -------+
  // |     16      |Hit::kInvalidVal|      0x000      |       0x07FFF998     |
  // +-------------+----------------+-----------------+----------------------+
  //
  // Add Hit 0x07FFF684 (DocumentId = 18, SectionId = 0, Flags = 4,
  // TermFrequency=125)
  // (Hit 0x07FFF998 - Hit 0x07FFF684 = 788)
  // +--bytes 0-4--+----- 5-9 ------+-- 10-12 --+-- 13-16 --+- 17 -+-- 18-19 --+
  // |      13     |Hit::kInvalidVal|   0x000   | 0x07FFF684| 125  |    788    |
  // +-------------+----------------+-----------+-----------+------+-----------+
  //
  // Add Hit 0x07FFF4D2 (DocumentId = 22, SectionId = 10, Flags = 2)
  // (Hit 0x07FFF684 - Hit 0x07FFF4D2 = 434)
  // +--bytes 0-4--+--- 5-9 ----+-- 10 --+-- 11-14 -+- 15-16 -+- 17 -+- 18-19 -+
  // |      9      |Hit::kInvVal|  0x00  |0x07FFF4D2|   434   | 125  |   788   |
  // +-------------+------------+--------+----------+---------+------+---------+
  //
  // Add Hit 0x07FFF40E (DocumentId = 23, SectionId = 1, Flags = 6,
  // TermFrequency = 87)
  // (Hit 0x07FFF684 - Hit 0x07FFF4D2 = 196) ALMOST FULL!
  // +--bytes 0-4-+---- 5-9 ----+- 10-12 -+- 13-14 -+- 15-16 -+- 17 -+- 18-19 -+
  // |Hit::kInvVal|0x07FFF40E,87|  0x000  |    196  |   434   |  125 |   788   |
  // +-------------+------------+---------+---------+---------+------+---------+
  //
  // Add Hit 0x07FFF320 (DocumentId = 27, SectionId = 4, Flags = 0)
  // FULL!
  // +--bytes 0-4--+---- 5-9 ----+- 10-13 -+-- 14-15 -+- 16-17 -+- 18 -+- 19-20
  // -+ | 0x07FFF320  |0x07FFF40E,87|  0x000  |    196   |   434   |  125 | 788
  // |
  // +-------------+-------------+---------+----------+---------+------+---------+
  PostingListUsed(void *posting_list_buffer, uint32_t size_in_bytes)
      : posting_list_buffer_(static_cast<uint8_t *>(posting_list_buffer)),
        size_in_bytes_(size_in_bytes) {}

  // Helpers to determine what state the posting list is in.
  bool full() const {
    return get_special_hit(0).ValueOrDie().is_valid() &&
           get_special_hit(1).ValueOrDie().is_valid();
  }
  bool almost_full() const {
    return !get_special_hit(0).ValueOrDie().is_valid();
  }
  bool empty() const {
    return get_special_hit(0).ValueOrDie().value() == size_in_bytes_ &&
           !get_special_hit(1).ValueOrDie().is_valid();
  }

  // Returns false if both special hits are invalid or if the offset value
  // stored in the special hit is less than kSpecialHitsSize or greater than
  // size_in_bytes_. Returns true, otherwise.
  bool IsPostingListValid() const;

  // Prepend hit to a posting list that is in the ALMOST_FULL state.
  // RETURNS:
  //  - OK, if successful
  //  - INVALID_ARGUMENT if hit is not less than the previously added hit.
  libtextclassifier3::Status PrependHitToAlmostFull(const Hit &hit);

  // Prepend hit to a posting list that is in the EMPTY state. This will always
  // succeed because there are no pre-existing hits and no validly constructed
  // posting list could fail to fit one hit.
  void PrependHitToEmpty(const Hit &hit);

  // Prepend hit to a posting list that is in the NOT_FULL state.
  // RETURNS:
  //  - OK, if successful
  //  - INVALID_ARGUMENT if hit is not less than the previously added hit.
  libtextclassifier3::Status PrependHitToNotFull(const Hit &hit,
                                                 uint32_t offset);

  // Reset contents to an empty posting list. This *must* be called if the
  // posting_list_buffer_ region is uninitialized.
  void Clear();

  // Returns either 0 (full state), sizeof(Hit) (almost_full state) or
  // a byte offset between kSpecialHitsSize and size_in_bytes_ (inclusive)
  // (not_full state).
  uint32_t get_start_byte_offset() const;

  // Sets the special hits to properly reflect what offset is (see layout
  // comment for further details).
  //
  // Returns false if offset > size_in_bytes_ or offset is (kSpecialHitsSize,
  // sizeof(Hit)) or offset is (sizeof(Hit), 0). True, otherwise.
  bool set_start_byte_offset(uint32_t offset);

  // Manipulate padded areas. We never store the same hit value twice
  // so a delta of 0 is a pad byte.

  // Returns offset of first non-pad byte.
  uint32_t GetPadEnd(uint32_t offset) const;

  // Fill padding between offset start and offset end with 0s.
  // Returns false if end > size_in_bytes_. True, otherwise.
  bool PadToEnd(uint32_t start, uint32_t end);

  // Helper for AppendHits/PopFrontHits. Adds limit number of hits to out or all
  // hits in the posting list if the posting list contains less than limit
  // number of hits. out can be NULL.
  //
  // NOTE: If called with limit=1, pop=true on a posting list that transitioned
  // from NOT_FULL directly to FULL, GetHitsInternal will not return the posting
  // list to NOT_FULL. Instead it will leave it in a valid state, but it will be
  // ALMOST_FULL.
  //
  // RETURNS:
  //   - OK on success
  //   - INTERNAL_ERROR if the posting list has been corrupted somehow.
  libtextclassifier3::Status GetHitsInternal(uint32_t limit, bool pop,
                                             std::vector<Hit> *out) const;

  // Retrieves the value stored in the index-th special hit.
  //
  // RETURNS:
  //   - A valid Hit, on success
  //   - INVALID_ARGUMENT if index is not less than kNumSpecialHits
  libtextclassifier3::StatusOr<Hit> get_special_hit(uint32_t index) const;

  // Sets the value stored in the index-th special hit to val. If index is not
  // less than kSpecialHitSize / sizeof(Hit), this has no effect.
  bool set_special_hit(uint32_t index, const Hit &val);

  // Prepends hit to the memory region [offset - sizeof(Hit), offset] and
  // returns the new beginning of the padded region.
  //
  // RETURNS:
  //   - The new beginning of the padded region, if successful.
  //   - INVALID_ARGUMENT if hit will not fit (uncompressed) between offset and
  // kSpecialHitsSize
  libtextclassifier3::StatusOr<uint32_t> PrependHitUncompressed(
      const Hit &hit, uint32_t offset);

  // If hit has a term frequency, consumes the term frequency at offset, updates
  // hit to include the term frequency and updates offset to reflect that the
  // term frequency has been consumed.
  //
  // RETURNS:
  //   - OK, if successful
  //   - INVALID_ARGUMENT if hit has a term frequency and offset +
  //   sizeof(Hit::TermFrequency) >=
  //     size_in_bytes_
  libtextclassifier3::Status ConsumeTermFrequencyIfPresent(
      Hit *hit, uint32_t *offset) const;

  // A byte array of size size_in_bytes_ containing encoded hits for this
  // posting list.
  uint8_t *posting_list_buffer_;  // does not own!
  uint32_t size_in_bytes_;
};

// Inlined functions. Implementation details below. Avert eyes!
template <class T, Hit (*GetHit)(const T &)>
uint32_t PostingListUsed::PrependHitArray(const T *array, uint32_t num_hits,
                                          bool keep_prepended) {
  if (!IsPostingListValid()) {
    return 0;
  }

  // Prepend hits working backwards from array[num_hits - 1].
  uint32_t i;
  for (i = 0; i < num_hits; ++i) {
    if (!PrependHit(GetHit(array[num_hits - i - 1])).ok()) {
      break;
    }
  }
  if (i != num_hits && !keep_prepended) {
    // Didn't fit. Undo everything and check that we have the same offset as
    // before. PopFrontHits guarantees that it will remove all 'i' hits so long
    // as there are at least 'i' hits in the posting list, which we know there
    // are.
    PopFrontHits(i);
  }
  return i;
}

}  // namespace lib
}  // namespace icing

#endif  // ICING_INDEX_MAIN_POSTING_LIST_USED_H_
