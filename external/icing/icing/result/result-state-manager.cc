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

#include "icing/result/result-state-manager.h"

#include "icing/proto/search.pb.h"
#include "icing/util/clock.h"
#include "icing/util/logging.h"
#include "icing/util/status-macros.h"

namespace icing {
namespace lib {

ResultStateManager::ResultStateManager(int max_total_hits,
                                       const DocumentStore& document_store)
    : document_store_(document_store),
      max_total_hits_(max_total_hits),
      num_total_hits_(0),
      random_generator_(GetSteadyTimeNanoseconds()) {}

libtextclassifier3::StatusOr<PageResultState>
ResultStateManager::RankAndPaginate(ResultState result_state) {
  if (!result_state.HasMoreResults()) {
    return absl_ports::InvalidArgumentError("ResultState has no results");
  }

  // Gets the number before calling GetNextPage() because num_returned() may
  // change after returning more results.
  int num_previously_returned = result_state.num_returned();
  int num_per_page = result_state.num_per_page();

  std::vector<ScoredDocumentHit> page_result_document_hits =
      result_state.GetNextPage(document_store_);

  SnippetContext snippet_context_copy = result_state.snippet_context();

  std::unordered_map<std::string, ProjectionTree> projection_tree_map_copy =
      result_state.projection_tree_map();
  if (!result_state.HasMoreResults()) {
    // No more pages, won't store ResultState, returns directly
    return PageResultState(
        std::move(page_result_document_hits), kInvalidNextPageToken,
        std::move(snippet_context_copy), std::move(projection_tree_map_copy),
        num_previously_returned, num_per_page);
  }

  absl_ports::unique_lock l(&mutex_);

  // ResultState has multiple pages, storing it
  uint64_t next_page_token = Add(std::move(result_state));

  return PageResultState(std::move(page_result_document_hits), next_page_token,
                         std::move(snippet_context_copy),
                         std::move(projection_tree_map_copy),
                         num_previously_returned, num_per_page);
}

uint64_t ResultStateManager::Add(ResultState result_state) {
  RemoveStatesIfNeeded(result_state);
  result_state.TruncateHitsTo(max_total_hits_);

  uint64_t new_token = GetUniqueToken();

  num_total_hits_ += result_state.num_remaining();
  result_state_map_.emplace(new_token, std::move(result_state));
  // Tracks the insertion order
  token_queue_.push(new_token);

  return new_token;
}

libtextclassifier3::StatusOr<PageResultState> ResultStateManager::GetNextPage(
    uint64_t next_page_token) {
  absl_ports::unique_lock l(&mutex_);

  const auto& state_iterator = result_state_map_.find(next_page_token);
  if (state_iterator == result_state_map_.end()) {
    return absl_ports::NotFoundError("next_page_token not found");
  }

  int num_returned = state_iterator->second.num_returned();
  int num_per_page = state_iterator->second.num_per_page();
  std::vector<ScoredDocumentHit> result_of_page =
      state_iterator->second.GetNextPage(document_store_);
  if (result_of_page.empty()) {
    // This shouldn't happen, all our active states should contain results, but
    // a sanity check here in case of any data inconsistency.
    InternalInvalidateResultState(next_page_token);
    return absl_ports::NotFoundError(
        "No more results, token has been invalidated.");
  }

  // Copies the SnippetContext in case the ResultState is invalidated.
  SnippetContext snippet_context_copy =
      state_iterator->second.snippet_context();

  std::unordered_map<std::string, ProjectionTree> projection_tree_map_copy =
      state_iterator->second.projection_tree_map();

  if (!state_iterator->second.HasMoreResults()) {
    InternalInvalidateResultState(next_page_token);
    next_page_token = kInvalidNextPageToken;
  }

  num_total_hits_ -= result_of_page.size();
  return PageResultState(
      result_of_page, next_page_token, std::move(snippet_context_copy),
      std::move(projection_tree_map_copy), num_returned, num_per_page);
}

void ResultStateManager::InvalidateResultState(uint64_t next_page_token) {
  if (next_page_token == kInvalidNextPageToken) {
    return;
  }

  absl_ports::unique_lock l(&mutex_);

  InternalInvalidateResultState(next_page_token);
}

void ResultStateManager::InvalidateAllResultStates() {
  absl_ports::unique_lock l(&mutex_);
  InternalInvalidateAllResultStates();
}

void ResultStateManager::InternalInvalidateAllResultStates() {
  result_state_map_.clear();
  invalidated_token_set_.clear();
  token_queue_ = std::queue<uint64_t>();
  num_total_hits_ = 0;
}

uint64_t ResultStateManager::GetUniqueToken() {
  uint64_t new_token = random_generator_();
  // There's a small chance of collision between the random numbers, here we're
  // trying to avoid any collisions by checking the keys.
  while (result_state_map_.find(new_token) != result_state_map_.end() ||
         invalidated_token_set_.find(new_token) !=
             invalidated_token_set_.end() ||
         new_token == kInvalidNextPageToken) {
    new_token = random_generator_();
  }
  return new_token;
}

void ResultStateManager::RemoveStatesIfNeeded(const ResultState& result_state) {
  if (result_state_map_.empty() || token_queue_.empty()) {
    return;
  }

  // 1. Check if this new result_state would take up the entire result state
  // manager budget.
  if (result_state.num_remaining() > max_total_hits_) {
    // This single result state will exceed our budget. Drop everything else to
    // accomodate it.
    InternalInvalidateAllResultStates();
    return;
  }

  // 2. Remove any tokens that were previously invalidated.
  while (!token_queue_.empty() &&
         invalidated_token_set_.find(token_queue_.front()) !=
             invalidated_token_set_.end()) {
    invalidated_token_set_.erase(token_queue_.front());
    token_queue_.pop();
  }

  // 3. If we're over budget, remove states from oldest to newest until we fit
  // into our budget.
  while (result_state.num_remaining() + num_total_hits_ > max_total_hits_) {
    InternalInvalidateResultState(token_queue_.front());
    token_queue_.pop();
  }
  invalidated_token_set_.clear();
}

void ResultStateManager::InternalInvalidateResultState(uint64_t token) {
  // Removes the entry in result_state_map_ and insert the token into
  // invalidated_token_set_. The entry in token_queue_ can't be easily removed
  // right now (may need O(n) time), so we leave it there and later completely
  // remove the token in RemoveStatesIfNeeded().
  auto itr = result_state_map_.find(token);
  if (itr != result_state_map_.end()) {
    num_total_hits_ -= itr->second.num_remaining();
    result_state_map_.erase(itr);
    invalidated_token_set_.insert(token);
  }
}

}  // namespace lib
}  // namespace icing
