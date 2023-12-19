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

#include "icing/result/result-state.h"

#include "icing/result/projection-tree.h"
#include "icing/scoring/ranker.h"
#include "icing/store/namespace-id.h"
#include "icing/util/logging.h"

namespace icing {
namespace lib {

SnippetContext CreateSnippetContext(SectionRestrictQueryTermsMap query_terms,
                                    const SearchSpecProto& search_spec,
                                    const ResultSpecProto& result_spec) {
  if (result_spec.snippet_spec().num_to_snippet() > 0 &&
      result_spec.snippet_spec().num_matches_per_property() > 0) {
    // Needs snippeting
    return SnippetContext(std::move(query_terms), result_spec.snippet_spec(),
                          search_spec.term_match_type());
  }
  return SnippetContext(/*query_terms_in=*/{},
                        ResultSpecProto::SnippetSpecProto::default_instance(),
                        TermMatchType::UNKNOWN);
}

ResultState::ResultState(std::vector<ScoredDocumentHit> scored_document_hits,
                         SectionRestrictQueryTermsMap query_terms,
                         const SearchSpecProto& search_spec,
                         const ScoringSpecProto& scoring_spec,
                         const ResultSpecProto& result_spec,
                         const DocumentStore& document_store)
    : scored_document_hits_(std::move(scored_document_hits)),
      snippet_context_(CreateSnippetContext(std::move(query_terms), search_spec,
                                            result_spec)),
      num_per_page_(result_spec.num_per_page()),
      num_returned_(0),
      scored_document_hit_comparator_(scoring_spec.order_by() ==
                                      ScoringSpecProto::Order::DESC) {
  for (const TypePropertyMask& type_field_mask :
       result_spec.type_property_masks()) {
    projection_tree_map_.insert(
        {type_field_mask.schema_type(), ProjectionTree(type_field_mask)});
  }

  for (const ResultSpecProto::ResultGrouping& result_grouping :
       result_spec.result_groupings()) {
    int group_id = group_result_limits_.size();
    group_result_limits_.push_back(result_grouping.max_results());
    for (const std::string& name_space : result_grouping.namespaces()) {
      auto namespace_id_or = document_store.GetNamespaceId(name_space);
      if (!namespace_id_or.ok()) {
        continue;
      }
      namespace_group_id_map_.insert({namespace_id_or.ValueOrDie(), group_id});
    }
  }
  BuildHeapInPlace(&scored_document_hits_, scored_document_hit_comparator_);
}

class GroupResultLimiter {
 public:
  GroupResultLimiter(
      const std::unordered_map<NamespaceId, int>& namespace_group_id_map,
      std::vector<int>& group_result_limits,
      const DocumentStore& document_store)
      : namespace_group_id_map_(namespace_group_id_map),
        group_result_limits_(&group_result_limits),
        document_store_(document_store) {}

  // Returns true if the scored_document_hit should be removed.
  bool operator()(const ScoredDocumentHit& scored_document_hit) {
    auto document_filter_data_or = document_store_.GetDocumentFilterData(
        scored_document_hit.document_id());
    if (!document_filter_data_or.ok()) {
      return true;
    }
    NamespaceId namespace_id =
        document_filter_data_or.ValueOrDie().namespace_id();
    auto iter = namespace_group_id_map_.find(namespace_id);
    if (iter == namespace_group_id_map_.end()) {
      return false;
    }
    int& count = group_result_limits_->at(iter->second);
    if (count <= 0) {
      return true;
    }
    --count;
    return false;
  }

 private:
  const std::unordered_map<NamespaceId, int>& namespace_group_id_map_;
  std::vector<int>* group_result_limits_;
  const DocumentStore& document_store_;
};

std::vector<ScoredDocumentHit> ResultState::GetNextPage(
    const DocumentStore& document_store) {
  int num_requested = num_per_page_;
  bool more_results_available = true;
  std::vector<ScoredDocumentHit> final_scored_document_hits;
  while (more_results_available && num_requested > 0) {
    std::vector<ScoredDocumentHit> scored_document_hits = PopTopResultsFromHeap(
        &scored_document_hits_, num_requested, scored_document_hit_comparator_);
    more_results_available = scored_document_hits.size() == num_requested;
    auto itr = std::remove_if(
        scored_document_hits.begin(), scored_document_hits.end(),
        GroupResultLimiter(namespace_group_id_map_, group_result_limits_,
                           document_store));
    scored_document_hits.erase(itr, scored_document_hits.end());
    final_scored_document_hits.reserve(final_scored_document_hits.size() +
                                       scored_document_hits.size());
    std::move(scored_document_hits.begin(), scored_document_hits.end(),
              std::back_inserter(final_scored_document_hits));
    num_requested = num_per_page_ - final_scored_document_hits.size();
  }

  num_returned_ += final_scored_document_hits.size();
  return final_scored_document_hits;
}

void ResultState::TruncateHitsTo(int new_size) {
  if (new_size < 0 || scored_document_hits_.size() <= new_size) {
    return;
  }

  // Copying the best new_size results.
  scored_document_hits_ = PopTopResultsFromHeap(
      &scored_document_hits_, new_size, scored_document_hit_comparator_);
}

}  // namespace lib
}  // namespace icing
