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

#include "icing/index/main/doc-hit-info-iterator-term-main.h"

#include <cstdint>
#include <memory>

#include "icing/text_classifier/lib3/utils/base/status.h"
#include "icing/absl_ports/canonical_errors.h"
#include "icing/absl_ports/str_cat.h"
#include "icing/index/hit/doc-hit-info.h"
#include "icing/index/main/posting-list-accessor.h"
#include "icing/index/main/posting-list-identifier.h"
#include "icing/legacy/core/icing-string-util.h"
#include "icing/schema/section.h"
#include "icing/store/document-id.h"
#include "icing/util/status-macros.h"

namespace icing {
namespace lib {

namespace {

std::string SectionIdMaskToString(SectionIdMask section_id_mask) {
  std::string mask(kMaxSectionId + 1, '0');
  for (SectionId i = kMaxSectionId; i >= 0; --i) {
    if (section_id_mask & (1U << i)) {
      mask[kMaxSectionId - i] = '1';
    }
  }
  return mask;
}

}  // namespace

libtextclassifier3::Status DocHitInfoIteratorTermMain::Advance() {
  if (posting_list_accessor_ == nullptr ||
      cached_doc_hit_infos_idx_ == (cached_doc_hit_infos_.size() - 2)) {
    // If we haven't retrieved any hits before or we've already returned all but
    // the last cached hit, then go get some more!
    // We hold back the last cached hit because it could have more hits on the
    // next posting list in the chain.
    libtextclassifier3::Status status = RetrieveMoreHits();
    if (!status.ok()) {
      if (!absl_ports::IsNotFound(status)) {
        // NOT_FOUND is expected to happen (not every term will be in the main
        // index!). Other errors are worth logging.
        ICING_LOG(ERROR) << "Failed to retrieve more hits "
                         << status.error_message();
      }
      return absl_ports::ResourceExhaustedError(
          "No more DocHitInfos in iterator");
    }
  } else {
    ++cached_doc_hit_infos_idx_;
  }
  if (cached_doc_hit_infos_idx_ == -1 ||
      cached_doc_hit_infos_idx_ >= cached_doc_hit_infos_.size()) {
    // Nothing more for the iterator to return. Set these members to invalid
    // values.
    doc_hit_info_ = DocHitInfo();
    hit_intersect_section_ids_mask_ = kSectionIdMaskNone;
    return absl_ports::ResourceExhaustedError(
        "No more DocHitInfos in iterator");
  }
  doc_hit_info_ = cached_doc_hit_infos_.at(cached_doc_hit_infos_idx_);
  hit_intersect_section_ids_mask_ = doc_hit_info_.hit_section_ids_mask();
  return libtextclassifier3::Status::OK;
}

libtextclassifier3::Status DocHitInfoIteratorTermMainExact::RetrieveMoreHits() {
  DocHitInfo last_doc_hit_info;
  if (!cached_doc_hit_infos_.empty()) {
    last_doc_hit_info = cached_doc_hit_infos_.back();
  }
  cached_doc_hit_infos_idx_ = 0;
  cached_doc_hit_infos_.clear();
  if (last_doc_hit_info.document_id() != kInvalidDocumentId) {
    // Carry over the last hit. It might need to be merged with the first hit of
    // of the next posting list in the chain.
    cached_doc_hit_infos_.push_back(last_doc_hit_info);
  }
  if (posting_list_accessor_ == nullptr) {
    ICING_ASSIGN_OR_RETURN(posting_list_accessor_,
                           main_index_->GetAccessorForExactTerm(term_));
  }

  ICING_ASSIGN_OR_RETURN(std::vector<Hit> hits,
                         posting_list_accessor_->GetNextHitsBatch());
  ++num_blocks_inspected_;
  cached_doc_hit_infos_.reserve(hits.size() + 1);
  for (const Hit& hit : hits) {
    // Check sections.
    if (((1u << hit.section_id()) & section_restrict_mask_) == 0) {
      continue;
    }
    // We want exact hits, skip prefix-only hits.
    if (hit.is_prefix_hit()) {
      continue;
    }
    if (cached_doc_hit_infos_.empty() ||
        hit.document_id() != cached_doc_hit_infos_.back().document_id()) {
      cached_doc_hit_infos_.push_back(DocHitInfo(hit.document_id()));
    }
    cached_doc_hit_infos_.back().UpdateSection(hit.section_id(),
                                               hit.term_frequency());
  }
  return libtextclassifier3::Status::OK;
}

std::string DocHitInfoIteratorTermMainExact::ToString() const {
  return absl_ports::StrCat(SectionIdMaskToString(section_restrict_mask_), ":",
                            term_);
}

libtextclassifier3::Status
DocHitInfoIteratorTermMainPrefix::RetrieveMoreHits() {
  DocHitInfo last_doc_hit_info;
  if (!cached_doc_hit_infos_.empty()) {
    last_doc_hit_info = cached_doc_hit_infos_.back();
  }
  cached_doc_hit_infos_idx_ = 0;
  cached_doc_hit_infos_.clear();
  if (last_doc_hit_info.document_id() != kInvalidDocumentId) {
    // Carry over the last hit. It might need to be merged with the first hit of
    // of the next posting list in the chain.
    cached_doc_hit_infos_.push_back(last_doc_hit_info);
  }

  ++num_blocks_inspected_;
  if (posting_list_accessor_ == nullptr) {
    ICING_ASSIGN_OR_RETURN(
        MainIndex::GetPrefixAccessorResult result,
        main_index_->GetAccessorForPrefixTerm(term_));
    posting_list_accessor_ = std::move(result.accessor);
    exact_ = result.exact;
  }
  ICING_ASSIGN_OR_RETURN(std::vector<Hit> hits,
                         posting_list_accessor_->GetNextHitsBatch());
  cached_doc_hit_infos_.reserve(hits.size());
  for (const Hit& hit : hits) {
    // Check sections.
    if (((1u << hit.section_id()) & section_restrict_mask_) == 0) {
      continue;
    }
    // If we only want hits from prefix sections.
    if (!exact_ && !hit.is_in_prefix_section()) {
      continue;
    }
    if (cached_doc_hit_infos_.empty() ||
        hit.document_id() != cached_doc_hit_infos_.back().document_id()) {
      cached_doc_hit_infos_.push_back(DocHitInfo(hit.document_id()));
    }
    cached_doc_hit_infos_.back().UpdateSection(hit.section_id(),
                                               hit.term_frequency());
  }
  return libtextclassifier3::Status::OK;
}

std::string DocHitInfoIteratorTermMainPrefix::ToString() const {
  return absl_ports::StrCat(SectionIdMaskToString(section_restrict_mask_), ":",
                            term_, "*");
}

}  // namespace lib
}  // namespace icing
