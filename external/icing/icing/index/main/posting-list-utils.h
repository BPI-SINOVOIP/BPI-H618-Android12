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

#ifndef ICING_INDEX_MAIN_POSTING_LIST_UTILS_H_
#define ICING_INDEX_MAIN_POSTING_LIST_UTILS_H_

#include <cstdint>

#include "icing/index/hit/hit.h"

namespace icing {
namespace lib {

namespace posting_list_utils {

// Represents the byte length of the two special hits described
// in the private section of posting-list-used.h.
inline constexpr uint32_t kNumSpecialHits = 2;
inline constexpr uint32_t kSpecialHitsSize = sizeof(Hit) * kNumSpecialHits;

constexpr uint32_t min_posting_list_size() { return kSpecialHitsSize; }

// For a posting list size to be valid, it must:
//   1) be sizeof(Hit) aligned
//   2) be equal to or larger than min_posting_list_size
//   3) be small enough to be encoded within a single Hit (5 bytes)
bool IsValidPostingListSize(uint32_t size_in_bytes);

}  // namespace posting_list_utils

}  // namespace lib
}  // namespace icing

#endif  // ICING_INDEX_MAIN_POSTING_LIST_UTILS_H_
