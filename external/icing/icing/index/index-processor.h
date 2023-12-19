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

#ifndef ICING_INDEX_INDEX_PROCESSOR_H_
#define ICING_INDEX_INDEX_PROCESSOR_H_

#include <cstdint>
#include <string>

#include "icing/text_classifier/lib3/utils/base/status.h"
#include "icing/index/index.h"
#include "icing/proto/document.pb.h"
#include "icing/schema/section-manager.h"
#include "icing/store/document-id.h"
#include "icing/tokenization/token.h"
#include "icing/transform/normalizer.h"
#include "icing/util/tokenized-document.h"

namespace icing {
namespace lib {

class IndexProcessor {
 public:
  struct Options {
    int32_t max_tokens_per_document;

    // Indicates how a document exceeding max_tokens_per_document should be
    // handled.
    enum class TokenLimitBehavior {
      // When set, the first max_tokens_per_document will be indexed. If the
      // token count exceeds max_tokens_per_document, a ResourceExhausted error
      // will be returned.
      kReturnError,
      // When set, the first max_tokens_per_document will be indexed. If the
      // token count exceeds max_tokens_per_document, OK will be returned.
      kSuppressError,
    };
    TokenLimitBehavior token_limit_behavior;
  };

  // Factory function to create an IndexProcessor which does not take ownership
  // of any input components, and all pointers must refer to valid objects that
  // outlive the created IndexProcessor instance.
  //
  // Returns:
  //   An IndexProcessor on success
  //   FAILED_PRECONDITION if any of the pointers is null.
  static libtextclassifier3::StatusOr<std::unique_ptr<IndexProcessor>> Create(
      const Normalizer* normalizer, Index* index, const Options& options,
      const Clock* clock);

  // Add tokenized document to the index, associated with document_id. If the
  // number of tokens in the document exceeds max_tokens_per_document, then only
  // the first max_tokens_per_document will be added to the index. All tokens of
  // length exceeding max_token_length will be shortened to max_token_length.
  //
  // Indexing a document *may* trigger an index merge. If a merge fails, then
  // all content in the index will be lost.
  //
  // If put_document_stats is present, the fields related to indexing will be
  // populated.
  //
  // Returns:
  //   INVALID_ARGUMENT if document_id is less than the document_id of a
  //   previously indexed document or tokenization fails.
  //   RESOURCE_EXHAUSTED if the index is full and can't add anymore content.
  //   DATA_LOSS if an attempt to merge the index fails and both indices are
  //       cleared as a result.
  //   NOT_FOUND if there is no definition for the document's schema type.
  //   INTERNAL_ERROR if any other errors occur
  libtextclassifier3::Status IndexDocument(
      const TokenizedDocument& tokenized_document, DocumentId document_id,
      PutDocumentStatsProto* put_document_stats = nullptr);

 private:
  IndexProcessor(const Normalizer* normalizer, Index* index,
                 const Options& options, const Clock* clock)
      : normalizer_(*normalizer),
        index_(index),
        options_(options),
        clock_(*clock) {}

  std::string NormalizeToken(const Token& token);

  const Normalizer& normalizer_;
  Index* const index_;
  const Options options_;
  const Clock& clock_;
};

}  // namespace lib
}  // namespace icing

#endif  // ICING_INDEX_INDEX_PROCESSOR_H_
