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

#include "icing/transform/icu/icu-normalizer.h"

#include <cctype>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

#include "icing/text_classifier/lib3/utils/base/statusor.h"
#include "icing/absl_ports/canonical_errors.h"
#include "icing/absl_ports/str_cat.h"
#include "icing/transform/normalizer.h"
#include "icing/util/i18n-utils.h"
#include "icing/util/logging.h"
#include "icing/util/status-macros.h"
#include "unicode/umachine.h"
#include "unicode/unorm2.h"
#include "unicode/utrans.h"

namespace icing {
namespace lib {

namespace {

// The following is the compound id used to tell UTransliterator how to
// transform terms. The basic normalization forms NFD (canonical normalization
// form decomposition) and NFKC (compatible normalization form composition)
// are applied as well as some other rules we need. More information at
// http://www.unicode.org/reports/tr15/
//
// Please note that the following rules don't support small hiragana to katakana
// transformation.
constexpr UChar kTransformRulesUtf16[] =
    u"Lower; "                      // Lowercase
    "Latin-ASCII; "                 // Map Latin characters to ASCII characters
    "Hiragana-Katakana; "           // Map hiragana to katakana
    "[:Latin:] NFD; "               // Decompose Latin letters
    "[:Nonspacing Mark:] Remove; "  // Remove accent / diacritic marks
    "NFKC";                         // Decompose and compose everything

// Length of the transform rules excluding the terminating NULL.
constexpr int kTransformRulesLength =
    sizeof(kTransformRulesUtf16) / sizeof(kTransformRulesUtf16[0]) - 1;

// Transforms a Unicode character with diacritics to its counterpart in ASCII
// range. E.g. "ü" -> "u". Result will be set to char_out. Returns true if
// the transformation is successful.
//
// NOTE: According to our convention this function should have returned
// StatusOr<char>. However, this function is performance-sensitive because is
// could be called on every Latin character in normalization, so we make it
// return a bool here to save a bit more time and memory.
bool DiacriticCharToAscii(const UNormalizer2* normalizer2, UChar32 uchar32_in,
                          char* char_out) {
  if (i18n_utils::IsAscii(uchar32_in)) {
    // The Unicode character is within ASCII range
    if (char_out != nullptr) {
      *char_out = uchar32_in;
    }
    return true;
  }

  // Maximum number of pieces a Unicode character can be decomposed into.
  // TODO(tjbarron) figure out if this number is proper.
  constexpr int kDecompositionBufferCapacity = 5;

  // A buffer used to store Unicode decomposition mappings of only one
  // character.
  UChar decomposition_buffer[kDecompositionBufferCapacity];

  // Decomposes the Unicode character, trying to get an ASCII char and some
  // diacritic chars.
  UErrorCode status = U_ZERO_ERROR;
  if (unorm2_getDecomposition(normalizer2, uchar32_in, &decomposition_buffer[0],
                              kDecompositionBufferCapacity, &status) > 0 &&
      !U_FAILURE(status) && i18n_utils::IsAscii(decomposition_buffer[0])) {
    if (char_out != nullptr) {
      *char_out = decomposition_buffer[0];
    }
    return true;
  }
  return false;
}

}  // namespace

// Creates a IcuNormalizer with a valid TermTransformer instance.
//
// Note: UTokenizer2 is also an option to normalize Unicode strings, but since
// we need some custom transform rules other than NFC/NFKC we have to use
// TermTransformer as a custom transform rule executor.
libtextclassifier3::StatusOr<std::unique_ptr<IcuNormalizer>>
IcuNormalizer::Create(int max_term_byte_size) {
  ICING_ASSIGN_OR_RETURN(
      std::unique_ptr<IcuNormalizer::TermTransformer> term_transformer,
      IcuNormalizer::TermTransformer::Create());

  return std::unique_ptr<IcuNormalizer>(
      new IcuNormalizer(std::move(term_transformer), max_term_byte_size));
}

IcuNormalizer::IcuNormalizer(
    std::unique_ptr<IcuNormalizer::TermTransformer> term_transformer,
    int max_term_byte_size)
    : term_transformer_(std::move(term_transformer)),
      max_term_byte_size_(max_term_byte_size) {}

std::string IcuNormalizer::NormalizeTerm(const std::string_view term) const {
  std::string normalized_text;

  if (term.empty()) {
    return normalized_text;
  }

  UErrorCode status = U_ZERO_ERROR;
  // ICU manages the singleton instance
  const UNormalizer2* normalizer2 = unorm2_getNFCInstance(&status);
  if (U_FAILURE(status)) {
    ICING_LOG(WARNING) << "Failed to create a UNormalizer2 instance";
  }

  // Checks if the first character is within ASCII range or can be transformed
  // into an ASCII char. Since the term is tokenized, we know that the whole
  // term can be transformed into ASCII if the first character can.
  UChar32 first_uchar32 =
      i18n_utils::GetUChar32At(term.data(), term.length(), 0);
  if (normalizer2 != nullptr && first_uchar32 != i18n_utils::kInvalidUChar32 &&
      DiacriticCharToAscii(normalizer2, first_uchar32, nullptr)) {
    // This is a faster method to normalize Latin terms.
    normalized_text = NormalizeLatin(normalizer2, term);
  } else {
    normalized_text = term_transformer_->Transform(term);
  }

  if (normalized_text.length() > max_term_byte_size_) {
    i18n_utils::SafeTruncateUtf8(&normalized_text, max_term_byte_size_);
  }

  return normalized_text;
}

std::string IcuNormalizer::NormalizeLatin(const UNormalizer2* normalizer2,
                                          const std::string_view term) const {
  std::string result;
  result.reserve(term.length());
  for (int i = 0; i < term.length(); i++) {
    if (i18n_utils::IsAscii(term[i])) {
      result.push_back(std::tolower(term[i]));
    } else if (i18n_utils::IsLeadUtf8Byte(term[i])) {
      UChar32 uchar32 = i18n_utils::GetUChar32At(term.data(), term.length(), i);
      if (uchar32 == i18n_utils::kInvalidUChar32) {
        ICING_LOG(WARNING) << "Unable to get uchar32 from " << term
                           << " at position" << i;
        continue;
      }
      char ascii_char;
      if (DiacriticCharToAscii(normalizer2, uchar32, &ascii_char)) {
        result.push_back(std::tolower(ascii_char));
      } else {
        // We don't know how to transform / decompose this Unicode character, it
        // probably means that some other Unicode characters are mixed with
        // Latin characters. This shouldn't happen if input term is properly
        // tokenized. We handle it here in case there're something wrong with
        // the tokenizers.
        int utf8_length = i18n_utils::GetUtf8Length(uchar32);
        absl_ports::StrAppend(&result, term.substr(i, utf8_length));
      }
    }
  }

  return result;
}

libtextclassifier3::StatusOr<std::unique_ptr<IcuNormalizer::TermTransformer>>
IcuNormalizer::TermTransformer::Create() {
  UErrorCode status = U_ZERO_ERROR;
  UTransliterator* term_transformer = utrans_openU(
      kTransformRulesUtf16, kTransformRulesLength, UTRANS_FORWARD,
      /*rules=*/nullptr, /*rulesLength=*/0, /*parseError=*/nullptr, &status);

  if (U_FAILURE(status)) {
    return absl_ports::InternalError("Failed to create UTransliterator.");
  }

  return std::unique_ptr<IcuNormalizer::TermTransformer>(
      new IcuNormalizer::TermTransformer(term_transformer));
}

IcuNormalizer::TermTransformer::TermTransformer(
    UTransliterator* u_transliterator)
    : u_transliterator_(u_transliterator) {}

IcuNormalizer::TermTransformer::~TermTransformer() {
  if (u_transliterator_ != nullptr) {
    utrans_close(u_transliterator_);
  }
}

std::string IcuNormalizer::TermTransformer::Transform(
    const std::string_view term) const {
  auto utf16_term_or = i18n_utils::Utf8ToUtf16(term);
  if (!utf16_term_or.ok()) {
    ICING_VLOG(0) << "Failed to convert UTF8 term '" << term << "' to UTF16";
    return std::string(term);
  }
  std::u16string utf16_term = std::move(utf16_term_or).ValueOrDie();
  UErrorCode status = U_ZERO_ERROR;
  int utf16_term_desired_length = utf16_term.length();
  int limit = utf16_term.length();
  utrans_transUChars(u_transliterator_, &utf16_term[0],
                     &utf16_term_desired_length, utf16_term.length(),
                     /*start=*/0, &limit, &status);

  // For most cases, one Unicode character is normalized to exact one Unicode
  // character according to our transformation rules. However, there could be
  // some rare cases where the normalized text is longer than the original
  // one. E.g. "¼" (1 character) -> "1/4" (3 characters). That causes a buffer
  // overflow error and we need to increase our buffer size and try again.
  if (status == U_BUFFER_OVERFLOW_ERROR) {
    // 'utf16_term_desired_length' has already been set to the desired value
    // by utrans_transUChars(), here we increase the buffer size to that
    // value.
    //
    // NOTE: we need to call resize() but not reserve() because values can't
    // be set at positions after length().
    int original_content_length = utf16_term.length();
    utf16_term.resize(utf16_term_desired_length);
    utf16_term_desired_length = original_content_length;
    limit = original_content_length;
    status = U_ZERO_ERROR;
    utrans_transUChars(u_transliterator_, &utf16_term[0],
                       &utf16_term_desired_length, utf16_term.length(),
                       /*start=*/0, &limit, &status);
  }

  if (U_FAILURE(status)) {
    // Failed to transform, return its original form.
    ICING_LOG(WARNING) << "Failed to normalize UTF8 term: " << term;
    return std::string(term);
  }

  auto utf8_term_or = i18n_utils::Utf16ToUtf8(utf16_term);
  if (!utf8_term_or.ok()) {
    ICING_VLOG(0) << "Failed to convert UTF16 term '" << term << "' to UTF8";
    return std::string(term);
  }
  return std::move(utf8_term_or).ValueOrDie();
}

}  // namespace lib
}  // namespace icing
