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

#ifndef ICING_SCORING_SCORED_DOCUMENT_HIT_H_
#define ICING_SCORING_SCORED_DOCUMENT_HIT_H_

#include <type_traits>

#include "icing/legacy/core/icing-packed-pod.h"
#include "icing/schema/section.h"
#include "icing/store/document-id.h"

namespace icing {
namespace lib {

// A data class containing information about the document, hit sections, and a
// score. The score is calculated against both the document and the hit
// sections.
class ScoredDocumentHit {
 public:
  ScoredDocumentHit(DocumentId document_id, SectionIdMask hit_section_id_mask,
                    double score)
      : document_id_(document_id),
        hit_section_id_mask_(hit_section_id_mask),
        score_(score) {}

  bool operator<(const ScoredDocumentHit& other) const {
    if (score() < other.score()) return true;
    if (score() > other.score()) return false;
    return document_id() < other.document_id();
  }

  DocumentId document_id() const { return document_id_; }

  SectionIdMask hit_section_id_mask() const { return hit_section_id_mask_; }

  double score() const { return score_; }

 private:
  DocumentId document_id_;
  SectionIdMask hit_section_id_mask_;
  double score_;
} __attribute__((packed));

static_assert(sizeof(ScoredDocumentHit) == 14,
              "Size of ScoredDocHit should be 14");
static_assert(icing_is_packed_pod<ScoredDocumentHit>::value, "go/icing-ubsan");

// A custom comparator for ScoredDocumentHit that determines which
// ScoredDocumentHit is better (should come first) based off of
// ScoredDocumentHit itself and the order of its score.
//
// Returns true if left is better than right according to score and order.
// Comparison is based off of score with ties broken by
// ScoredDocumentHit.document_id().
class ScoredDocumentHitComparator {
 public:
  explicit ScoredDocumentHitComparator(bool is_descending = true)
      : is_descending_(is_descending) {}

  bool operator()(const ScoredDocumentHit& lhs,
                  const ScoredDocumentHit& rhs) const {
    return is_descending_ == !(lhs < rhs);
  }

 private:
  bool is_descending_;
};

}  // namespace lib
}  // namespace icing

#endif  // ICING_SCORING_SCORED_DOCUMENT_HIT_H_
