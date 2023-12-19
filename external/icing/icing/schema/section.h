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

#ifndef ICING_SCHEMA_SECTION_H_
#define ICING_SCHEMA_SECTION_H_

#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "icing/proto/schema.pb.h"
#include "icing/proto/term.pb.h"

namespace icing {
namespace lib {

using SectionId = int8_t;
// 4 bits for 16 values. NOTE: Increasing this value means that SectionIdMask
// must increase from an int16_t to an int32_t
inline constexpr int kSectionIdBits = 4;
inline constexpr SectionId kInvalidSectionId = (1 << kSectionIdBits);
inline constexpr SectionId kMaxSectionId = kInvalidSectionId - 1;
inline constexpr SectionId kMinSectionId = 0;
constexpr bool IsSectionIdValid(SectionId section_id) {
  return section_id >= kMinSectionId && section_id <= kMaxSectionId;
}

using SectionIdMask = int16_t;
inline constexpr SectionIdMask kSectionIdMaskAll = ~SectionIdMask{0};
inline constexpr SectionIdMask kSectionIdMaskNone = SectionIdMask{0};

static_assert(
    kMaxSectionId < 8 * sizeof(SectionIdMask),
    "SectionIdMask is not large enough to represent all section values!");

struct SectionMetadata {
  // Dot-joined property names, representing the location of section inside an
  // document. E.g. "property1.property2"
  std::string path;

  // A unique id of property within a type config
  SectionId id;

  // How strings should be tokenized. It is invalid for a section to have
  // tokenizer == 'NONE'.
  StringIndexingConfig::TokenizerType::Code tokenizer;

  // How tokens in this section should be matched.
  //
  // TermMatchType::UNKNOWN:
  //   Terms will not match anything
  //
  // TermMatchType::PREFIX:
  //   Terms will be stored as a prefix match, "fool" matches "foo" and "fool"
  //
  // TermMatchType::EXACT_ONLY:
  //   Terms will be only stored as an exact match, "fool" only matches "fool"
  TermMatchType::Code term_match_type = TermMatchType::UNKNOWN;

  SectionMetadata(SectionId id_in, TermMatchType::Code term_match_type_in,
                  StringIndexingConfig::TokenizerType::Code tokenizer,
                  std::string&& path_in)
      : path(std::move(path_in)),
        id(id_in),
        tokenizer(tokenizer),
        term_match_type(term_match_type_in) {}
};

// Section is an icing internal concept similar to document property but with
// extra metadata. The content can be a value or the combination of repeated
// values of a property.
struct Section {
  SectionMetadata metadata;
  std::vector<std::string_view> content;

  Section(SectionMetadata&& metadata_in,
          std::vector<std::string_view>&& content_in)
      : metadata(std::move(metadata_in)), content(std::move(content_in)) {}
};

}  // namespace lib
}  // namespace icing

#endif  // ICING_SCHEMA_SECTION_H_
