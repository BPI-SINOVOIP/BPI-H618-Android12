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

package com.android.media.audiotestharness.server;

import static org.junit.Assert.assertArrayEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.fail;

import com.android.media.audiotestharness.proto.AudioTestHarnessGrpc;
import com.android.media.audiotestharness.proto.AudioTestHarnessService;
import com.android.media.audiotestharness.server.utility.PortUtility;

import com.google.inject.Guice;
import com.google.inject.Injector;

import io.grpc.ManagedChannel;
import io.grpc.ManagedChannelBuilder;

import org.junit.Ignore;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

import java.io.IOException;
import java.net.ServerSocket;
import java.util.Iterator;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

/** Tests for the {@link AudioTestHarnessGrpcServer} class. */
@RunWith(JUnit4.class)
public class AudioTestHarnessGrpcServerTests {

    /**
     * Tests for the {@link AudioTestHarnessGrpcServer#create(int, ExecutorService, Injector)},
     * {@link AudioTestHarnessGrpcServer#createOnPort(int)}, and {@link
     * AudioTestHarnessGrpcServer#createWithDefault()} methods.
     */
    @Test(expected = NullPointerException.class)
    public void create_throwsNullPointerException_nullExecutor() throws Exception {
        AudioTestHarnessGrpcServer.create(
                /* port= */ 80, /* executor= */ null, Guice.createInjector());
    }

    @Test(expected = NullPointerException.class)
    public void create_throwsNullPointerException_nullInjector() throws Exception {
        AudioTestHarnessGrpcServer.create(
                /* port= */ 80, Executors.newSingleThreadExecutor(), /* injector= */ null);
    }

    @Test
    public void createWithDefault_returnsNonNullResult() throws Exception {
        assertNotNull(AudioTestHarnessGrpcServer.createWithDefault());
    }

    @Test
    public void createWithPort_returnsNonNullResult() throws Exception {
        assertNotNull(AudioTestHarnessGrpcServer.createOnPort(/* port= */ 80));
    }

    /** Tests for the {@link AudioTestHarnessGrpcServer#open()} method. */
    @Test
    public void open_buildsAndStartsGrpcServerAsExpected() throws Exception {
        int port = PortUtility.nextAvailablePort();
        AudioTestHarnessGrpcServer server =
                setUpTestServer(port, Executors.newSingleThreadExecutor());

        assertPortOpen(port);

        server.open();

        assertServerRunningAsExpected(port);
    }

    // Per (b/175643926) the following two tests are ignored since they do not run within Forrest.
    // They do run (and pass) locally.

    @Ignore
    @Test(expected = IOException.class)
    public void open_throwsIOExceptionForMissingServiceDependency() throws Exception {
        AudioTestHarnessGrpcServer.create(
                        8080, Executors.newSingleThreadExecutor(), Guice.createInjector())
                .open();
    }

    @Ignore
    @Test(expected = IOException.class)
    public void open_throwsIOException_forPortTaken() throws Exception {
        int port = PortUtility.nextAvailablePort();
        AudioTestHarnessGrpcServer server =
                setUpTestServer(port, Executors.newSingleThreadExecutor());

        new ServerSocket(port);
        server.open();
    }

    /** Tests for the {@link AudioTestHarnessGrpcServer#close()} method. */
    @Test
    public void close_shutsDownGrpcServerAsExpected() throws Exception {
        int port = PortUtility.nextAvailablePort();
        ExecutorService executorService = Executors.newSingleThreadExecutor();
        AudioTestHarnessGrpcServer server = setUpTestServer(port, executorService);

        server.open();
        assertServerRunningAsExpected(port);

        server.close();
        assertPortOpen(port);
    }

    /**
     * Builds a {@link AudioTestHarnessGrpcServer} on the specified port with the specified {@link
     * ExecutorService} for testing purposes using the test implementation of the service.
     *
     * <p>While not the standard method of testing gRPC services, these tests aim to ensure that a
     * working, production-ready server is properly set up and torn down by the {@link
     * AudioTestHarnessGrpcServer} class. Thus, it uses an actual web server and executor to ensure
     * that requires can be properly sent and received and that the setup and tear down actions
     * operate as expected.
     */
    private static AudioTestHarnessGrpcServer setUpTestServer(
            int port, ExecutorService executorService) {

        // Build Guice injector to inject test implementation of the Audio Test Harness service.
        Injector injector =
                Guice.createInjector(
                        binder ->
                                binder.bind(AudioTestHarnessGrpc.AudioTestHarnessImplBase.class)
                                        .to(AudioTestHarnessTestImpl.class));
        return AudioTestHarnessGrpcServer.create(port, executorService, injector);
    }

    /**
     * Verifies that the test gRPC Server was set up correctly by attempting to communicate with it
     * over the specified port.
     */
    private static void assertServerRunningAsExpected(int port) {
        // Attempt to communicate with running host over defined port.
        ManagedChannel managedChannel =
                ManagedChannelBuilder.forAddress("0.0.0.0", port).usePlaintext().build();
        AudioTestHarnessGrpc.AudioTestHarnessBlockingStub stub =
                AudioTestHarnessGrpc.newBlockingStub(managedChannel);
        Iterator<AudioTestHarnessService.CaptureChunk> chunks =
                stub.capture(AudioTestHarnessService.CaptureRequest.getDefaultInstance());

        // Verify that the received message matches the expected message sent by the test
        // implementation.
        AudioTestHarnessService.CaptureChunk chunk = chunks.next();
        assertArrayEquals(AudioTestHarnessTestImpl.MESSAGE, chunk.getData().toByteArray());
        assertFalse(chunks.hasNext());

        managedChannel.shutdown();
    }

    /**
     * Verifies that a given port is open by attempting to create a {@link ServerSocket} on the
     * port.
     */
    private static void assertPortOpen(int port) {
        try {
            new ServerSocket(port).close();
        } catch (IOException ioe) {
            fail("Expected Port to be open, but was actually not.");
        }
    }
}
