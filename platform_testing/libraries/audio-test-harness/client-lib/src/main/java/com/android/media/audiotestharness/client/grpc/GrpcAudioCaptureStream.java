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

package com.android.media.audiotestharness.client.grpc;

import com.android.media.audiotestharness.client.core.AudioCaptureStream;
import com.android.media.audiotestharness.proto.AudioTestHarnessGrpc;

import java.io.IOException;

/** {@link AudioCaptureStream} that utilizes gRPC as its transfer mechanism. */
public class GrpcAudioCaptureStream extends AudioCaptureStream {

    // TODO(b/168817017): Implement this class to utlize gRPC and Piped I/O streams to provide an
    // InputStream from which the audio samples can be read.

    private final AudioTestHarnessGrpc.AudioTestHarnessStub mAudioTestHarnessStub;

    private GrpcAudioCaptureStream(AudioTestHarnessGrpc.AudioTestHarnessStub audioTestHarnessStub) {
        mAudioTestHarnessStub = audioTestHarnessStub;
    }

    static GrpcAudioCaptureStream create(
            AudioTestHarnessGrpc.AudioTestHarnessStub audioTestHarnessStub) {
        return new GrpcAudioCaptureStream(audioTestHarnessStub);
    }

    @Override
    public int read(byte[] b) throws IOException {
        return super.read(b);
    }

    @Override
    public int read(byte[] b, int off, int len) throws IOException {
        return super.read(b, off, len);
    }

    @Override
    public long skip(long n) throws IOException {
        return super.skip(n);
    }

    @Override
    public int available() throws IOException {
        return super.available();
    }

    @Override
    public void close() throws IOException {
        super.close();
    }

    @Override
    public synchronized void mark(int readlimit) {
        super.mark(readlimit);
    }

    @Override
    public synchronized void reset() throws IOException {
        super.reset();
    }

    @Override
    public boolean markSupported() {
        return super.markSupported();
    }

    @Override
    public int read() throws IOException {
        return 0;
    }
}
