/*
 * Copyright (C) 2020 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.android.timezone.location.storage.io.write;

import com.android.timezone.location.storage.util.BitwiseUtils;

import java.io.BufferedOutputStream;
import java.io.Closeable;
import java.io.DataOutputStream;
import java.io.Flushable;
import java.io.IOException;
import java.io.OutputStream;

/**
 * A stream-based writer of typed data that can be read back by
 * {@link com.android.timezone.location.storage.block.read.BlockData} and
 * {@link com.android.timezone.location.storage.io.read.TypedInputStream}
 */
public final class TypedOutputStream implements Flushable, Closeable {

    private final DataOutputStream dos;

    /** Creates an instance, wrapping the supplied stream. */
    public TypedOutputStream(OutputStream out) {
        dos = new DataOutputStream(new BufferedOutputStream(out, 8192));
    }

    /**
     * Writes {@code value} as a sequence of network-ordered bytes. {@code byteCount} must be
     * between 1 and 8 inclusive. All bits not to be written in {@code value} <em>must</em> be
     * zero'd otherwise an {@link IllegalArgumentException} will be thrown.
     */
    public void writeVarByteValue(int byteCount, long value) throws IOException {
        if (byteCount < 1 || byteCount > 8) {
            throw new IllegalArgumentException("byteCount " + byteCount + " out of range");
        }

        // To avoid errors, we check for zeros in the bytes we're not writing.
        if (byteCount < 8) {
            long unusedBits = value & BitwiseUtils.getHighBitsMask((8 - byteCount) * 8);
            if (unusedBits != 0) {
                throw new IllegalArgumentException("Bits not to be written should be zero,"
                        + " unusedBits=" + unusedBits);
            }
        }

        // Write the high bytes first.
        for (int i = byteCount - 1; i >= 0; i--) {
            byte b = (byte) (value >>> (i * 8));
            dos.write(b);
        }
    }

    /**
     * Writes {@code unsignedValue} as an unsigned byte. If {@code unsignedValue} is outside of the
     * range 0-255 inclusive, an {@link IllegalArgumentException} is thrown.
     */
    public void writeUnsignedByte(int unsignedValue) throws IOException {
        if (unsignedValue < 0 || unsignedValue > 255) {
            throw new IllegalArgumentException("unsignedValue=" + unsignedValue + " is negative");
        }
        dos.writeByte(unsignedValue);
    }

    /**
     * Writes {@code b} as a signed byte. If {@code b} is outside of the range -128-127 inclusive,
     * an {@link IllegalArgumentException} is thrown.
     */
    public void writeByte(int b) throws IOException {
        BitwiseUtils.checkSignedValueInRange(Byte.SIZE, b);
        dos.writeByte(b);
    }

    /**
     * Writes {@code bytes}.
     */
    public void writeBytes(byte[] bytes) throws IOException {
        dos.write(bytes);
    }

    /**
     * Writes {@code len} {@code bytes} starting at {@code off}.
     */
    public void writeBytes(byte[] bytes, int off, int len) throws IOException {
        dos.write(bytes, off, len);
    }

    /**
     * Writes a tiny (<= 255 entry) byte array as an unsigned byte (length) followed by the bytes.
     */
    public void writeTinyByteArray(byte[] bytes) throws IOException {
        writeUnsignedByte(bytes.length);
        dos.write(bytes);
    }

    /**
     * Writes a tiny (<= 255 entry) char array as an unsigned byte (length) followed by the chars.
     */
    public void writeTinyCharArray(char[] chars) throws IOException {
        writeUnsignedByte(chars.length);
        for (int i = 0; i < chars.length; i++) {
            dos.writeChar(chars[i]);
        }
    }

    /**
     * Writes {@code v} as an 16-bit value in network byte order. If {@code v} is outside of the
     * char range, an {@link IllegalArgumentException} is thrown.
     */
    public void writeChar(int v) throws IOException {
        BitwiseUtils.checkUnsignedValueInRange(Character.SIZE, v);
        dos.writeChar(v);
    }

    /**
     * Writes {@code v} as an 32-bit value in network byte order.
     */
    public void writeInt(int v) throws IOException {
        dos.writeInt(v);
    }

    /**
     * Writes {@code v} as an 64-bit value in network byte order.
     */
    public void writeLong(long v) throws IOException {
        dos.writeLong(v);
    }

    @Override
    public void close() throws IOException {
        dos.close();
    }

    @Override
    public void flush() throws IOException {
        dos.flush();
    }
}
