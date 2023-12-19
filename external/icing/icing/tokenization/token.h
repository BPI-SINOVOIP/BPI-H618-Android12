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

#ifndef ICING_TOKENIZATION_TOKEN_H_
#define ICING_TOKENIZATION_TOKEN_H_

#include <string_view>

namespace icing {
namespace lib {

struct Token {
  enum Type {
    // Common types
    REGULAR,  // A token without special meanings, the value of it will be
              // indexed or searched directly

    // Types only used in raw query
    QUERY_OR,         // Indicates OR logic between its left and right tokens
    QUERY_EXCLUSION,  // Indicates exclusion operation on next token
    QUERY_PROPERTY,   // Indicates property restrict on next token
    QUERY_LEFT_PARENTHESES,   // Left parentheses
    QUERY_RIGHT_PARENTHESES,  // Right parentheses

    // Indicates errors
    INVALID,
  };

  // The input text should outlive the Token instance.
  explicit Token(Type type_in, std::string_view text_in = "")
      : type(type_in), text(text_in) {}

  // The type of token
  const Type type;

  // The content of token
  const std::string_view text;
};

}  // namespace lib
}  // namespace icing

#endif  // ICING_TOKENIZATION_TOKEN_H_
