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

#ifndef ICING_INDEX_ITERATOR_DOC_HIT_INFO_ITERATOR_TEST_UTIL_H_
#define ICING_INDEX_ITERATOR_DOC_HIT_INFO_ITERATOR_TEST_UTIL_H_

#include <string>
#include <utility>
#include <vector>

#include "icing/text_classifier/lib3/utils/base/status.h"
#include "icing/absl_ports/canonical_errors.h"
#include "icing/absl_ports/str_cat.h"
#include "icing/index/hit/doc-hit-info.h"
#include "icing/index/iterator/doc-hit-info-iterator.h"
#include "icing/legacy/core/icing-string-util.h"
#include "icing/schema/section.h"
#include "icing/store/document-id.h"

namespace icing {
namespace lib {

// Dummy class to help with testing. It starts with an kInvalidDocumentId doc
// hit info until an Advance is called (like normal DocHitInfoIterators). It
// will then proceed to return the doc_hit_infos in order as Advance's are
// called. After all doc_hit_infos are returned, Advance will return a NotFound
// error (also like normal DocHitInfoIterators).
class DocHitInfoIteratorDummy : public DocHitInfoIterator {
 public:
  DocHitInfoIteratorDummy() = default;
  explicit DocHitInfoIteratorDummy(std::vector<DocHitInfo> doc_hit_infos,
                                   std::string term = "")
      : doc_hit_infos_(std::move(doc_hit_infos)), term_(std::move(term)) {}

  libtextclassifier3::Status Advance() override {
    if (index_ < doc_hit_infos_.size()) {
      doc_hit_info_ = doc_hit_infos_.at(index_);
      index_++;
      return libtextclassifier3::Status::OK;
    }

    return absl_ports::ResourceExhaustedError(
        "No more DocHitInfos in iterator");
  }

  // Imitates behavior of DocHitInfoIteratorTermMain/DocHitInfoIteratorTermLite
  void PopulateMatchedTermsStats(
      std::vector<TermMatchInfo>* matched_terms_stats,
      SectionIdMask filtering_section_mask = kSectionIdMaskAll) const override {
    if (doc_hit_info_.document_id() == kInvalidDocumentId) {
      // Current hit isn't valid, return.
      return;
    }
    SectionIdMask section_mask =
        doc_hit_info_.hit_section_ids_mask() & filtering_section_mask;
    SectionIdMask section_mask_copy = section_mask;
    std::array<Hit::TermFrequency, kMaxSectionId> section_term_frequencies = {
        Hit::kNoTermFrequency};
    while (section_mask_copy) {
      SectionId section_id = __builtin_ctz(section_mask_copy);
      section_term_frequencies.at(section_id) =
          doc_hit_info_.hit_term_frequency(section_id);
      section_mask_copy &= ~(1u << section_id);
    }
    TermMatchInfo term_stats(term_, section_mask,
                             std::move(section_term_frequencies));

    for (auto& cur_term_stats : *matched_terms_stats) {
      if (cur_term_stats.term == term_stats.term) {
        // Same docId and same term, we don't need to add the term and the term
        // frequency should always be the same
        return;
      }
    }
    matched_terms_stats->push_back(term_stats);
  }

  void set_hit_intersect_section_ids_mask(
      SectionIdMask hit_intersect_section_ids_mask) {
    hit_intersect_section_ids_mask_ = hit_intersect_section_ids_mask;
  }

  int32_t GetNumBlocksInspected() const override {
    return num_blocks_inspected_;
  }

  void SetNumBlocksInspected(int32_t num_blocks_inspected) {
    num_blocks_inspected_ = num_blocks_inspected;
  }

  int32_t GetNumLeafAdvanceCalls() const override {
    return num_leaf_advance_calls_;
  }

  void SetNumLeafAdvanceCalls(int32_t num_leaf_advance_calls) {
    num_leaf_advance_calls_ = num_leaf_advance_calls;
  }

  std::string ToString() const override {
    std::string ret = "<";
    for (auto& doc_hit_info : doc_hit_infos_) {
      absl_ports::StrAppend(&ret, IcingStringUtil::StringPrintf(
                                      "[%d,%d]", doc_hit_info.document_id(),
                                      doc_hit_info.hit_section_ids_mask()));
    }
    absl_ports::StrAppend(&ret, ">");
    return ret;
  }

 private:
  int32_t index_ = 0;
  int32_t num_blocks_inspected_ = 0;
  int32_t num_leaf_advance_calls_ = 0;
  std::vector<DocHitInfo> doc_hit_infos_;
  std::string term_;
};

inline std::vector<DocumentId> GetDocumentIds(DocHitInfoIterator* iterator) {
  std::vector<DocumentId> ids;
  while (iterator->Advance().ok()) {
    ids.push_back(iterator->doc_hit_info().document_id());
  }
  return ids;
}

inline std::vector<DocHitInfo> GetDocHitInfos(DocHitInfoIterator* iterator) {
  std::vector<DocHitInfo> doc_hit_infos;
  while (iterator->Advance().ok()) {
    doc_hit_infos.push_back(iterator->doc_hit_info());
  }
  return doc_hit_infos;
}

}  // namespace lib
}  // namespace icing

#endif  // ICING_INDEX_ITERATOR_DOC_HIT_INFO_ITERATOR_TEST_UTIL_H_
