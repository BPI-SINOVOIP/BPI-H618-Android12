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

package com.google.android.icing;

import android.util.Log;
import androidx.annotation.NonNull;
import com.google.android.icing.proto.DeleteByNamespaceResultProto;
import com.google.android.icing.proto.DeleteByQueryResultProto;
import com.google.android.icing.proto.DeleteBySchemaTypeResultProto;
import com.google.android.icing.proto.DeleteResultProto;
import com.google.android.icing.proto.DocumentProto;
import com.google.android.icing.proto.GetAllNamespacesResultProto;
import com.google.android.icing.proto.GetOptimizeInfoResultProto;
import com.google.android.icing.proto.GetResultProto;
import com.google.android.icing.proto.GetResultSpecProto;
import com.google.android.icing.proto.GetSchemaResultProto;
import com.google.android.icing.proto.GetSchemaTypeResultProto;
import com.google.android.icing.proto.IcingSearchEngineOptions;
import com.google.android.icing.proto.InitializeResultProto;
import com.google.android.icing.proto.OptimizeResultProto;
import com.google.android.icing.proto.PersistToDiskResultProto;
import com.google.android.icing.proto.PersistType;
import com.google.android.icing.proto.PutResultProto;
import com.google.android.icing.proto.ReportUsageResultProto;
import com.google.android.icing.proto.ResetResultProto;
import com.google.android.icing.proto.ResultSpecProto;
import com.google.android.icing.proto.SchemaProto;
import com.google.android.icing.proto.ScoringSpecProto;
import com.google.android.icing.proto.SearchResultProto;
import com.google.android.icing.proto.SearchSpecProto;
import com.google.android.icing.proto.SetSchemaResultProto;
import com.google.android.icing.proto.StatusProto;
import com.google.android.icing.proto.StorageInfoResultProto;
import com.google.android.icing.proto.UsageReport;
import com.google.protobuf.ExtensionRegistryLite;
import com.google.protobuf.InvalidProtocolBufferException;
import java.io.Closeable;

/**
 * Java wrapper to access native APIs in external/icing/icing/icing-search-engine.h
 *
 * <p>If this instance has been closed, the instance is no longer usable.
 *
 * <p>Keep this class to be non-Final so that it can be mocked in AppSearch.
 *
 * <p>NOTE: This class is NOT thread-safe.
 */
public class IcingSearchEngine implements Closeable {

  private static final String TAG = "IcingSearchEngine";
  private static final ExtensionRegistryLite EXTENSION_REGISTRY_LITE =
      ExtensionRegistryLite.getEmptyRegistry();

  private long nativePointer;

  private boolean closed = false;

  static {
    // NOTE: This can fail with an UnsatisfiedLinkError
    System.loadLibrary("icing");
  }

  /** @throws IllegalStateException if IcingSearchEngine fails to be created */
  public IcingSearchEngine(@NonNull IcingSearchEngineOptions options) {
    nativePointer = nativeCreate(options.toByteArray());
    if (nativePointer == 0) {
      Log.e(TAG, "Failed to create IcingSearchEngine.");
      throw new IllegalStateException("Failed to create IcingSearchEngine.");
    }
  }

  private void throwIfClosed() {
    if (closed) {
      throw new IllegalStateException("Trying to use a closed IcingSearchEngine instance.");
    }
  }

  @Override
  public void close() {
    if (closed) {
      return;
    }

    if (nativePointer != 0) {
      nativeDestroy(this);
    }
    nativePointer = 0;
    closed = true;
  }

  @Override
  protected void finalize() throws Throwable {
    close();
    super.finalize();
  }

  @NonNull
  public InitializeResultProto initialize() {
    throwIfClosed();

    byte[] initializeResultBytes = nativeInitialize(this);
    if (initializeResultBytes == null) {
      Log.e(TAG, "Received null InitializeResult from native.");
      return InitializeResultProto.newBuilder()
          .setStatus(StatusProto.newBuilder().setCode(StatusProto.Code.INTERNAL))
          .build();
    }

    try {
      return InitializeResultProto.parseFrom(initializeResultBytes, EXTENSION_REGISTRY_LITE);
    } catch (InvalidProtocolBufferException e) {
      Log.e(TAG, "Error parsing InitializeResultProto.", e);
      return InitializeResultProto.newBuilder()
          .setStatus(StatusProto.newBuilder().setCode(StatusProto.Code.INTERNAL))
          .build();
    }
  }

  @NonNull
  public SetSchemaResultProto setSchema(@NonNull SchemaProto schema) {
    return setSchema(schema, /*ignoreErrorsAndDeleteDocuments=*/ false);
  }

  @NonNull
  public SetSchemaResultProto setSchema(
      @NonNull SchemaProto schema, boolean ignoreErrorsAndDeleteDocuments) {
    throwIfClosed();

    byte[] setSchemaResultBytes =
        nativeSetSchema(this, schema.toByteArray(), ignoreErrorsAndDeleteDocuments);
    if (setSchemaResultBytes == null) {
      Log.e(TAG, "Received null SetSchemaResultProto from native.");
      return SetSchemaResultProto.newBuilder()
          .setStatus(StatusProto.newBuilder().setCode(StatusProto.Code.INTERNAL))
          .build();
    }

    try {
      return SetSchemaResultProto.parseFrom(setSchemaResultBytes, EXTENSION_REGISTRY_LITE);
    } catch (InvalidProtocolBufferException e) {
      Log.e(TAG, "Error parsing SetSchemaResultProto.", e);
      return SetSchemaResultProto.newBuilder()
          .setStatus(StatusProto.newBuilder().setCode(StatusProto.Code.INTERNAL))
          .build();
    }
  }

  @NonNull
  public GetSchemaResultProto getSchema() {
    throwIfClosed();

    byte[] getSchemaResultBytes = nativeGetSchema(this);
    if (getSchemaResultBytes == null) {
      Log.e(TAG, "Received null GetSchemaResultProto from native.");
      return GetSchemaResultProto.newBuilder()
          .setStatus(StatusProto.newBuilder().setCode(StatusProto.Code.INTERNAL))
          .build();
    }

    try {
      return GetSchemaResultProto.parseFrom(getSchemaResultBytes, EXTENSION_REGISTRY_LITE);
    } catch (InvalidProtocolBufferException e) {
      Log.e(TAG, "Error parsing GetSchemaResultProto.", e);
      return GetSchemaResultProto.newBuilder()
          .setStatus(StatusProto.newBuilder().setCode(StatusProto.Code.INTERNAL))
          .build();
    }
  }

  @NonNull
  public GetSchemaTypeResultProto getSchemaType(@NonNull String schemaType) {
    throwIfClosed();

    byte[] getSchemaTypeResultBytes = nativeGetSchemaType(this, schemaType);
    if (getSchemaTypeResultBytes == null) {
      Log.e(TAG, "Received null GetSchemaTypeResultProto from native.");
      return GetSchemaTypeResultProto.newBuilder()
          .setStatus(StatusProto.newBuilder().setCode(StatusProto.Code.INTERNAL))
          .build();
    }

    try {
      return GetSchemaTypeResultProto.parseFrom(getSchemaTypeResultBytes, EXTENSION_REGISTRY_LITE);
    } catch (InvalidProtocolBufferException e) {
      Log.e(TAG, "Error parsing GetSchemaTypeResultProto.", e);
      return GetSchemaTypeResultProto.newBuilder()
          .setStatus(StatusProto.newBuilder().setCode(StatusProto.Code.INTERNAL))
          .build();
    }
  }

  @NonNull
  public PutResultProto put(@NonNull DocumentProto document) {
    throwIfClosed();

    byte[] putResultBytes = nativePut(this, document.toByteArray());
    if (putResultBytes == null) {
      Log.e(TAG, "Received null PutResultProto from native.");
      return PutResultProto.newBuilder()
          .setStatus(StatusProto.newBuilder().setCode(StatusProto.Code.INTERNAL))
          .build();
    }

    try {
      return PutResultProto.parseFrom(putResultBytes, EXTENSION_REGISTRY_LITE);
    } catch (InvalidProtocolBufferException e) {
      Log.e(TAG, "Error parsing PutResultProto.", e);
      return PutResultProto.newBuilder()
          .setStatus(StatusProto.newBuilder().setCode(StatusProto.Code.INTERNAL))
          .build();
    }
  }

  @NonNull
  public GetResultProto get(
      @NonNull String namespace, @NonNull String uri, @NonNull GetResultSpecProto getResultSpec) {
    throwIfClosed();

    byte[] getResultBytes = nativeGet(this, namespace, uri, getResultSpec.toByteArray());
    if (getResultBytes == null) {
      Log.e(TAG, "Received null GetResultProto from native.");
      return GetResultProto.newBuilder()
          .setStatus(StatusProto.newBuilder().setCode(StatusProto.Code.INTERNAL))
          .build();
    }

    try {
      return GetResultProto.parseFrom(getResultBytes, EXTENSION_REGISTRY_LITE);
    } catch (InvalidProtocolBufferException e) {
      Log.e(TAG, "Error parsing GetResultProto.", e);
      return GetResultProto.newBuilder()
          .setStatus(StatusProto.newBuilder().setCode(StatusProto.Code.INTERNAL))
          .build();
    }
  }

  @NonNull
  public ReportUsageResultProto reportUsage(@NonNull UsageReport usageReport) {
    throwIfClosed();

    byte[] reportUsageResultBytes = nativeReportUsage(this, usageReport.toByteArray());
    if (reportUsageResultBytes == null) {
      Log.e(TAG, "Received null ReportUsageResultProto from native.");
      return ReportUsageResultProto.newBuilder()
          .setStatus(StatusProto.newBuilder().setCode(StatusProto.Code.INTERNAL))
          .build();
    }

    try {
      return ReportUsageResultProto.parseFrom(reportUsageResultBytes, EXTENSION_REGISTRY_LITE);
    } catch (InvalidProtocolBufferException e) {
      Log.e(TAG, "Error parsing ReportUsageResultProto.", e);
      return ReportUsageResultProto.newBuilder()
          .setStatus(StatusProto.newBuilder().setCode(StatusProto.Code.INTERNAL))
          .build();
    }
  }

  @NonNull
  public GetAllNamespacesResultProto getAllNamespaces() {
    throwIfClosed();

    byte[] getAllNamespacesResultBytes = nativeGetAllNamespaces(this);
    if (getAllNamespacesResultBytes == null) {
      Log.e(TAG, "Received null GetAllNamespacesResultProto from native.");
      return GetAllNamespacesResultProto.newBuilder()
          .setStatus(StatusProto.newBuilder().setCode(StatusProto.Code.INTERNAL))
          .build();
    }

    try {
      return GetAllNamespacesResultProto.parseFrom(
          getAllNamespacesResultBytes, EXTENSION_REGISTRY_LITE);
    } catch (InvalidProtocolBufferException e) {
      Log.e(TAG, "Error parsing GetAllNamespacesResultProto.", e);
      return GetAllNamespacesResultProto.newBuilder()
          .setStatus(StatusProto.newBuilder().setCode(StatusProto.Code.INTERNAL))
          .build();
    }
  }

  @NonNull
  public SearchResultProto search(
      @NonNull SearchSpecProto searchSpec,
      @NonNull ScoringSpecProto scoringSpec,
      @NonNull ResultSpecProto resultSpec) {
    throwIfClosed();

    byte[] searchResultBytes =
        nativeSearch(
            this, searchSpec.toByteArray(), scoringSpec.toByteArray(), resultSpec.toByteArray());
    if (searchResultBytes == null) {
      Log.e(TAG, "Received null SearchResultProto from native.");
      return SearchResultProto.newBuilder()
          .setStatus(StatusProto.newBuilder().setCode(StatusProto.Code.INTERNAL))
          .build();
    }

    try {
      return SearchResultProto.parseFrom(searchResultBytes, EXTENSION_REGISTRY_LITE);
    } catch (InvalidProtocolBufferException e) {
      Log.e(TAG, "Error parsing SearchResultProto.", e);
      return SearchResultProto.newBuilder()
          .setStatus(StatusProto.newBuilder().setCode(StatusProto.Code.INTERNAL))
          .build();
    }
  }

  @NonNull
  public SearchResultProto getNextPage(long nextPageToken) {
    throwIfClosed();

    byte[] searchResultBytes = nativeGetNextPage(this, nextPageToken);
    if (searchResultBytes == null) {
      Log.e(TAG, "Received null SearchResultProto from native.");
      return SearchResultProto.newBuilder()
          .setStatus(StatusProto.newBuilder().setCode(StatusProto.Code.INTERNAL))
          .build();
    }

    try {
      return SearchResultProto.parseFrom(searchResultBytes, EXTENSION_REGISTRY_LITE);
    } catch (InvalidProtocolBufferException e) {
      Log.e(TAG, "Error parsing SearchResultProto.", e);
      return SearchResultProto.newBuilder()
          .setStatus(StatusProto.newBuilder().setCode(StatusProto.Code.INTERNAL))
          .build();
    }
  }

  @NonNull
  public void invalidateNextPageToken(long nextPageToken) {
    throwIfClosed();

    nativeInvalidateNextPageToken(this, nextPageToken);
  }

  @NonNull
  public DeleteResultProto delete(@NonNull String namespace, @NonNull String uri) {
    throwIfClosed();

    byte[] deleteResultBytes = nativeDelete(this, namespace, uri);
    if (deleteResultBytes == null) {
      Log.e(TAG, "Received null DeleteResultProto from native.");
      return DeleteResultProto.newBuilder()
          .setStatus(StatusProto.newBuilder().setCode(StatusProto.Code.INTERNAL))
          .build();
    }

    try {
      return DeleteResultProto.parseFrom(deleteResultBytes, EXTENSION_REGISTRY_LITE);
    } catch (InvalidProtocolBufferException e) {
      Log.e(TAG, "Error parsing DeleteResultProto.", e);
      return DeleteResultProto.newBuilder()
          .setStatus(StatusProto.newBuilder().setCode(StatusProto.Code.INTERNAL))
          .build();
    }
  }

  @NonNull
  public DeleteByNamespaceResultProto deleteByNamespace(@NonNull String namespace) {
    throwIfClosed();

    byte[] deleteByNamespaceResultBytes = nativeDeleteByNamespace(this, namespace);
    if (deleteByNamespaceResultBytes == null) {
      Log.e(TAG, "Received null DeleteByNamespaceResultProto from native.");
      return DeleteByNamespaceResultProto.newBuilder()
          .setStatus(StatusProto.newBuilder().setCode(StatusProto.Code.INTERNAL))
          .build();
    }

    try {
      return DeleteByNamespaceResultProto.parseFrom(
          deleteByNamespaceResultBytes, EXTENSION_REGISTRY_LITE);
    } catch (InvalidProtocolBufferException e) {
      Log.e(TAG, "Error parsing DeleteByNamespaceResultProto.", e);
      return DeleteByNamespaceResultProto.newBuilder()
          .setStatus(StatusProto.newBuilder().setCode(StatusProto.Code.INTERNAL))
          .build();
    }
  }

  @NonNull
  public DeleteBySchemaTypeResultProto deleteBySchemaType(@NonNull String schemaType) {
    throwIfClosed();

    byte[] deleteBySchemaTypeResultBytes = nativeDeleteBySchemaType(this, schemaType);
    if (deleteBySchemaTypeResultBytes == null) {
      Log.e(TAG, "Received null DeleteBySchemaTypeResultProto from native.");
      return DeleteBySchemaTypeResultProto.newBuilder()
          .setStatus(StatusProto.newBuilder().setCode(StatusProto.Code.INTERNAL))
          .build();
    }

    try {
      return DeleteBySchemaTypeResultProto.parseFrom(
          deleteBySchemaTypeResultBytes, EXTENSION_REGISTRY_LITE);
    } catch (InvalidProtocolBufferException e) {
      Log.e(TAG, "Error parsing DeleteBySchemaTypeResultProto.", e);
      return DeleteBySchemaTypeResultProto.newBuilder()
          .setStatus(StatusProto.newBuilder().setCode(StatusProto.Code.INTERNAL))
          .build();
    }
  }

  @NonNull
  public DeleteByQueryResultProto deleteByQuery(@NonNull SearchSpecProto searchSpec) {
    throwIfClosed();

    byte[] deleteResultBytes = nativeDeleteByQuery(this, searchSpec.toByteArray());
    if (deleteResultBytes == null) {
      Log.e(TAG, "Received null DeleteResultProto from native.");
      return DeleteByQueryResultProto.newBuilder()
          .setStatus(StatusProto.newBuilder().setCode(StatusProto.Code.INTERNAL))
          .build();
    }

    try {
      return DeleteByQueryResultProto.parseFrom(deleteResultBytes, EXTENSION_REGISTRY_LITE);
    } catch (InvalidProtocolBufferException e) {
      Log.e(TAG, "Error parsing DeleteResultProto.", e);
      return DeleteByQueryResultProto.newBuilder()
          .setStatus(StatusProto.newBuilder().setCode(StatusProto.Code.INTERNAL))
          .build();
    }
  }

  @NonNull
  public PersistToDiskResultProto persistToDisk(@NonNull PersistType.Code persistTypeCode) {
    throwIfClosed();

    byte[] persistToDiskResultBytes = nativePersistToDisk(this, persistTypeCode.getNumber());
    if (persistToDiskResultBytes == null) {
      Log.e(TAG, "Received null PersistToDiskResultProto from native.");
      return PersistToDiskResultProto.newBuilder()
          .setStatus(StatusProto.newBuilder().setCode(StatusProto.Code.INTERNAL))
          .build();
    }

    try {
      return PersistToDiskResultProto.parseFrom(persistToDiskResultBytes, EXTENSION_REGISTRY_LITE);
    } catch (InvalidProtocolBufferException e) {
      Log.e(TAG, "Error parsing PersistToDiskResultProto.", e);
      return PersistToDiskResultProto.newBuilder()
          .setStatus(StatusProto.newBuilder().setCode(StatusProto.Code.INTERNAL))
          .build();
    }
  }

  @NonNull
  public OptimizeResultProto optimize() {
    throwIfClosed();

    byte[] optimizeResultBytes = nativeOptimize(this);
    if (optimizeResultBytes == null) {
      Log.e(TAG, "Received null OptimizeResultProto from native.");
      return OptimizeResultProto.newBuilder()
          .setStatus(StatusProto.newBuilder().setCode(StatusProto.Code.INTERNAL))
          .build();
    }

    try {
      return OptimizeResultProto.parseFrom(optimizeResultBytes, EXTENSION_REGISTRY_LITE);
    } catch (InvalidProtocolBufferException e) {
      Log.e(TAG, "Error parsing OptimizeResultProto.", e);
      return OptimizeResultProto.newBuilder()
          .setStatus(StatusProto.newBuilder().setCode(StatusProto.Code.INTERNAL))
          .build();
    }
  }

  @NonNull
  public GetOptimizeInfoResultProto getOptimizeInfo() {
    throwIfClosed();

    byte[] getOptimizeInfoResultBytes = nativeGetOptimizeInfo(this);
    if (getOptimizeInfoResultBytes == null) {
      Log.e(TAG, "Received null GetOptimizeInfoResultProto from native.");
      return GetOptimizeInfoResultProto.newBuilder()
          .setStatus(StatusProto.newBuilder().setCode(StatusProto.Code.INTERNAL))
          .build();
    }

    try {
      return GetOptimizeInfoResultProto.parseFrom(
          getOptimizeInfoResultBytes, EXTENSION_REGISTRY_LITE);
    } catch (InvalidProtocolBufferException e) {
      Log.e(TAG, "Error parsing GetOptimizeInfoResultProto.", e);
      return GetOptimizeInfoResultProto.newBuilder()
          .setStatus(StatusProto.newBuilder().setCode(StatusProto.Code.INTERNAL))
          .build();
    }
  }

  @NonNull
  public StorageInfoResultProto getStorageInfo() {
    throwIfClosed();

    byte[] storageInfoResultProtoBytes = nativeGetStorageInfo(this);
    if (storageInfoResultProtoBytes == null) {
      Log.e(TAG, "Received null StorageInfoResultProto from native.");
      return StorageInfoResultProto.newBuilder()
          .setStatus(StatusProto.newBuilder().setCode(StatusProto.Code.INTERNAL))
          .build();
    }

    try {
      return StorageInfoResultProto.parseFrom(
          storageInfoResultProtoBytes, EXTENSION_REGISTRY_LITE);
    } catch (InvalidProtocolBufferException e) {
      Log.e(TAG, "Error parsing GetOptimizeInfoResultProto.", e);
      return StorageInfoResultProto.newBuilder()
          .setStatus(StatusProto.newBuilder().setCode(StatusProto.Code.INTERNAL))
          .build();
    }
  }

  @NonNull
  public ResetResultProto reset() {
    throwIfClosed();

    byte[] resetResultBytes = nativeReset(this);
    if (resetResultBytes == null) {
      Log.e(TAG, "Received null ResetResultProto from native.");
      return ResetResultProto.newBuilder()
          .setStatus(StatusProto.newBuilder().setCode(StatusProto.Code.INTERNAL))
          .build();
    }

    try {
      return ResetResultProto.parseFrom(resetResultBytes, EXTENSION_REGISTRY_LITE);
    } catch (InvalidProtocolBufferException e) {
      Log.e(TAG, "Error parsing ResetResultProto.", e);
      return ResetResultProto.newBuilder()
          .setStatus(StatusProto.newBuilder().setCode(StatusProto.Code.INTERNAL))
          .build();
    }
  }

  private static native long nativeCreate(byte[] icingSearchEngineOptionsBytes);

  private static native void nativeDestroy(IcingSearchEngine instance);

  private static native byte[] nativeInitialize(IcingSearchEngine instance);

  private static native byte[] nativeSetSchema(
      IcingSearchEngine instance, byte[] schemaBytes, boolean ignoreErrorsAndDeleteDocuments);

  private static native byte[] nativeGetSchema(IcingSearchEngine instance);

  private static native byte[] nativeGetSchemaType(IcingSearchEngine instance, String schemaType);

  private static native byte[] nativePut(IcingSearchEngine instance, byte[] documentBytes);

  private static native byte[] nativeGet(
      IcingSearchEngine instance, String namespace, String uri, byte[] getResultSpecBytes);

  private static native byte[] nativeReportUsage(
      IcingSearchEngine instance, byte[] usageReportBytes);

  private static native byte[] nativeGetAllNamespaces(IcingSearchEngine instance);

  private static native byte[] nativeSearch(
      IcingSearchEngine instance,
      byte[] searchSpecBytes,
      byte[] scoringSpecBytes,
      byte[] resultSpecBytes);

  private static native byte[] nativeGetNextPage(IcingSearchEngine instance, long nextPageToken);

  private static native void nativeInvalidateNextPageToken(
      IcingSearchEngine instance, long nextPageToken);

  private static native byte[] nativeDelete(
      IcingSearchEngine instance, String namespace, String uri);

  private static native byte[] nativeDeleteByNamespace(
      IcingSearchEngine instance, String namespace);

  private static native byte[] nativeDeleteBySchemaType(
      IcingSearchEngine instance, String schemaType);

  private static native byte[] nativeDeleteByQuery(
      IcingSearchEngine instance, byte[] searchSpecBytes);

  private static native byte[] nativePersistToDisk(IcingSearchEngine instance, int persistType);

  private static native byte[] nativeOptimize(IcingSearchEngine instance);

  private static native byte[] nativeGetOptimizeInfo(IcingSearchEngine instance);

  private static native byte[] nativeGetStorageInfo(IcingSearchEngine instance);

  private static native byte[] nativeReset(IcingSearchEngine instance);
}
