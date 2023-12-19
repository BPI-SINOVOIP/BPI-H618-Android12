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

#ifndef ICING_INDEX_ITERATOR_DOC_HIT_INFO_ITERATOR_H_
#define ICING_INDEX_ITERATOR_DOC_HIT_INFO_ITERATOR_H_

#include <array>
#include <cstdint>
#include <string>
#include <string_view>

#include "icing/text_classifier/lib3/utils/base/status.h"
#include "icing/text_classifier/lib3/utils/base/statusor.h"
#include "icing/absl_ports/canonical_errors.h"
#include "icing/index/hit/doc-hit-info.h"
#include "icing/schema/section.h"
#include "icing/store/document-id.h"

namespace icing {
namespace lib {

// Data structure that maps a single matched query term to its section mask
// and the list of term frequencies.
// TODO(b/158603837): add stat on whether the matched terms are prefix matched
// or not. This information will be used to boost exact match.
struct TermMatchInfo {
  std::string_view term;
  // SectionIdMask associated to the term.
  SectionIdMask section_ids_mask;
  // Array with fixed size kMaxSectionId. For every section id, i.e.
  // vector index, it stores the term frequency of the term.
  std::array<Hit::TermFrequency, kMaxSectionId> term_frequencies;

  explicit TermMatchInfo(
      std::string_view term, SectionIdMask section_ids_mask,
      std::array<Hit::TermFrequency, kMaxSectionId> term_frequencies)
      : term(term),
        section_ids_mask(section_ids_mask),
        term_frequencies(std::move(term_frequencies)) {}
};

// Iterator over DocHitInfos (collapsed Hits) in REVERSE document_id order.
//
// NOTE: You must call Advance() before calling hit_info() or
// hit_intersect_section_ids_mask().
//
// Example:
// DocHitInfoIterator itr = GetIterator(...);
// while (itr.Advance()) {
//   HandleDocHitInfo(itr.hit_info());
// }
class DocHitInfoIterator {
 public:
  virtual ~DocHitInfoIterator() = default;

  // Returns:
  //   OK if was able to advance to a new document_id.
  //   INVALID_ARGUMENT if there are less than 2 iterators for an AND/OR
  //       iterator
  //   RESOUCE_EXHAUSTED if we've run out of document_ids to iterate over
  virtual libtextclassifier3::Status Advance() = 0;

  // Returns the DocHitInfo that the iterator is currently at. The DocHitInfo
  // will have a kInvalidDocumentId if Advance() was not called after
  // construction or if Advance returned an error.
  const DocHitInfo& doc_hit_info() const { return doc_hit_info_; }

  // SectionIdMask representing which sections (if any) have matched *ALL* query
  // terms for the current document_id.
  SectionIdMask hit_intersect_section_ids_mask() const {
    return hit_intersect_section_ids_mask_;
  }

  // Gets the number of flash index blocks that have been read as a
  // result of operations on this object.
  virtual int32_t GetNumBlocksInspected() const = 0;

  // HitIterators may be constructed into trees. Internal nodes will return the
  // sum of the number of Advance() calls to all leaf nodes. Leaf nodes will
  // return the number of times Advance() was called on it.
  virtual int32_t GetNumLeafAdvanceCalls() const = 0;

  // A string representing the iterator.
  virtual std::string ToString() const = 0;

  // For the last hit docid, retrieves all the matched query terms and other
  // stats, see TermMatchInfo.
  // filtering_section_mask filters the matching sections and should be set only
  // by DocHitInfoIteratorSectionRestrict.
  // If Advance() wasn't called after construction, Advance() returned false or
  // the concrete HitIterator didn't override this method, the vectors aren't
  // populated.
  virtual void PopulateMatchedTermsStats(
      std::vector<TermMatchInfo>* matched_terms_stats,
      SectionIdMask filtering_section_mask = kSectionIdMaskAll) const {}

 protected:
  DocHitInfo doc_hit_info_;
  SectionIdMask hit_intersect_section_ids_mask_ = kSectionIdMaskNone;

  // Helper function to advance the given iterator to at most the given
  // document_id.
  libtextclassifier3::StatusOr<DocumentId> AdvanceTo(DocHitInfoIterator* it,
                                                     DocumentId document_id) {
    while (it->Advance().ok()) {
      if (it->doc_hit_info().document_id() <= document_id) {
        return it->doc_hit_info().document_id();
      }
    }

    // Didn't find anything for the other iterator, reset to invalid values and
    // return.
    doc_hit_info_ = DocHitInfo(kInvalidDocumentId);
    hit_intersect_section_ids_mask_ = kSectionIdMaskNone;
    return absl_ports::ResourceExhaustedError(
        "No more DocHitInfos in iterator");
  }
};  // namespace DocHitInfoIterator

}  // namespace lib
}  // namespace icing

#endif  // ICING_INDEX_ITERATOR_DOC_HIT_INFO_ITERATOR_H_
