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

#include "icing/index/iterator/doc-hit-info-iterator-filter.h"

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>
#include <vector>

#include "icing/text_classifier/lib3/utils/base/status.h"
#include "icing/absl_ports/canonical_errors.h"
#include "icing/index/hit/doc-hit-info.h"
#include "icing/index/iterator/doc-hit-info-iterator.h"
#include "icing/schema/schema-store.h"
#include "icing/schema/section.h"
#include "icing/store/document-filter-data.h"
#include "icing/store/document-id.h"
#include "icing/store/document-store.h"

namespace icing {
namespace lib {

DocHitInfoIteratorFilter::DocHitInfoIteratorFilter(
    std::unique_ptr<DocHitInfoIterator> delegate,
    const DocumentStore* document_store, const SchemaStore* schema_store,
    const Options& options)
    : delegate_(std::move(delegate)),
      document_store_(*document_store),
      schema_store_(*schema_store),
      options_(options) {
  // Precompute all the NamespaceIds
  for (std::string_view name_space : options_.namespaces) {
    auto namespace_id_or = document_store_.GetNamespaceId(name_space);

    // If we can't find the NamespaceId, just throw it away
    if (namespace_id_or.ok()) {
      target_namespace_ids_.emplace(namespace_id_or.ValueOrDie());
    }
  }

  // Precompute all the SchemaTypeIds
  for (std::string_view schema_type : options_.schema_types) {
    auto schema_type_id_or = schema_store_.GetSchemaTypeId(schema_type);

    // If we can't find the SchemaTypeId, just throw it away
    if (schema_type_id_or.ok()) {
      target_schema_type_ids_.emplace(schema_type_id_or.ValueOrDie());
    }
  }
}

libtextclassifier3::Status DocHitInfoIteratorFilter::Advance() {
  while (delegate_->Advance().ok()) {
    if (!document_store_.DoesDocumentExist(
            delegate_->doc_hit_info().document_id())) {
      // Document doesn't exist, keep searching. This handles deletions and
      // expired documents.
      continue;
    }

    // Try to get the DocumentFilterData
    auto document_filter_data_or = document_store_.GetDocumentFilterData(
        delegate_->doc_hit_info().document_id());
    if (!document_filter_data_or.ok()) {
      // Didn't find the DocumentFilterData in the filter cache. This could be
      // because the DocumentId isn't valid or the filter cache is in some
      // invalid state. This is bad, but not the query's responsibility to fix,
      // so just skip this result for now.
      continue;
    }
    // We should be guaranteed that this exists now.
    DocumentFilterData data = std::move(document_filter_data_or).ValueOrDie();

    if (!options_.namespaces.empty() &&
        target_namespace_ids_.count(data.namespace_id()) == 0) {
      // Doesn't match one of the specified namespaces. Keep searching
      continue;
    }

    if (!options_.schema_types.empty() &&
        target_schema_type_ids_.count(data.schema_type_id()) == 0) {
      // Doesn't match one of the specified schema types. Keep searching
      continue;
    }

    // Satisfied all our specified filters
    doc_hit_info_ = delegate_->doc_hit_info();
    hit_intersect_section_ids_mask_ =
        delegate_->hit_intersect_section_ids_mask();
    return libtextclassifier3::Status::OK;
  }

  // Didn't find anything on the delegate iterator.
  doc_hit_info_ = DocHitInfo(kInvalidDocumentId);
  hit_intersect_section_ids_mask_ = kSectionIdMaskNone;
  return absl_ports::ResourceExhaustedError("No more DocHitInfos in iterator");
}

int32_t DocHitInfoIteratorFilter::GetNumBlocksInspected() const {
  return delegate_->GetNumBlocksInspected();
}

int32_t DocHitInfoIteratorFilter::GetNumLeafAdvanceCalls() const {
  return delegate_->GetNumLeafAdvanceCalls();
}

std::string DocHitInfoIteratorFilter::ToString() const {
  return delegate_->ToString();
}

}  // namespace lib
}  // namespace icing
