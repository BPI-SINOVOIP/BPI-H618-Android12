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
package com.google.android.enterprise.connectedapps;

import android.os.Parcel;
import android.os.RemoteException;
import com.google.android.enterprise.connectedapps.internal.ByteUtilities;
import com.google.auto.value.AutoValue;
import org.checkerframework.checker.nullness.qual.Nullable;

public class TestService extends ICrossProfileService.Stub {

  @AutoValue
  abstract static class LoggedCrossProfileMethodCall {
    abstract long getCrossProfileTypeIdentifier();

    abstract long getMethodIdentifier();

    abstract Parcel getParams();

    @Nullable
    abstract ICrossProfileCallback callback();

    static LoggedCrossProfileMethodCall create(
        long crossProfileTypeIdentifier,
        long methodIdentifier,
        Parcel params,
        ICrossProfileCallback callback) {
      return new AutoValue_TestService_LoggedCrossProfileMethodCall(
          crossProfileTypeIdentifier, methodIdentifier, params, callback);
    }
  }

  private LoggedCrossProfileMethodCall lastCall;
  private Parcel responseParcel = Parcel.obtain(); // Recycled in #setResponseParcel

  LoggedCrossProfileMethodCall lastCall() {
    return lastCall;
  }

  /**
   * Set the parcel to be returned from a call to this service.
   *
   * <p>The previously set parcel will be recycled.
   */
  void setResponseParcel(Parcel responseParcel) {
    this.responseParcel.recycle();
    this.responseParcel = responseParcel;
  }

  @Override
  public void prepareCall(long callId, int blockId, int numBytes, byte[] paramsBytes) {}

  @Override
  public byte[] call(
      long callId,
      int blockId,
      long crossProfileTypeIdentifier,
      int methodIdentifier,
      byte[] paramsBytes,
      ICrossProfileCallback callback)
      throws RemoteException {

    Parcel parcel = Parcel.obtain(); // Recycled by this method on next call
    parcel.unmarshall(paramsBytes, 0, paramsBytes.length);
    parcel.setDataPosition(0);

    if (lastCall != null) {
      lastCall.getParams().recycle();
    }

    lastCall =
        LoggedCrossProfileMethodCall.create(
            crossProfileTypeIdentifier, methodIdentifier, parcel, callback);

    byte[] parcelBytes = responseParcel.marshall();
    return prepareResponse(parcelBytes);
  }

  private static byte[] prepareResponse(byte[] parcelBytes) {
    // This doesn't deal with large responses.
    return ByteUtilities.joinByteArrays(new byte[] {0}, parcelBytes);
  }

  @Override
  public byte[] fetchResponse(long callId, int blockId) {
    return null;
  }
}
