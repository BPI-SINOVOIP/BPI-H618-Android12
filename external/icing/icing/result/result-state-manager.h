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

#ifndef ICING_RESULT_RESULT_STATE_MANAGER_H_
#define ICING_RESULT_RESULT_STATE_MANAGER_H_

#include <queue>
#include <random>
#include <unordered_map>
#include <unordered_set>

#include "icing/text_classifier/lib3/utils/base/statusor.h"
#include "icing/absl_ports/mutex.h"
#include "icing/proto/scoring.pb.h"
#include "icing/proto/search.pb.h"
#include "icing/result/page-result-state.h"
#include "icing/result/result-state.h"

namespace icing {
namespace lib {

// This should be the same as the default value of
// SearchResultProto.next_page_token.
inline constexpr uint64_t kInvalidNextPageToken = 0;

// Used to store and manage ResultState.
class ResultStateManager {
 public:
  explicit ResultStateManager(int max_total_hits,
                              const DocumentStore& document_store);

  ResultStateManager(const ResultStateManager&) = delete;
  ResultStateManager& operator=(const ResultStateManager&) = delete;

  // Ranks the results and returns the first page of them. The result object
  // PageResultState contains a next_page_token which can be used to fetch more
  // pages later. It will be set to a default value 0 if there're no more pages.
  //
  // NOTE: it's caller's responsibility not to call this method with the same
  // ResultState more than once, otherwise duplicate states will be stored
  // internally.
  //
  // Returns:
  //   A PageResultState on success
  //   INVALID_ARGUMENT if the input state contains no results
  libtextclassifier3::StatusOr<PageResultState> RankAndPaginate(
      ResultState result_state) ICING_LOCKS_EXCLUDED(mutex_);

  // Retrieves and returns the next page of results wrapped in PageResultState.
  // The returned results won't exist in ResultStateManager anymore. If the
  // query has no more pages after this retrieval, the input token will be
  // invalidated.
  //
  // Returns:
  //   PageResultState on success, guaranteed to have non-empty results
  //   NOT_FOUND if failed to find any more results
  libtextclassifier3::StatusOr<PageResultState> GetNextPage(
      uint64_t next_page_token) ICING_LOCKS_EXCLUDED(mutex_);

  // Invalidates the result state associated with the given next-page token.
  void InvalidateResultState(uint64_t next_page_token)
      ICING_LOCKS_EXCLUDED(mutex_);

  // Invalidates all result states / tokens currently in ResultStateManager.
  void InvalidateAllResultStates() ICING_LOCKS_EXCLUDED(mutex_);

 private:
  absl_ports::shared_mutex mutex_;

  const DocumentStore& document_store_;

  // The maximum number of scored document hits that all result states may
  // have. When a new result state is added such that num_total_hits_ would
  // exceed max_total_hits_, the oldest result states are evicted until
  // num_total_hits_ is below max_total_hits.
  const int max_total_hits_;

  // The number of scored document hits that all result states currently held by
  // the result state manager have.
  int num_total_hits_;

  // A hash map of (next-page token -> result state)
  std::unordered_map<uint64_t, ResultState> result_state_map_
      ICING_GUARDED_BY(mutex_);

  // A queue used to track the insertion order of tokens
  std::queue<uint64_t> token_queue_ ICING_GUARDED_BY(mutex_);

  // A set to temporarily store the invalidated tokens before they're finally
  // removed from token_queue_. We store the invalidated tokens to ensure the
  // uniqueness of new generated tokens.
  std::unordered_set<uint64_t> invalidated_token_set_ ICING_GUARDED_BY(mutex_);

  // A random 64-bit number generator
  std::mt19937_64 random_generator_ ICING_GUARDED_BY(mutex_);

  // Puts a new result state into the internal storage and returns a next-page
  // token associated with it. The token is guaranteed to be unique among all
  // currently valid tokens. When the maximum number of result states is
  // reached, the oldest / firstly added result state will be removed to make
  // room for the new state.
  uint64_t Add(ResultState result_state) ICING_EXCLUSIVE_LOCKS_REQUIRED(mutex_);

  // Helper method to generate a next-page token that is unique among all
  // existing tokens in token_queue_.
  uint64_t GetUniqueToken() ICING_EXCLUSIVE_LOCKS_REQUIRED(mutex_);

  // Helper method to remove old states to make room for incoming states.
  void RemoveStatesIfNeeded(const ResultState& result_state)
      ICING_EXCLUSIVE_LOCKS_REQUIRED(mutex_);

  // Helper method to remove a result state from result_state_map_, the token
  // will then be temporarily kept in invalidated_token_set_ until it's finally
  // removed from token_queue_.
  void InternalInvalidateResultState(uint64_t token)
      ICING_EXCLUSIVE_LOCKS_REQUIRED(mutex_);

  // Internal method to invalidates all result states / tokens currently in
  // ResultStateManager. We need this separate method so that other public
  // methods don't need to call InvalidateAllResultStates(). Public methods
  // calling each other may cause deadlock issues.
  void InternalInvalidateAllResultStates()
      ICING_EXCLUSIVE_LOCKS_REQUIRED(mutex_);
};

}  // namespace lib
}  // namespace icing

#endif  // ICING_RESULT_RESULT_STATE_MANAGER_H_
