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

/**
 * Interface used by callbacks used when calling {@link CrossProfileSender#callAsync(long, int,
 * Parcel, LocalCallback, long)}.
 */
public interface LocalCallback {

  /**
   * Pass a result into the callback.
   *
   * @param methodIdentifier The method being responded to.
   * @param params The result encoded in a {@link Parcel}. This should not be recycled.
   */
  void onResult(int methodIdentifier, Parcel params);

  /**
   * Pass an exception into the callback.
   *
   * @param exception The exception encoded in a {@link Parcel}. This should not be recycled.
   */
  void onException(Parcel exception);
}
