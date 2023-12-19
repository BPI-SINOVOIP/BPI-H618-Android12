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

#ifndef ICING_SCHEMA_SCHEMA_STORE_H_
#define ICING_SCHEMA_SCHEMA_STORE_H_

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

#include "icing/text_classifier/lib3/utils/base/status.h"
#include "icing/text_classifier/lib3/utils/base/statusor.h"
#include "icing/file/file-backed-proto.h"
#include "icing/file/filesystem.h"
#include "icing/proto/document.pb.h"
#include "icing/proto/logging.pb.h"
#include "icing/proto/schema.pb.h"
#include "icing/proto/storage.pb.h"
#include "icing/schema/schema-util.h"
#include "icing/schema/section-manager.h"
#include "icing/schema/section.h"
#include "icing/store/document-filter-data.h"
#include "icing/store/key-mapper.h"
#include "icing/util/clock.h"
#include "icing/util/crc32.h"

namespace icing {
namespace lib {

// Holds the ground truth schema proto. Tracks compatible changes to the schema
// and will update any derived data based on the schema proto, such as Sections,
// SchemaTypeConfigs, PropertyConfigs, and SchemaTypeIds. To ensure they have
// the most up-to-date data, callers should not save instances themselves and
// should always call Get* from the SchemaStore.
class SchemaStore {
 public:
  struct Header {
    static constexpr int32_t kMagic = 0x72650d0a;

    // Holds the magic as a quick sanity check against file corruption.
    int32_t magic;

    // Checksum of the SchemaStore's sub-component's checksums.
    uint32_t checksum;
  };

  // Holds information on what may have been affected by the new schema. This is
  // generally data that other classes may depend on from the SchemaStore,
  // so that we can know if we should go update those classes as well.
  struct SetSchemaResult {
    // Whether we are able to write the schema as determined by SetSchema's
    // arguments. This boolean reflects SetSchema's logic, and does not reflect
    // any system level IO errors that may prevent the schema from being written
    // to file.
    bool success = false;

    // Whether the new schema changes invalidate the index.
    bool index_incompatible = false;

    // SchemaTypeIds of schema types can be reassigned new SchemaTypeIds if:
    //   1. Schema types are added in the middle of the SchemaProto
    //   2. Schema types are removed from the middle of the SchemaProto
    //   3. Schema types are reordered in the SchemaProto
    //
    // SchemaTypeIds are not changed if schema types are added/removed to the
    // end of the SchemaProto.
    std::unordered_set<SchemaTypeId> old_schema_type_ids_changed;

    // Schema types that have been removed from the new schema. Represented by
    // the `schema_type` field in the SchemaTypeConfigProto.
    std::unordered_set<std::string> schema_types_deleted_by_name;

    // Schema types that have been removed from the new schema. Represented by
    // the SchemaTypeId assigned to this SchemaTypeConfigProto in the *old*
    // schema.
    std::unordered_set<SchemaTypeId> schema_types_deleted_by_id;

    // Schema types whose SchemaTypeConfigProto has changed in an incompatible
    // manner in the new schema. Compatibility determined in
    // SchemaUtil::ComputeCompatibilityDelta. Represented by the `schema_type`
    // field in the SchemaTypeConfigProto.
    std::unordered_set<std::string> schema_types_incompatible_by_name;

    // Schema types whose SchemaTypeConfigProto has changed in an incompatible
    // manner in the new schema. Compatibility determined in
    // SchemaUtil::ComputeCompatibilityDelta. Represented by the SchemaTypeId
    // assigned to this SchemaTypeConfigProto in the *old* schema.
    std::unordered_set<SchemaTypeId> schema_types_incompatible_by_id;
  };

  // Factory function to create a SchemaStore which does not take ownership
  // of any input components, and all pointers must refer to valid objects that
  // outlive the created SchemaStore instance. The base_dir must already exist.
  // There does not need to be an existing schema already.
  //
  // If initialize_stats is present, the fields related to SchemaStore will be
  // populated.
  //
  // Returns:
  //   A SchemaStore on success
  //   FAILED_PRECONDITION on any null pointer input
  //   INTERNAL_ERROR on any IO errors
  static libtextclassifier3::StatusOr<std::unique_ptr<SchemaStore>> Create(
      const Filesystem* filesystem, const std::string& base_dir,
      const Clock* clock, InitializeStatsProto* initialize_stats = nullptr);

  // Not copyable
  SchemaStore(const SchemaStore&) = delete;
  SchemaStore& operator=(const SchemaStore&) = delete;

  // Persists and updates checksum of subcomponents.
  ~SchemaStore();

  // Retrieve the current schema if it exists. Caller does not get ownership of
  // the schema proto and modifying the returned pointer does not affect the
  // underlying schema proto.
  //
  // Returns:
  //   SchemaProto* if exists
  //   INTERNAL_ERROR on any IO errors
  //   NOT_FOUND_ERROR if a schema hasn't been set before
  libtextclassifier3::StatusOr<const SchemaProto*> GetSchema() const;

  // Update our current schema if it's compatible. Does not accept incompatible
  // schema. Compatibility rules defined by
  // SchemaUtil::ComputeCompatibilityDelta.
  //
  // If ignore_errors_and_delete_documents is set to true, then incompatible
  // schema are allowed and we'll force set the schema, meaning
  // SetSchemaResult.success will always be true.
  //
  // Returns:
  //   SetSchemaResult that encapsulates the differences between the old and new
  //   schema, as well as if the new schema can be set.
  //   INTERNAL_ERROR on any IO errors
  libtextclassifier3::StatusOr<const SetSchemaResult> SetSchema(
      const SchemaProto& new_schema,
      bool ignore_errors_and_delete_documents = false);
  libtextclassifier3::StatusOr<const SetSchemaResult> SetSchema(
      SchemaProto&& new_schema,
      bool ignore_errors_and_delete_documents = false);

  // Get the SchemaTypeConfigProto of schema_type name.
  //
  // Returns:
  //   SchemaTypeConfigProto on success
  //   FAILED_PRECONDITION if schema hasn't been set yet
  //   NOT_FOUND if schema type name doesn't exist
  //   INTERNAL on any I/O errors
  libtextclassifier3::StatusOr<const SchemaTypeConfigProto*>
  GetSchemaTypeConfig(std::string_view schema_type) const;

  // Returns the SchemaTypeId of the passed in schema type
  //
  // Returns:
  //   SchemaTypeId on success
  //   FAILED_PRECONDITION if schema hasn't been set yet
  //   NOT_FOUND_ERROR if we don't know about the schema type
  //   INTERNAL_ERROR on IO error
  libtextclassifier3::StatusOr<SchemaTypeId> GetSchemaTypeId(
      std::string_view schema_type) const;

  // Finds content of a section by section path (e.g. property1.property2)
  //
  // Returns:
  //   A string of content on success
  //   FAILED_PRECONDITION if schema hasn't been set yet
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
  //   FAILED_PRECONDITION if schema hasn't been set yet
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
  //   FAILED_PRECONDITION if schema hasn't been set yet
  //   INVALID_ARGUMENT if schema type id or section is invalid
  libtextclassifier3::StatusOr<const SectionMetadata*> GetSectionMetadata(
      SchemaTypeId schema_type_id, SectionId section_id) const;

  // Extracts all sections from the given document, sections are sorted by
  // section id in increasing order. Section ids start from 0. Sections with
  // empty content won't be returned.
  //
  // Returns:
  //   A list of sections on success
  //   FAILED_PRECONDITION if schema hasn't been set yet
  //   NOT_FOUND if type config name of document not found
  libtextclassifier3::StatusOr<std::vector<Section>> ExtractSections(
      const DocumentProto& document) const;

  // Syncs all the data changes to disk.
  //
  // Returns:
  //   OK on success
  //   INTERNAL on I/O errors.
  libtextclassifier3::Status PersistToDisk();

  // Computes the combined checksum of the schema store - includes the ground
  // truth and all derived files.
  //
  // Returns:
  //   Combined checksum on success
  //   INTERNAL_ERROR on compute error
  libtextclassifier3::StatusOr<Crc32> ComputeChecksum() const;

  // Calculates the StorageInfo for the Schema Store.
  //
  // If an IO error occurs while trying to calculate the value for a field, then
  // that field will be set to -1.
  SchemaStoreStorageInfoProto GetStorageInfo() const;

 private:
  // Use SchemaStore::Create instead.
  explicit SchemaStore(const Filesystem* filesystem, std::string base_dir,
                       const Clock* clock);

  // Handles initializing the SchemaStore and regenerating any data if needed.
  //
  // Returns:
  //   OK on success
  //   INTERNAL_ERROR on IO error
  libtextclassifier3::Status Initialize(InitializeStatsProto* initialize_stats);

  // Creates sub-components and verifies the integrity of each sub-component.
  //
  // Returns:
  //   OK on success
  //   INTERNAL_ERROR on IO error
  libtextclassifier3::Status InitializeDerivedFiles();

  // Populates any derived data structures off of the schema.
  //
  // Returns:
  //   OK on success
  //   NOT_FOUND_ERROR if a schema proto has not been set
  //   INTERNAL_ERROR on any IO errors
  libtextclassifier3::Status RegenerateDerivedFiles();

  // Checks if the header exists already. This does not create the header file
  // if it doesn't exist.
  bool HeaderExists();

  // Update and replace the header file. Creates the header file if it doesn't
  // exist.
  //
  // Returns:
  //   OK on success
  //   INTERNAL on I/O error
  libtextclassifier3::Status UpdateHeader(const Crc32& checksum);

  // Resets the unique_ptr to the schema_type_mapper_, deletes the underlying
  // file, and re-creates a new instance of the schema_type_mapper_. Does not
  // populate the schema_type_mapper_.
  //
  // Returns any IO errors.
  libtextclassifier3::Status ResetSchemaTypeMapper();

  libtextclassifier3::Status CheckSchemaSet() const {
    return has_schema_successfully_set_
               ? libtextclassifier3::Status::OK
               : absl_ports::FailedPreconditionError("Schema not set yet.");
  }

  const Filesystem& filesystem_;
  const std::string base_dir_;
  const Clock& clock_;

  // Used internally to indicate whether the class has been successfully
  // initialized with a valid schema. Will be false if Initialize failed or no
  // schema has ever been set.
  bool has_schema_successfully_set_ = false;

  // Cached schema
  FileBackedProto<SchemaProto> schema_file_;

  // A hash map of (type config name -> type config), allows faster lookup of
  // type config in schema. The O(1) type config access makes schema-related and
  // section-related operations faster.
  SchemaUtil::TypeConfigMap type_config_map_;

  // Maps schema types to a densely-assigned unique id.
  std::unique_ptr<KeyMapper<SchemaTypeId>> schema_type_mapper_;

  // Manager of indexed section related metadata.
  std::unique_ptr<const SectionManager> section_manager_;
};

}  // namespace lib
}  // namespace icing

#endif  // ICING_SCHEMA_SCHEMA_STORE_H_
