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

#ifndef ICING_RESULT_RETRIEVER_H_
#define ICING_RESULT_RETRIEVER_H_

#include <utility>
#include <vector>

#include "icing/text_classifier/lib3/utils/base/statusor.h"
#include "icing/proto/search.pb.h"
#include "icing/proto/term.pb.h"
#include "icing/query/query-terms.h"
#include "icing/result/page-result-state.h"
#include "icing/result/snippet-context.h"
#include "icing/result/snippet-retriever.h"
#include "icing/schema/schema-store.h"
#include "icing/schema/section.h"
#include "icing/scoring/scored-document-hit.h"
#include "icing/store/document-id.h"
#include "icing/store/document-store.h"
#include "icing/tokenization/language-segmenter.h"
#include "icing/transform/normalizer.h"

namespace icing {
namespace lib {

class ResultRetriever {
 public:
  // Factory function to create a ResultRetriever which does not take ownership
  // of any input components, and all pointers must refer to valid objects that
  // outlive the created ResultRetriever instance.
  //
  // Returns:
  //   A ResultRetriever on success
  //   FAILED_PRECONDITION on any null pointer input
  static libtextclassifier3::StatusOr<std::unique_ptr<ResultRetriever>> Create(
      const DocumentStore* doc_store, const SchemaStore* schema_store,
      const LanguageSegmenter* language_segmenter, const Normalizer* normalizer,
      bool ignore_bad_document_ids = true);

  // Retrieves results (pairs of DocumentProtos and SnippetProtos) with the
  // given document and snippet information. The expected number of documents to
  // return is the number of all scored document hits inside PageResultState.
  // The number of snippets to return is based on the total number of snippets
  // needed and number of snippets that have already been returned previously
  // for the same query. The order of results returned is the same as the order
  // of scored document hits inside PageResultState.
  //
  // "ignore_bad_document_ids" from constructor indicates whether to ignore
  // invalid and non-existing document ids. If it's true, errors on some
  // document ids will be ignored and valid documents will be returned,
  // otherwise any error will be returned immediately. Note that IO errors will
  // always be returned.
  //
  // Returns when ignore_bad_document_ids is true:
  //   A list of ResultProto on success
  //   INTERNAL_ERROR on IO error
  //
  // Returns when ignore_bad_document_ids is false:
  //   A list of ResultProto on success
  //   INVALID_ARGUMENT if any document_id < 0
  //   NOT_FOUND if any doc doesn't exist or has been deleted
  //   INTERNAL_ERROR on IO error
  libtextclassifier3::StatusOr<std::vector<SearchResultProto::ResultProto>>
  RetrieveResults(const PageResultState& page_result_state) const;

 private:
  explicit ResultRetriever(const DocumentStore* doc_store,
                           std::unique_ptr<SnippetRetriever> snippet_retriever,
                           bool ignore_bad_document_ids)
      : doc_store_(*doc_store),
        snippet_retriever_(std::move(snippet_retriever)),
        ignore_bad_document_ids_(ignore_bad_document_ids) {}

  const DocumentStore& doc_store_;
  std::unique_ptr<SnippetRetriever> snippet_retriever_;
  const bool ignore_bad_document_ids_;
};

}  // namespace lib
}  // namespace icing

#endif  // ICING_RESULT_RETRIEVER_H_
