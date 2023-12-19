/*
 * Copyright 2021 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   https://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package com.google.android.enterprise.connectedapps.internal;

import android.os.RemoteException;
import com.google.android.enterprise.connectedapps.ICrossProfileCallback;
import com.google.android.enterprise.connectedapps.ICrossProfileService;
import org.checkerframework.checker.nullness.qual.Nullable;

/**
 * Implementation of {@link ParcelCallSender} used when making synchronous or asynchronous
 * cross-profile calls.
 */
public final class CrossProfileParcelCallSender extends ParcelCallSender {

  private final ICrossProfileService wrappedService;
  private final long crossProfileTypeIdentifier;
  private final int methodIdentifier;
  private final @Nullable ICrossProfileCallback callback;

  public CrossProfileParcelCallSender(
      ICrossProfileService service,
      long crossProfileTypeIdentifier,
      int methodIdentifier,
      @Nullable ICrossProfileCallback callback) {
    if (service == null) {
      throw new NullPointerException("service must not be null");
    }

    wrappedService = service;
    this.crossProfileTypeIdentifier = crossProfileTypeIdentifier;
    this.methodIdentifier = methodIdentifier;
    this.callback = callback;
  }

  @Override
  void prepareCall(long callId, int blockId, int numBytes, byte[] params) throws RemoteException {
    wrappedService.prepareCall(callId, blockId, numBytes, params);
  }

  @Override
  byte[] call(long callId, int blockId, byte[] params) throws RemoteException {
    return wrappedService.call(
        callId, blockId, crossProfileTypeIdentifier, methodIdentifier, params, callback);
  }

  @Override
  byte[] fetchResponse(long callId, int blockId) throws RemoteException {
    return wrappedService.fetchResponse(callId, blockId);
  }
}
