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

#ifndef ICING_INDEX_HIT_DOC_HIT_INFO_H_
#define ICING_INDEX_HIT_DOC_HIT_INFO_H_

#include <limits>

#include "icing/index/hit/hit.h"
#include "icing/legacy/core/icing-packed-pod.h"
#include "icing/schema/section.h"
#include "icing/store/document-id.h"

namespace icing {
namespace lib {

// DocHitInfo provides a collapsed view of all hits for a specific doc.
// Hits contain a document_id, section_id and a term frequency. The
// information in multiple hits is collapse into a DocHitInfo by providing a
// SectionIdMask of all sections that contained a hit for this term as well as
// the highest term frequency of any hit for each section.
class DocHitInfo {
 public:
  explicit DocHitInfo(DocumentId document_id_in = kInvalidDocumentId,
                      SectionIdMask hit_section_ids_mask = kSectionIdMaskNone)
      : document_id_(document_id_in),
        hit_section_ids_mask_(hit_section_ids_mask) {
    memset(hit_term_frequency_, Hit::kNoTermFrequency,
           sizeof(hit_term_frequency_));
  }

  DocumentId document_id() const { return document_id_; }

  void set_document_id(DocumentId document_id) { document_id_ = document_id; }

  SectionIdMask hit_section_ids_mask() const { return hit_section_ids_mask_; }

  void set_hit_section_ids_mask(SectionIdMask section_id_mask) {
    hit_section_ids_mask_ = section_id_mask;
  }

  Hit::TermFrequency hit_term_frequency(SectionId section_id) const {
    return hit_term_frequency_[section_id];
  }

  bool operator<(const DocHitInfo& other) const;
  bool operator==(const DocHitInfo& other) const {
    return (*this < other) == (other < *this);
  }

  // Updates the hit_section_ids_mask and hit_term_frequency for the
  // section, if necessary.
  void UpdateSection(SectionId section_id,
                     Hit::TermFrequency hit_term_frequency);

  // Merges the sections of other into this. The hit_section_ids_masks are or'd;
  // if this.hit_term_frequency_[sectionId] has already been defined,
  // other.hit_term_frequency_[sectionId] value is ignored.
  //
  // This does not affect the DocumentId of this or other. If callers care about
  // only merging sections for DocHitInfos with the same DocumentId, callers
  // should check this themselves.
  void MergeSectionsFrom(const DocHitInfo& other);

 private:
  DocumentId document_id_;
  SectionIdMask hit_section_ids_mask_;
  Hit::TermFrequency hit_term_frequency_[kMaxSectionId + 1];
} __attribute__((packed));
static_assert(sizeof(DocHitInfo) == 22, "");
// TODO(b/138991332) decide how to remove/replace all is_packed_pod assertions.
static_assert(icing_is_packed_pod<DocHitInfo>::value, "go/icing-ubsan");
static_assert(
    sizeof(Hit::TermFrequency) == 1,
    "Change how hit_term_frequency_ is initialized if changing the type "
    "of Hit::TermFrequency");

}  // namespace lib
}  // namespace icing

#endif  // ICING_INDEX_HIT_DOC_HIT_INFO_H_
