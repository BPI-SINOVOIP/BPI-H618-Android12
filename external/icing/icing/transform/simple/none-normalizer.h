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

#ifndef ICING_TRANSFORM_SIMPLE_NONE_NORMALIZER_H_
#define ICING_TRANSFORM_SIMPLE_NONE_NORMALIZER_H_

#include <string>
#include <string_view>

#include "icing/transform/normalizer.h"

namespace icing {
namespace lib {

// This normalizer is not meant for production use. Currently only used to get
// the Icing library to compile in Jetpack.
//
// No normalization is done, but the term is truncated if it exceeds
// max_term_byte_size.
class NoneNormalizer : public Normalizer {
 public:
  explicit NoneNormalizer(int max_term_byte_size)
      : max_term_byte_size_(max_term_byte_size){};

  std::string NormalizeTerm(std::string_view term) const override {
    if (term.length() > max_term_byte_size_) {
      return std::string(term.substr(0, max_term_byte_size_));
    }
    return std::string(term);
  }

 private:
  // The maximum term length allowed after normalization.
  int max_term_byte_size_;
};

}  // namespace lib
}  // namespace icing

#endif  // ICING_TRANSFORM_SIMPLE_NONE_NORMALIZER_H_
