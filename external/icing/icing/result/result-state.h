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

#ifndef ICING_RESULT_RESULT_STATE_H_
#define ICING_RESULT_RESULT_STATE_H_

#include <iostream>
#include <vector>

#include "icing/proto/scoring.pb.h"
#include "icing/proto/search.pb.h"
#include "icing/result/projection-tree.h"
#include "icing/result/snippet-context.h"
#include "icing/scoring/scored-document-hit.h"
#include "icing/store/document-store.h"
#include "icing/store/namespace-id.h"

namespace icing {
namespace lib {

// Used to hold information needed across multiple pagination requests of the
// same query. Stored in ResultStateManager.
class ResultState {
 public:
  ResultState(std::vector<ScoredDocumentHit> scored_document_hits,
              SectionRestrictQueryTermsMap query_terms,
              const SearchSpecProto& search_spec,
              const ScoringSpecProto& scoring_spec,
              const ResultSpecProto& result_spec,
              const DocumentStore& document_store);

  // Returns the next page of results. The size of page is passed in from
  // ResultSpecProto in constructor. Calling this method could increase the
  // value of num_returned(), so be careful of the order of calling these
  // methods.
  std::vector<ScoredDocumentHit> GetNextPage(
      const DocumentStore& document_store);

  // Truncates the vector of ScoredDocumentHits to the given size. The best
  // ScoredDocumentHits are kept.
  void TruncateHitsTo(int new_size);

  // Returns if the current state has more results to return.
  bool HasMoreResults() const { return !scored_document_hits_.empty(); }

  // Returns a SnippetContext generated from the specs passed in via
  // constructor.
  const SnippetContext& snippet_context() const { return snippet_context_; }

  // Returns a vector of TypePropertyMasks generated from the specs passed in
  // via constructor.
  const std::unordered_map<std::string, ProjectionTree>& projection_tree_map()
      const {
    return projection_tree_map_;
  }

  int num_per_page() const { return num_per_page_; }

  // The number of results that have already been returned. This number is
  // increased when GetNextPage() is called.
  int num_returned() const { return num_returned_; }

  // The number of results yet to be returned. This number is decreased when
  // GetNextPage is called.
  int num_remaining() const { return scored_document_hits_.size(); }

 private:
  // The scored document hits. It represents a heap data structure when ranking
  // is required so that we can get top K hits in O(KlgN) time. If no ranking is
  // required, it's just a vector of ScoredDocumentHits in the original order.
  std::vector<ScoredDocumentHit> scored_document_hits_;

  // Information needed for snippeting.
  SnippetContext snippet_context_;

  // Information needed for projection.
  std::unordered_map<std::string, ProjectionTree> projection_tree_map_;

  // A map between namespace id and the id of the group that it appears in.
  std::unordered_map<NamespaceId, int> namespace_group_id_map_;

  // The count of remaining results to return for a group where group id is the
  // index.
  std::vector<int> group_result_limits_;

  // Number of results to return in each page.
  int num_per_page_;

  // Number of results that have already been returned.
  int num_returned_;

  // Used to compare two scored document hits.
  ScoredDocumentHitComparator scored_document_hit_comparator_;
};

}  // namespace lib
}  // namespace icing

#endif  // ICING_RESULT_RESULT_STATE_H_
