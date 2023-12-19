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

#ifndef ICING_TOKENIZATION_ICU_ICU_LANGUAGE_SEGMENTER_H_
#define ICING_TOKENIZATION_ICU_ICU_LANGUAGE_SEGMENTER_H_

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "icing/text_classifier/lib3/utils/base/statusor.h"
#include "icing/tokenization/language-segmenter.h"

namespace icing {
namespace lib {

// This class is used to segment sentences into words based on rules
// (https://unicode.org/reports/tr29/#Word_Boundaries) and language
// understanding. Based on the basic segmentation done by UBreakIterator,
// some extra rules are applied in this class:
//
// 1. All ASCII terms will be returned.
// 2. For non-ASCII terms, only the alphabetic terms are returned, which means
//    non-ASCII punctuation and special characters are left out.
// 3. Multiple continuous whitespaces are treated as one.
//
// The rules above are common to the high-level tokenizers that might use this
// class. Other special tokenization logic will be in each tokenizer.
class IcuLanguageSegmenter : public LanguageSegmenter {
 public:
  explicit IcuLanguageSegmenter(std::string locale);

  IcuLanguageSegmenter(const IcuLanguageSegmenter&) = delete;
  IcuLanguageSegmenter& operator=(const IcuLanguageSegmenter&) = delete;

  // The segmentation depends on the language detected in the input text.
  //
  // Note: It could happen that the language detected from text is wrong, then
  // there would be a small chance that the text is segmented incorrectly.
  //
  // Returns:
  //   An iterator of terms on success
  //   INTERNAL_ERROR if any error occurs
  libtextclassifier3::StatusOr<std::unique_ptr<LanguageSegmenter::Iterator>>
  Segment(std::string_view text) const override;

  // The segmentation depends on the language detected in the input text.
  //
  // Note: It could happen that the language detected from text is wrong, then
  // there would be a small chance that the text is segmented incorrectly.
  //
  // Returns:
  //   A list of terms on success
  //   INTERNAL_ERROR if any error occurs
  libtextclassifier3::StatusOr<std::vector<std::string_view>> GetAllTerms(
      std::string_view text) const override;

 private:
  // Used to help segment text
  const std::string locale_;
};

}  // namespace lib
}  // namespace icing

#endif  // ICING_TOKENIZATION_ICU_ICU_LANGUAGE_SEGMENTER_H_
