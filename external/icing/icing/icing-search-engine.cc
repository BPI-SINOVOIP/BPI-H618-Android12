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

#include "icing/icing-search-engine.h"

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "icing/text_classifier/lib3/utils/base/status.h"
#include "icing/text_classifier/lib3/utils/base/statusor.h"
#include "icing/absl_ports/annotate.h"
#include "icing/absl_ports/canonical_errors.h"
#include "icing/absl_ports/mutex.h"
#include "icing/absl_ports/str_cat.h"
#include "icing/file/destructible-file.h"
#include "icing/file/file-backed-proto.h"
#include "icing/file/filesystem.h"
#include "icing/index/hit/doc-hit-info.h"
#include "icing/index/index-processor.h"
#include "icing/index/index.h"
#include "icing/index/iterator/doc-hit-info-iterator.h"
#include "icing/legacy/index/icing-filesystem.h"
#include "icing/proto/document.pb.h"
#include "icing/proto/initialize.pb.h"
#include "icing/proto/internal/optimize.pb.h"
#include "icing/proto/logging.pb.h"
#include "icing/proto/optimize.pb.h"
#include "icing/proto/persist.pb.h"
#include "icing/proto/reset.pb.h"
#include "icing/proto/schema.pb.h"
#include "icing/proto/search.pb.h"
#include "icing/proto/status.pb.h"
#include "icing/query/query-processor.h"
#include "icing/result/projection-tree.h"
#include "icing/result/projector.h"
#include "icing/result/result-retriever.h"
#include "icing/schema/schema-store.h"
#include "icing/schema/schema-util.h"
#include "icing/schema/section.h"
#include "icing/scoring/ranker.h"
#include "icing/scoring/scored-document-hit.h"
#include "icing/scoring/scoring-processor.h"
#include "icing/store/document-id.h"
#include "icing/store/document-store.h"
#include "icing/tokenization/language-segmenter-factory.h"
#include "icing/tokenization/language-segmenter.h"
#include "icing/transform/normalizer-factory.h"
#include "icing/transform/normalizer.h"
#include "icing/util/clock.h"
#include "icing/util/crc32.h"
#include "icing/util/logging.h"
#include "icing/util/status-macros.h"
#include "icing/util/tokenized-document.h"
#include "unicode/uloc.h"

namespace icing {
namespace lib {

namespace {

constexpr std::string_view kDocumentSubfolderName = "document_dir";
constexpr std::string_view kIndexSubfolderName = "index_dir";
constexpr std::string_view kSchemaSubfolderName = "schema_dir";
constexpr std::string_view kSetSchemaMarkerFilename = "set_schema_marker";
constexpr std::string_view kOptimizeStatusFilename = "optimize_status";

libtextclassifier3::Status ValidateOptions(
    const IcingSearchEngineOptions& options) {
  // These options are only used in IndexProcessor, which won't be created
  // until the first Put call. So they must be checked here, so that any
  // errors can be surfaced in Initialize.
  if (options.max_tokens_per_doc() <= 0) {
    return absl_ports::InvalidArgumentError(
        "Options::max_tokens_per_doc must be greater than zero.");
  }
  return libtextclassifier3::Status::OK;
}

libtextclassifier3::Status ValidateResultSpec(
    const ResultSpecProto& result_spec) {
  if (result_spec.num_per_page() < 0) {
    return absl_ports::InvalidArgumentError(
        "ResultSpecProto.num_per_page cannot be negative.");
  }
  std::unordered_set<std::string> unique_namespaces;
  for (const ResultSpecProto::ResultGrouping& result_grouping :
       result_spec.result_groupings()) {
    if (result_grouping.max_results() <= 0) {
      return absl_ports::InvalidArgumentError(
          "Cannot specify a result grouping with max results <= 0.");
    }
    for (const std::string& name_space : result_grouping.namespaces()) {
      if (unique_namespaces.count(name_space) > 0) {
        return absl_ports::InvalidArgumentError(
            "Namespaces must be unique across result groups.");
      }
      unique_namespaces.insert(name_space);
    }
  }
  return libtextclassifier3::Status::OK;
}

libtextclassifier3::Status ValidateSearchSpec(
    const SearchSpecProto& search_spec,
    const PerformanceConfiguration& configuration) {
  if (search_spec.query().size() > configuration.max_query_length) {
    return absl_ports::InvalidArgumentError(
        absl_ports::StrCat("SearchSpecProto.query is longer than the maximum "
                           "allowed query length: ",
                           std::to_string(configuration.max_query_length)));
  }
  return libtextclassifier3::Status::OK;
}

IndexProcessor::Options CreateIndexProcessorOptions(
    const IcingSearchEngineOptions& options) {
  IndexProcessor::Options index_processor_options;
  index_processor_options.max_tokens_per_document =
      options.max_tokens_per_doc();
  index_processor_options.token_limit_behavior =
      IndexProcessor::Options::TokenLimitBehavior::kSuppressError;
  return index_processor_options;
}

// Document store files are in a standalone subfolder for easier file
// management. We can delete and recreate the subfolder and not touch/affect
// anything else.
std::string MakeDocumentDirectoryPath(const std::string& base_dir) {
  return absl_ports::StrCat(base_dir, "/", kDocumentSubfolderName);
}

// Makes a temporary folder path for the document store which will be used
// during full optimization.
std::string MakeDocumentTemporaryDirectoryPath(const std::string& base_dir) {
  return absl_ports::StrCat(base_dir, "/", kDocumentSubfolderName,
                            "_optimize_tmp");
}

// Index files are in a standalone subfolder because for easier file management.
// We can delete and recreate the subfolder and not touch/affect anything
// else.
std::string MakeIndexDirectoryPath(const std::string& base_dir) {
  return absl_ports::StrCat(base_dir, "/", kIndexSubfolderName);
}

// SchemaStore files are in a standalone subfolder for easier file management.
// We can delete and recreate the subfolder and not touch/affect anything
// else.
std::string MakeSchemaDirectoryPath(const std::string& base_dir) {
  return absl_ports::StrCat(base_dir, "/", kSchemaSubfolderName);
}
std::string MakeSetSchemaMarkerFilePath(const std::string& base_dir) {
  return absl_ports::StrCat(base_dir, "/", kSetSchemaMarkerFilename);
}

void TransformStatus(const libtextclassifier3::Status& internal_status,
                     StatusProto* status_proto) {
  StatusProto::Code code;
  if (!internal_status.ok()) {
    ICING_LOG(WARNING) << "Error: " << internal_status.error_code()
                       << ", Message: " << internal_status.error_message();
  }
  switch (internal_status.CanonicalCode()) {
    case libtextclassifier3::StatusCode::OK:
      code = StatusProto::OK;
      break;
    case libtextclassifier3::StatusCode::DATA_LOSS:
      code = StatusProto::WARNING_DATA_LOSS;
      break;
    case libtextclassifier3::StatusCode::INVALID_ARGUMENT:
      code = StatusProto::INVALID_ARGUMENT;
      break;
    case libtextclassifier3::StatusCode::NOT_FOUND:
      code = StatusProto::NOT_FOUND;
      break;
    case libtextclassifier3::StatusCode::FAILED_PRECONDITION:
      code = StatusProto::FAILED_PRECONDITION;
      break;
    case libtextclassifier3::StatusCode::ABORTED:
      code = StatusProto::ABORTED;
      break;
    case libtextclassifier3::StatusCode::INTERNAL:
      // TODO(b/147699081): Cleanup our internal use of INTERNAL since it
      // doesn't match with what it *should* indicate as described in
      // go/icing-library-apis.
      code = StatusProto::INTERNAL;
      break;
    case libtextclassifier3::StatusCode::RESOURCE_EXHAUSTED:
      // TODO(b/147699081): Note that we don't detect all cases of OUT_OF_SPACE
      // (e.g. if the document log is full). And we use RESOURCE_EXHAUSTED
      // internally to indicate other resources are exhausted (e.g.
      // DocHitInfos) - although none of these are exposed through the API.
      // Consider separating the two cases out more clearly.
      code = StatusProto::OUT_OF_SPACE;
      break;
    case libtextclassifier3::StatusCode::ALREADY_EXISTS:
      code = StatusProto::ALREADY_EXISTS;
      break;
    case libtextclassifier3::StatusCode::CANCELLED:
      [[fallthrough]];
    case libtextclassifier3::StatusCode::UNKNOWN:
      [[fallthrough]];
    case libtextclassifier3::StatusCode::DEADLINE_EXCEEDED:
      [[fallthrough]];
    case libtextclassifier3::StatusCode::PERMISSION_DENIED:
      [[fallthrough]];
    case libtextclassifier3::StatusCode::OUT_OF_RANGE:
      [[fallthrough]];
    case libtextclassifier3::StatusCode::UNIMPLEMENTED:
      [[fallthrough]];
    case libtextclassifier3::StatusCode::UNAVAILABLE:
      [[fallthrough]];
    case libtextclassifier3::StatusCode::UNAUTHENTICATED:
      // Other internal status codes aren't supported externally yet. If it
      // should be supported, add another switch-case above.
      ICING_LOG(ERROR) << IcingStringUtil::StringPrintf(
          "Internal status code %d not supported in the external API",
          internal_status.error_code());
      code = StatusProto::UNKNOWN;
      break;
  }
  status_proto->set_code(code);
  status_proto->set_message(internal_status.error_message());
}

}  // namespace

IcingSearchEngine::IcingSearchEngine(const IcingSearchEngineOptions& options,
                                     std::unique_ptr<const JniCache> jni_cache)
    : IcingSearchEngine(options, std::make_unique<Filesystem>(),
                        std::make_unique<IcingFilesystem>(),
                        std::make_unique<Clock>(), std::move(jni_cache)) {}

IcingSearchEngine::IcingSearchEngine(
    IcingSearchEngineOptions options,
    std::unique_ptr<const Filesystem> filesystem,
    std::unique_ptr<const IcingFilesystem> icing_filesystem,
    std::unique_ptr<Clock> clock, std::unique_ptr<const JniCache> jni_cache)
    : options_(std::move(options)),
      filesystem_(std::move(filesystem)),
      icing_filesystem_(std::move(icing_filesystem)),
      clock_(std::move(clock)),
      jni_cache_(std::move(jni_cache)) {
  ICING_VLOG(1) << "Creating IcingSearchEngine in dir: " << options_.base_dir();
}

IcingSearchEngine::~IcingSearchEngine() {
  if (initialized_) {
    if (PersistToDisk(PersistType::FULL).status().code() != StatusProto::OK) {
      ICING_LOG(ERROR)
          << "Error persisting to disk in IcingSearchEngine destructor";
    }
  }
}

InitializeResultProto IcingSearchEngine::Initialize() {
  // This method does both read and write so we need a writer lock. Using two
  // locks (reader and writer) has the chance to be interrupted during
  // switching.
  absl_ports::unique_lock l(&mutex_);
  return InternalInitialize();
}

InitializeResultProto IcingSearchEngine::InternalInitialize() {
  ICING_VLOG(1) << "Initializing IcingSearchEngine in dir: "
                << options_.base_dir();

  // Measure the latency of the initialization process.
  std::unique_ptr<Timer> initialize_timer = clock_->GetNewTimer();

  InitializeResultProto result_proto;
  StatusProto* result_status = result_proto.mutable_status();
  InitializeStatsProto* initialize_stats =
      result_proto.mutable_initialize_stats();
  if (initialized_) {
    // Already initialized.
    result_status->set_code(StatusProto::OK);
    initialize_stats->set_latency_ms(
        initialize_timer->GetElapsedMilliseconds());
    initialize_stats->set_num_documents(document_store_->num_documents());
    return result_proto;
  }

  libtextclassifier3::Status status = InitializeMembers(initialize_stats);
  if (status.ok() || absl_ports::IsDataLoss(status)) {
    initialized_ = true;
  }
  TransformStatus(status, result_status);
  initialize_stats->set_latency_ms(initialize_timer->GetElapsedMilliseconds());
  return result_proto;
}

libtextclassifier3::Status IcingSearchEngine::InitializeMembers(
    InitializeStatsProto* initialize_stats) {
  ICING_RETURN_ERROR_IF_NULL(initialize_stats);
  ICING_RETURN_IF_ERROR(InitializeOptions());
  ICING_RETURN_IF_ERROR(InitializeSchemaStore(initialize_stats));

  // TODO(b/156383798) : Resolve how to specify the locale.
  language_segmenter_factory::SegmenterOptions segmenter_options(
      ULOC_US, jni_cache_.get());
  TC3_ASSIGN_OR_RETURN(language_segmenter_, language_segmenter_factory::Create(
                                                std::move(segmenter_options)));

  TC3_ASSIGN_OR_RETURN(normalizer_,
                       normalizer_factory::Create(options_.max_token_length()));

  std::string marker_filepath =
      MakeSetSchemaMarkerFilePath(options_.base_dir());
  libtextclassifier3::Status status;
  if (absl_ports::IsNotFound(schema_store_->GetSchema().status())) {
    // The schema was either lost or never set before. Wipe out the doc store
    // and index directories and initialize them from scratch.
    const std::string doc_store_dir =
        MakeDocumentDirectoryPath(options_.base_dir());
    const std::string index_dir = MakeIndexDirectoryPath(options_.base_dir());
    if (!filesystem_->DeleteDirectoryRecursively(doc_store_dir.c_str()) ||
        !filesystem_->DeleteDirectoryRecursively(index_dir.c_str())) {
      return absl_ports::InternalError(absl_ports::StrCat(
          "Could not delete directories: ", index_dir, " and ", doc_store_dir));
    }
    ICING_RETURN_IF_ERROR(InitializeDocumentStore(
        /*force_recovery_and_revalidate_documents=*/false, initialize_stats));
    status = InitializeIndex(initialize_stats);
  } else if (filesystem_->FileExists(marker_filepath.c_str())) {
    // If the marker file is still around then something wonky happened when we
    // last tried to set the schema.
    ICING_RETURN_IF_ERROR(InitializeDocumentStore(
        /*force_recovery_and_revalidate_documents=*/true, initialize_stats));
    initialize_stats->set_document_store_recovery_cause(
        InitializeStatsProto::SCHEMA_CHANGES_OUT_OF_SYNC);

    // We're going to need to build the index from scratch. So just delete its
    // files now.
    const std::string index_dir = MakeIndexDirectoryPath(options_.base_dir());
    Index::Options index_options(index_dir, options_.index_merge_size());
    if (!filesystem_->DeleteDirectoryRecursively(index_dir.c_str()) ||
        !filesystem_->CreateDirectoryRecursively(index_dir.c_str())) {
      return absl_ports::InternalError(
          absl_ports::StrCat("Could not recreate directory: ", index_dir));
    }
    ICING_ASSIGN_OR_RETURN(index_,
                           Index::Create(index_options, filesystem_.get(),
                                         icing_filesystem_.get()));

    std::unique_ptr<Timer> restore_timer = clock_->GetNewTimer();
    IndexRestorationResult restore_result = RestoreIndexIfNeeded();
    status = std::move(restore_result.status);
    // DATA_LOSS means that we have successfully initialized and re-added
    // content to the index. Some indexed content was lost, but otherwise the
    // index is in a valid state and can be queried.
    if (!status.ok() && !absl_ports::IsDataLoss(status)) {
      return status;
    }

    // Delete the marker file to indicate that everything is now in sync with
    // whatever changes were made to the schema.
    filesystem_->DeleteFile(marker_filepath.c_str());

    initialize_stats->set_index_restoration_latency_ms(
        restore_timer->GetElapsedMilliseconds());
    initialize_stats->set_index_restoration_cause(
        InitializeStatsProto::SCHEMA_CHANGES_OUT_OF_SYNC);
  } else {
    ICING_RETURN_IF_ERROR(InitializeDocumentStore(
        /*force_recovery_and_revalidate_documents=*/false, initialize_stats));
    status = InitializeIndex(initialize_stats);
    if (!status.ok() && !absl_ports::IsDataLoss(status)) {
      return status;
    }
  }

  result_state_manager_ = std::make_unique<ResultStateManager>(
      performance_configuration_.max_num_total_hits, *document_store_);

  return status;
}

libtextclassifier3::Status IcingSearchEngine::InitializeOptions() {
  ICING_RETURN_IF_ERROR(ValidateOptions(options_));

  // Make sure the base directory exists
  if (!filesystem_->CreateDirectoryRecursively(options_.base_dir().c_str())) {
    return absl_ports::InternalError(absl_ports::StrCat(
        "Could not create directory: ", options_.base_dir()));
  }

  return libtextclassifier3::Status::OK;
}

libtextclassifier3::Status IcingSearchEngine::InitializeSchemaStore(
    InitializeStatsProto* initialize_stats) {
  ICING_RETURN_ERROR_IF_NULL(initialize_stats);

  const std::string schema_store_dir =
      MakeSchemaDirectoryPath(options_.base_dir());
  // Make sure the sub-directory exists
  if (!filesystem_->CreateDirectoryRecursively(schema_store_dir.c_str())) {
    return absl_ports::InternalError(
        absl_ports::StrCat("Could not create directory: ", schema_store_dir));
  }
  ICING_ASSIGN_OR_RETURN(
      schema_store_, SchemaStore::Create(filesystem_.get(), schema_store_dir,
                                         clock_.get(), initialize_stats));

  return libtextclassifier3::Status::OK;
}

libtextclassifier3::Status IcingSearchEngine::InitializeDocumentStore(
    bool force_recovery_and_revalidate_documents,
    InitializeStatsProto* initialize_stats) {
  ICING_RETURN_ERROR_IF_NULL(initialize_stats);

  const std::string document_dir =
      MakeDocumentDirectoryPath(options_.base_dir());
  // Make sure the sub-directory exists
  if (!filesystem_->CreateDirectoryRecursively(document_dir.c_str())) {
    return absl_ports::InternalError(
        absl_ports::StrCat("Could not create directory: ", document_dir));
  }
  ICING_ASSIGN_OR_RETURN(
      DocumentStore::CreateResult create_result,
      DocumentStore::Create(
          filesystem_.get(), document_dir, clock_.get(), schema_store_.get(),
          force_recovery_and_revalidate_documents, initialize_stats));
  document_store_ = std::move(create_result.document_store);

  return libtextclassifier3::Status::OK;
}

libtextclassifier3::Status IcingSearchEngine::InitializeIndex(
    InitializeStatsProto* initialize_stats) {
  ICING_RETURN_ERROR_IF_NULL(initialize_stats);

  const std::string index_dir = MakeIndexDirectoryPath(options_.base_dir());
  // Make sure the sub-directory exists
  if (!filesystem_->CreateDirectoryRecursively(index_dir.c_str())) {
    return absl_ports::InternalError(
        absl_ports::StrCat("Could not create directory: ", index_dir));
  }
  Index::Options index_options(index_dir, options_.index_merge_size());

  InitializeStatsProto::RecoveryCause recovery_cause;
  auto index_or =
      Index::Create(index_options, filesystem_.get(), icing_filesystem_.get());
  if (!index_or.ok()) {
    if (!filesystem_->DeleteDirectoryRecursively(index_dir.c_str()) ||
        !filesystem_->CreateDirectoryRecursively(index_dir.c_str())) {
      return absl_ports::InternalError(
          absl_ports::StrCat("Could not recreate directory: ", index_dir));
    }

    recovery_cause = InitializeStatsProto::IO_ERROR;

    // Try recreating it from scratch and re-indexing everything.
    ICING_ASSIGN_OR_RETURN(index_,
                           Index::Create(index_options, filesystem_.get(),
                                         icing_filesystem_.get()));
  } else {
    // Index was created fine.
    index_ = std::move(index_or).ValueOrDie();
    // If a recover does have to happen, then it must be because the index is
    // out of sync with the document store.
    recovery_cause = InitializeStatsProto::INCONSISTENT_WITH_GROUND_TRUTH;
  }

  std::unique_ptr<Timer> restore_timer = clock_->GetNewTimer();
  IndexRestorationResult restore_result = RestoreIndexIfNeeded();
  if (restore_result.needed_restoration) {
    initialize_stats->set_index_restoration_latency_ms(
        restore_timer->GetElapsedMilliseconds());
    initialize_stats->set_index_restoration_cause(recovery_cause);
  }
  return restore_result.status;
}

SetSchemaResultProto IcingSearchEngine::SetSchema(
    const SchemaProto& new_schema, bool ignore_errors_and_delete_documents) {
  return SetSchema(SchemaProto(new_schema), ignore_errors_and_delete_documents);
}

SetSchemaResultProto IcingSearchEngine::SetSchema(
    SchemaProto&& new_schema, bool ignore_errors_and_delete_documents) {
  ICING_VLOG(1) << "Setting new Schema";

  SetSchemaResultProto result_proto;
  StatusProto* result_status = result_proto.mutable_status();

  absl_ports::unique_lock l(&mutex_);
  if (!initialized_) {
    result_status->set_code(StatusProto::FAILED_PRECONDITION);
    result_status->set_message("IcingSearchEngine has not been initialized!");
    return result_proto;
  }

  auto lost_previous_schema_or = LostPreviousSchema();
  if (!lost_previous_schema_or.ok()) {
    TransformStatus(lost_previous_schema_or.status(), result_status);
    return result_proto;
  }
  bool lost_previous_schema = lost_previous_schema_or.ValueOrDie();

  std::string marker_filepath =
      MakeSetSchemaMarkerFilePath(options_.base_dir());
  // Create the marker file indicating that we are going to apply a schema
  // change. No need to write anything to the marker file - its existence is the
  // only thing that matters. The marker file is used to indicate if we
  // encountered a crash or a power loss while updating the schema and other
  // files. So set it up to be deleted as long as we return from this function.
  DestructibleFile marker_file(marker_filepath, filesystem_.get());

  auto set_schema_result_or = schema_store_->SetSchema(
      std::move(new_schema), ignore_errors_and_delete_documents);
  if (!set_schema_result_or.ok()) {
    TransformStatus(set_schema_result_or.status(), result_status);
    return result_proto;
  }
  const SchemaStore::SetSchemaResult set_schema_result =
      set_schema_result_or.ValueOrDie();

  for (const std::string& deleted_type :
       set_schema_result.schema_types_deleted_by_name) {
    result_proto.add_deleted_schema_types(deleted_type);
  }

  for (const std::string& incompatible_type :
       set_schema_result.schema_types_incompatible_by_name) {
    result_proto.add_incompatible_schema_types(incompatible_type);
  }

  libtextclassifier3::Status status;
  if (set_schema_result.success) {
    if (lost_previous_schema) {
      // No previous schema to calculate a diff against. We have to go through
      // and revalidate all the Documents in the DocumentStore
      status = document_store_->UpdateSchemaStore(schema_store_.get());
      if (!status.ok()) {
        TransformStatus(status, result_status);
        return result_proto;
      }
    } else if (!set_schema_result.old_schema_type_ids_changed.empty() ||
               !set_schema_result.schema_types_incompatible_by_id.empty() ||
               !set_schema_result.schema_types_deleted_by_id.empty()) {
      status = document_store_->OptimizedUpdateSchemaStore(schema_store_.get(),
                                                           set_schema_result);
      if (!status.ok()) {
        TransformStatus(status, result_status);
        return result_proto;
      }
    }

    if (lost_previous_schema || set_schema_result.index_incompatible) {
      // Clears all index files
      status = index_->Reset();
      if (!status.ok()) {
        TransformStatus(status, result_status);
        return result_proto;
      }

      IndexRestorationResult restore_result = RestoreIndexIfNeeded();
      // DATA_LOSS means that we have successfully re-added content to the
      // index. Some indexed content was lost, but otherwise the index is in a
      // valid state and can be queried.
      if (!restore_result.status.ok() &&
          !absl_ports::IsDataLoss(restore_result.status)) {
        TransformStatus(status, result_status);
        return result_proto;
      }
    }

    result_status->set_code(StatusProto::OK);
  } else {
    result_status->set_code(StatusProto::FAILED_PRECONDITION);
    result_status->set_message("Schema is incompatible.");
  }

  return result_proto;
}

GetSchemaResultProto IcingSearchEngine::GetSchema() {
  GetSchemaResultProto result_proto;
  StatusProto* result_status = result_proto.mutable_status();

  absl_ports::shared_lock l(&mutex_);
  if (!initialized_) {
    result_status->set_code(StatusProto::FAILED_PRECONDITION);
    result_status->set_message("IcingSearchEngine has not been initialized!");
    return result_proto;
  }

  auto schema_or = schema_store_->GetSchema();
  if (!schema_or.ok()) {
    TransformStatus(schema_or.status(), result_status);
    return result_proto;
  }

  result_status->set_code(StatusProto::OK);
  *result_proto.mutable_schema() = *std::move(schema_or).ValueOrDie();
  return result_proto;
}

GetSchemaTypeResultProto IcingSearchEngine::GetSchemaType(
    std::string_view schema_type) {
  GetSchemaTypeResultProto result_proto;
  StatusProto* result_status = result_proto.mutable_status();

  absl_ports::shared_lock l(&mutex_);
  if (!initialized_) {
    result_status->set_code(StatusProto::FAILED_PRECONDITION);
    result_status->set_message("IcingSearchEngine has not been initialized!");
    return result_proto;
  }

  auto type_config_or = schema_store_->GetSchemaTypeConfig(schema_type);
  if (!type_config_or.ok()) {
    TransformStatus(type_config_or.status(), result_status);
    return result_proto;
  }

  result_status->set_code(StatusProto::OK);
  *result_proto.mutable_schema_type_config() = *(type_config_or.ValueOrDie());
  return result_proto;
}

PutResultProto IcingSearchEngine::Put(const DocumentProto& document) {
  return Put(DocumentProto(document));
}

PutResultProto IcingSearchEngine::Put(DocumentProto&& document) {
  ICING_VLOG(1) << "Writing document to document store";

  std::unique_ptr<Timer> put_timer = clock_->GetNewTimer();

  PutResultProto result_proto;
  StatusProto* result_status = result_proto.mutable_status();
  PutDocumentStatsProto* put_document_stats =
      result_proto.mutable_put_document_stats();

  // Lock must be acquired before validation because the DocumentStore uses
  // the schema file to validate, and the schema could be changed in
  // SetSchema() which is protected by the same mutex.
  absl_ports::unique_lock l(&mutex_);
  if (!initialized_) {
    result_status->set_code(StatusProto::FAILED_PRECONDITION);
    result_status->set_message("IcingSearchEngine has not been initialized!");
    put_document_stats->set_latency_ms(put_timer->GetElapsedMilliseconds());
    return result_proto;
  }

  auto tokenized_document_or = TokenizedDocument::Create(
      schema_store_.get(), language_segmenter_.get(), std::move(document));
  if (!tokenized_document_or.ok()) {
    TransformStatus(tokenized_document_or.status(), result_status);
    put_document_stats->set_latency_ms(put_timer->GetElapsedMilliseconds());
    return result_proto;
  }
  TokenizedDocument tokenized_document(
      std::move(tokenized_document_or).ValueOrDie());

  auto document_id_or =
      document_store_->Put(tokenized_document.document(),
                           tokenized_document.num_tokens(), put_document_stats);
  if (!document_id_or.ok()) {
    TransformStatus(document_id_or.status(), result_status);
    put_document_stats->set_latency_ms(put_timer->GetElapsedMilliseconds());
    return result_proto;
  }
  DocumentId document_id = document_id_or.ValueOrDie();

  auto index_processor_or = IndexProcessor::Create(
      normalizer_.get(), index_.get(), CreateIndexProcessorOptions(options_),
      clock_.get());
  if (!index_processor_or.ok()) {
    TransformStatus(index_processor_or.status(), result_status);
    put_document_stats->set_latency_ms(put_timer->GetElapsedMilliseconds());
    return result_proto;
  }
  std::unique_ptr<IndexProcessor> index_processor =
      std::move(index_processor_or).ValueOrDie();

  auto status = index_processor->IndexDocument(tokenized_document, document_id,
                                               put_document_stats);

  TransformStatus(status, result_status);
  put_document_stats->set_latency_ms(put_timer->GetElapsedMilliseconds());
  return result_proto;
}

GetResultProto IcingSearchEngine::Get(const std::string_view name_space,
                                      const std::string_view uri,
                                      const GetResultSpecProto& result_spec) {
  GetResultProto result_proto;
  StatusProto* result_status = result_proto.mutable_status();

  absl_ports::shared_lock l(&mutex_);
  if (!initialized_) {
    result_status->set_code(StatusProto::FAILED_PRECONDITION);
    result_status->set_message("IcingSearchEngine has not been initialized!");
    return result_proto;
  }

  auto document_or = document_store_->Get(name_space, uri);
  if (!document_or.ok()) {
    TransformStatus(document_or.status(), result_status);
    return result_proto;
  }

  DocumentProto document = std::move(document_or).ValueOrDie();
  std::unique_ptr<ProjectionTree> type_projection_tree;
  std::unique_ptr<ProjectionTree> wildcard_projection_tree;
  for (const TypePropertyMask& type_field_mask :
       result_spec.type_property_masks()) {
    if (type_field_mask.schema_type() == document.schema()) {
      type_projection_tree = std::make_unique<ProjectionTree>(type_field_mask);
    } else if (type_field_mask.schema_type() ==
               ProjectionTree::kSchemaTypeWildcard) {
      wildcard_projection_tree =
          std::make_unique<ProjectionTree>(type_field_mask);
    }
  }

  // Apply projection
  if (type_projection_tree != nullptr) {
    projector::Project(type_projection_tree->root().children, &document);
  } else if (wildcard_projection_tree != nullptr) {
    projector::Project(wildcard_projection_tree->root().children, &document);
  }

  result_status->set_code(StatusProto::OK);
  *result_proto.mutable_document() = std::move(document);
  return result_proto;
}

ReportUsageResultProto IcingSearchEngine::ReportUsage(
    const UsageReport& usage_report) {
  ReportUsageResultProto result_proto;
  StatusProto* result_status = result_proto.mutable_status();

  absl_ports::unique_lock l(&mutex_);
  if (!initialized_) {
    result_status->set_code(StatusProto::FAILED_PRECONDITION);
    result_status->set_message("IcingSearchEngine has not been initialized!");
    return result_proto;
  }

  libtextclassifier3::Status status =
      document_store_->ReportUsage(usage_report);
  TransformStatus(status, result_status);
  return result_proto;
}

GetAllNamespacesResultProto IcingSearchEngine::GetAllNamespaces() {
  GetAllNamespacesResultProto result_proto;
  StatusProto* result_status = result_proto.mutable_status();

  absl_ports::shared_lock l(&mutex_);
  if (!initialized_) {
    result_status->set_code(StatusProto::FAILED_PRECONDITION);
    result_status->set_message("IcingSearchEngine has not been initialized!");
    return result_proto;
  }

  std::vector<std::string> namespaces = document_store_->GetAllNamespaces();

  for (const std::string& namespace_ : namespaces) {
    result_proto.add_namespaces(namespace_);
  }

  result_status->set_code(StatusProto::OK);
  return result_proto;
}

DeleteResultProto IcingSearchEngine::Delete(const std::string_view name_space,
                                            const std::string_view uri) {
  ICING_VLOG(1) << "Deleting document from doc store";

  DeleteResultProto result_proto;
  StatusProto* result_status = result_proto.mutable_status();

  absl_ports::unique_lock l(&mutex_);
  if (!initialized_) {
    result_status->set_code(StatusProto::FAILED_PRECONDITION);
    result_status->set_message("IcingSearchEngine has not been initialized!");
    return result_proto;
  }

  DeleteStatsProto* delete_stats = result_proto.mutable_delete_stats();
  delete_stats->set_delete_type(DeleteStatsProto::DeleteType::SINGLE);

  std::unique_ptr<Timer> delete_timer = clock_->GetNewTimer();
  // TODO(b/144458732): Implement a more robust version of TC_RETURN_IF_ERROR
  // that can support error logging.
  libtextclassifier3::Status status = document_store_->Delete(name_space, uri);
  if (!status.ok()) {
    ICING_LOG(ERROR) << status.error_message()
                     << "Failed to delete Document. namespace: " << name_space
                     << ", uri: " << uri;
    TransformStatus(status, result_status);
    return result_proto;
  }

  result_status->set_code(StatusProto::OK);
  delete_stats->set_latency_ms(delete_timer->GetElapsedMilliseconds());
  delete_stats->set_num_documents_deleted(1);
  return result_proto;
}

DeleteByNamespaceResultProto IcingSearchEngine::DeleteByNamespace(
    const std::string_view name_space) {
  ICING_VLOG(1) << "Deleting namespace from doc store";

  DeleteByNamespaceResultProto delete_result;
  StatusProto* result_status = delete_result.mutable_status();
  absl_ports::unique_lock l(&mutex_);
  if (!initialized_) {
    result_status->set_code(StatusProto::FAILED_PRECONDITION);
    result_status->set_message("IcingSearchEngine has not been initialized!");
    return delete_result;
  }

  DeleteStatsProto* delete_stats = delete_result.mutable_delete_stats();
  delete_stats->set_delete_type(DeleteStatsProto::DeleteType::NAMESPACE);

  std::unique_ptr<Timer> delete_timer = clock_->GetNewTimer();
  // TODO(b/144458732): Implement a more robust version of TC_RETURN_IF_ERROR
  // that can support error logging.
  DocumentStore::DeleteByGroupResult doc_store_result =
      document_store_->DeleteByNamespace(name_space);
  if (!doc_store_result.status.ok()) {
    ICING_LOG(ERROR) << doc_store_result.status.error_message()
                     << "Failed to delete Namespace: " << name_space;
    TransformStatus(doc_store_result.status, result_status);
    return delete_result;
  }

  result_status->set_code(StatusProto::OK);
  delete_stats->set_latency_ms(delete_timer->GetElapsedMilliseconds());
  delete_stats->set_num_documents_deleted(doc_store_result.num_docs_deleted);
  return delete_result;
}

DeleteBySchemaTypeResultProto IcingSearchEngine::DeleteBySchemaType(
    const std::string_view schema_type) {
  ICING_VLOG(1) << "Deleting type from doc store";

  DeleteBySchemaTypeResultProto delete_result;
  StatusProto* result_status = delete_result.mutable_status();
  absl_ports::unique_lock l(&mutex_);
  if (!initialized_) {
    result_status->set_code(StatusProto::FAILED_PRECONDITION);
    result_status->set_message("IcingSearchEngine has not been initialized!");
    return delete_result;
  }

  DeleteStatsProto* delete_stats = delete_result.mutable_delete_stats();
  delete_stats->set_delete_type(DeleteStatsProto::DeleteType::SCHEMA_TYPE);

  std::unique_ptr<Timer> delete_timer = clock_->GetNewTimer();
  // TODO(b/144458732): Implement a more robust version of TC_RETURN_IF_ERROR
  // that can support error logging.
  DocumentStore::DeleteByGroupResult doc_store_result =
      document_store_->DeleteBySchemaType(schema_type);
  if (!doc_store_result.status.ok()) {
    ICING_LOG(ERROR) << doc_store_result.status.error_message()
                     << "Failed to delete SchemaType: " << schema_type;
    TransformStatus(doc_store_result.status, result_status);
    return delete_result;
  }

  result_status->set_code(StatusProto::OK);
  delete_stats->set_latency_ms(delete_timer->GetElapsedMilliseconds());
  delete_stats->set_num_documents_deleted(doc_store_result.num_docs_deleted);
  return delete_result;
}

DeleteByQueryResultProto IcingSearchEngine::DeleteByQuery(
    const SearchSpecProto& search_spec) {
  ICING_VLOG(1) << "Deleting documents for query " << search_spec.query()
                << " from doc store";

  DeleteByQueryResultProto result_proto;
  StatusProto* result_status = result_proto.mutable_status();

  absl_ports::unique_lock l(&mutex_);
  if (!initialized_) {
    result_status->set_code(StatusProto::FAILED_PRECONDITION);
    result_status->set_message("IcingSearchEngine has not been initialized!");
    return result_proto;
  }

  DeleteStatsProto* delete_stats = result_proto.mutable_delete_stats();
  delete_stats->set_delete_type(DeleteStatsProto::DeleteType::QUERY);


  std::unique_ptr<Timer> delete_timer = clock_->GetNewTimer();
  libtextclassifier3::Status status =
      ValidateSearchSpec(search_spec, performance_configuration_);
  if (!status.ok()) {
    TransformStatus(status, result_status);
    return result_proto;
  }

  // Gets unordered results from query processor
  auto query_processor_or = QueryProcessor::Create(
      index_.get(), language_segmenter_.get(), normalizer_.get(),
      document_store_.get(), schema_store_.get());
  if (!query_processor_or.ok()) {
    TransformStatus(query_processor_or.status(), result_status);
    return result_proto;
  }
  std::unique_ptr<QueryProcessor> query_processor =
      std::move(query_processor_or).ValueOrDie();

  auto query_results_or = query_processor->ParseSearch(search_spec);
  if (!query_results_or.ok()) {
    TransformStatus(query_results_or.status(), result_status);
    return result_proto;
  }
  QueryProcessor::QueryResults query_results =
      std::move(query_results_or).ValueOrDie();

  ICING_VLOG(2) << "Deleting the docs that matched the query.";
  int num_deleted = 0;

  while (query_results.root_iterator->Advance().ok()) {
    ICING_VLOG(3) << "Deleting doc "
                  << query_results.root_iterator->doc_hit_info().document_id();
    ++num_deleted;
    status = document_store_->Delete(
        query_results.root_iterator->doc_hit_info().document_id());
    if (!status.ok()) {
      TransformStatus(status, result_status);
      return result_proto;
    }
  }

  if (num_deleted > 0) {
    result_proto.mutable_status()->set_code(StatusProto::OK);
  } else {
    result_proto.mutable_status()->set_code(StatusProto::NOT_FOUND);
    result_proto.mutable_status()->set_message(
        "No documents matched the query to delete by!");
  }
  delete_stats->set_latency_ms(delete_timer->GetElapsedMilliseconds());
  delete_stats->set_num_documents_deleted(num_deleted);
  return result_proto;
}

PersistToDiskResultProto IcingSearchEngine::PersistToDisk(
    PersistType::Code persist_type) {
  ICING_VLOG(1) << "Persisting data to disk";

  PersistToDiskResultProto result_proto;
  StatusProto* result_status = result_proto.mutable_status();

  absl_ports::unique_lock l(&mutex_);
  if (!initialized_) {
    result_status->set_code(StatusProto::FAILED_PRECONDITION);
    result_status->set_message("IcingSearchEngine has not been initialized!");
    return result_proto;
  }

  auto status = InternalPersistToDisk(persist_type);
  TransformStatus(status, result_status);
  return result_proto;
}

// Optimizes Icing's storage
//
// Steps:
// 1. Flush data to disk.
// 2. Copy data needed to a tmp directory.
// 3. Swap current directory and tmp directory.
OptimizeResultProto IcingSearchEngine::Optimize() {
  ICING_VLOG(1) << "Optimizing icing storage";

  OptimizeResultProto result_proto;
  StatusProto* result_status = result_proto.mutable_status();

  absl_ports::unique_lock l(&mutex_);
  if (!initialized_) {
    result_status->set_code(StatusProto::FAILED_PRECONDITION);
    result_status->set_message("IcingSearchEngine has not been initialized!");
    return result_proto;
  }

  std::unique_ptr<Timer> optimize_timer = clock_->GetNewTimer();
  OptimizeStatsProto* optimize_stats = result_proto.mutable_optimize_stats();
  int64_t before_size = filesystem_->GetDiskUsage(options_.base_dir().c_str());
  if (before_size != Filesystem::kBadFileSize) {
    optimize_stats->set_storage_size_before(before_size);
  } else {
    // Set -1 as a sentinel value when failures occur.
    optimize_stats->set_storage_size_before(-1);
  }

  // Flushes data to disk before doing optimization
  auto status = InternalPersistToDisk(PersistType::FULL);
  if (!status.ok()) {
    TransformStatus(status, result_status);
    return result_proto;
  }

  // TODO(b/143646633): figure out if we need to optimize index and doc store
  // at the same time.
  std::unique_ptr<Timer> optimize_doc_store_timer = clock_->GetNewTimer();
  libtextclassifier3::Status optimization_status =
      OptimizeDocumentStore(optimize_stats);
  optimize_stats->set_document_store_optimize_latency_ms(
      optimize_doc_store_timer->GetElapsedMilliseconds());

  if (!optimization_status.ok() &&
      !absl_ports::IsDataLoss(optimization_status)) {
    // The status now is either ABORTED_ERROR or INTERNAL_ERROR.
    // If ABORTED_ERROR, Icing should still be working.
    // If INTERNAL_ERROR, we're having IO errors or other errors that we can't
    // recover from.
    TransformStatus(optimization_status, result_status);
    return result_proto;
  }

  // The status is either OK or DATA_LOSS. The optimized document store is
  // guaranteed to work, so we update index according to the new document store.
  std::unique_ptr<Timer> optimize_index_timer = clock_->GetNewTimer();
  libtextclassifier3::Status index_reset_status = index_->Reset();
  if (!index_reset_status.ok()) {
    status = absl_ports::Annotate(
        absl_ports::InternalError("Failed to reset index after optimization."),
        index_reset_status.error_message());
    TransformStatus(status, result_status);
    return result_proto;
  }

  IndexRestorationResult index_restoration_status = RestoreIndexIfNeeded();
  optimize_stats->set_index_restoration_latency_ms(
      optimize_index_timer->GetElapsedMilliseconds());
  // DATA_LOSS means that we have successfully re-added content to the index.
  // Some indexed content was lost, but otherwise the index is in a valid state
  // and can be queried.
  if (!index_restoration_status.status.ok() &&
      !absl_ports::IsDataLoss(index_restoration_status.status)) {
    status = absl_ports::Annotate(
        absl_ports::InternalError(
            "Failed to reindex documents after optimization."),
        index_restoration_status.status.error_message());

    TransformStatus(status, result_status);
    return result_proto;
  }

  // Read the optimize status to get the time that we last ran.
  std::string optimize_status_filename =
      absl_ports::StrCat(options_.base_dir(), "/", kOptimizeStatusFilename);
  FileBackedProto<OptimizeStatusProto> optimize_status_file(
      *filesystem_, optimize_status_filename);
  auto optimize_status_or = optimize_status_file.Read();
  int64_t current_time = clock_->GetSystemTimeMilliseconds();
  if (optimize_status_or.ok()) {
    // If we have trouble reading the status or this is the first time that
    // we've ever run, don't set this field.
    optimize_stats->set_time_since_last_optimize_ms(
        current_time - optimize_status_or.ValueOrDie()
                           ->last_successful_optimize_run_time_ms());
  }

  // Update the status for this run and write it.
  auto optimize_status = std::make_unique<OptimizeStatusProto>();
  optimize_status->set_last_successful_optimize_run_time_ms(current_time);
  optimize_status_file.Write(std::move(optimize_status));

  int64_t after_size = filesystem_->GetDiskUsage(options_.base_dir().c_str());
  if (after_size != Filesystem::kBadFileSize) {
    optimize_stats->set_storage_size_after(after_size);
  } else {
    // Set -1 as a sentinel value when failures occur.
    optimize_stats->set_storage_size_after(-1);
  }
  optimize_stats->set_latency_ms(optimize_timer->GetElapsedMilliseconds());

  TransformStatus(optimization_status, result_status);
  return result_proto;
}

GetOptimizeInfoResultProto IcingSearchEngine::GetOptimizeInfo() {
  ICING_VLOG(1) << "Getting optimize info from IcingSearchEngine";

  GetOptimizeInfoResultProto result_proto;
  StatusProto* result_status = result_proto.mutable_status();

  absl_ports::shared_lock l(&mutex_);
  if (!initialized_) {
    result_status->set_code(StatusProto::FAILED_PRECONDITION);
    result_status->set_message("IcingSearchEngine has not been initialized!");
    return result_proto;
  }

  // Read the optimize status to get the time that we last ran.
  std::string optimize_status_filename =
      absl_ports::StrCat(options_.base_dir(), "/", kOptimizeStatusFilename);
  FileBackedProto<OptimizeStatusProto> optimize_status_file(
      *filesystem_, optimize_status_filename);
  auto optimize_status_or = optimize_status_file.Read();
  int64_t current_time = clock_->GetSystemTimeMilliseconds();

  if (optimize_status_or.ok()) {
    // If we have trouble reading the status or this is the first time that
    // we've ever run, don't set this field.
    result_proto.set_time_since_last_optimize_ms(
        current_time - optimize_status_or.ValueOrDie()
                           ->last_successful_optimize_run_time_ms());
  }

  // Get stats from DocumentStore
  auto doc_store_optimize_info_or = document_store_->GetOptimizeInfo();
  if (!doc_store_optimize_info_or.ok()) {
    TransformStatus(doc_store_optimize_info_or.status(), result_status);
    return result_proto;
  }
  DocumentStore::OptimizeInfo doc_store_optimize_info =
      doc_store_optimize_info_or.ValueOrDie();
  result_proto.set_optimizable_docs(doc_store_optimize_info.optimizable_docs);

  if (doc_store_optimize_info.optimizable_docs == 0) {
    // Can return early since there's nothing to calculate on the index side
    result_proto.set_estimated_optimizable_bytes(0);
    result_status->set_code(StatusProto::OK);
    return result_proto;
  }

  // Get stats from Index.
  auto index_elements_size_or = index_->GetElementsSize();
  if (!index_elements_size_or.ok()) {
    TransformStatus(index_elements_size_or.status(), result_status);
    return result_proto;
  }
  int64_t index_elements_size = index_elements_size_or.ValueOrDie();

  // Sum up the optimizable sizes from DocumentStore and Index
  result_proto.set_estimated_optimizable_bytes(
      index_elements_size * doc_store_optimize_info.optimizable_docs /
          doc_store_optimize_info.total_docs +
      doc_store_optimize_info.estimated_optimizable_bytes);

  result_status->set_code(StatusProto::OK);
  return result_proto;
}

StorageInfoResultProto IcingSearchEngine::GetStorageInfo() {
  StorageInfoResultProto result;
  absl_ports::shared_lock l(&mutex_);
  if (!initialized_) {
    result.mutable_status()->set_code(StatusProto::FAILED_PRECONDITION);
    result.mutable_status()->set_message(
        "IcingSearchEngine has not been initialized!");
    return result;
  }

  int64_t index_size = filesystem_->GetDiskUsage(options_.base_dir().c_str());
  if (index_size != Filesystem::kBadFileSize) {
    result.mutable_storage_info()->set_total_storage_size(index_size);
  } else {
    result.mutable_storage_info()->set_total_storage_size(-1);
  }
  *result.mutable_storage_info()->mutable_document_storage_info() =
      document_store_->GetStorageInfo();
  *result.mutable_storage_info()->mutable_schema_store_storage_info() =
      schema_store_->GetStorageInfo();
  *result.mutable_storage_info()->mutable_index_storage_info() =
      index_->GetStorageInfo();
  result.mutable_status()->set_code(StatusProto::OK);
  return result;
}

libtextclassifier3::Status IcingSearchEngine::InternalPersistToDisk(
    PersistType::Code persist_type) {
  if (persist_type == PersistType::LITE) {
    return document_store_->PersistToDisk(persist_type);
  }
  ICING_RETURN_IF_ERROR(schema_store_->PersistToDisk());
  ICING_RETURN_IF_ERROR(document_store_->PersistToDisk(PersistType::FULL));
  ICING_RETURN_IF_ERROR(index_->PersistToDisk());

  return libtextclassifier3::Status::OK;
}

SearchResultProto IcingSearchEngine::Search(
    const SearchSpecProto& search_spec, const ScoringSpecProto& scoring_spec,
    const ResultSpecProto& result_spec) {
  SearchResultProto result_proto;
  StatusProto* result_status = result_proto.mutable_status();
  // TODO(b/146008613) Explore ideas to make this function read-only.
  absl_ports::unique_lock l(&mutex_);
  if (!initialized_) {
    result_status->set_code(StatusProto::FAILED_PRECONDITION);
    result_status->set_message("IcingSearchEngine has not been initialized!");
    return result_proto;
  }

  QueryStatsProto* query_stats = result_proto.mutable_query_stats();
  query_stats->set_query_length(search_spec.query().length());
  std::unique_ptr<Timer> overall_timer = clock_->GetNewTimer();

  libtextclassifier3::Status status = ValidateResultSpec(result_spec);
  if (!status.ok()) {
    TransformStatus(status, result_status);
    return result_proto;
  }
  status = ValidateSearchSpec(search_spec, performance_configuration_);
  if (!status.ok()) {
    TransformStatus(status, result_status);
    return result_proto;
  }

  query_stats->set_num_namespaces_filtered(
      search_spec.namespace_filters_size());
  query_stats->set_num_schema_types_filtered(
      search_spec.schema_type_filters_size());
  query_stats->set_ranking_strategy(scoring_spec.rank_by());
  query_stats->set_is_first_page(true);
  query_stats->set_requested_page_size(result_spec.num_per_page());

  std::unique_ptr<Timer> component_timer = clock_->GetNewTimer();
  // Gets unordered results from query processor
  auto query_processor_or = QueryProcessor::Create(
      index_.get(), language_segmenter_.get(), normalizer_.get(),
      document_store_.get(), schema_store_.get());
  if (!query_processor_or.ok()) {
    TransformStatus(query_processor_or.status(), result_status);
    return result_proto;
  }
  std::unique_ptr<QueryProcessor> query_processor =
      std::move(query_processor_or).ValueOrDie();

  auto query_results_or = query_processor->ParseSearch(search_spec);
  if (!query_results_or.ok()) {
    TransformStatus(query_results_or.status(), result_status);
    return result_proto;
  }
  QueryProcessor::QueryResults query_results =
      std::move(query_results_or).ValueOrDie();
  query_stats->set_parse_query_latency_ms(
      component_timer->GetElapsedMilliseconds());

  int term_count = 0;
  for (const auto& section_and_terms : query_results.query_terms) {
    term_count += section_and_terms.second.size();
  }
  query_stats->set_num_terms(term_count);

  component_timer = clock_->GetNewTimer();
  // Scores but does not rank the results.
  libtextclassifier3::StatusOr<std::unique_ptr<ScoringProcessor>>
      scoring_processor_or =
          ScoringProcessor::Create(scoring_spec, document_store_.get());
  if (!scoring_processor_or.ok()) {
    TransformStatus(scoring_processor_or.status(), result_status);
    return result_proto;
  }
  std::unique_ptr<ScoringProcessor> scoring_processor =
      std::move(scoring_processor_or).ValueOrDie();
  std::vector<ScoredDocumentHit> result_document_hits =
      scoring_processor->Score(std::move(query_results.root_iterator),
                               performance_configuration_.num_to_score,
                               &query_results.query_term_iterators);
  query_stats->set_scoring_latency_ms(
      component_timer->GetElapsedMilliseconds());
  query_stats->set_num_documents_scored(result_document_hits.size());

  // Returns early for empty result
  if (result_document_hits.empty()) {
    result_status->set_code(StatusProto::OK);
    return result_proto;
  }

  component_timer = clock_->GetNewTimer();
  // Ranks and paginates results
  libtextclassifier3::StatusOr<PageResultState> page_result_state_or =
      result_state_manager_->RankAndPaginate(ResultState(
          std::move(result_document_hits), std::move(query_results.query_terms),
          search_spec, scoring_spec, result_spec, *document_store_));
  if (!page_result_state_or.ok()) {
    TransformStatus(page_result_state_or.status(), result_status);
    return result_proto;
  }
  PageResultState page_result_state =
      std::move(page_result_state_or).ValueOrDie();
  query_stats->set_ranking_latency_ms(
      component_timer->GetElapsedMilliseconds());

  component_timer = clock_->GetNewTimer();
  // Retrieves the document protos and snippets if requested
  auto result_retriever_or =
      ResultRetriever::Create(document_store_.get(), schema_store_.get(),
                              language_segmenter_.get(), normalizer_.get());
  if (!result_retriever_or.ok()) {
    result_state_manager_->InvalidateResultState(
        page_result_state.next_page_token);
    TransformStatus(result_retriever_or.status(), result_status);
    return result_proto;
  }
  std::unique_ptr<ResultRetriever> result_retriever =
      std::move(result_retriever_or).ValueOrDie();

  libtextclassifier3::StatusOr<std::vector<SearchResultProto::ResultProto>>
      results_or = result_retriever->RetrieveResults(page_result_state);
  if (!results_or.ok()) {
    result_state_manager_->InvalidateResultState(
        page_result_state.next_page_token);
    TransformStatus(results_or.status(), result_status);
    return result_proto;
  }
  std::vector<SearchResultProto::ResultProto> results =
      std::move(results_or).ValueOrDie();

  // Assembles the final search result proto
  result_proto.mutable_results()->Reserve(results.size());
  for (SearchResultProto::ResultProto& result : results) {
    result_proto.mutable_results()->Add(std::move(result));
  }
  result_status->set_code(StatusProto::OK);
  if (page_result_state.next_page_token != kInvalidNextPageToken) {
    result_proto.set_next_page_token(page_result_state.next_page_token);
  }
  query_stats->set_document_retrieval_latency_ms(
      component_timer->GetElapsedMilliseconds());
  query_stats->set_latency_ms(overall_timer->GetElapsedMilliseconds());
  query_stats->set_num_results_returned_current_page(
      result_proto.results_size());
  query_stats->set_num_results_with_snippets(
      std::min(result_proto.results_size(),
               result_spec.snippet_spec().num_to_snippet()));
  return result_proto;
}

SearchResultProto IcingSearchEngine::GetNextPage(uint64_t next_page_token) {
  SearchResultProto result_proto;
  StatusProto* result_status = result_proto.mutable_status();

  // ResultStateManager has its own writer lock, so here we only need a reader
  // lock for other components.
  absl_ports::shared_lock l(&mutex_);
  if (!initialized_) {
    result_status->set_code(StatusProto::FAILED_PRECONDITION);
    result_status->set_message("IcingSearchEngine has not been initialized!");
    return result_proto;
  }

  QueryStatsProto* query_stats = result_proto.mutable_query_stats();
  query_stats->set_is_first_page(false);

  std::unique_ptr<Timer> overall_timer = clock_->GetNewTimer();
  libtextclassifier3::StatusOr<PageResultState> page_result_state_or =
      result_state_manager_->GetNextPage(next_page_token);

  if (!page_result_state_or.ok()) {
    if (absl_ports::IsNotFound(page_result_state_or.status())) {
      // NOT_FOUND means an empty result.
      result_status->set_code(StatusProto::OK);
    } else {
      // Real error, pass up.
      TransformStatus(page_result_state_or.status(), result_status);
    }
    return result_proto;
  }

  PageResultState page_result_state =
      std::move(page_result_state_or).ValueOrDie();
  query_stats->set_requested_page_size(page_result_state.requested_page_size);

  // Retrieves the document protos.
  auto result_retriever_or =
      ResultRetriever::Create(document_store_.get(), schema_store_.get(),
                              language_segmenter_.get(), normalizer_.get());
  if (!result_retriever_or.ok()) {
    TransformStatus(result_retriever_or.status(), result_status);
    return result_proto;
  }
  std::unique_ptr<ResultRetriever> result_retriever =
      std::move(result_retriever_or).ValueOrDie();

  libtextclassifier3::StatusOr<std::vector<SearchResultProto::ResultProto>>
      results_or = result_retriever->RetrieveResults(page_result_state);
  if (!results_or.ok()) {
    TransformStatus(results_or.status(), result_status);
    return result_proto;
  }
  std::vector<SearchResultProto::ResultProto> results =
      std::move(results_or).ValueOrDie();

  // Assembles the final search result proto
  result_proto.mutable_results()->Reserve(results.size());
  for (SearchResultProto::ResultProto& result : results) {
    result_proto.mutable_results()->Add(std::move(result));
  }

  result_status->set_code(StatusProto::OK);
  if (page_result_state.next_page_token != kInvalidNextPageToken) {
    result_proto.set_next_page_token(page_result_state.next_page_token);
  }

  // The only thing that we're doing is document retrieval. So document
  // retrieval latency and overall latency are the same and can use the same
  // timer.
  query_stats->set_document_retrieval_latency_ms(
      overall_timer->GetElapsedMilliseconds());
  query_stats->set_latency_ms(overall_timer->GetElapsedMilliseconds());
  query_stats->set_num_results_returned_current_page(
      result_proto.results_size());
  int num_left_to_snippet =
      std::max(page_result_state.snippet_context.snippet_spec.num_to_snippet() -
                   page_result_state.num_previously_returned,
               0);
  query_stats->set_num_results_with_snippets(
      std::min(result_proto.results_size(), num_left_to_snippet));
  return result_proto;
}

void IcingSearchEngine::InvalidateNextPageToken(uint64_t next_page_token) {
  absl_ports::shared_lock l(&mutex_);
  if (!initialized_) {
    ICING_LOG(ERROR) << "IcingSearchEngine has not been initialized!";
    return;
  }
  result_state_manager_->InvalidateResultState(next_page_token);
}

libtextclassifier3::Status IcingSearchEngine::OptimizeDocumentStore(
    OptimizeStatsProto* optimize_stats) {
  // Gets the current directory path and an empty tmp directory path for
  // document store optimization.
  const std::string current_document_dir =
      MakeDocumentDirectoryPath(options_.base_dir());
  const std::string temporary_document_dir =
      MakeDocumentTemporaryDirectoryPath(options_.base_dir());
  if (!filesystem_->DeleteDirectoryRecursively(
          temporary_document_dir.c_str()) ||
      !filesystem_->CreateDirectoryRecursively(
          temporary_document_dir.c_str())) {
    return absl_ports::AbortedError(absl_ports::StrCat(
        "Failed to create a tmp directory: ", temporary_document_dir));
  }

  // Copies valid document data to tmp directory
  auto optimize_status = document_store_->OptimizeInto(
      temporary_document_dir, language_segmenter_.get(), optimize_stats);

  // Handles error if any
  if (!optimize_status.ok()) {
    filesystem_->DeleteDirectoryRecursively(temporary_document_dir.c_str());
    return absl_ports::Annotate(
        absl_ports::AbortedError("Failed to optimize document store"),
        optimize_status.error_message());
  }

  // result_state_manager_ depends on document_store_. So we need to reset it at
  // the same time that we reset the document_store_.
  result_state_manager_.reset();
  document_store_.reset();

  // When swapping files, always put the current working directory at the
  // second place because it is renamed at the latter position so we're less
  // vulnerable to errors.
  if (!filesystem_->SwapFiles(temporary_document_dir.c_str(),
                              current_document_dir.c_str())) {
    ICING_LOG(ERROR) << "Failed to swap files";

    // Ensures that current directory is still present.
    if (!filesystem_->CreateDirectoryRecursively(
            current_document_dir.c_str())) {
      // Can't even create the old directory. Mark as uninitialized and return
      // INTERNAL.
      initialized_ = false;
      return absl_ports::InternalError(
          "Failed to create file directory for document store");
    }

    // Tries to rebuild document store if swapping fails, to avoid leaving the
    // system in the broken state for future operations.
    auto create_result_or =
        DocumentStore::Create(filesystem_.get(), current_document_dir,
                              clock_.get(), schema_store_.get());
    // TODO(b/144458732): Implement a more robust version of
    // TC_ASSIGN_OR_RETURN that can support error logging.
    if (!create_result_or.ok()) {
      // Unable to create DocumentStore from the old file. Mark as uninitialized
      // and return INTERNAL.
      initialized_ = false;
      ICING_LOG(ERROR) << "Failed to create document store instance";
      return absl_ports::Annotate(
          absl_ports::InternalError("Failed to create document store instance"),
          create_result_or.status().error_message());
    }
    document_store_ = std::move(create_result_or.ValueOrDie().document_store);
    result_state_manager_ = std::make_unique<ResultStateManager>(
        performance_configuration_.max_num_total_hits, *document_store_);

    // Potential data loss
    // TODO(b/147373249): Find a way to detect true data loss error
    return absl_ports::DataLossError(
        "Failed to optimize document store, there might be data loss");
  }

  // Recreates the doc store instance
  auto create_result_or =
      DocumentStore::Create(filesystem_.get(), current_document_dir,
                            clock_.get(), schema_store_.get());
  if (!create_result_or.ok()) {
    // Unable to create DocumentStore from the new file. Mark as uninitialized
    // and return INTERNAL.
    initialized_ = false;
    return absl_ports::InternalError(
        "Document store has been optimized, but a valid document store "
        "instance can't be created");
  }
  document_store_ = std::move(create_result_or.ValueOrDie().document_store);
  result_state_manager_ = std::make_unique<ResultStateManager>(
      performance_configuration_.max_num_total_hits, *document_store_);

  // Deletes tmp directory
  if (!filesystem_->DeleteDirectoryRecursively(
          temporary_document_dir.c_str())) {
    ICING_LOG(ERROR) << "Document store has been optimized, but it failed to "
                        "delete temporary file directory";
  }
  return libtextclassifier3::Status::OK;
}

IcingSearchEngine::IndexRestorationResult
IcingSearchEngine::RestoreIndexIfNeeded() {
  DocumentId last_stored_document_id =
      document_store_->last_added_document_id();
  DocumentId last_indexed_document_id = index_->last_added_document_id();

  if (last_stored_document_id == last_indexed_document_id) {
    // No need to recover.
    return {libtextclassifier3::Status::OK, false};
  }

  if (last_stored_document_id == kInvalidDocumentId) {
    // Document store is empty but index is not. Reset the index.
    return {index_->Reset(), false};
  }

  // TruncateTo ensures that the index does not hold any data that is not
  // present in the ground truth. If the document store lost some documents,
  // TruncateTo will ensure that the index does not contain any hits from those
  // lost documents. If the index does not contain any hits for documents with
  // document id greater than last_stored_document_id, then TruncateTo will have
  // no effect.
  auto status = index_->TruncateTo(last_stored_document_id);
  if (!status.ok()) {
    return {status, false};
  }
  // Last indexed document id may have changed thanks to TruncateTo.
  last_indexed_document_id = index_->last_added_document_id();
  DocumentId first_document_to_reindex =
      (last_indexed_document_id != kInvalidDocumentId)
          ? index_->last_added_document_id() + 1
          : kMinDocumentId;
  if (first_document_to_reindex > last_stored_document_id) {
    // Nothing to restore. Just return.
    return {libtextclassifier3::Status::OK, false};
  }

  auto index_processor_or = IndexProcessor::Create(
      normalizer_.get(), index_.get(), CreateIndexProcessorOptions(options_),
      clock_.get());
  if (!index_processor_or.ok()) {
    return {index_processor_or.status(), true};
  }
  std::unique_ptr<IndexProcessor> index_processor =
      std::move(index_processor_or).ValueOrDie();

  ICING_VLOG(1) << "Restoring index by replaying documents from document id "
                << first_document_to_reindex << " to document id "
                << last_stored_document_id;
  libtextclassifier3::Status overall_status;
  for (DocumentId document_id = first_document_to_reindex;
       document_id <= last_stored_document_id; ++document_id) {
    libtextclassifier3::StatusOr<DocumentProto> document_or =
        document_store_->Get(document_id);

    if (!document_or.ok()) {
      if (absl_ports::IsInvalidArgument(document_or.status()) ||
          absl_ports::IsNotFound(document_or.status())) {
        // Skips invalid and non-existing documents.
        continue;
      } else {
        // Returns other errors
        return {document_or.status(), true};
      }
    }
    DocumentProto document(std::move(document_or).ValueOrDie());

    libtextclassifier3::StatusOr<TokenizedDocument> tokenized_document_or =
        TokenizedDocument::Create(schema_store_.get(),
                                  language_segmenter_.get(),
                                  std::move(document));
    if (!tokenized_document_or.ok()) {
      return {tokenized_document_or.status(), true};
    }
    TokenizedDocument tokenized_document(
        std::move(tokenized_document_or).ValueOrDie());

    libtextclassifier3::Status status =
        index_processor->IndexDocument(tokenized_document, document_id);
    if (!status.ok()) {
      if (!absl_ports::IsDataLoss(status)) {
        // Real error. Stop recovering and pass it up.
        return {status, true};
      }
      // Just a data loss. Keep trying to add the remaining docs, but report the
      // data loss when we're done.
      overall_status = status;
    }
  }

  return {overall_status, true};
}

libtextclassifier3::StatusOr<bool> IcingSearchEngine::LostPreviousSchema() {
  auto status_or = schema_store_->GetSchema();
  if (status_or.ok()) {
    // Found a schema.
    return false;
  }

  if (!absl_ports::IsNotFound(status_or.status())) {
    // Any other type of error
    return status_or.status();
  }

  // We know: We don't have a schema now.
  //
  // We know: If no documents have been added, then the last_added_document_id
  // will be invalid.
  //
  // So: If documents have been added before and we don't have a schema now,
  // then that means we must have had a schema at some point. Since we wouldn't
  // accept documents without a schema to validate them against.
  return document_store_->last_added_document_id() != kInvalidDocumentId;
}

ResetResultProto IcingSearchEngine::Reset() {
  ICING_VLOG(1) << "Resetting IcingSearchEngine";

  ResetResultProto result_proto;
  StatusProto* result_status = result_proto.mutable_status();

  absl_ports::unique_lock l(&mutex_);

  initialized_ = false;

  // Resets members variables
  schema_store_.reset();
  document_store_.reset();
  language_segmenter_.reset();
  normalizer_.reset();
  index_.reset();

  if (!filesystem_->DeleteDirectoryRecursively(options_.base_dir().c_str())) {
    result_status->set_code(StatusProto::INTERNAL);
    return result_proto;
  }

  if (InternalInitialize().status().code() != StatusProto::OK) {
    // We shouldn't hit the following Initialize errors:
    //   NOT_FOUND: all data was cleared, we aren't expecting anything
    //   DATA_LOSS: all data was cleared, we aren't expecting anything
    //   RESOURCE_EXHAUSTED: just deleted files, shouldn't run out of space
    //
    // We can't tell if Initialize failed and left Icing in an inconsistent
    // state or if it was a temporary I/O error. Group everything under INTERNAL
    // to be safe.
    //
    // TODO(b/147699081): Once Initialize returns the proper ABORTED/INTERNAL
    // status code, we can just propagate it up from here.
    result_status->set_code(StatusProto::INTERNAL);
    return result_proto;
  }

  result_status->set_code(StatusProto::OK);
  return result_proto;
}

}  // namespace lib
}  // namespace icing
