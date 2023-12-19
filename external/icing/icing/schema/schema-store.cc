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

#include "icing/schema/schema-store.h"

#include <algorithm>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include "icing/text_classifier/lib3/utils/base/status.h"
#include "icing/text_classifier/lib3/utils/base/statusor.h"
#include "icing/absl_ports/canonical_errors.h"
#include "icing/absl_ports/str_cat.h"
#include "icing/file/file-backed-proto.h"
#include "icing/file/filesystem.h"
#include "icing/proto/document.pb.h"
#include "icing/proto/schema.pb.h"
#include "icing/schema/schema-util.h"
#include "icing/schema/section-manager.h"
#include "icing/schema/section.h"
#include "icing/store/document-filter-data.h"
#include "icing/store/key-mapper.h"
#include "icing/util/crc32.h"
#include "icing/util/logging.h"
#include "icing/util/status-macros.h"

namespace icing {
namespace lib {

namespace {

constexpr char kSchemaStoreHeaderFilename[] = "schema_store_header";
constexpr char kSchemaFilename[] = "schema.pb";
constexpr char kSchemaTypeMapperFilename[] = "schema_type_mapper";

// A KeyMapper stores its data across 3 arrays internally. Giving each array
// 128KiB for storage means the entire KeyMapper requires 384KiB.
constexpr int32_t kSchemaTypeMapperMaxSize = 3 * 128 * 1024;  // 384 KiB

const std::string MakeHeaderFilename(const std::string& base_dir) {
  return absl_ports::StrCat(base_dir, "/", kSchemaStoreHeaderFilename);
}

const std::string MakeSchemaFilename(const std::string& base_dir) {
  return absl_ports::StrCat(base_dir, "/", kSchemaFilename);
}

const std::string MakeSchemaTypeMapperFilename(const std::string& base_dir) {
  return absl_ports::StrCat(base_dir, "/", kSchemaTypeMapperFilename);
}

// Assuming that SchemaTypeIds are assigned to schema types based on their order
// in the SchemaProto. Check if the schema type->SchemaTypeId mapping would
// change with the new schema.
std::unordered_set<SchemaTypeId> SchemaTypeIdsChanged(
    const SchemaProto& old_schema, const SchemaProto& new_schema) {
  std::unordered_set<SchemaTypeId> old_schema_type_ids_changed;

  std::unordered_map<std::string, int> old_types_and_index;
  for (int i = 0; i < old_schema.types_size(); ++i) {
    old_types_and_index.emplace(old_schema.types(i).schema_type(), i);
  }

  std::unordered_map<std::string, int> new_types_and_index;
  for (int i = 0; i < new_schema.types_size(); ++i) {
    new_types_and_index.emplace(new_schema.types(i).schema_type(), i);
  }

  for (const auto& old_type_index : old_types_and_index) {
    const auto& iter = new_types_and_index.find(old_type_index.first);
    // We only care if the type exists in both the old and new schema. If the
    // type has been deleted, then it'll be captured in
    // SetSchemaResult.schema_types_deleted*. If the type has been added in the
    // new schema then we also don't care because nothing needs to be updated.
    if (iter != new_types_and_index.end()) {
      // Since the SchemaTypeId of the schema type is just the index of it in
      // the SchemaProto, compare the index and save it if it's not the same
      if (old_type_index.second != iter->second) {
        old_schema_type_ids_changed.emplace(old_type_index.second);
      }
    }
  }

  return old_schema_type_ids_changed;
}

}  // namespace

libtextclassifier3::StatusOr<std::unique_ptr<SchemaStore>> SchemaStore::Create(
    const Filesystem* filesystem, const std::string& base_dir,
    const Clock* clock, InitializeStatsProto* initialize_stats) {
  ICING_RETURN_ERROR_IF_NULL(filesystem);
  ICING_RETURN_ERROR_IF_NULL(clock);

  std::unique_ptr<SchemaStore> schema_store = std::unique_ptr<SchemaStore>(
      new SchemaStore(filesystem, base_dir, clock));
  ICING_RETURN_IF_ERROR(schema_store->Initialize(initialize_stats));
  return schema_store;
}

SchemaStore::SchemaStore(const Filesystem* filesystem, std::string base_dir,
                         const Clock* clock)
    : filesystem_(*filesystem),
      base_dir_(std::move(base_dir)),
      clock_(*clock),
      schema_file_(*filesystem, MakeSchemaFilename(base_dir_)) {}

SchemaStore::~SchemaStore() {
  if (has_schema_successfully_set_) {
    if (!PersistToDisk().ok()) {
      ICING_LOG(ERROR) << "Error persisting to disk in SchemaStore destructor";
    }
  }
}

libtextclassifier3::Status SchemaStore::Initialize(
    InitializeStatsProto* initialize_stats) {
  auto schema_proto_or = GetSchema();
  if (absl_ports::IsNotFound(schema_proto_or.status())) {
    // Don't have an existing schema proto, that's fine
    return libtextclassifier3::Status::OK;
  } else if (!schema_proto_or.ok()) {
    // Real error when trying to read the existing schema
    return schema_proto_or.status();
  }
  has_schema_successfully_set_ = true;

  if (!InitializeDerivedFiles().ok()) {
    ICING_VLOG(3)
        << "Couldn't find derived files or failed to initialize them, "
           "regenerating derived files for SchemaStore.";
    std::unique_ptr<Timer> regenerate_timer = clock_.GetNewTimer();
    if (initialize_stats != nullptr) {
      initialize_stats->set_schema_store_recovery_cause(
          InitializeStatsProto::IO_ERROR);
    }
    ICING_RETURN_IF_ERROR(RegenerateDerivedFiles());
    if (initialize_stats != nullptr) {
      initialize_stats->set_schema_store_recovery_latency_ms(
          regenerate_timer->GetElapsedMilliseconds());
    }
  }

  if (initialize_stats != nullptr) {
    initialize_stats->set_num_schema_types(type_config_map_.size());
  }

  return libtextclassifier3::Status::OK;
}

libtextclassifier3::Status SchemaStore::InitializeDerivedFiles() {
  if (!HeaderExists()) {
    // Without a header, we don't know if things are consistent between each
    // other so the caller should just regenerate everything from ground truth.
    return absl_ports::InternalError("SchemaStore header doesn't exist");
  }

  SchemaStore::Header header;
  if (!filesystem_.Read(MakeHeaderFilename(base_dir_).c_str(), &header,
                        sizeof(header))) {
    return absl_ports::InternalError(
        absl_ports::StrCat("Couldn't read: ", MakeHeaderFilename(base_dir_)));
  }

  if (header.magic != SchemaStore::Header::kMagic) {
    return absl_ports::InternalError(absl_ports::StrCat(
        "Invalid header kMagic for file: ", MakeHeaderFilename(base_dir_)));
  }

  ICING_ASSIGN_OR_RETURN(
      schema_type_mapper_,
      KeyMapper<SchemaTypeId>::Create(filesystem_,
                                      MakeSchemaTypeMapperFilename(base_dir_),
                                      kSchemaTypeMapperMaxSize));

  ICING_ASSIGN_OR_RETURN(Crc32 checksum, ComputeChecksum());
  if (checksum.Get() != header.checksum) {
    return absl_ports::InternalError(
        "Combined checksum of SchemaStore was inconsistent");
  }

  // Update our in-memory data structures
  type_config_map_.clear();
  ICING_ASSIGN_OR_RETURN(const SchemaProto* schema_proto, GetSchema());
  for (const SchemaTypeConfigProto& type_config : schema_proto->types()) {
    // Update our type_config_map_
    type_config_map_.emplace(type_config.schema_type(), type_config);
  }
  ICING_ASSIGN_OR_RETURN(
      section_manager_,
      SectionManager::Create(type_config_map_, schema_type_mapper_.get()));

  return libtextclassifier3::Status::OK;
}

libtextclassifier3::Status SchemaStore::RegenerateDerivedFiles() {
  ICING_ASSIGN_OR_RETURN(const SchemaProto* schema_proto, GetSchema());

  ICING_RETURN_IF_ERROR(ResetSchemaTypeMapper());
  type_config_map_.clear();

  for (const SchemaTypeConfigProto& type_config : schema_proto->types()) {
    // Update our type_config_map_
    type_config_map_.emplace(type_config.schema_type(), type_config);

    // Assign a SchemaTypeId to the type
    ICING_RETURN_IF_ERROR(schema_type_mapper_->Put(
        type_config.schema_type(), schema_type_mapper_->num_keys()));
  }

  ICING_ASSIGN_OR_RETURN(
      section_manager_,
      SectionManager::Create(type_config_map_, schema_type_mapper_.get()));

  // Write the header
  ICING_ASSIGN_OR_RETURN(Crc32 checksum, ComputeChecksum());
  ICING_RETURN_IF_ERROR(UpdateHeader(checksum));

  return libtextclassifier3::Status::OK;
}

bool SchemaStore::HeaderExists() {
  if (!filesystem_.FileExists(MakeHeaderFilename(base_dir_).c_str())) {
    return false;
  }

  int64_t file_size =
      filesystem_.GetFileSize(MakeHeaderFilename(base_dir_).c_str());

  // If it's been truncated to size 0 before, we consider it to be a new file
  return file_size != 0 && file_size != Filesystem::kBadFileSize;
}

libtextclassifier3::Status SchemaStore::UpdateHeader(const Crc32& checksum) {
  // Write the header
  SchemaStore::Header header;
  header.magic = SchemaStore::Header::kMagic;
  header.checksum = checksum.Get();

  ScopedFd scoped_fd(
      filesystem_.OpenForWrite(MakeHeaderFilename(base_dir_).c_str()));
  // This should overwrite the header.
  if (!scoped_fd.is_valid() ||
      !filesystem_.Write(scoped_fd.get(), &header, sizeof(header)) ||
      !filesystem_.DataSync(scoped_fd.get())) {
    return absl_ports::InternalError(absl_ports::StrCat(
        "Failed to write SchemaStore header: ", MakeHeaderFilename(base_dir_)));
  }
  return libtextclassifier3::Status::OK;
}

libtextclassifier3::Status SchemaStore::ResetSchemaTypeMapper() {
  // TODO(b/139734457): Replace ptr.reset()->Delete->Create flow with Reset().
  schema_type_mapper_.reset();
  // TODO(b/144458732): Implement a more robust version of TC_RETURN_IF_ERROR
  // that can support error logging.
  libtextclassifier3::Status status = KeyMapper<SchemaTypeId>::Delete(
      filesystem_, MakeSchemaTypeMapperFilename(base_dir_));
  if (!status.ok()) {
    ICING_LOG(ERROR) << status.error_message()
                     << "Failed to delete old schema_type mapper";
    return status;
  }
  ICING_ASSIGN_OR_RETURN(
      schema_type_mapper_,
      KeyMapper<SchemaTypeId>::Create(filesystem_,
                                      MakeSchemaTypeMapperFilename(base_dir_),
                                      kSchemaTypeMapperMaxSize));

  return libtextclassifier3::Status::OK;
}

libtextclassifier3::StatusOr<Crc32> SchemaStore::ComputeChecksum() const {
  Crc32 total_checksum;
  if (!has_schema_successfully_set_) {
    // Nothing to checksum
    return total_checksum;
  }
  ICING_ASSIGN_OR_RETURN(const SchemaProto* schema_proto, GetSchema());
  Crc32 schema_checksum;
  schema_checksum.Append(schema_proto->SerializeAsString());

  Crc32 schema_type_mapper_checksum = schema_type_mapper_->ComputeChecksum();

  total_checksum.Append(std::to_string(schema_checksum.Get()));
  total_checksum.Append(std::to_string(schema_type_mapper_checksum.Get()));

  return total_checksum;
}

libtextclassifier3::StatusOr<const SchemaProto*> SchemaStore::GetSchema()
    const {
  return schema_file_.Read();
}

// TODO(cassiewang): Consider removing this definition of SetSchema if it's not
// needed by production code. It's currently being used by our tests, but maybe
// it's trivial to change our test code to also use the
// SetSchema(SchemaProto&& new_schema)
libtextclassifier3::StatusOr<const SchemaStore::SetSchemaResult>
SchemaStore::SetSchema(const SchemaProto& new_schema,
                       bool ignore_errors_and_delete_documents) {
  return SetSchema(SchemaProto(new_schema), ignore_errors_and_delete_documents);
}

libtextclassifier3::StatusOr<const SchemaStore::SetSchemaResult>
SchemaStore::SetSchema(SchemaProto&& new_schema,
                       bool ignore_errors_and_delete_documents) {
  ICING_ASSIGN_OR_RETURN(SchemaUtil::DependencyMap new_dependency_map,
                         SchemaUtil::Validate(new_schema));

  SetSchemaResult result;

  auto schema_proto_or = GetSchema();
  if (absl_ports::IsNotFound(schema_proto_or.status())) {
    // We don't have a pre-existing schema, so anything is valid.
    result.success = true;
  } else if (!schema_proto_or.ok()) {
    // Real error
    return schema_proto_or.status();
  } else {
    // At this point, we're guaranteed that we have a schema.
    const SchemaProto old_schema = *schema_proto_or.ValueOrDie();

    // Assume we can set the schema unless proven otherwise.
    result.success = true;

    if (new_schema.SerializeAsString() == old_schema.SerializeAsString()) {
      // Same schema as before. No need to update anything
      return result;
    }

    // Different schema, track the differences and see if we can still write it
    SchemaUtil::SchemaDelta schema_delta =
        SchemaUtil::ComputeCompatibilityDelta(old_schema, new_schema,
                                              new_dependency_map);

    // An incompatible index is fine, we can just reindex
    result.index_incompatible = schema_delta.index_incompatible;

    for (const auto& schema_type : schema_delta.schema_types_deleted) {
      // We currently don't support deletions, so mark this as not possible.
      // This will change once we allow force-set schemas.
      result.success = false;

      result.schema_types_deleted_by_name.emplace(schema_type);

      ICING_ASSIGN_OR_RETURN(SchemaTypeId schema_type_id,
                             GetSchemaTypeId(schema_type));
      result.schema_types_deleted_by_id.emplace(schema_type_id);
    }

    for (const auto& schema_type : schema_delta.schema_types_incompatible) {
      // We currently don't support incompatible schemas, so mark this as
      // not possible. This will change once we allow force-set schemas.
      result.success = false;

      result.schema_types_incompatible_by_name.emplace(schema_type);

      ICING_ASSIGN_OR_RETURN(SchemaTypeId schema_type_id,
                             GetSchemaTypeId(schema_type));
      result.schema_types_incompatible_by_id.emplace(schema_type_id);
    }

    // SchemaTypeIds changing is fine, we can update the DocumentStore
    result.old_schema_type_ids_changed =
        SchemaTypeIdsChanged(old_schema, new_schema);
  }

  // We can force set the schema if the caller has told us to ignore any errors
  result.success = result.success || ignore_errors_and_delete_documents;

  if (result.success) {
    // Write the schema (and potentially overwrite a previous schema)
    ICING_RETURN_IF_ERROR(
        schema_file_.Write(std::make_unique<SchemaProto>(new_schema)));
    has_schema_successfully_set_ = true;

    ICING_RETURN_IF_ERROR(RegenerateDerivedFiles());
  }

  return result;
}

libtextclassifier3::StatusOr<const SchemaTypeConfigProto*>
SchemaStore::GetSchemaTypeConfig(std::string_view schema_type) const {
  ICING_RETURN_IF_ERROR(CheckSchemaSet());
  const auto& type_config_iter =
      type_config_map_.find(std::string(schema_type));
  if (type_config_iter == type_config_map_.end()) {
    return absl_ports::NotFoundError(
        absl_ports::StrCat("Schema type config '", schema_type, "' not found"));
  }
  return &type_config_iter->second;
}

libtextclassifier3::StatusOr<SchemaTypeId> SchemaStore::GetSchemaTypeId(
    std::string_view schema_type) const {
  ICING_RETURN_IF_ERROR(CheckSchemaSet());
  return schema_type_mapper_->Get(schema_type);
}

libtextclassifier3::StatusOr<std::vector<std::string_view>>
SchemaStore::GetStringSectionContent(const DocumentProto& document,
                                     std::string_view section_path) const {
  ICING_RETURN_IF_ERROR(CheckSchemaSet());
  return section_manager_->GetStringSectionContent(document, section_path);
}

libtextclassifier3::StatusOr<std::vector<std::string_view>>
SchemaStore::GetStringSectionContent(const DocumentProto& document,
                                     SectionId section_id) const {
  ICING_RETURN_IF_ERROR(CheckSchemaSet());
  return section_manager_->GetStringSectionContent(document, section_id);
}

libtextclassifier3::StatusOr<const SectionMetadata*>
SchemaStore::GetSectionMetadata(SchemaTypeId schema_type_id,
                                SectionId section_id) const {
  ICING_RETURN_IF_ERROR(CheckSchemaSet());
  return section_manager_->GetSectionMetadata(schema_type_id, section_id);
}

libtextclassifier3::StatusOr<std::vector<Section>> SchemaStore::ExtractSections(
    const DocumentProto& document) const {
  ICING_RETURN_IF_ERROR(CheckSchemaSet());
  return section_manager_->ExtractSections(document);
}

libtextclassifier3::Status SchemaStore::PersistToDisk() {
  if (!has_schema_successfully_set_) {
    return libtextclassifier3::Status::OK;
  }
  ICING_RETURN_IF_ERROR(schema_type_mapper_->PersistToDisk());
  // Write the header
  ICING_ASSIGN_OR_RETURN(Crc32 checksum, ComputeChecksum());
  ICING_RETURN_IF_ERROR(UpdateHeader(checksum));

  return libtextclassifier3::Status::OK;
}

SchemaStoreStorageInfoProto SchemaStore::GetStorageInfo() const {
  SchemaStoreStorageInfoProto storage_info;
  int64_t directory_size = filesystem_.GetDiskUsage(base_dir_.c_str());
  if (directory_size != Filesystem::kBadFileSize) {
    storage_info.set_schema_store_size(directory_size);
  } else {
    storage_info.set_schema_store_size(-1);
  }
  ICING_ASSIGN_OR_RETURN(const SchemaProto* schema, GetSchema(), storage_info);
  storage_info.set_num_schema_types(schema->types_size());
  int total_sections = 0;
  int num_types_sections_exhausted = 0;
  for (const SchemaTypeConfigProto& type : schema->types()) {
    auto sections_list_or =
        section_manager_->GetMetadataList(type.schema_type());
    if (!sections_list_or.ok()) {
      continue;
    }
    total_sections += sections_list_or.ValueOrDie()->size();
    if (sections_list_or.ValueOrDie()->size() == kMaxSectionId + 1) {
      ++num_types_sections_exhausted;
    }
  }

  storage_info.set_num_total_sections(total_sections);
  storage_info.set_num_schema_types_sections_exhausted(
      num_types_sections_exhausted);
  return storage_info;
}

}  // namespace lib
}  // namespace icing
