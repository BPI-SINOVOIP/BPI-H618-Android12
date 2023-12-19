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

#ifndef ICING_SCHEMA_SECTION_MANAGER_H_
#define ICING_SCHEMA_SECTION_MANAGER_H_

#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "icing/text_classifier/lib3/utils/base/statusor.h"
#include "icing/proto/document.pb.h"
#include "icing/schema/schema-util.h"
#include "icing/schema/section.h"
#include "icing/store/document-filter-data.h"
#include "icing/store/key-mapper.h"

namespace icing {
namespace lib {

inline constexpr std::string_view kPropertySeparator = ".";
inline constexpr std::string_view kLBracket = "[";
inline constexpr std::string_view kRBracket = "]";

// This class provides section-related operations. It assigns sections according
// to type configs and extracts section / sections from documents.
class SectionManager {
 public:
  SectionManager(const SectionManager&) = delete;
  SectionManager& operator=(const SectionManager&) = delete;

  // Factory function to create a SectionManager which does not take ownership
  // of any input components, and all pointers must refer to valid objects that
  // outlive the created SectionManager instance.
  //
  // Returns:
  //   A SectionManager on success
  //   FAILED_PRECONDITION on any null pointer input
  //   INVALID_ARGUMENT if infinite loop detected in the type configs
  //   OUT_OF_RANGE if number of properties need indexing exceeds the max number
  //   NOT_FOUND if any type config name not found in the map
  static libtextclassifier3::StatusOr<std::unique_ptr<SectionManager>> Create(
      const SchemaUtil::TypeConfigMap& type_config_map,
      const KeyMapper<SchemaTypeId>* schema_type_mapper);

  // Finds content of a section by section path (e.g. property1.property2)
  //
  // Returns:
  //   A string of content on success
  //   NOT_FOUND if:
  //     1. Property is optional and not found in the document
  //     2. section_path is invalid
  //     3. Content is empty
  libtextclassifier3::StatusOr<std::vector<std::string_view>>
  GetStringSectionContent(const DocumentProto& document,
                          std::string_view section_path) const;

  // Finds content of a section by id
  //
  // Returns:
  //   A string of content on success
  //   INVALID_ARGUMENT if section id is invalid
  //   NOT_FOUND if type config name of document not found
  libtextclassifier3::StatusOr<std::vector<std::string_view>>
  GetStringSectionContent(const DocumentProto& document,
                          SectionId section_id) const;

  // Returns the SectionMetadata associated with the SectionId that's in the
  // SchemaTypeId.
  //
  // Returns:
  //   pointer to SectionMetadata on success
  //   INVALID_ARGUMENT if schema type id or section is invalid
  libtextclassifier3::StatusOr<const SectionMetadata*> GetSectionMetadata(
      SchemaTypeId schema_type_id, SectionId section_id) const;

  // Extracts all sections from the given document, sections are sorted by
  // section id in increasing order. Section ids start from 0. Sections with
  // empty content won't be returned.
  //
  // Returns:
  //   A list of sections on success
  //   NOT_FOUND if type config name of document not found
  libtextclassifier3::StatusOr<std::vector<Section>> ExtractSections(
      const DocumentProto& document) const;

  // Returns:
  //   - On success, the section metadatas for the specified type
  //   - NOT_FOUND if the type config name is not present in the schema
  libtextclassifier3::StatusOr<const std::vector<SectionMetadata>*>
  GetMetadataList(const std::string& type_config_name) const;

 private:
  // Use SectionManager::Create() to instantiate
  explicit SectionManager(
      const KeyMapper<SchemaTypeId>* schema_type_mapper,
      std::vector<std::vector<SectionMetadata>>&& section_metadata_cache);

  // Maps schema types to a densely-assigned unique id.
  const KeyMapper<SchemaTypeId>& schema_type_mapper_;

  // The index of section_metadata_cache_ corresponds to a schema type's
  // SchemaTypeId. At that SchemaTypeId index, we store an inner vector. The
  // inner vector's index corresponds to a section's SectionId. At the SectionId
  // index, we store the SectionMetadata of that section.
  //
  // For example, pretend "email" had a SchemaTypeId of 0 and it had a section
  // called "subject" with a SectionId of 1. Then there would exist a vector
  // that holds the "subject" property's SectionMetadata at index 1. This vector
  // would be stored at index 0 of the section_metadata_cache_ vector.
  const std::vector<std::vector<SectionMetadata>> section_metadata_cache_;
};

}  // namespace lib
}  // namespace icing

#endif  // ICING_SCHEMA_SECTION_MANAGER_H_
