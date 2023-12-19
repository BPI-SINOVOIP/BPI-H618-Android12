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

#include "icing/util/character-iterator.h"

namespace icing {
namespace lib {

namespace {

// Returns the lead byte of the UTF-8 character that includes the byte at
// current_byte_index within it.
int GetUTF8StartPosition(std::string_view text, int current_byte_index) {
  while (!i18n_utils::IsLeadUtf8Byte(text[current_byte_index])) {
    --current_byte_index;
  }
  return current_byte_index;
}

}  // namespace

bool CharacterIterator::MoveToUtf8(int desired_utf8_index) {
  return (desired_utf8_index > utf8_index_) ? AdvanceToUtf8(desired_utf8_index)
                                            : RewindToUtf8(desired_utf8_index);
}

bool CharacterIterator::AdvanceToUtf8(int desired_utf8_index) {
  if (desired_utf8_index > text_.length()) {
    // Enforce the requirement.
    return false;
  }
  // Need to work forwards.
  while (utf8_index_ < desired_utf8_index) {
    UChar32 uchar32 =
        i18n_utils::GetUChar32At(text_.data(), text_.length(), utf8_index_);
    if (uchar32 == i18n_utils::kInvalidUChar32) {
      // Unable to retrieve a valid UTF-32 character at the previous position.
      return false;
    }
    int utf8_length = i18n_utils::GetUtf8Length(uchar32);
    if (utf8_index_ + utf8_length > desired_utf8_index) {
      // Ah! Don't go too far!
      break;
    }
    utf8_index_ += utf8_length;
    utf16_index_ += i18n_utils::GetUtf16Length(uchar32);
    ++utf32_index_;
  }
  return true;
}

bool CharacterIterator::RewindToUtf8(int desired_utf8_index) {
  if (desired_utf8_index < 0) {
    // Enforce the requirement.
    return false;
  }
  // Need to work backwards.
  while (utf8_index_ > desired_utf8_index) {
    --utf8_index_;
    utf8_index_ = GetUTF8StartPosition(text_, utf8_index_);
    if (utf8_index_ < 0) {
      // Somehow, there wasn't a single UTF-8 lead byte at
      // requested_byte_index or an earlier byte.
      return false;
    }
    // We've found the start of a unicode char!
    UChar32 uchar32 =
        i18n_utils::GetUChar32At(text_.data(), text_.length(), utf8_index_);
    if (uchar32 == i18n_utils::kInvalidUChar32) {
      // Unable to retrieve a valid UTF-32 character at the previous position.
      return false;
    }
    utf16_index_ -= i18n_utils::GetUtf16Length(uchar32);
    --utf32_index_;
  }
  return true;
}

bool CharacterIterator::MoveToUtf16(int desired_utf16_index) {
  return (desired_utf16_index > utf16_index_)
             ? AdvanceToUtf16(desired_utf16_index)
             : RewindToUtf16(desired_utf16_index);
}

bool CharacterIterator::AdvanceToUtf16(int desired_utf16_index) {
  while (utf16_index_ < desired_utf16_index) {
    UChar32 uchar32 =
        i18n_utils::GetUChar32At(text_.data(), text_.length(), utf8_index_);
    if (uchar32 == i18n_utils::kInvalidUChar32) {
      // Unable to retrieve a valid UTF-32 character at the previous position.
      return false;
    }
    int utf16_length = i18n_utils::GetUtf16Length(uchar32);
    if (utf16_index_ + utf16_length > desired_utf16_index) {
      // Ah! Don't go too far!
      break;
    }
    int utf8_length = i18n_utils::GetUtf8Length(uchar32);
    if (utf8_index_ + utf8_length > text_.length()) {
      // Enforce the requirement.
      return false;
    }
    utf8_index_ += utf8_length;
    utf16_index_ += utf16_length;
    ++utf32_index_;
  }
  return true;
}

bool CharacterIterator::RewindToUtf16(int desired_utf16_index) {
  if (desired_utf16_index < 0) {
    return false;
  }
  while (utf16_index_ > desired_utf16_index) {
    --utf8_index_;
    utf8_index_ = GetUTF8StartPosition(text_, utf8_index_);
    if (utf8_index_ < 0) {
      // Somehow, there wasn't a single UTF-8 lead byte at
      // requested_byte_index or an earlier byte.
      return false;
    }
    // We've found the start of a unicode char!
    UChar32 uchar32 =
        i18n_utils::GetUChar32At(text_.data(), text_.length(), utf8_index_);
    if (uchar32 == i18n_utils::kInvalidUChar32) {
      // Unable to retrieve a valid UTF-32 character at the previous position.
      return false;
    }
    utf16_index_ -= i18n_utils::GetUtf16Length(uchar32);
    --utf32_index_;
  }
  return true;
}

bool CharacterIterator::MoveToUtf32(int desired_utf32_index) {
  return (desired_utf32_index > utf32_index_)
             ? AdvanceToUtf32(desired_utf32_index)
             : RewindToUtf32(desired_utf32_index);
}

bool CharacterIterator::AdvanceToUtf32(int desired_utf32_index) {
  while (utf32_index_ < desired_utf32_index) {
    UChar32 uchar32 =
        i18n_utils::GetUChar32At(text_.data(), text_.length(), utf8_index_);
    if (uchar32 == i18n_utils::kInvalidUChar32) {
      // Unable to retrieve a valid UTF-32 character at the previous position.
      return false;
    }
    int utf16_length = i18n_utils::GetUtf16Length(uchar32);
    int utf8_length = i18n_utils::GetUtf8Length(uchar32);
    if (utf8_index_ + utf8_length > text_.length()) {
      // Enforce the requirement.
      return false;
    }
    utf8_index_ += utf8_length;
    utf16_index_ += utf16_length;
    ++utf32_index_;
  }
  return true;
}

bool CharacterIterator::RewindToUtf32(int desired_utf32_index) {
  if (desired_utf32_index < 0) {
    return false;
  }
  while (utf32_index_ > desired_utf32_index) {
    --utf8_index_;
    utf8_index_ = GetUTF8StartPosition(text_, utf8_index_);
    if (utf8_index_ < 0) {
      // Somehow, there wasn't a single UTF-8 lead byte at
      // requested_byte_index or an earlier byte.
      return false;
    }
    // We've found the start of a unicode char!
    UChar32 uchar32 =
        i18n_utils::GetUChar32At(text_.data(), text_.length(), utf8_index_);
    if (uchar32 == i18n_utils::kInvalidUChar32) {
      // Unable to retrieve a valid UTF-32 character at the previous position.
      return false;
    }
    utf16_index_ -= i18n_utils::GetUtf16Length(uchar32);
    --utf32_index_;
  }
  return true;
}

}  // namespace lib
}  // namespace icing
