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

package com.android.timezone.location.tools.dump;

import com.android.timezone.location.storage.util.Visitor;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.OutputStreamWriter;
import java.io.PrintWriter;
import java.nio.charset.StandardCharsets;

/** Helper methods for dumping data to files. */
public final class DumpUtils {

    private DumpUtils() {
    }

    public static PrintWriter createPrintWriter(File file) throws Visitor.VisitException {
        try {
            OutputStreamWriter utf8Writer = new OutputStreamWriter(
                    new FileOutputStream(file), StandardCharsets.UTF_8);
            return new PrintWriter(utf8Writer);
        } catch (IOException e) {
            throw new Visitor.VisitException(e);
        }
    }

    public static File generateDumpFile(File dir, String filePrefix, int blockId, int maxBlockId) {
        final String fileSuffix = ".txt";

        int blockIdLength = hexStringLength(maxBlockId);
        StringBuilder sb = new StringBuilder(
                filePrefix.length() + blockIdLength + fileSuffix.length());
        sb.append(filePrefix);

        String blockIdHex = zeroPadHex(blockIdLength, blockId);
        sb.append(blockIdHex);
        sb.append(fileSuffix);
        return new File(dir, sb.toString());
    }

    public static int hexStringLength(int value) {
        int bitsNeeded = Integer.SIZE - Integer.numberOfLeadingZeros(value);
        return (bitsNeeded + 3) / 4;
    }

    public static int binaryStringLength(int value) {
        return Integer.SIZE - Integer.numberOfLeadingZeros(value);
    }

    public static String zeroPadHex(int length, int value) {
        String hexString = Integer.toHexString(value);
        int unpaddedLength = hexString.length();
        if (unpaddedLength >= length) {
            return hexString;
        }
        return zeroPad(length, hexString);
    }

    public static String zeroPadBinary(int length, int value) {
        String binaryString = Integer.toBinaryString(value);
        int unpaddedLength = binaryString.length();
        if (unpaddedLength >= length) {
            return binaryString;
        }
        return zeroPad(length, binaryString);
    }

    private static String zeroPad(int length, String unpaddedValue) {
        StringBuilder sb = new StringBuilder(length);
        int zeroPadding = length - unpaddedValue.length();
        for (int i = 0; i < zeroPadding; i++) {
            sb.append('0');
        }
        sb.append(unpaddedValue);
        return sb.toString();
    }
}