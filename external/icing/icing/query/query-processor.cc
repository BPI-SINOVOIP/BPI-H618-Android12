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

#include "icing/query/query-processor.h"

#include <deque>
#include <memory>
#include <stack>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include "icing/text_classifier/lib3/utils/base/statusor.h"
#include "icing/absl_ports/canonical_errors.h"
#include "icing/absl_ports/str_cat.h"
#include "icing/index/index.h"
#include "icing/index/iterator/doc-hit-info-iterator-all-document-id.h"
#include "icing/index/iterator/doc-hit-info-iterator-and.h"
#include "icing/index/iterator/doc-hit-info-iterator-filter.h"
#include "icing/index/iterator/doc-hit-info-iterator-not.h"
#include "icing/index/iterator/doc-hit-info-iterator-or.h"
#include "icing/index/iterator/doc-hit-info-iterator-section-restrict.h"
#include "icing/index/iterator/doc-hit-info-iterator.h"
#include "icing/proto/search.pb.h"
#include "icing/query/query-terms.h"
#include "icing/schema/schema-store.h"
#include "icing/schema/section.h"
#include "icing/store/document-id.h"
#include "icing/store/document-store.h"
#include "icing/tokenization/language-segmenter.h"
#include "icing/tokenization/raw-query-tokenizer.h"
#include "icing/tokenization/token.h"
#include "icing/tokenization/tokenizer-factory.h"
#include "icing/tokenization/tokenizer.h"
#include "icing/transform/normalizer.h"
#include "icing/util/status-macros.h"

namespace icing {
namespace lib {

namespace {

// State of the current query parser state. This is specific to how the raw
// query is parsed/stored.
struct ParserStateFrame {
  std::vector<std::unique_ptr<DocHitInfoIterator>> and_iterators;
  std::vector<std::unique_ptr<DocHitInfoIterator>> or_iterators;

  // If the last independent token was an OR, then we need to treat the next
  // resulting iterator as part of an or_iterator
  bool saw_or = false;

  // If the last independent token was an exclusion, then we need to treat the
  // next resulting iterator as being excluded.
  bool saw_exclude = false;

  // If the last independent token was a property/section filter, then we need
  // to save the section name so we can create a section filter iterator.
  std::string_view section_restrict = "";
};

// Combines any OR and AND iterators together into one iterator.
std::unique_ptr<DocHitInfoIterator> ProcessParserStateFrame(
    ParserStateFrame parser_state_frame,
    const DocumentId last_added_document_id) {
  if (parser_state_frame.and_iterators.empty() &&
      parser_state_frame.or_iterators.empty()) {
    // No terms specified, treat an empty query as retrieving all documents.
    //
    // We don't use the index_.last_added_document_id here because it's possible
    // that documents exist in the DocumentStore, but were not successfully
    // indexed. So to return *all* documents and not just *all indexed*
    // documents, we use the DocumentStore's last_added_document_id
    return std::make_unique<DocHitInfoIteratorAllDocumentId>(
        last_added_document_id);
  }

  if (!parser_state_frame.or_iterators.empty()) {
    // Combine all the ORs first since they have higher priority, then add it to
    // the ANDs.
    parser_state_frame.and_iterators.push_back(
        CreateOrIterator(std::move(parser_state_frame.or_iterators)));
  }
  return CreateAndIterator(std::move(parser_state_frame.and_iterators));
}

}  // namespace

libtextclassifier3::StatusOr<std::unique_ptr<QueryProcessor>>
QueryProcessor::Create(Index* index,
                       const LanguageSegmenter* language_segmenter,
                       const Normalizer* normalizer,
                       const DocumentStore* document_store,
                       const SchemaStore* schema_store) {
  ICING_RETURN_ERROR_IF_NULL(index);
  ICING_RETURN_ERROR_IF_NULL(language_segmenter);
  ICING_RETURN_ERROR_IF_NULL(normalizer);
  ICING_RETURN_ERROR_IF_NULL(document_store);
  ICING_RETURN_ERROR_IF_NULL(schema_store);

  return std::unique_ptr<QueryProcessor>(new QueryProcessor(
      index, language_segmenter, normalizer, document_store, schema_store));
}

QueryProcessor::QueryProcessor(Index* index,
                               const LanguageSegmenter* language_segmenter,
                               const Normalizer* normalizer,
                               const DocumentStore* document_store,
                               const SchemaStore* schema_store)
    : index_(*index),
      language_segmenter_(*language_segmenter),
      normalizer_(*normalizer),
      document_store_(*document_store),
      schema_store_(*schema_store) {}

DocHitInfoIteratorFilter::Options QueryProcessor::getFilterOptions(
    const SearchSpecProto& search_spec) {
  DocHitInfoIteratorFilter::Options options;

  if (search_spec.namespace_filters_size() > 0) {
    options.namespaces =
        std::vector<std::string_view>(search_spec.namespace_filters().begin(),
                                      search_spec.namespace_filters().end());
  }

  if (search_spec.schema_type_filters_size() > 0) {
    options.schema_types =
        std::vector<std::string_view>(search_spec.schema_type_filters().begin(),
                                      search_spec.schema_type_filters().end());
  }
  return options;
}

libtextclassifier3::StatusOr<QueryProcessor::QueryResults>
QueryProcessor::ParseSearch(const SearchSpecProto& search_spec) {
  ICING_ASSIGN_OR_RETURN(QueryResults results, ParseRawQuery(search_spec));

  DocHitInfoIteratorFilter::Options options = getFilterOptions(search_spec);
  results.root_iterator = std::make_unique<DocHitInfoIteratorFilter>(
      std::move(results.root_iterator), &document_store_, &schema_store_,
      options);
  return results;
}

// TODO(cassiewang): Collect query stats to populate the SearchResultsProto
libtextclassifier3::StatusOr<QueryProcessor::QueryResults>
QueryProcessor::ParseRawQuery(const SearchSpecProto& search_spec) {
  DocHitInfoIteratorFilter::Options options = getFilterOptions(search_spec);

  // Tokenize the incoming raw query
  //
  // TODO(cassiewang): Consider caching/creating a tokenizer factory that will
  // cache the n most recently used tokenizers. So we don't have to recreate
  // this on every new query, if they'll all be raw queries.
  ICING_ASSIGN_OR_RETURN(
      std::unique_ptr<Tokenizer> raw_query_tokenizer,
      tokenizer_factory::CreateQueryTokenizer(tokenizer_factory::RAW_QUERY,
                                              &language_segmenter_));

  ICING_ASSIGN_OR_RETURN(std::vector<Token> tokens,
                         raw_query_tokenizer->TokenizeAll(search_spec.query()));

  std::stack<ParserStateFrame> frames;
  frames.emplace();

  QueryResults results;
  // Process all the tokens
  for (int i = 0; i < tokens.size(); i++) {
    const Token& token = tokens.at(i);
    std::unique_ptr<DocHitInfoIterator> result_iterator;

    // TODO(cassiewang): Handle negation tokens
    switch (token.type) {
      case Token::Type::QUERY_LEFT_PARENTHESES: {
        frames.emplace(ParserStateFrame());
        break;
      }
      case Token::Type::QUERY_RIGHT_PARENTHESES: {
        if (frames.empty()) {
          return absl_ports::InternalError(
              "Encountered empty stack of ParserStateFrames");
        }
        result_iterator = ProcessParserStateFrame(
            std::move(frames.top()), document_store_.last_added_document_id());
        frames.pop();
        break;
      }
      case Token::Type::QUERY_EXCLUSION: {
        if (frames.empty()) {
          return absl_ports::InternalError(
              "Encountered empty stack of ParserStateFrames");
        }
        frames.top().saw_exclude = true;
        break;
      }
      case Token::Type::QUERY_OR: {
        if (frames.empty()) {
          return absl_ports::InternalError(
              "Encountered empty stack of ParserStateFrames");
        }
        frames.top().saw_or = true;
        break;
      }
      case Token::Type::QUERY_PROPERTY: {
        if (frames.empty()) {
          return absl_ports::InternalError(
              "Encountered empty stack of ParserStateFrames");
        }

        frames.top().section_restrict = token.text;
        break;
      }
      case Token::Type::REGULAR: {
        if (frames.empty()) {
          return absl_ports::InternalError(
              "Encountered empty stack of ParserStateFrames");
        }

        std::string normalized_text = normalizer_.NormalizeTerm(token.text);

        // TODO(cassiewang): Consider removing the use of a section mask in the
        // term iterator, or constructing a best-effort SectionIdMask based on
        // the section filter. For some combination of schema type filters and
        // section filters, we can't encapsulate the perfect
        // SchemaTypeId-SectionId sets with just a SectionIdMask. So we
        // over-retrieve hits and have to do a post-filter anyways. With a
        // SectionIdMask, we might be able to narrow down our SectionIds, but
        // we'll still over-retrieve hits a bit. So at that point, it's a
        // tradeoff of
        //
        //   1.1 Go to SchemaStore and iterate over the schema to calculate a
        //   SectionIdMask
        //   1.2 Use SectionIdMask and save some hit buffer memory
        //   1.3 Do a post-filter to double check SchemaTypeId-SectionId combo
        //
        // vs
        //
        //   2.1 Use SectionIdMaskAll and use more hit buffer memory
        //   2.2 Do a post-filter to double check SchemaTypeId-SectionId combo
        //
        // We do the same amount of disk reads, so it may be dependent on how
        // big the schema is and/or how popular schema type filtering and
        // section filtering is.

        ICING_ASSIGN_OR_RETURN(
            result_iterator,
            index_.GetIterator(normalized_text, kSectionIdMaskAll,
                               search_spec.term_match_type()));

        // Add term iterator and terms to match if this is not a negation term.
        // WARNING: setting query terms at this point is not compatible with
        // group-level excludes, group-level sections restricts or excluded
        // section restricts. Those are not currently supported. If they became
        // supported, this handling for query terms would need to be altered.
        if (!frames.top().saw_exclude) {
          ICING_ASSIGN_OR_RETURN(
              std::unique_ptr<DocHitInfoIterator> term_iterator,
              index_.GetIterator(normalized_text, kSectionIdMaskAll,
                                 search_spec.term_match_type()));

          results.query_term_iterators[normalized_text] =
              std::make_unique<DocHitInfoIteratorFilter>(
                  std::move(term_iterator), &document_store_, &schema_store_,
                  options);

          results.query_terms[frames.top().section_restrict].insert(
              std::move(normalized_text));
        }
        break;
      }
      case Token::Type::INVALID:
        [[fallthrough]];
      default:
        // This wouldn't happen if tokenizer and query processor both work
        // correctly. An unknown token indicates inconsistency between tokenizer
        // and query processor, so we return an internal error here.
        return absl_ports::InternalError(absl_ports::StrCat(
            "Encountered unknown token while processing query: ", token.text));
    }

    // Did we get an iterator out of this token?
    if (result_iterator) {
      if (frames.empty()) {
        return absl_ports::InternalError(
            "Encountered empty stack of ParserStateFrames");
      }

      // NOTE: Order matters!! We must apply the section restrict first, then
      // the NOT operator.
      //
      // Imagine a query like [-subject:foo] which means we
      // want to get documents that don't have the term 'foo' in section
      // 'subject'.
      //
      // Assume some Document_0:
      //   { "subject": "foo" }
      //
      // And assume some Document_1:
      //   { "subject": "bar" }
      //
      // If we use the IteratorNot first, then we'll get DocHitInfos that
      // represent DocumentIds without any section hits like
      // DocHitInfo(document_id_1, kSectionIdMaskNone). Then, when we try to
      // apply the IteratorSectionRestrict, no SectionIds in the mask will match
      // the SectionId of 'subject' and we won't return any results.
      //
      // If we use the IteratorSectionRestrict first, then we'll get a
      // DocHitInfo for Document_0. Then with the IteratorNot, we can get the
      // rest of the Documents excluding Document_0, and get Document_1 as a
      // correct result.
      //
      // TODO(cassiewang): The point is a bit moot right now since we don't even
      // support this functionality. But add tests for this once we do support
      // more advanced section restricts with grouping, negation, etc.
      if (!frames.top().section_restrict.empty()) {
        // We saw a section restrict earlier, wrap the result iterator in
        // the section restrict
        result_iterator = std::make_unique<DocHitInfoIteratorSectionRestrict>(
            std::move(result_iterator), &document_store_, &schema_store_,
            frames.top().section_restrict);

        frames.top().section_restrict = "";
      }

      // Check if we need to NOT/exclude this iterator
      if (frames.top().saw_exclude) {
        result_iterator = std::make_unique<DocHitInfoIteratorNot>(
            std::move(result_iterator),
            document_store_.last_added_document_id());
        frames.top().saw_exclude = false;
      }

      if (i < tokens.size() - 1 &&
          tokens.at(i + 1).type == Token::Type::QUERY_OR) {
        // This isn't the last token, and the next token is an OR. Then we
        // should OR this iterator with the next iterator, (e.g. if the query
        // was "A OR B", we would be processing "A" right now)
        frames.top().or_iterators.push_back(std::move(result_iterator));
      } else if (frames.top().saw_or) {
        // This isn't the first token, and the previous token was an OR. Then
        // we should OR this iterator with the previous iterator (e.g. if the
        // query was "A OR (B C)", we would be processing the iterator for "(B
        // C)" right now)
        frames.top().or_iterators.push_back(std::move(result_iterator));
        frames.top().saw_or = false;
      } else {
        // If we're not trying to OR this iterator, we AND everything else.
        if (!frames.top().or_iterators.empty()) {
          // Accumulate the previous OR iterators if there were any.
          frames.top().and_iterators.push_back(
              CreateOrIterator(std::move(frames.top().or_iterators)));
          frames.top().or_iterators =
              std::vector<std::unique_ptr<DocHitInfoIterator>>();
        }
        frames.top().and_iterators.push_back(std::move(result_iterator));
      }
    }
  }

  // Guaranteed that we have some iterators to return. Need to do one last
  // combining since we could have ORs and ANDs.
  if (frames.size() != 1) {
    return absl_ports::InternalError(
        "Encountered invalid state of ParserStateFrames stack");
  }
  results.root_iterator = ProcessParserStateFrame(
      std::move(frames.top()), document_store_.last_added_document_id());
  return results;
}

}  // namespace lib
}  // namespace icing
