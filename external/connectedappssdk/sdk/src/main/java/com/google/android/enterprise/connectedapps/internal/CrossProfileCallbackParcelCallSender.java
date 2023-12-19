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

/** Implementation of {@link ParcelCallSender} used when passing a callback return value. */
public class CrossProfileCallbackParcelCallSender extends ParcelCallSender {

  private final ICrossProfileCallback callback;
  private final int methodIdentifier;

  public CrossProfileCallbackParcelCallSender(
      ICrossProfileCallback callback, int methodIdentifier) {
    if (callback == null) {
      throw new NullPointerException("callback must not be null");
    }
    this.callback = callback;
    this.methodIdentifier = methodIdentifier;
  }

  /** Relays to {@link ICrossProfileCallback#prepareResult(long, int, int, byte[])} */
  @Override
  void prepareCall(long callId, int blockId, int totalBytes, byte[] bytes) throws RemoteException {
    callback.prepareResult(callId, blockId, totalBytes, bytes);
  }

  /**
   * Relays to {@link ICrossProfileCallback#onResult(long, int, int, byte[])}.
   *
   * <p>Always returns empty byte array.
   */
  @Override
  byte[] call(long callId, int blockId, byte[] bytes) throws RemoteException {
    callback.onResult(callId, blockId, methodIdentifier, bytes);
    return new byte[0];
  }

  /**
   * Callbacks cannot themselves return values, so this method will always throw an {@link
   * IllegalStateException}.
   */
  @Override
  byte[] fetchResponse(long callId, int blockId) throws RemoteException {
    throw new IllegalStateException();
  }
}
