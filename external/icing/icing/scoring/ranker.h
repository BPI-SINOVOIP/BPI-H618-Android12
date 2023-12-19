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

#ifndef ICING_SCORING_RANKER_H_
#define ICING_SCORING_RANKER_H_

#include <vector>

#include "icing/scoring/scored-document-hit.h"

// Provides functionality to get the top N results from an unsorted vector.
namespace icing {
namespace lib {

// Builds a heap of scored document hits. The same vector is used to store the
// heap structure.
//
// REQUIRED: scored_document_hits is not null.
void BuildHeapInPlace(
    std::vector<ScoredDocumentHit>* scored_document_hits,
    const ScoredDocumentHitComparator& scored_document_hit_comparator);

// Returns the top num_results results from the given heap and remove those
// results from the heap. An empty vector will be returned if heap is empty.
//
// REQUIRED: scored_document_hits_heap is not null.
std::vector<ScoredDocumentHit> PopTopResultsFromHeap(
    std::vector<ScoredDocumentHit>* scored_document_hits_heap, int num_results,
    const ScoredDocumentHitComparator& scored_document_hit_comparator);

}  // namespace lib
}  // namespace icing

#endif  // ICING_SCORING_RANKER_H_
