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

#include "icing/index/iterator/doc-hit-info-iterator-section-restrict.h"

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

#include "icing/text_classifier/lib3/utils/base/status.h"
#include "icing/text_classifier/lib3/utils/base/statusor.h"
#include "icing/absl_ports/canonical_errors.h"
#include "icing/absl_ports/str_cat.h"
#include "icing/index/hit/doc-hit-info.h"
#include "icing/index/iterator/doc-hit-info-iterator.h"
#include "icing/schema/schema-store.h"
#include "icing/schema/section.h"
#include "icing/store/document-filter-data.h"
#include "icing/store/document-id.h"
#include "icing/store/document-store.h"

namespace icing {
namespace lib {

DocHitInfoIteratorSectionRestrict::DocHitInfoIteratorSectionRestrict(
    std::unique_ptr<DocHitInfoIterator> delegate,
    const DocumentStore* document_store, const SchemaStore* schema_store,
    std::string_view target_section)
    : delegate_(std::move(delegate)),
      document_store_(*document_store),
      schema_store_(*schema_store),
      target_section_(target_section) {}

libtextclassifier3::Status DocHitInfoIteratorSectionRestrict::Advance() {
  while (delegate_->Advance().ok()) {
    DocumentId document_id = delegate_->doc_hit_info().document_id();

    SectionIdMask section_id_mask =
        delegate_->doc_hit_info().hit_section_ids_mask();

    auto data_or = document_store_.GetDocumentFilterData(document_id);
    if (!data_or.ok()) {
      // Ran into some error retrieving information on this hit, skip
      continue;
    }

    // Guaranteed that the DocumentFilterData exists at this point
    DocumentFilterData data = std::move(data_or).ValueOrDie();
    SchemaTypeId schema_type_id = data.schema_type_id();

    // A hit can be in multiple sections at once, need to check that at least
    // one of the confirmed section ids match the name of the target section
    while (section_id_mask != 0) {
      // There was a hit in this section id
      SectionId section_id = __builtin_ctz(section_id_mask);

      auto section_metadata_or =
          schema_store_.GetSectionMetadata(schema_type_id, section_id);

      if (section_metadata_or.ok()) {
        const SectionMetadata* section_metadata =
            section_metadata_or.ValueOrDie();

        if (section_metadata->path == target_section_) {
          // The hit was in the target section name, return OK/found
          doc_hit_info_ = delegate_->doc_hit_info();
          hit_intersect_section_ids_mask_ = 1u << section_id;
          return libtextclassifier3::Status::OK;
        }
      }

      // Mark this section as checked
      section_id_mask &= ~(1U << section_id);
    }

    // Didn't find a matching section name for this hit. Continue.
  }

  // Didn't find anything on the delegate iterator.
  doc_hit_info_ = DocHitInfo(kInvalidDocumentId);
  hit_intersect_section_ids_mask_ = kSectionIdMaskNone;
  return absl_ports::ResourceExhaustedError("No more DocHitInfos in iterator");
}

int32_t DocHitInfoIteratorSectionRestrict::GetNumBlocksInspected() const {
  return delegate_->GetNumBlocksInspected();
}

int32_t DocHitInfoIteratorSectionRestrict::GetNumLeafAdvanceCalls() const {
  return delegate_->GetNumLeafAdvanceCalls();
}

std::string DocHitInfoIteratorSectionRestrict::ToString() const {
  return absl_ports::StrCat(target_section_, ": ", delegate_->ToString());
}

}  // namespace lib
}  // namespace icing
