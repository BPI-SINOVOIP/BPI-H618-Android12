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

#include "icing/index/lite/doc-hit-info-iterator-term-lite.h"

#include <cstdint>

#include "icing/text_classifier/lib3/utils/base/status.h"
#include "icing/absl_ports/canonical_errors.h"
#include "icing/absl_ports/str_cat.h"
#include "icing/index/hit/doc-hit-info.h"
#include "icing/schema/section.h"
#include "icing/util/logging.h"
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

libtextclassifier3::Status DocHitInfoIteratorTermLite::Advance() {
  if (cached_hits_idx_ == -1) {
    libtextclassifier3::Status status = RetrieveMoreHits();
    if (!status.ok()) {
      ICING_LOG(ERROR) << "Failed to retrieve more hits "
                       << status.error_message();
      return absl_ports::ResourceExhaustedError(
          "No more DocHitInfos in iterator");
    }
  } else {
    ++cached_hits_idx_;
  }
  if (cached_hits_idx_ == -1 || cached_hits_idx_ >= cached_hits_.size()) {
    // Nothing more for the iterator to return. Set these members to invalid
    // values.
    doc_hit_info_ = DocHitInfo();
    hit_intersect_section_ids_mask_ = kSectionIdMaskNone;
    return absl_ports::ResourceExhaustedError(
        "No more DocHitInfos in iterator");
  }
  doc_hit_info_ = cached_hits_.at(cached_hits_idx_);
  hit_intersect_section_ids_mask_ = doc_hit_info_.hit_section_ids_mask();
  return libtextclassifier3::Status::OK;
}

libtextclassifier3::Status DocHitInfoIteratorTermLiteExact::RetrieveMoreHits() {
  // Exact match only. All hits in lite lexicon are exact.
  ICING_ASSIGN_OR_RETURN(uint32_t tvi, lite_index_->GetTermId(term_));
  ICING_ASSIGN_OR_RETURN(uint32_t term_id,
                         term_id_codec_->EncodeTvi(tvi, TviType::LITE));
  lite_index_->AppendHits(term_id, section_restrict_mask_,
                          /*only_from_prefix_sections=*/false, &cached_hits_);
  cached_hits_idx_ = 0;
  return libtextclassifier3::Status::OK;
}

std::string DocHitInfoIteratorTermLiteExact::ToString() const {
  return absl_ports::StrCat(SectionIdMaskToString(section_restrict_mask_), ":",
                            term_);
}

libtextclassifier3::Status
DocHitInfoIteratorTermLitePrefix::RetrieveMoreHits() {
  // Take union of lite terms.
  int term_len = term_.length();
  int terms_matched = 0;
  for (LiteIndex::PrefixIterator it = lite_index_->FindTermPrefixes(term_);
       it.IsValid(); it.Advance()) {
    bool exact_match = strlen(it.GetKey()) == term_len;
    ICING_ASSIGN_OR_RETURN(
        uint32_t term_id,
        term_id_codec_->EncodeTvi(it.GetValueIndex(), TviType::LITE));
    lite_index_->AppendHits(term_id, section_restrict_mask_,
                            /*only_from_prefix_sections=*/!exact_match,
                            &cached_hits_);
    ++terms_matched;
  }
  if (terms_matched > 1) {
    SortAndDedupeDocumentIds();
  }
  cached_hits_idx_ = 0;
  return libtextclassifier3::Status::OK;
}

void DocHitInfoIteratorTermLitePrefix::SortAndDedupeDocumentIds() {
  // Re-sort cached document_ids and merge sections.
  sort(cached_hits_.begin(), cached_hits_.end());

  int idx = 0;
  for (int i = 1; i < cached_hits_.size(); ++i) {
    const DocHitInfo& hit_info = cached_hits_.at(i);
    DocHitInfo& collapsed_hit_info = cached_hits_.at(idx);
    if (collapsed_hit_info.document_id() == hit_info.document_id()) {
      collapsed_hit_info.MergeSectionsFrom(hit_info);
    } else {
      // New document_id.
      cached_hits_.at(++idx) = hit_info;
    }
  }
  // idx points to last doc hit info.
  cached_hits_.resize(idx + 1);
}

std::string DocHitInfoIteratorTermLitePrefix::ToString() const {
  return absl_ports::StrCat(SectionIdMaskToString(section_restrict_mask_), ":",
                            term_, "*");
}

}  // namespace lib
}  // namespace icing
