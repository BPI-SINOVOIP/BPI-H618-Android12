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

// Copyright 2014 Google Inc. All Rights Reserved.
// Author: sbanacho@google.com (Scott Banachowski)
// Author: csyoung@google.com (C. Sean Young)

#ifndef ICING_LEGACY_INDEX_ICING_COMMON_TYPES_H_
#define ICING_LEGACY_INDEX_ICING_COMMON_TYPES_H_

#include "icing/legacy/core/icing-core-types.h"

// Protocol buffers are shared across several components.
namespace com {
namespace google {
namespace android {
namespace gms {
namespace icing {
namespace lib {

class ClientFileGroup;
class Document;
class Document_Section;
class DocumentStoreStatusProto;
class IMEUpdate;
class IMEUpdateResponse;
class IndexCorpusScoringConfig;
class IndexCorpusScoringConfig_Section;
class IndexScoringConfig;
class InitStatus;
class InitStatus_CorpusInitInfo;
class PendingDeleteUsageReport;
class PhraseAffinityRequest;
class QueryResponse;
class QueryResponse_Corpus;
class QueryResponse_Corpus_Section;
class QueryResponse_Corpus_Tag;
class QueryRequestSpec;
class QueryRequestSpec_CorpusSpec;
class QueryRequestSpec_SectionSpec;
class ResponseDebugInfo;
class ResultDebugInfo;
class SectionConfig;
class SuggestionResponse;
class SuggestionResponse_Suggestion;
class UsageReportsResponse;
class UsageStats;
class UsageStats_Corpus;

}  // namespace lib
}  // namespace icing
}  // namespace gms
}  // namespace android
}  // namespace google
}  // namespace com

namespace icing {
namespace lib {

// Typedefs.
using IcingDocId = uint32_t;

using IcingSectionId = uint32_t;

using IcingCorpusId = uint16_t;
using IcingSectionIdMask = uint16_t;

using IcingTagsCount = uint16_t;

using IcingSequenceNumber = int64_t;

using IcingScore = uint64_t;

constexpr size_t kIcingMaxTokenLen = 30;  // default shared between query
                                          // processor and indexer
constexpr int kIcingQueryTermLimit = 50;  // Maximum number of terms in a query
constexpr int kIcingMaxVariantsPerToken = 10;  // Maximum number of variants

// LINT.IfChange
constexpr int kIcingDocIdBits = 20;  // 1M docs
constexpr IcingDocId kIcingInvalidDocId = (1u << kIcingDocIdBits) - 1;
constexpr IcingDocId kIcingMaxDocId = kIcingInvalidDocId - 1;
// LINT.ThenChange(//depot/google3/wireless/android/icing/plx/google_sql_common_macros.sql)

constexpr int kIcingDocScoreBits = 32;

constexpr int kIcingSectionIdBits = 4;  // 4 bits for 16 values
constexpr IcingSectionId kIcingMaxSectionId = (1u << kIcingSectionIdBits) - 1;
constexpr IcingSectionId kIcingInvalidSectionId = kIcingMaxSectionId + 1;
constexpr IcingSectionIdMask kIcingSectionIdMaskAll = ~IcingSectionIdMask{0};
constexpr IcingSectionIdMask kIcingSectionIdMaskNone = IcingSectionIdMask{0};

constexpr int kIcingCorpusIdBits = 15;  // 32K
constexpr IcingCorpusId kIcingInvalidCorpusId = (1u << kIcingCorpusIdBits) - 1;
constexpr IcingCorpusId kIcingMaxCorpusId = kIcingInvalidCorpusId - 1;

constexpr size_t kIcingMaxSearchableDocumentSize = (1u << 16) - 1;  // 64K
// Max num tokens per document. 64KB is our original maximum (searchable)
// document size. We clip if document exceeds this.
constexpr uint32_t kIcingMaxNumTokensPerDoc =
    kIcingMaxSearchableDocumentSize / 5;
constexpr uint32_t kIcingMaxNumHitsPerDocument =
    kIcingMaxNumTokensPerDoc * kIcingMaxVariantsPerToken;

constexpr IcingTagsCount kIcingInvalidTagCount = ~IcingTagsCount{0};
constexpr IcingTagsCount kIcingMaxTagCount = kIcingInvalidTagCount - 1;

// Location refers to document storage.
constexpr uint64_t kIcingInvalidLocation = ~uint64_t{0};
constexpr uint64_t kIcingMaxDocStoreWriteLocation = uint64_t{1}
                                                    << 32;  // 4bytes.

// Dump symbols in the proto namespace.
using namespace ::com::google::android::gms::icing;  // NOLINT(build/namespaces)
}  // namespace lib
}  // namespace icing

#endif  // ICING_LEGACY_INDEX_ICING_COMMON_TYPES_H_
