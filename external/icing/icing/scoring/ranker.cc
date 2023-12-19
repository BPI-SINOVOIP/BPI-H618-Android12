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

#include "icing/scoring/ranker.h"

#include <algorithm>
#include <vector>

#include "icing/text_classifier/lib3/utils/base/statusor.h"
#include "icing/absl_ports/canonical_errors.h"
#include "icing/scoring/scored-document-hit.h"
#include "icing/util/logging.h"

namespace icing {
namespace lib {

namespace {
// For all the heap manipulations in this file, we use a vector to represent the
// heap. The element at index 0 is the root node. For any node at index i, its
// left child node is at 2 * i + 1, its right child node is at 2 * i + 2.

// Helper function to wrap the heapify algorithm, it heapifies the target
// subtree node in place.
void Heapify(
    std::vector<ScoredDocumentHit>* scored_document_hits,
    int target_subtree_root_index,
    const ScoredDocumentHitComparator& scored_document_hit_comparator) {
  const int heap_size = scored_document_hits->size();
  if (target_subtree_root_index >= heap_size) {
    return;
  }

  // Initializes subtree root as the current best node.
  int best = target_subtree_root_index;
  // If we represent a heap in an array/vector, indices of left and right
  // children can be calculated.
  const int left = target_subtree_root_index * 2 + 1;
  const int right = target_subtree_root_index * 2 + 2;

  // If left child is better than current best
  if (left < heap_size &&
      scored_document_hit_comparator(scored_document_hits->at(left),
                                     scored_document_hits->at(best))) {
    best = left;
  }

  // If right child is better than current best
  if (right < heap_size &&
      scored_document_hit_comparator(scored_document_hits->at(right),
                                     scored_document_hits->at(best))) {
    best = right;
  }

  // If the best is not the subtree root, swap and continue heapifying the lower
  // level subtree
  if (best != target_subtree_root_index) {
    std::swap(scored_document_hits->at(best),
              scored_document_hits->at(target_subtree_root_index));
    Heapify(scored_document_hits, best, scored_document_hit_comparator);
  }
}

// Helper function to extract the root from the heap. The heap structure will be
// maintained.
//
// Returns:
//   The current root element on success
//   RESOURCE_EXHAUSTED_ERROR if heap is empty
libtextclassifier3::StatusOr<ScoredDocumentHit> PopRoot(
    std::vector<ScoredDocumentHit>* scored_document_hits_heap,
    const ScoredDocumentHitComparator& scored_document_hit_comparator) {
  if (scored_document_hits_heap->empty()) {
    // An invalid ScoredDocumentHit
    return absl_ports::ResourceExhaustedError("Heap is empty");
  }

  // Steps to extract root from heap:
  // 1. copy out root
  ScoredDocumentHit root = scored_document_hits_heap->at(0);
  const size_t last_node_index = scored_document_hits_heap->size() - 1;
  // 2. swap root and the last node
  std::swap(scored_document_hits_heap->at(0),
            scored_document_hits_heap->at(last_node_index));
  // 3. remove last node
  scored_document_hits_heap->pop_back();
  // 4. heapify root
  Heapify(scored_document_hits_heap, /*target_subtree_root_index=*/0,
          scored_document_hit_comparator);
  return root;
}

}  // namespace

void BuildHeapInPlace(
    std::vector<ScoredDocumentHit>* scored_document_hits,
    const ScoredDocumentHitComparator& scored_document_hit_comparator) {
  const int heap_size = scored_document_hits->size();
  // Since we use a vector to represent the heap, [size / 2 - 1] is the index
  // of the parent node of the last node.
  for (int subtree_root_index = heap_size / 2 - 1; subtree_root_index >= 0;
       subtree_root_index--) {
    Heapify(scored_document_hits, subtree_root_index,
            scored_document_hit_comparator);
  }
}

std::vector<ScoredDocumentHit> PopTopResultsFromHeap(
    std::vector<ScoredDocumentHit>* scored_document_hits_heap, int num_results,
    const ScoredDocumentHitComparator& scored_document_hit_comparator) {
  std::vector<ScoredDocumentHit> scored_document_hit_result;
  int result_size = std::min(
      num_results, static_cast<int>(scored_document_hits_heap->size()));
  while (result_size-- > 0) {
    libtextclassifier3::StatusOr<ScoredDocumentHit> next_best_document_hit_or =
        PopRoot(scored_document_hits_heap, scored_document_hit_comparator);
    if (next_best_document_hit_or.ok()) {
      scored_document_hit_result.push_back(
          std::move(next_best_document_hit_or).ValueOrDie());
    } else {
      ICING_VLOG(1) << next_best_document_hit_or.status().error_message();
    }
  }
  return scored_document_hit_result;
}

}  // namespace lib
}  // namespace icing
