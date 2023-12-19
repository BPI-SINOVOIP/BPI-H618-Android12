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

#include "icing/legacy/index/icing-lite-index-options.h"

namespace icing {
namespace lib {

namespace {

uint32_t CalculateHitBufferSize(uint32_t hit_buffer_want_merge_bytes) {
  constexpr uint32_t kHitBufferSlopMult = 2;

  // Add a 2x slop for the hit buffer. We need to make sure we can at
  // least fit one document with index variants.
  // TODO(b/111690435) Move LiteIndex::Element to a separate file so that this
  // can use sizeof(LiteIndex::Element)
  uint32_t hit_capacity_elts_with_slop =
      hit_buffer_want_merge_bytes / sizeof(uint64_t);
  // Add some slop for index variants on top of max num tokens.
  hit_capacity_elts_with_slop += kIcingMaxNumHitsPerDocument;
  hit_capacity_elts_with_slop *= kHitBufferSlopMult;

  return hit_capacity_elts_with_slop;
}

IcingDynamicTrie::Options CalculateTrieOptions(uint32_t hit_buffer_size) {
  // The default min is 1/5th of the main index lexicon, which can
  // hold >1M terms. We don't need values so value size is 0. We
  // conservatively scale from there.
  //
  // We can give this a lot of headroom because overestimating the
  // requirement has minimal resource impact.
  double scaling_factor =
      std::max(1.0, static_cast<double>(hit_buffer_size) / (100u << 10));
  return IcingDynamicTrie::Options((200u << 10) * scaling_factor,
                                   (200u << 10) * scaling_factor,
                                   (1u << 20) * scaling_factor, 0);
}

}  // namespace

IcingLiteIndexOptions::IcingLiteIndexOptions(
    const std::string& filename_base, uint32_t hit_buffer_want_merge_bytes)
    : filename_base(filename_base),
      hit_buffer_want_merge_bytes(hit_buffer_want_merge_bytes) {
  hit_buffer_size = CalculateHitBufferSize(hit_buffer_want_merge_bytes);
  lexicon_options = CalculateTrieOptions(hit_buffer_size);
  display_mappings_options = CalculateTrieOptions(hit_buffer_size);
}

}  // namespace lib
}  // namespace icing
