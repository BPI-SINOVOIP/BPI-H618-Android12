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

#include <jni.h>

#include <string>

#include "icing/jni/jni-cache.h"
#include <google/protobuf/message_lite.h>
#include "icing/absl_ports/status_imports.h"
#include "icing/icing-search-engine.h"
#include "icing/proto/document.pb.h"
#include "icing/proto/initialize.pb.h"
#include "icing/proto/optimize.pb.h"
#include "icing/proto/persist.pb.h"
#include "icing/proto/schema.pb.h"
#include "icing/proto/scoring.pb.h"
#include "icing/proto/search.pb.h"
#include "icing/proto/storage.pb.h"
#include "icing/proto/usage.pb.h"
#include "icing/util/status-macros.h"

namespace {

// JNI string constants
// Matches field name of IcingSearchEngine#nativePointer.
const char kNativePointerField[] = "nativePointer";

bool ParseProtoFromJniByteArray(JNIEnv* env, jbyteArray bytes,
                                google::protobuf::MessageLite* protobuf) {
  int bytes_size = env->GetArrayLength(bytes);
  uint8_t* bytes_ptr = static_cast<uint8_t*>(
      env->GetPrimitiveArrayCritical(bytes, /*isCopy=*/nullptr));
  bool parsed = protobuf->ParseFromArray(bytes_ptr, bytes_size);
  env->ReleasePrimitiveArrayCritical(bytes, bytes_ptr, /*mode=*/0);

  return parsed;
}

jbyteArray SerializeProtoToJniByteArray(
    JNIEnv* env, const google::protobuf::MessageLite& protobuf) {
  int size = protobuf.ByteSizeLong();
  jbyteArray ret = env->NewByteArray(size);
  if (ret == nullptr) {
    ICING_LOG(ERROR) << "Failed to allocated bytes for jni protobuf";
    return nullptr;
  }

  uint8_t* ret_buf = static_cast<uint8_t*>(
      env->GetPrimitiveArrayCritical(ret, /*isCopy=*/nullptr));
  protobuf.SerializeWithCachedSizesToArray(ret_buf);
  env->ReleasePrimitiveArrayCritical(ret, ret_buf, 0);
  return ret;
}

icing::lib::IcingSearchEngine* GetIcingSearchEnginePointer(JNIEnv* env,
                                                           jobject object) {
  jclass cls = env->GetObjectClass(object);
  jfieldID field_id = env->GetFieldID(cls, kNativePointerField, "J");
  jlong native_pointer = env->GetLongField(object, field_id);
  return reinterpret_cast<icing::lib::IcingSearchEngine*>(native_pointer);
}

}  // namespace

extern "C" {

jint JNI_OnLoad(JavaVM* vm, void* reserved) {
  JNIEnv* env;
  if (vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) != JNI_OK) {
    ICING_LOG(ERROR) << "ERROR: GetEnv failed";
    return JNI_ERR;
  }

  return JNI_VERSION_1_6;
}

JNIEXPORT jlong JNICALL
Java_com_google_android_icing_IcingSearchEngine_nativeCreate(
    JNIEnv* env, jclass clazz, jbyteArray icing_search_engine_options_bytes) {
  icing::lib::IcingSearchEngineOptions options;
  if (!ParseProtoFromJniByteArray(env, icing_search_engine_options_bytes,
                                  &options)) {
    ICING_LOG(ERROR)
        << "Failed to parse IcingSearchEngineOptions in nativeCreate";
    return 0;
  }

  std::unique_ptr<const icing::lib::JniCache> jni_cache;
#ifdef ICING_REVERSE_JNI_SEGMENTATION
  ICING_ASSIGN_OR_RETURN(jni_cache, icing::lib::JniCache::Create(env), 0);
#endif  // ICING_REVERSE_JNI_SEGMENTATION
  icing::lib::IcingSearchEngine* icing =
      new icing::lib::IcingSearchEngine(options, std::move(jni_cache));
  return reinterpret_cast<jlong>(icing);
}

JNIEXPORT void JNICALL
Java_com_google_android_icing_IcingSearchEngine_nativeDestroy(
    JNIEnv* env, jclass clazz, jobject object) {
  icing::lib::IcingSearchEngine* icing =
      GetIcingSearchEnginePointer(env, object);
  delete icing;
}

JNIEXPORT jbyteArray JNICALL
Java_com_google_android_icing_IcingSearchEngine_nativeInitialize(
    JNIEnv* env, jclass clazz, jobject object) {
  icing::lib::IcingSearchEngine* icing =
      GetIcingSearchEnginePointer(env, object);

  icing::lib::InitializeResultProto initialize_result_proto =
      icing->Initialize();

  return SerializeProtoToJniByteArray(env, initialize_result_proto);
}

JNIEXPORT jbyteArray JNICALL
Java_com_google_android_icing_IcingSearchEngine_nativeSetSchema(
    JNIEnv* env, jclass clazz, jobject object, jbyteArray schema_bytes,
    jboolean ignore_errors_and_delete_documents) {
  icing::lib::IcingSearchEngine* icing =
      GetIcingSearchEnginePointer(env, object);

  icing::lib::SchemaProto schema_proto;
  if (!ParseProtoFromJniByteArray(env, schema_bytes, &schema_proto)) {
    ICING_LOG(ERROR) << "Failed to parse SchemaProto in nativeSetSchema";
    return nullptr;
  }

  icing::lib::SetSchemaResultProto set_schema_result_proto = icing->SetSchema(
      std::move(schema_proto), ignore_errors_and_delete_documents);

  return SerializeProtoToJniByteArray(env, set_schema_result_proto);
}

JNIEXPORT jbyteArray JNICALL
Java_com_google_android_icing_IcingSearchEngine_nativeGetSchema(
    JNIEnv* env, jclass clazz, jobject object) {
  icing::lib::IcingSearchEngine* icing =
      GetIcingSearchEnginePointer(env, object);

  icing::lib::GetSchemaResultProto get_schema_result_proto = icing->GetSchema();

  return SerializeProtoToJniByteArray(env, get_schema_result_proto);
}

JNIEXPORT jbyteArray JNICALL
Java_com_google_android_icing_IcingSearchEngine_nativeGetSchemaType(
    JNIEnv* env, jclass clazz, jobject object, jstring schema_type) {
  icing::lib::IcingSearchEngine* icing =
      GetIcingSearchEnginePointer(env, object);

  const char* native_schema_type =
      env->GetStringUTFChars(schema_type, /*isCopy=*/nullptr);
  icing::lib::GetSchemaTypeResultProto get_schema_type_result_proto =
      icing->GetSchemaType(native_schema_type);

  return SerializeProtoToJniByteArray(env, get_schema_type_result_proto);
}

JNIEXPORT jbyteArray JNICALL
Java_com_google_android_icing_IcingSearchEngine_nativePut(
    JNIEnv* env, jclass clazz, jobject object, jbyteArray document_bytes) {
  icing::lib::IcingSearchEngine* icing =
      GetIcingSearchEnginePointer(env, object);

  icing::lib::DocumentProto document_proto;
  if (!ParseProtoFromJniByteArray(env, document_bytes, &document_proto)) {
    ICING_LOG(ERROR) << "Failed to parse DocumentProto in nativePut";
    return nullptr;
  }

  icing::lib::PutResultProto put_result_proto =
      icing->Put(std::move(document_proto));

  return SerializeProtoToJniByteArray(env, put_result_proto);
}

JNIEXPORT jbyteArray JNICALL
Java_com_google_android_icing_IcingSearchEngine_nativeGet(
    JNIEnv* env, jclass clazz, jobject object, jstring name_space, jstring uri,
    jbyteArray result_spec_bytes) {
  icing::lib::IcingSearchEngine* icing =
      GetIcingSearchEnginePointer(env, object);

  const char* native_name_space =
      env->GetStringUTFChars(name_space, /*isCopy=*/nullptr);
  const char* native_uri = env->GetStringUTFChars(uri, /*isCopy=*/nullptr);
  icing::lib::GetResultSpecProto get_result_spec;
  if (!ParseProtoFromJniByteArray(env, result_spec_bytes, &get_result_spec)) {
    ICING_LOG(ERROR) << "Failed to parse GetResultSpecProto in nativeGet";
    return nullptr;
  }
  icing::lib::GetResultProto get_result_proto =
      icing->Get(native_name_space, native_uri, get_result_spec);

  return SerializeProtoToJniByteArray(env, get_result_proto);
}

JNIEXPORT jbyteArray JNICALL
Java_com_google_android_icing_IcingSearchEngine_nativeReportUsage(
    JNIEnv* env, jclass clazz, jobject object, jbyteArray usage_report_bytes) {
  icing::lib::IcingSearchEngine* icing =
      GetIcingSearchEnginePointer(env, object);

  icing::lib::UsageReport usage_report;
  if (!ParseProtoFromJniByteArray(env, usage_report_bytes, &usage_report)) {
    ICING_LOG(ERROR) << "Failed to parse UsageReport in nativeReportUsage";
    return nullptr;
  }

  icing::lib::ReportUsageResultProto report_usage_result_proto =
      icing->ReportUsage(usage_report);

  return SerializeProtoToJniByteArray(env, report_usage_result_proto);
}

JNIEXPORT jbyteArray JNICALL
Java_com_google_android_icing_IcingSearchEngine_nativeGetAllNamespaces(
    JNIEnv* env, jclass clazz, jobject object) {
  icing::lib::IcingSearchEngine* icing =
      GetIcingSearchEnginePointer(env, object);

  icing::lib::GetAllNamespacesResultProto get_all_namespaces_result_proto =
      icing->GetAllNamespaces();

  return SerializeProtoToJniByteArray(env, get_all_namespaces_result_proto);
}

JNIEXPORT jbyteArray JNICALL
Java_com_google_android_icing_IcingSearchEngine_nativeGetNextPage(
    JNIEnv* env, jclass clazz, jobject object, jlong next_page_token) {
  icing::lib::IcingSearchEngine* icing =
      GetIcingSearchEnginePointer(env, object);

  icing::lib::SearchResultProto next_page_result_proto =
      icing->GetNextPage(next_page_token);

  return SerializeProtoToJniByteArray(env, next_page_result_proto);
}

JNIEXPORT void JNICALL
Java_com_google_android_icing_IcingSearchEngine_nativeInvalidateNextPageToken(
    JNIEnv* env, jclass clazz, jobject object, jlong next_page_token) {
  icing::lib::IcingSearchEngine* icing =
      GetIcingSearchEnginePointer(env, object);

  icing->InvalidateNextPageToken(next_page_token);

  return;
}

JNIEXPORT jbyteArray JNICALL
Java_com_google_android_icing_IcingSearchEngine_nativeSearch(
    JNIEnv* env, jclass clazz, jobject object, jbyteArray search_spec_bytes,
    jbyteArray scoring_spec_bytes, jbyteArray result_spec_bytes) {
  icing::lib::IcingSearchEngine* icing =
      GetIcingSearchEnginePointer(env, object);

  icing::lib::SearchSpecProto search_spec_proto;
  if (!ParseProtoFromJniByteArray(env, search_spec_bytes, &search_spec_proto)) {
    ICING_LOG(ERROR) << "Failed to parse SearchSpecProto in nativeSearch";
    return nullptr;
  }

  icing::lib::ScoringSpecProto scoring_spec_proto;
  if (!ParseProtoFromJniByteArray(env, scoring_spec_bytes,
                                  &scoring_spec_proto)) {
    ICING_LOG(ERROR) << "Failed to parse ScoringSpecProto in nativeSearch";
    return nullptr;
  }

  icing::lib::ResultSpecProto result_spec_proto;
  if (!ParseProtoFromJniByteArray(env, result_spec_bytes, &result_spec_proto)) {
    ICING_LOG(ERROR) << "Failed to parse ResultSpecProto in nativeSearch";
    return nullptr;
  }

  icing::lib::SearchResultProto search_result_proto =
      icing->Search(search_spec_proto, scoring_spec_proto, result_spec_proto);

  return SerializeProtoToJniByteArray(env, search_result_proto);
}

JNIEXPORT jbyteArray JNICALL
Java_com_google_android_icing_IcingSearchEngine_nativeDelete(
    JNIEnv* env, jclass clazz, jobject object, jstring name_space,
    jstring uri) {
  icing::lib::IcingSearchEngine* icing =
      GetIcingSearchEnginePointer(env, object);

  const char* native_name_space =
      env->GetStringUTFChars(name_space, /*isCopy=*/nullptr);
  const char* native_uri = env->GetStringUTFChars(uri, /*isCopy=*/nullptr);
  icing::lib::DeleteResultProto delete_result_proto =
      icing->Delete(native_name_space, native_uri);

  return SerializeProtoToJniByteArray(env, delete_result_proto);
}

JNIEXPORT jbyteArray JNICALL
Java_com_google_android_icing_IcingSearchEngine_nativeDeleteByNamespace(
    JNIEnv* env, jclass clazz, jobject object, jstring name_space) {
  icing::lib::IcingSearchEngine* icing =
      GetIcingSearchEnginePointer(env, object);

  const char* native_name_space =
      env->GetStringUTFChars(name_space, /*isCopy=*/nullptr);
  icing::lib::DeleteByNamespaceResultProto delete_by_namespace_result_proto =
      icing->DeleteByNamespace(native_name_space);

  return SerializeProtoToJniByteArray(env, delete_by_namespace_result_proto);
}

JNIEXPORT jbyteArray JNICALL
Java_com_google_android_icing_IcingSearchEngine_nativeDeleteBySchemaType(
    JNIEnv* env, jclass clazz, jobject object, jstring schema_type) {
  icing::lib::IcingSearchEngine* icing =
      GetIcingSearchEnginePointer(env, object);

  const char* native_schema_type =
      env->GetStringUTFChars(schema_type, /*isCopy=*/nullptr);
  icing::lib::DeleteBySchemaTypeResultProto delete_by_schema_type_result_proto =
      icing->DeleteBySchemaType(native_schema_type);

  return SerializeProtoToJniByteArray(env, delete_by_schema_type_result_proto);
}

JNIEXPORT jbyteArray JNICALL
Java_com_google_android_icing_IcingSearchEngine_nativeDeleteByQuery(
    JNIEnv* env, jclass clazz, jobject object, jbyteArray search_spec_bytes) {
  icing::lib::IcingSearchEngine* icing =
      GetIcingSearchEnginePointer(env, object);

  icing::lib::SearchSpecProto search_spec_proto;
  if (!ParseProtoFromJniByteArray(env, search_spec_bytes, &search_spec_proto)) {
    ICING_LOG(ERROR) << "Failed to parse SearchSpecProto in nativeSearch";
    return nullptr;
  }
  icing::lib::DeleteByQueryResultProto delete_result_proto =
      icing->DeleteByQuery(search_spec_proto);

  return SerializeProtoToJniByteArray(env, delete_result_proto);
}

JNIEXPORT jbyteArray JNICALL
Java_com_google_android_icing_IcingSearchEngine_nativePersistToDisk(
    JNIEnv* env, jclass clazz, jobject object, jint persist_type_code) {
  icing::lib::IcingSearchEngine* icing =
      GetIcingSearchEnginePointer(env, object);

  if (!icing::lib::PersistType::Code_IsValid(persist_type_code)) {
    ICING_LOG(ERROR) << persist_type_code
                     << " is an invalid value for PersistType::Code";
    return nullptr;
  }
  icing::lib::PersistType::Code persist_type_code_enum =
      static_cast<icing::lib::PersistType::Code>(persist_type_code);
  icing::lib::PersistToDiskResultProto persist_to_disk_result_proto =
      icing->PersistToDisk(persist_type_code_enum);

  return SerializeProtoToJniByteArray(env, persist_to_disk_result_proto);
}

JNIEXPORT jbyteArray JNICALL
Java_com_google_android_icing_IcingSearchEngine_nativeOptimize(
    JNIEnv* env, jclass clazz, jobject object) {
  icing::lib::IcingSearchEngine* icing =
      GetIcingSearchEnginePointer(env, object);

  icing::lib::OptimizeResultProto optimize_result_proto = icing->Optimize();

  return SerializeProtoToJniByteArray(env, optimize_result_proto);
}

JNIEXPORT jbyteArray JNICALL
Java_com_google_android_icing_IcingSearchEngine_nativeGetOptimizeInfo(
    JNIEnv* env, jclass clazz, jobject object) {
  icing::lib::IcingSearchEngine* icing =
      GetIcingSearchEnginePointer(env, object);

  icing::lib::GetOptimizeInfoResultProto get_optimize_info_result_proto =
      icing->GetOptimizeInfo();

  return SerializeProtoToJniByteArray(env, get_optimize_info_result_proto);
}

JNIEXPORT jbyteArray JNICALL
Java_com_google_android_icing_IcingSearchEngine_nativeGetStorageInfo(
    JNIEnv* env, jclass clazz, jobject object) {
  icing::lib::IcingSearchEngine* icing =
      GetIcingSearchEnginePointer(env, object);

  icing::lib::StorageInfoResultProto storage_info_result_proto =
      icing->GetStorageInfo();

  return SerializeProtoToJniByteArray(env, storage_info_result_proto);
}

JNIEXPORT jbyteArray JNICALL
Java_com_google_android_icing_IcingSearchEngine_nativeReset(
    JNIEnv* env, jclass clazz, jobject object) {
  icing::lib::IcingSearchEngine* icing =
      GetIcingSearchEnginePointer(env, object);

  icing::lib::ResetResultProto reset_result_proto = icing->Reset();

  return SerializeProtoToJniByteArray(env, reset_result_proto);
}

}  // extern "C"
