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

#ifndef ICING_TESTING_COMMON_MATCHERS_H_
#define ICING_TESTING_COMMON_MATCHERS_H_

#include <cmath>

#include "icing/text_classifier/lib3/utils/base/status.h"
#include "icing/text_classifier/lib3/utils/base/status_macros.h"
#include "icing/text_classifier/lib3/utils/base/statusor.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "icing/absl_ports/str_join.h"
#include "icing/index/hit/doc-hit-info.h"
#include "icing/legacy/core/icing-string-util.h"
#include "icing/proto/search.pb.h"
#include "icing/schema/schema-store.h"
#include "icing/schema/section.h"
#include "icing/util/status-macros.h"

namespace icing {
namespace lib {

// Used to match Token(Token::Type type, std::string_view text)
MATCHER_P2(EqualsToken, type, text, "") {
  std::string arg_string(arg.text.data(), arg.text.length());
  if (arg.type != type || arg.text != text) {
    *result_listener << IcingStringUtil::StringPrintf(
        "(Expected: type=%d, text=\"%s\". Actual: type=%d, text=\"%s\")", type,
        text, arg.type, arg_string.c_str());
    return false;
  }
  return true;
}

// Used to match a DocHitInfo
MATCHER_P2(EqualsDocHitInfo, document_id, section_ids, "") {
  const DocHitInfo& actual = arg;
  SectionIdMask section_mask = kSectionIdMaskNone;
  for (SectionId section_id : section_ids) {
    section_mask |= 1U << section_id;
  }
  *result_listener << IcingStringUtil::StringPrintf(
      "(actual is {document_id=%d, section_mask=%d}, but expected was "
      "{document_id=%d, section_mask=%d}.)",
      actual.document_id(), actual.hit_section_ids_mask(), document_id,
      section_mask);
  return actual.document_id() == document_id &&
         actual.hit_section_ids_mask() == section_mask;
}

// Used to match a DocHitInfo
MATCHER_P2(EqualsDocHitInfoWithTermFrequency, document_id,
           section_ids_to_term_frequencies_map, "") {
  const DocHitInfo& actual = arg;
  SectionIdMask section_mask = kSectionIdMaskNone;

  bool term_frequency_as_expected = true;
  std::vector<Hit::TermFrequency> expected_tfs;
  std::vector<Hit::TermFrequency> actual_tfs;
  for (auto itr = section_ids_to_term_frequencies_map.begin();
       itr != section_ids_to_term_frequencies_map.end(); itr++) {
    SectionId section_id = itr->first;
    section_mask |= 1U << section_id;
    expected_tfs.push_back(itr->second);
    actual_tfs.push_back(actual.hit_term_frequency(section_id));
    if (actual.hit_term_frequency(section_id) != itr->second) {
      term_frequency_as_expected = false;
    }
  }
  std::string actual_term_frequencies = absl_ports::StrCat(
      "[", absl_ports::StrJoin(actual_tfs, ",", absl_ports::NumberFormatter()),
      "]");
  std::string expected_term_frequencies = absl_ports::StrCat(
      "[",
      absl_ports::StrJoin(expected_tfs, ",", absl_ports::NumberFormatter()),
      "]");
  *result_listener << IcingStringUtil::StringPrintf(
      "(actual is {document_id=%d, section_mask=%d, term_frequencies=%s}, but "
      "expected was "
      "{document_id=%d, section_mask=%d, term_frequencies=%s}.)",
      actual.document_id(), actual.hit_section_ids_mask(),
      actual_term_frequencies.c_str(), document_id, section_mask,
      expected_term_frequencies.c_str());
  return actual.document_id() == document_id &&
         actual.hit_section_ids_mask() == section_mask &&
         term_frequency_as_expected;
}

// Used to match a ScoredDocumentHit
MATCHER_P(EqualsScoredDocumentHit, expected_scored_document_hit, "") {
  if (arg.document_id() != expected_scored_document_hit.document_id() ||
      arg.hit_section_id_mask() !=
          expected_scored_document_hit.hit_section_id_mask() ||
      std::fabs(arg.score() - expected_scored_document_hit.score()) > 1e-6) {
    *result_listener << IcingStringUtil::StringPrintf(
        "Expected: document_id=%d, hit_section_id_mask=%d, score=%.2f. Actual: "
        "document_id=%d, hit_section_id_mask=%d, score=%.2f",
        expected_scored_document_hit.document_id(),
        expected_scored_document_hit.hit_section_id_mask(),
        expected_scored_document_hit.score(), arg.document_id(),
        arg.hit_section_id_mask(), arg.score());
    return false;
  }
  return true;
}

MATCHER_P(EqualsSetSchemaResult, expected, "") {
  const SchemaStore::SetSchemaResult& actual = arg;

  if (actual.success == expected.success &&
      actual.index_incompatible == expected.index_incompatible &&
      actual.old_schema_type_ids_changed ==
          expected.old_schema_type_ids_changed &&
      actual.schema_types_deleted_by_name ==
          expected.schema_types_deleted_by_name &&
      actual.schema_types_deleted_by_id ==
          expected.schema_types_deleted_by_id &&
      actual.schema_types_incompatible_by_name ==
          expected.schema_types_incompatible_by_name &&
      actual.schema_types_incompatible_by_id ==
          expected.schema_types_incompatible_by_id) {
    return true;
  }

  // Format schema_type_ids_changed
  std::string actual_old_schema_type_ids_changed = absl_ports::StrCat(
      "[",
      absl_ports::StrJoin(actual.old_schema_type_ids_changed, ",",
                          absl_ports::NumberFormatter()),
      "]");

  std::string expected_old_schema_type_ids_changed = absl_ports::StrCat(
      "[",
      absl_ports::StrJoin(expected.old_schema_type_ids_changed, ",",
                          absl_ports::NumberFormatter()),
      "]");

  // Format schema_types_deleted_by_name
  std::string actual_schema_types_deleted_by_name = absl_ports::StrCat(
      "[", absl_ports::StrJoin(actual.schema_types_deleted_by_name, ","), "]");

  std::string expected_schema_types_deleted_by_name = absl_ports::StrCat(
      "[", absl_ports::StrJoin(expected.schema_types_deleted_by_name, ","),
      "]");

  // Format schema_types_deleted_by_id
  std::string actual_schema_types_deleted_by_id = absl_ports::StrCat(
      "[",
      absl_ports::StrJoin(actual.schema_types_deleted_by_id, ",",
                          absl_ports::NumberFormatter()),
      "]");

  std::string expected_schema_types_deleted_by_id = absl_ports::StrCat(
      "[",
      absl_ports::StrJoin(expected.schema_types_deleted_by_id, ",",
                          absl_ports::NumberFormatter()),
      "]");

  // Format schema_types_incompatible_by_name
  std::string actual_schema_types_incompatible_by_name = absl_ports::StrCat(
      "[", absl_ports::StrJoin(actual.schema_types_incompatible_by_name, ","),
      "]");

  std::string expected_schema_types_incompatible_by_name = absl_ports::StrCat(
      "[", absl_ports::StrJoin(expected.schema_types_incompatible_by_name, ","),
      "]");

  // Format schema_types_incompatible_by_id
  std::string actual_schema_types_incompatible_by_id = absl_ports::StrCat(
      "[",
      absl_ports::StrJoin(actual.schema_types_incompatible_by_id, ",",
                          absl_ports::NumberFormatter()),
      "]");

  std::string expected_schema_types_incompatible_by_id = absl_ports::StrCat(
      "[",
      absl_ports::StrJoin(expected.schema_types_incompatible_by_id, ",",
                          absl_ports::NumberFormatter()),
      "]");

  *result_listener << IcingStringUtil::StringPrintf(
      "\nExpected {\n"
      "\tsuccess=%d,\n"
      "\tindex_incompatible=%d,\n"
      "\told_schema_type_ids_changed=%s,\n"
      "\tschema_types_deleted_by_name=%s,\n"
      "\tschema_types_deleted_by_id=%s,\n"
      "\tschema_types_incompatible_by_name=%s,\n"
      "\tschema_types_incompatible_by_id=%s\n"
      "}\n"
      "Actual {\n"
      "\tsuccess=%d,\n"
      "\tindex_incompatible=%d,\n"
      "\told_schema_type_ids_changed=%s,\n"
      "\tschema_types_deleted_by_name=%s,\n"
      "\tschema_types_deleted_by_id=%s,\n"
      "\tschema_types_incompatible_by_name=%s,\n"
      "\tschema_types_incompatible_by_id=%s\n"
      "}\n",
      expected.success, expected.index_incompatible,
      expected_old_schema_type_ids_changed.c_str(),
      expected_schema_types_deleted_by_name.c_str(),
      expected_schema_types_deleted_by_id.c_str(),
      expected_schema_types_incompatible_by_name.c_str(),
      expected_schema_types_incompatible_by_id.c_str(), actual.success,
      actual.index_incompatible, actual_old_schema_type_ids_changed.c_str(),
      actual_schema_types_deleted_by_name.c_str(),
      actual_schema_types_deleted_by_id.c_str(),
      actual_schema_types_incompatible_by_name.c_str(),
      actual_schema_types_incompatible_by_id.c_str());

  return false;
}

std::string StatusCodeToString(libtextclassifier3::StatusCode code) {
  switch (code) {
    case libtextclassifier3::StatusCode::OK:
      return "OK";
    case libtextclassifier3::StatusCode::CANCELLED:
      return "CANCELLED";
    case libtextclassifier3::StatusCode::UNKNOWN:
      return "UNKNOWN";
    case libtextclassifier3::StatusCode::INVALID_ARGUMENT:
      return "INVALID_ARGUMENT";
    case libtextclassifier3::StatusCode::DEADLINE_EXCEEDED:
      return "DEADLINE_EXCEEDED";
    case libtextclassifier3::StatusCode::NOT_FOUND:
      return "NOT_FOUND";
    case libtextclassifier3::StatusCode::ALREADY_EXISTS:
      return "ALREADY_EXISTS";
    case libtextclassifier3::StatusCode::PERMISSION_DENIED:
      return "PERMISSION_DENIED";
    case libtextclassifier3::StatusCode::RESOURCE_EXHAUSTED:
      return "RESOURCE_EXHAUSTED";
    case libtextclassifier3::StatusCode::FAILED_PRECONDITION:
      return "FAILED_PRECONDITION";
    case libtextclassifier3::StatusCode::ABORTED:
      return "ABORTED";
    case libtextclassifier3::StatusCode::OUT_OF_RANGE:
      return "OUT_OF_RANGE";
    case libtextclassifier3::StatusCode::UNIMPLEMENTED:
      return "UNIMPLEMENTED";
    case libtextclassifier3::StatusCode::INTERNAL:
      return "INTERNAL";
    case libtextclassifier3::StatusCode::UNAVAILABLE:
      return "UNAVAILABLE";
    case libtextclassifier3::StatusCode::DATA_LOSS:
      return "DATA_LOSS";
    case libtextclassifier3::StatusCode::UNAUTHENTICATED:
      return "UNAUTHENTICATED";
    default:
      return "";
  }
}

std::string ProtoStatusCodeToString(StatusProto::Code code) {
  switch (code) {
    case StatusProto::OK:
      return "OK";
    case StatusProto::UNKNOWN:
      return "UNKNOWN";
    case StatusProto::INVALID_ARGUMENT:
      return "INVALID_ARGUMENT";
    case StatusProto::NOT_FOUND:
      return "NOT_FOUND";
    case StatusProto::ALREADY_EXISTS:
      return "ALREADY_EXISTS";
    case StatusProto::OUT_OF_SPACE:
      return "OUT_OF_SPACE";
    case StatusProto::FAILED_PRECONDITION:
      return "FAILED_PRECONDITION";
    case StatusProto::ABORTED:
      return "ABORTED";
    case StatusProto::INTERNAL:
      return "INTERNAL";
    case StatusProto::WARNING_DATA_LOSS:
      return "WARNING_DATA_LOSS";
    default:
      return "";
  }
}

MATCHER(IsOk, "") {
  libtextclassifier3::StatusAdapter adapter(arg);
  if (adapter.status().ok()) {
    return true;
  }
  *result_listener << IcingStringUtil::StringPrintf(
      "Expected OK, actual was (%s:%s)",
      StatusCodeToString(adapter.status().CanonicalCode()).c_str(),
      adapter.status().error_message().c_str());
  return false;
}

MATCHER_P(IsOkAndHolds, matcher, "") {
  if (!arg.ok()) {
    *result_listener << IcingStringUtil::StringPrintf(
        "Expected OK, actual was (%s:%s)",
        StatusCodeToString(arg.status().CanonicalCode()).c_str(),
        arg.status().error_message().c_str());
    return false;
  }
  return ExplainMatchResult(matcher, arg.ValueOrDie(), result_listener);
}

MATCHER_P(StatusIs, status_code, "") {
  libtextclassifier3::StatusAdapter adapter(arg);
  if (adapter.status().CanonicalCode() == status_code) {
    return true;
  }
  *result_listener << IcingStringUtil::StringPrintf(
      "Expected (%s:), actual was (%s:%s)",
      StatusCodeToString(status_code).c_str(),
      StatusCodeToString(adapter.status().CanonicalCode()).c_str(),
      adapter.status().error_message().c_str());
  return false;
}

MATCHER_P2(StatusIs, status_code, error_matcher, "") {
  libtextclassifier3::StatusAdapter adapter(arg);
  if (adapter.status().CanonicalCode() != status_code) {
    *result_listener << IcingStringUtil::StringPrintf(
        "Expected (%s:), actual was (%s:%s)",
        StatusCodeToString(status_code).c_str(),
        StatusCodeToString(adapter.status().CanonicalCode()).c_str(),
        adapter.status().error_message().c_str());
    return false;
  }
  return ExplainMatchResult(error_matcher, adapter.status().error_message(),
                            result_listener);
}

MATCHER(ProtoIsOk, "") {
  if (arg.code() == StatusProto::OK) {
    return true;
  }
  *result_listener << IcingStringUtil::StringPrintf(
      "Expected OK, actual was (%s:%s)",
      ProtoStatusCodeToString(arg.code()).c_str(), arg.message().c_str());
  return false;
}

MATCHER_P(ProtoStatusIs, status_code, "") {
  if (arg.code() == status_code) {
    return true;
  }
  *result_listener << IcingStringUtil::StringPrintf(
      "Expected (%s:), actual was (%s:%s)",
      ProtoStatusCodeToString(status_code).c_str(),
      ProtoStatusCodeToString(arg.code()).c_str(), arg.message().c_str());
  return false;
}

MATCHER_P2(ProtoStatusIs, status_code, error_matcher, "") {
  if (arg.code() != status_code) {
    *result_listener << IcingStringUtil::StringPrintf(
        "Expected (%s:), actual was (%s:%s)",
        ProtoStatusCodeToString(status_code).c_str(),
        ProtoStatusCodeToString(arg.code()).c_str(), arg.message().c_str());
    return false;
  }
  return ExplainMatchResult(error_matcher, arg.message(), result_listener);
}

MATCHER_P(EqualsSearchResultIgnoreStatsAndScores, expected, "") {
  SearchResultProto actual_copy = arg;
  actual_copy.clear_query_stats();
  actual_copy.clear_debug_info();
  for (SearchResultProto::ResultProto& result :
       *actual_copy.mutable_results()) {
    result.clear_score();
  }

  SearchResultProto expected_copy = expected;
  expected_copy.clear_query_stats();
  expected_copy.clear_debug_info();
  for (SearchResultProto::ResultProto& result :
       *expected_copy.mutable_results()) {
    result.clear_score();
  }
  return ExplainMatchResult(testing::EqualsProto(expected_copy), actual_copy,
                            result_listener);
}

// TODO(tjbarron) Remove this once icing has switched to depend on TC3 Status
#define ICING_STATUS_MACROS_CONCAT_NAME(x, y) \
  ICING_STATUS_MACROS_CONCAT_IMPL(x, y)
#define ICING_STATUS_MACROS_CONCAT_IMPL(x, y) x##y

#define ICING_EXPECT_OK(func) EXPECT_THAT(func, IsOk())
#define ICING_ASSERT_OK(func) ASSERT_THAT(func, IsOk())
#define ICING_ASSERT_OK_AND_ASSIGN(lhs, rexpr)                             \
  ICING_ASSERT_OK_AND_ASSIGN_IMPL(                                         \
      ICING_STATUS_MACROS_CONCAT_NAME(_status_or_value, __COUNTER__), lhs, \
      rexpr)
#define ICING_ASSERT_OK_AND_ASSIGN_IMPL(statusor, lhs, rexpr) \
  auto statusor = (rexpr);                                    \
  ICING_ASSERT_OK(statusor.status());                         \
  lhs = std::move(statusor).ValueOrDie()

}  // namespace lib
}  // namespace icing

#endif  // ICING_TESTING_COMMON_MATCHERS_H_
