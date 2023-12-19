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

import static com.google.android.enterprise.connectedapps.CrossProfileSender.MAX_BYTES_PER_BLOCK;

import android.os.Parcel;
import android.os.RemoteException;
import android.os.TransactionTooLargeException;
import android.util.Log;
import com.google.android.enterprise.connectedapps.exceptions.UnavailableProfileException;
import java.nio.ByteBuffer;
import java.util.Arrays;
import java.util.UUID;

/**
 * This represents a single action of (sending a {@link Parcel} and possibly fetching a response,
 * which may be split up over many calls (if the payload is large).
 *
 * <p>The receiver should relay calls to a {@link ParcelCallReceiver}.
 */
abstract class ParcelCallSender {

  private static final long RETRY_DELAY_MILLIS = 10;
  private static final int MAX_RETRIES = 10;

  /**
   * The arguments passed to this should be passed to {@link ParcelCallReceiver#prepareCall(long,
   * int, int, byte[])}.
   */
  abstract void prepareCall(long callId, int blockId, int totalBytes, byte[] bytes)
      throws RemoteException;

  private void prepareCallAndRetry(
      long callId, int blockId, int totalBytes, byte[] bytes, int retries) throws RemoteException {
    while (true) {
      try {
        prepareCall(callId, blockId, totalBytes, bytes);
        break;
      } catch (TransactionTooLargeException e) {
        if (retries-- <= 0) {
          throw e;
        }

        try {
          Thread.sleep(RETRY_DELAY_MILLIS);
        } catch (InterruptedException ex) {
          Log.w("ParcelCallSender", "Interrupted on prepare retry", ex);
          // If we can't sleep we'll just try again immediately
        }
      }
    }
  }

  /**
   * The arguments passed to this should be passed to {@link
   * ParcelCallReceiver#getPreparedCall(long, int, byte[])} and used to complete the call.
   */
  abstract byte[] call(long callId, int blockId, byte[] bytes) throws RemoteException;

  private byte[] callAndRetry(long callId, int blockId, byte[] bytes, int retries)
      throws RemoteException {
    while (true) {
      try {
        return call(callId, blockId, bytes);
      } catch (TransactionTooLargeException e) {
        if (retries-- <= 0) {
          throw e;
        }

        try {
          Thread.sleep(RETRY_DELAY_MILLIS);
        } catch (InterruptedException ex) {
          Log.w("ParcelCallSender", "Interrupted on prepare retry", ex);
          // If we can't sleep we'll just try again immediately
        }
      }
    }
  }

  /**
   * The arguments passed to this should be passed to {@link
   * ParcelCallReceiver#getPreparedResponse(long, int)}.
   */
  abstract byte[] fetchResponse(long callId, int blockId) throws RemoteException;

  private byte[] fetchResponseAndRetry(long callId, int blockId, int retries)
      throws RemoteException {
    while (true) {
      try {
        return fetchResponse(callId, blockId);
      } catch (TransactionTooLargeException e) {
        if (retries-- <= 0) {
          throw e;
        }

        try {
          Thread.sleep(RETRY_DELAY_MILLIS);
        } catch (InterruptedException ex) {
          Log.w("ParcelCallSender", "Interrupted on prepare retry", ex);
          // If we can't sleep we'll just try again immediately
        }
      }
    }
  }

  /**
   * Use the prepareCall(long, int, int, byte[])} and {@link #call(long, int, byte[])} methods to
   * make a call.
   *
   * <p>The returned {@link Parcel} must be recycled after use.
   *
   * <p>Returns {@code null} if the call does not return anything
   *
   * @throws UnavailableProfileException if any call fails
   */
  public Parcel makeParcelCall(Parcel parcel) throws UnavailableProfileException {
    long callIdentifier = UUID.randomUUID().getMostSignificantBits();
    byte[] bytes = parcel.marshall();
    try {
      int numberOfBlocks = (int) Math.ceil(bytes.length * 1.0 / MAX_BYTES_PER_BLOCK);
      int blockIdentifier = 0;

      if (numberOfBlocks > 1) {
        byte[] block = new byte[MAX_BYTES_PER_BLOCK];

        // Loop through all but the last one and send them over to be cached (retrying any failures)
        while (blockIdentifier < numberOfBlocks - 1) {
          System.arraycopy(
              bytes, blockIdentifier * MAX_BYTES_PER_BLOCK, block, 0, MAX_BYTES_PER_BLOCK);

          // Since we know block size is below the limit any errors will be temporary so we should
          // retry
          prepareCallAndRetry(callIdentifier, blockIdentifier, bytes.length, block, MAX_RETRIES);
          blockIdentifier++;
        }

        bytes = Arrays.copyOfRange(bytes, blockIdentifier * MAX_BYTES_PER_BLOCK, bytes.length);
      }

      // Since we know block size is below the limit any errors will be temporary so we should retry
      byte[] returnBytes = callAndRetry(callIdentifier, blockIdentifier, bytes, MAX_RETRIES);

      if (returnBytes.length == 0) {
        return null;
      }

      return fetchResponseParcel(callIdentifier, returnBytes);
    } catch (RemoteException e) {
      throw new UnavailableProfileException("Could not access other profile", e);
    }
  }

  /**
   * Use the {@link ParcelCallSender#prepareCall(long, int, int, byte[])} and {@link
   * ParcelCallSender#fetchResponse(long, int)} methods to fetch a prepared response.
   *
   * <p>The returned {@link Parcel} must be recycled after use.
   *
   * @throws UnavailableProfileException if any call fails
   */
  private Parcel fetchResponseParcel(long callIdentifier, byte[] returnBytes)
      throws UnavailableProfileException {

    // returnBytes[0] is 0 if the bytes are complete, or 1 if we need to fetch more
    int byteOffset = 1;
    if (returnBytes[0] == 1) {
      // returnBytes[1] - returnBytes[4] are an int representing the total size of the return
      // value
      int totalBytes = ByteBuffer.wrap(returnBytes).getInt(/* index= */ 1);

      try {
        returnBytes = fetchReturnBytes(totalBytes, callIdentifier, returnBytes);
      } catch (RemoteException e) {
        throw new UnavailableProfileException("Could not access other profile", e);
      }
      byteOffset = 0;
    }
    Parcel p = Parcel.obtain(); // Recycled by caller
    p.unmarshall(
        returnBytes, /* offset= */ byteOffset, /* length= */ returnBytes.length - byteOffset);
    p.setDataPosition(0);
    return p;
  }

  private byte[] fetchReturnBytes(int totalBytes, long callId, byte[] initialBytes)
      throws RemoteException {
    byte[] returnBytes = new byte[totalBytes];

    // Skip the first 5 bytes which are used for status
    System.arraycopy(
        initialBytes,
        /* srcPos= */ 5,
        returnBytes,
        /* destPos= */ 0,
        /* length= */ MAX_BYTES_PER_BLOCK);

    int numberOfBlocks = (int) Math.ceil(totalBytes * 1.0 / MAX_BYTES_PER_BLOCK);

    for (int block = 1; block < numberOfBlocks; block++) { // Skip 0 as we already have it
      // Since we know block size is below the limit any errors will be temporary so we should retry
      byte[] bytes = fetchResponseAndRetry(callId, block, MAX_RETRIES);
      System.arraycopy(
          bytes,
          /* srcPos= */ 0,
          returnBytes,
          /* destPos= */ block * MAX_BYTES_PER_BLOCK,
          /* length= */ bytes.length);
    }
    return returnBytes;
  }
}
