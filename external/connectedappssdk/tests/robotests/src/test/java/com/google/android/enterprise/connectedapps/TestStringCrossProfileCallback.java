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

import android.os.IBinder;
import android.os.Parcel;
import android.os.RemoteException;
import com.google.android.enterprise.connectedapps.internal.ParcelUtilities;

public class TestStringCrossProfileCallback implements ICrossProfileCallback {

  public int lastReceivedMethodIdentifier = -1;
  public String lastReceivedMethodParam;
  public Throwable lastReceivedException;

  @Override
  public void prepareResult(long callId, int blockId, int numBytes, byte[] params) {}

  @Override
  public void onResult(long callId, int blockId, int methodIdentifier, byte[] paramsBytes)
      throws RemoteException {
    lastReceivedMethodIdentifier = methodIdentifier;
    Parcel parcel = Parcel.obtain(); // Recycled in this method
    parcel.unmarshall(paramsBytes, 0, paramsBytes.length);
    parcel.setDataPosition(0);
    lastReceivedMethodParam = parcel.readString();
    parcel.recycle();
  }

  @Override
  public void onException(long callId, int blockId, byte[] paramsBytes) throws RemoteException {
    Parcel parcel = Parcel.obtain(); // Recycled in this method
    parcel.unmarshall(paramsBytes, 0, paramsBytes.length);
    parcel.setDataPosition(0);

    lastReceivedException = ParcelUtilities.readThrowableFromParcel(parcel);
    parcel.recycle();
  }

  @Override
  public IBinder asBinder() {
    return null;
  }
}
