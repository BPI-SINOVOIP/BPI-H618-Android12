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

#ifndef ICING_TESTING_RANDOM_STRING_H_
#define ICING_TESTING_RANDOM_STRING_H_

#include <random>
#include <string>

namespace icing {
namespace lib {

inline constexpr std::string_view kAlNumAlphabet =
    "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";

template <typename Gen>
std::string RandomString(const std::string_view alphabet, size_t len,
                         Gen* gen) {
  std::uniform_int_distribution<size_t> uniform(0u, alphabet.size() - 1);
  std::string result(len, '\0');
  std::generate(
      std::begin(result), std::end(result),
      [&gen, &alphabet, &uniform]() { return alphabet[uniform(*gen)]; });

  return result;
}

}  // namespace lib
}  // namespace icing

#endif  // ICING_TESTING_RANDOM_STRING_H_
