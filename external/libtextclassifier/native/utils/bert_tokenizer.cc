/*
 * Copyright (C) 2018 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "utils/bert_tokenizer.h"

#include <string>

#include "annotator/types.h"
#include "utils/tokenizer-utils.h"
#include "utils/utf8/unicodetext.h"
#include "utils/utf8/unilib.h"
#include "absl/strings/string_view.h"

namespace libtextclassifier3 {

FlatHashMapBackedWordpiece::FlatHashMapBackedWordpiece(
    const std::vector<std::string>& vocab)
    : vocab_{vocab} {
  for (int i = 0; i < vocab_.size(); ++i) {
    index_map_[vocab_[i]] = i;
  }
}

LookupStatus FlatHashMapBackedWordpiece::Contains(absl::string_view key,
                                                  bool* value) const {
  *value = index_map_.contains(key);
  return LookupStatus();
}

bool FlatHashMapBackedWordpiece::LookupId(const absl::string_view key,
                                          int* result) const {
  auto it = index_map_.find(key);
  if (it == index_map_.end()) {
    return false;
  }
  *result = it->second;
  return true;
}

bool FlatHashMapBackedWordpiece::LookupWord(int vocab_id,
                                            absl::string_view* result) const {
  if (vocab_id >= vocab_.size() || vocab_id < 0) {
    return false;
  }
  *result = vocab_[vocab_id];
  return true;
}

TokenizerResult BertTokenizer::TokenizeSingleToken(const std::string& token) {
  std::vector<std::string> tokens = {token};
  return BertTokenizer::Tokenize(tokens);
}

TokenizerResult BertTokenizer::Tokenize(const std::string& input) {
  std::vector<std::string> tokens = PreTokenize(input);
  return BertTokenizer::Tokenize(tokens);
}

TokenizerResult BertTokenizer::Tokenize(
    const std::vector<std::string>& tokens) {
  WordpieceTokenizerResult result;
  std::vector<std::string>& subwords = result.subwords;
  std::vector<int>& wp_absolute_begin_offset = result.wp_begin_offset;
  std::vector<int>& wp_absolute_end_offset = result.wp_end_offset;

  for (int token_index = 0; token_index < tokens.size(); token_index++) {
    auto& token = tokens[token_index];
    int num_word_pieces = 0;
    LookupStatus status = WordpieceTokenize(
        token, options_.max_bytes_per_token, options_.max_chars_per_subtoken,
        options_.suffix_indicator, options_.use_unknown_token,
        options_.unknown_token, options_.split_unknown_chars, &vocab_,
        &subwords, &wp_absolute_begin_offset, &wp_absolute_end_offset,
        &num_word_pieces);

    if (!status.success) {
      return std::move(result);
    }
  }

  return std::move(result);
}

// This replicates how the original bert_tokenizer from the tflite-support
// library pretokenize text by using regex_split with these default regexes.
// It splits the text on spaces, punctuations and chinese characters and
// output all the tokens except spaces.
// So far, the only difference between this and the original implementation
// we are aware of is that the original regexes has 8 ranges of chinese
// unicodes. We have all these 8 ranges plus two extra ranges.
std::vector<std::string> BertTokenizer::PreTokenize(
    const absl::string_view input) {
  const std::vector<Token> tokens =
      TokenizeOnWhiteSpacePunctuationAndChineseLetter(input);
  std::vector<std::string> token_texts;
  std::transform(tokens.begin(), tokens.end(), std::back_inserter(token_texts),
                 [](Token const& token) { return std::move(token.value); });

  return token_texts;
}

}  // namespace libtextclassifier3
