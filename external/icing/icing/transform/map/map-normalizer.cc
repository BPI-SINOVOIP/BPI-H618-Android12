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

#include "icing/transform/map/map-normalizer.h"

#include <ctype.h>

#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>

#include "icing/absl_ports/str_cat.h"
#include "icing/transform/map/normalization-map.h"
#include "icing/util/i18n-utils.h"
#include "icing/util/logging.h"
#include "unicode/utypes.h"

namespace icing {
namespace lib {

std::string MapNormalizer::NormalizeTerm(std::string_view term) const {
  std::string normalized_text;
  normalized_text.reserve(term.length());

  for (int i = 0; i < term.length(); ++i) {
    if (i18n_utils::IsAscii(term[i])) {
      // The original character has 1 byte.
      normalized_text.push_back(std::tolower(term[i]));
    } else if (i18n_utils::IsLeadUtf8Byte(term[i])) {
      UChar32 uchar32 = i18n_utils::GetUChar32At(term.data(), term.length(), i);
      if (uchar32 == i18n_utils::kInvalidUChar32) {
        ICING_LOG(WARNING) << "Unable to get uchar32 from " << term
                           << " at position" << i;
        continue;
      }
      int utf8_length = i18n_utils::GetUtf8Length(uchar32);
      if (i18n_utils::GetUtf16Length(uchar32) > 1) {
        // All the characters we need to normalize can be encoded into a
        // single char16_t. If this character needs more than 1 char16_t code
        // unit, we can skip normalization and append it directly.
        absl_ports::StrAppend(&normalized_text, term.substr(i, utf8_length));
        continue;
      }
      // The original character can be encoded into a single char16_t.
      const std::unordered_map<char16_t, char16_t>& normalization_map =
          GetNormalizationMap();
      auto iterator = normalization_map.find(static_cast<char16_t>(uchar32));
      if (iterator != normalization_map.end()) {
        // Found a normalization mapping. The normalized character (stored in a
        // char16_t) can have 1 or 2 bytes.
        if (i18n_utils::IsAscii(iterator->second)) {
          // The normalized character has 1 byte.
          normalized_text.push_back(
              std::tolower(static_cast<char>(iterator->second)));
        } else {
          // The normalized character has 2 bytes.
          i18n_utils::AppendUchar32ToUtf8(&normalized_text, iterator->second);
        }
      } else {
        // Normalization mapping not found, append the original character.
        absl_ports::StrAppend(&normalized_text, term.substr(i, utf8_length));
      }
    }
  }

  if (normalized_text.length() > max_term_byte_size_) {
    i18n_utils::SafeTruncateUtf8(&normalized_text, max_term_byte_size_);
  }

  return normalized_text;
}

}  // namespace lib
}  // namespace icing
