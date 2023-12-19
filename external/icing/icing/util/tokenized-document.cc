// Copyright (C) 2020 Google LLC
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

#include "icing/util/tokenized-document.h"

#include <string>
#include <string_view>
#include <vector>

#include "icing/text_classifier/lib3/utils/base/status.h"
#include "icing/proto/document.pb.h"
#include "icing/schema/schema-store.h"
#include "icing/schema/section.h"
#include "icing/tokenization/language-segmenter.h"
#include "icing/tokenization/tokenizer-factory.h"
#include "icing/tokenization/tokenizer.h"
#include "icing/util/document-validator.h"
#include "icing/util/status-macros.h"

namespace icing {
namespace lib {

libtextclassifier3::StatusOr<TokenizedDocument> TokenizedDocument::Create(
    const SchemaStore* schema_store,
    const LanguageSegmenter* language_segmenter, DocumentProto document) {
  TokenizedDocument tokenized_document(std::move(document));
  ICING_RETURN_IF_ERROR(
      tokenized_document.Tokenize(schema_store, language_segmenter));
  return tokenized_document;
}

TokenizedDocument::TokenizedDocument(DocumentProto document)
    : document_(std::move(document)) {}

libtextclassifier3::Status TokenizedDocument::Tokenize(
    const SchemaStore* schema_store,
    const LanguageSegmenter* language_segmenter) {
  DocumentValidator validator(schema_store);
  ICING_RETURN_IF_ERROR(validator.Validate(document_));

  ICING_ASSIGN_OR_RETURN(std::vector<Section> sections,
                         schema_store->ExtractSections(document_));
  for (const Section& section : sections) {
    ICING_ASSIGN_OR_RETURN(std::unique_ptr<Tokenizer> tokenizer,
                           tokenizer_factory::CreateIndexingTokenizer(
                               section.metadata.tokenizer, language_segmenter));
    std::vector<std::string_view> token_sequence;
    for (std::string_view subcontent : section.content) {
      ICING_ASSIGN_OR_RETURN(std::unique_ptr<Tokenizer::Iterator> itr,
                             tokenizer->Tokenize(subcontent));
      while (itr->Advance()) {
        token_sequence.push_back(itr->GetToken().text);
      }
    }
    tokenized_sections_.emplace_back(SectionMetadata(section.metadata),
                                     std::move(token_sequence));
  }

  return libtextclassifier3::Status::OK;
}

}  // namespace lib
}  // namespace icing
