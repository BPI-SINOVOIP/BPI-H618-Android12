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

import static com.google.android.enterprise.connectedapps.StringUtilities.randomString;
import static com.google.common.truth.Truth.assertThat;
import static org.junit.Assert.assertThrows;

import android.os.Parcel;
import android.os.RemoteException;
import android.os.TransactionTooLargeException;
import com.google.android.enterprise.connectedapps.exceptions.UnavailableProfileException;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.RobolectricTestRunner;

@RunWith(RobolectricTestRunner.class)
public class ParcelCallSenderTest {

  static class TestParcelCallSender extends ParcelCallSender {

    int failPrepareCalls = 0;
    int failCalls = 0;
    int failFetchResponse = 0;

    private final ParcelCallReceiver parcelCallReceiver = new ParcelCallReceiver();

    @Override
    void prepareCall(long callId, int blockId, int totalBytes, byte[] bytes)
        throws RemoteException {
      if (failPrepareCalls-- > 0) {
        throw new TransactionTooLargeException();
      }

      parcelCallReceiver.prepareCall(callId, blockId, totalBytes, bytes);
    }

    @Override
    byte[] call(long callId, int blockId, byte[] bytes) throws RemoteException {
      if (failCalls-- > 0) {
        throw new TransactionTooLargeException();
      }

      return parcelCallReceiver.prepareResponse(
          callId, parcelCallReceiver.getPreparedCall(callId, blockId, bytes));
    }

    @Override
    byte[] fetchResponse(long callId, int blockId) throws RemoteException {
      if (failFetchResponse-- > 0) {
        throw new TransactionTooLargeException();
      }

      return parcelCallReceiver.getPreparedResponse(callId, blockId);
    }
  }

  private final TestParcelCallSender parcelCallSender = new TestParcelCallSender();
  private static final String LARGE_STRING = randomString(1500000); // 3Mb
  private static final Parcel LARGE_PARCEL = Parcel.obtain();

  static {
    LARGE_PARCEL.writeString(LARGE_STRING);
  }

  @Test
  public void makeParcelCall_prepareCallHasError_retriesUntilSuccess()
      throws UnavailableProfileException {
    parcelCallSender.failPrepareCalls = 5;

    assertThat(parcelCallSender.makeParcelCall(LARGE_PARCEL).readString()).isEqualTo(LARGE_STRING);
  }

  @Test
  public void makeParcelCall_prepareCallHasError_failsAfter10Retries() {
    parcelCallSender.failPrepareCalls = 11;

    assertThrows(
        UnavailableProfileException.class, () -> parcelCallSender.makeParcelCall(LARGE_PARCEL));
  }

  @Test
  public void makeParcelCall_callHasError_retriesUntilSuccess() throws UnavailableProfileException {
    parcelCallSender.failCalls = 5;

    assertThat(parcelCallSender.makeParcelCall(LARGE_PARCEL).readString()).isEqualTo(LARGE_STRING);
  }

  @Test
  public void makeParcelCall_callHasError_failsAfter10Retries() {
    parcelCallSender.failCalls = 11;

    assertThrows(
        UnavailableProfileException.class, () -> parcelCallSender.makeParcelCall(LARGE_PARCEL));
  }

  @Test
  public void makeParcelCall_fetchResponseHasError_retriesUntilSuccess()
      throws UnavailableProfileException {
    parcelCallSender.failFetchResponse = 5;

    assertThat(parcelCallSender.makeParcelCall(LARGE_PARCEL).readString()).isEqualTo(LARGE_STRING);
  }

  @Test
  public void makeParcelCall_fetchResponseHasError_failsAfter10Retries() {
    parcelCallSender.failFetchResponse = 11;

    assertThrows(
        UnavailableProfileException.class, () -> parcelCallSender.makeParcelCall(LARGE_PARCEL));
  }
}
