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

import android.os.Parcel;
import android.util.Log;
import com.google.android.enterprise.connectedapps.ICrossProfileCallback;
import com.google.android.enterprise.connectedapps.exceptions.UnavailableProfileException;

/**
 * Implementation of {@link FutureResultWriter} used for writing results to a {@link
 * ICrossProfileCallback}.
 */
public class CrossProfileFutureResultWriter<E> implements FutureResultWriter<E> {

  private final ICrossProfileCallback callback;
  private final Bundler bundler;
  private final BundlerType bundlerType;

  public CrossProfileFutureResultWriter(
      ICrossProfileCallback callback, Bundler bundler, BundlerType bundlerType) {
    if (callback == null || bundler == null || bundlerType == null) {
      throw new NullPointerException();
    }
    this.callback = callback;
    this.bundler = bundler;
    this.bundlerType = bundlerType;
  }

  @Override
  public void onSuccess(E result) {
    Parcel parcel = Parcel.obtain(); // Recycled in this method
    bundler.writeToParcel(parcel, result, bundlerType, /* flags= */ 0);

    try {
      CrossProfileCallbackParcelCallSender parcelCallSender =
          new CrossProfileCallbackParcelCallSender(callback, /* methodIdentifier= */ 0);
      parcelCallSender.makeParcelCall(parcel);
    } catch (UnavailableProfileException e) {
      Log.e("FutureResult", "Connection was dropped before response");
    } catch (RuntimeException e) {
      onFailure(new UnavailableProfileException("Error when writing result of future", e));
    } finally {
      parcel.recycle();
    }
  }

  @Override
  public void onFailure(Throwable throwable) {
    Parcel parcel = Parcel.obtain(); // Recycled in this method
    ParcelUtilities.writeThrowableToParcel(parcel, throwable);

    try {
      CrossProfileCallbackExceptionParcelCallSender parcelCallSender =
          new CrossProfileCallbackExceptionParcelCallSender(callback);
      parcelCallSender.makeParcelCall(parcel);
    } catch (UnavailableProfileException e) {
      Log.e("FutureResult", "Connection was dropped before response");
    } finally {
      parcel.recycle();
    }
  }
}
