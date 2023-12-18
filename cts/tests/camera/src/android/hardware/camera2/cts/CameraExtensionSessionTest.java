/*
 * Copyright 2020 The Android Open Source Project
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

package android.hardware.camera2.cts;

import static org.mockito.Matchers.any;
import static org.mockito.Matchers.anyInt;
import static org.mockito.Matchers.anyLong;
import static org.mockito.Matchers.eq;
import static org.mockito.Mockito.atLeastOnce;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.timeout;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;

import com.android.compatibility.common.util.DeviceReportLog;
import com.android.compatibility.common.util.ResultType;
import com.android.compatibility.common.util.ResultUnit;
import com.android.compatibility.common.util.Stat;
import com.android.ex.camera2.blocking.BlockingSessionCallback;
import com.android.ex.camera2.blocking.BlockingExtensionSessionCallback;
import com.android.ex.camera2.blocking.BlockingStateCallback;
import com.android.ex.camera2.exceptions.TimeoutRuntimeException;

import android.graphics.ImageFormat;
import android.graphics.SurfaceTexture;
import android.hardware.HardwareBuffer;
import android.hardware.camera2.CameraCaptureSession;
import android.hardware.camera2.CameraDevice;
import android.hardware.camera2.CameraExtensionCharacteristics;
import android.hardware.camera2.CameraExtensionSession;
import android.hardware.camera2.CaptureRequest;
import android.hardware.camera2.CaptureResult;
import android.hardware.camera2.cts.helpers.CameraErrorCollector;
import android.hardware.camera2.cts.helpers.StaticMetadata;
import android.hardware.camera2.cts.testcases.Camera2AndroidTestRule;
import android.hardware.camera2.params.ExtensionSessionConfiguration;
import android.hardware.camera2.params.OutputConfiguration;
import android.hardware.camera2.params.SessionConfiguration;
import android.media.ExifInterface;
import android.media.Image;
import android.media.ImageReader;
import android.os.SystemClock;
import android.util.Range;
import android.util.Size;

import static android.hardware.camera2.cts.CameraTestUtils.*;
import static android.hardware.cts.helpers.CameraUtils.*;

import android.util.Log;
import android.view.Surface;
import android.view.TextureView;

import androidx.test.platform.app.InstrumentationRegistry;
import androidx.test.rule.ActivityTestRule;

import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.Parameterized;

import java.io.IOException;
import java.util.ArrayList;
import java.util.List;
import java.util.Set;
import java.util.stream.Collectors;

@RunWith(Parameterized.class)
public class CameraExtensionSessionTest extends Camera2ParameterizedTestCase {
    private static final String TAG = "CameraExtensionSessionTest";
    private static final boolean VERBOSE = Log.isLoggable(TAG, Log.VERBOSE);
    private static final long WAIT_FOR_COMMAND_TO_COMPLETE_MS = 5000;
    private static final long REPEATING_REQUEST_TIMEOUT_MS = 5000;
    public static final int MULTI_FRAME_CAPTURE_IMAGE_TIMEOUT_MS = 10000;

    private SurfaceTexture mSurfaceTexture = null;
    private Camera2AndroidTestRule mTestRule = null;

    private DeviceReportLog mReportLog;

    @Override
    public void setUp() throws Exception {
        super.setUp();
        mTestRule = new Camera2AndroidTestRule(mContext);
        mTestRule.before();
    }

    @Override
    public void tearDown() throws Exception {
        if (mTestRule != null) {
            mTestRule.after();
        }
        if (mSurfaceTexture != null) {
            mSurfaceTexture.release();
            mSurfaceTexture = null;
        }
        super.tearDown();
    }

    @Rule
    public ActivityTestRule<CameraExtensionTestActivity> mActivityRule =
            new ActivityTestRule<>(CameraExtensionTestActivity.class);

    private void updatePreviewSurfaceTexture() {
        if (mSurfaceTexture != null) {
            return;
        }

        TextureView textureView = mActivityRule.getActivity().getTextureView();
        mSurfaceTexture = getAvailableSurfaceTexture(WAIT_FOR_COMMAND_TO_COMPLETE_MS, textureView);
        assertNotNull("Failed to acquire valid preview surface texture!", mSurfaceTexture);
    }

    // Verify that camera extension sessions can be created and closed as expected.
    @Test
    public void testBasicExtensionLifecycle() throws Exception {
        for (String id : mCameraIdsUnderTest) {
            StaticMetadata staticMeta =
                    new StaticMetadata(mTestRule.getCameraManager().getCameraCharacteristics(id));
            if (!staticMeta.isColorOutputSupported()) {
                continue;
            }
            updatePreviewSurfaceTexture();
            CameraExtensionCharacteristics extensionChars =
                    mTestRule.getCameraManager().getCameraExtensionCharacteristics(id);
            List<Integer> supportedExtensions = extensionChars.getSupportedExtensions();
            for (Integer extension : supportedExtensions) {
                List<Size> extensionSizes = extensionChars.getExtensionSupportedSizes(extension,
                        mSurfaceTexture.getClass());
                Size maxSize = CameraTestUtils.getMaxSize(extensionSizes.toArray(new Size[0]));
                mSurfaceTexture.setDefaultBufferSize(maxSize.getWidth(), maxSize.getHeight());
                OutputConfiguration outputConfig = new OutputConfiguration(
                        OutputConfiguration.SURFACE_GROUP_ID_NONE,
                        new Surface(mSurfaceTexture));
                List<OutputConfiguration> outputConfigs = new ArrayList<>();
                outputConfigs.add(outputConfig);

                BlockingExtensionSessionCallback sessionListener =
                        new BlockingExtensionSessionCallback(
                                mock(CameraExtensionSession.StateCallback.class));
                ExtensionSessionConfiguration configuration =
                        new ExtensionSessionConfiguration(extension, outputConfigs,
                                new HandlerExecutor(mTestRule.getHandler()), sessionListener);

                try {
                    mTestRule.openDevice(id);
                    CameraDevice camera = mTestRule.getCamera();
                    camera.createExtensionSession(configuration);
                    CameraExtensionSession extensionSession = sessionListener.waitAndGetSession(
                            SESSION_CONFIGURE_TIMEOUT_MS);

                    extensionSession.close();
                    sessionListener.getStateWaiter().waitForState(
                            BlockingExtensionSessionCallback.SESSION_CLOSED,
                            SESSION_CLOSE_TIMEOUT_MS);
                } finally {
                    mTestRule.closeDevice(id);
                }
            }
        }
    }

    // Verify that regular camera sessions close as expected after creating a camera extension
    // session.
    @Test
    public void testCloseCaptureSession() throws Exception {
        for (String id : mCameraIdsUnderTest) {
            StaticMetadata staticMeta =
                    new StaticMetadata(mTestRule.getCameraManager().getCameraCharacteristics(id));
            if (!staticMeta.isColorOutputSupported()) {
                continue;
            }
            updatePreviewSurfaceTexture();
            CameraExtensionCharacteristics extensionChars =
                    mTestRule.getCameraManager().getCameraExtensionCharacteristics(id);
            List<Integer> supportedExtensions = extensionChars.getSupportedExtensions();
            for (Integer extension : supportedExtensions) {
                List<Size> extensionSizes = extensionChars.getExtensionSupportedSizes(extension,
                        mSurfaceTexture.getClass());
                Size maxSize = CameraTestUtils.getMaxSize(extensionSizes.toArray(new Size[0]));
                mSurfaceTexture.setDefaultBufferSize(maxSize.getWidth(), maxSize.getHeight());
                Surface repeatingSurface = new Surface(mSurfaceTexture);
                OutputConfiguration textureOutput = new OutputConfiguration(
                        OutputConfiguration.SURFACE_GROUP_ID_NONE, repeatingSurface);
                List<OutputConfiguration> outputs = new ArrayList<>();
                outputs.add(textureOutput);
                BlockingSessionCallback regularSessionListener = new BlockingSessionCallback(
                        mock(CameraCaptureSession.StateCallback.class));
                SessionConfiguration regularConfiguration = new SessionConfiguration(
                        SessionConfiguration.SESSION_REGULAR, outputs,
                        new HandlerExecutor(mTestRule.getHandler()), regularSessionListener);

                BlockingExtensionSessionCallback sessionListener =
                        new BlockingExtensionSessionCallback(mock(
                                CameraExtensionSession.StateCallback.class));
                ExtensionSessionConfiguration configuration =
                        new ExtensionSessionConfiguration(extension, outputs,
                                new HandlerExecutor(mTestRule.getHandler()), sessionListener);

                try {
                    mTestRule.openDevice(id);
                    mTestRule.getCamera().createCaptureSession(regularConfiguration);

                    CameraCaptureSession session =
                            regularSessionListener
                                    .waitAndGetSession(SESSION_CONFIGURE_TIMEOUT_MS);
                    assertNotNull(session);

                    CameraDevice camera = mTestRule.getCamera();
                    camera.createExtensionSession(configuration);
                    CameraExtensionSession extensionSession = sessionListener.waitAndGetSession(
                            SESSION_CONFIGURE_TIMEOUT_MS);
                    assertNotNull(extensionSession);

                    regularSessionListener.getStateWaiter().waitForState(
                            BlockingExtensionSessionCallback.SESSION_CLOSED,
                            SESSION_CLOSE_TIMEOUT_MS);

                    extensionSession.close();
                    sessionListener.getStateWaiter().waitForState(
                            BlockingExtensionSessionCallback.SESSION_CLOSED,
                            SESSION_CLOSE_TIMEOUT_MS);
                } finally {
                    mTestRule.closeDevice(id);
                }
            }
        }
    }

    // Verify that camera extension sessions close as expected when creating a regular capture
    // session.
    @Test
    public void testCloseExtensionSession() throws Exception {
        for (String id : mCameraIdsUnderTest) {
            StaticMetadata staticMeta =
                    new StaticMetadata(mTestRule.getCameraManager().getCameraCharacteristics(id));
            if (!staticMeta.isColorOutputSupported()) {
                continue;
            }
            updatePreviewSurfaceTexture();
            CameraExtensionCharacteristics extensionChars =
                    mTestRule.getCameraManager().getCameraExtensionCharacteristics(id);
            List<Integer> supportedExtensions = extensionChars.getSupportedExtensions();
            for (Integer extension : supportedExtensions) {
                List<Size> extensionSizes = extensionChars.getExtensionSupportedSizes(extension,
                        mSurfaceTexture.getClass());
                Size maxSize = CameraTestUtils.getMaxSize(extensionSizes.toArray(new Size[0]));
                mSurfaceTexture.setDefaultBufferSize(maxSize.getWidth(), maxSize.getHeight());
                Surface surface = new Surface(mSurfaceTexture);
                OutputConfiguration textureOutput = new OutputConfiguration(
                        OutputConfiguration.SURFACE_GROUP_ID_NONE, surface);
                List<OutputConfiguration> outputs = new ArrayList<>();
                outputs.add(textureOutput);
                BlockingSessionCallback regularSessionListener = new BlockingSessionCallback(
                        mock(CameraCaptureSession.StateCallback.class));
                SessionConfiguration regularConfiguration = new SessionConfiguration(
                        SessionConfiguration.SESSION_REGULAR, outputs,
                        new HandlerExecutor(mTestRule.getHandler()), regularSessionListener);

                BlockingExtensionSessionCallback sessionListener =
                        new BlockingExtensionSessionCallback(mock(
                                CameraExtensionSession.StateCallback.class));
                ExtensionSessionConfiguration configuration =
                        new ExtensionSessionConfiguration(extension, outputs,
                                new HandlerExecutor(mTestRule.getHandler()), sessionListener);

                try {
                    mTestRule.openDevice(id);
                    CameraDevice camera = mTestRule.getCamera();
                    camera.createExtensionSession(configuration);
                    CameraExtensionSession extensionSession = sessionListener.waitAndGetSession(
                            SESSION_CONFIGURE_TIMEOUT_MS);
                    assertNotNull(extensionSession);

                    mTestRule.getCamera().createCaptureSession(regularConfiguration);
                    sessionListener.getStateWaiter().waitForState(
                            BlockingExtensionSessionCallback.SESSION_CLOSED,
                            SESSION_CLOSE_TIMEOUT_MS);

                    CameraCaptureSession session =
                            regularSessionListener.waitAndGetSession(
                                    SESSION_CONFIGURE_TIMEOUT_MS);
                    session.close();
                    regularSessionListener.getStateWaiter().waitForState(
                            BlockingSessionCallback.SESSION_CLOSED, SESSION_CLOSE_TIMEOUT_MS);
                } finally {
                    mTestRule.closeDevice(id);
                }
            }
        }
    }

    // Verify camera device query
    @Test
    public void testGetDevice() throws Exception {
        for (String id : mCameraIdsUnderTest) {
            StaticMetadata staticMeta =
                    new StaticMetadata(mTestRule.getCameraManager().getCameraCharacteristics(id));
            if (!staticMeta.isColorOutputSupported()) {
                continue;
            }
            updatePreviewSurfaceTexture();
            CameraExtensionCharacteristics extensionChars =
                    mTestRule.getCameraManager().getCameraExtensionCharacteristics(id);
            List<Integer> supportedExtensions = extensionChars.getSupportedExtensions();
            for (Integer extension : supportedExtensions) {
                List<Size> extensionSizes = extensionChars.getExtensionSupportedSizes(extension,
                        mSurfaceTexture.getClass());
                Size maxSize = CameraTestUtils.getMaxSize(extensionSizes.toArray(new Size[0]));
                mSurfaceTexture.setDefaultBufferSize(maxSize.getWidth(), maxSize.getHeight());
                OutputConfiguration privateOutput = new OutputConfiguration(
                        OutputConfiguration.SURFACE_GROUP_ID_NONE,
                        new Surface(mSurfaceTexture));
                List<OutputConfiguration> outputConfigs = new ArrayList<>();
                outputConfigs.add(privateOutput);

                BlockingExtensionSessionCallback sessionListener =
                        new BlockingExtensionSessionCallback(
                                mock(CameraExtensionSession.StateCallback.class));
                ExtensionSessionConfiguration configuration =
                        new ExtensionSessionConfiguration(extension, outputConfigs,
                                new HandlerExecutor(mTestRule.getHandler()), sessionListener);

                try {
                    mTestRule.openDevice(id);
                    CameraDevice camera = mTestRule.getCamera();
                    camera.createExtensionSession(configuration);
                    CameraExtensionSession extensionSession = sessionListener.waitAndGetSession(
                            SESSION_CONFIGURE_TIMEOUT_MS);

                    assertEquals("Unexpected/Invalid camera device", mTestRule.getCamera(),
                            extensionSession.getDevice());
                } finally {
                    mTestRule.closeDevice(id);
                }

                try {
                    sessionListener.getStateWaiter().waitForState(
                            BlockingExtensionSessionCallback.SESSION_CLOSED,
                            SESSION_CLOSE_TIMEOUT_MS);
                    fail("should get TimeoutRuntimeException due to previously closed camera "
                            + "device");
                } catch (TimeoutRuntimeException e) {
                    // Expected, per API spec we should not receive any further session callbacks
                    // besides the device state 'onClosed' callback.
                }
            }
        }
    }

    // Test case for repeating/stopRepeating on all supported extensions and expected state/capture
    // callbacks.
    @Test
    public void testRepeatingCapture() throws Exception {
        for (String id : mCameraIdsUnderTest) {
            StaticMetadata staticMeta =
                    new StaticMetadata(mTestRule.getCameraManager().getCameraCharacteristics(id));
            if (!staticMeta.isColorOutputSupported()) {
                continue;
            }
            updatePreviewSurfaceTexture();
            CameraExtensionCharacteristics extensionChars =
                    mTestRule.getCameraManager().getCameraExtensionCharacteristics(id);
            List<Integer> supportedExtensions = extensionChars.getSupportedExtensions();
            for (Integer extension : supportedExtensions) {
                List<Size> extensionSizes = extensionChars.getExtensionSupportedSizes(extension,
                        mSurfaceTexture.getClass());
                Size maxSize =
                        CameraTestUtils.getMaxSize(extensionSizes.toArray(new Size[0]));
                mSurfaceTexture.setDefaultBufferSize(maxSize.getWidth(),
                        maxSize.getHeight());
                Surface texturedSurface = new Surface(mSurfaceTexture);

                List<OutputConfiguration> outputConfigs = new ArrayList<>();
                outputConfigs.add(new OutputConfiguration(
                        OutputConfiguration.SURFACE_GROUP_ID_NONE, texturedSurface));

                BlockingExtensionSessionCallback sessionListener =
                        new BlockingExtensionSessionCallback(mock(
                                CameraExtensionSession.StateCallback.class));
                ExtensionSessionConfiguration configuration =
                        new ExtensionSessionConfiguration(extension, outputConfigs,
                                new HandlerExecutor(mTestRule.getHandler()),
                                sessionListener);

                try {
                    mTestRule.openDevice(id);
                    CameraDevice camera = mTestRule.getCamera();
                    camera.createExtensionSession(configuration);
                    CameraExtensionSession extensionSession =
                            sessionListener.waitAndGetSession(
                                    SESSION_CONFIGURE_TIMEOUT_MS);
                    assertNotNull(extensionSession);

                    CaptureRequest.Builder captureBuilder =
                            mTestRule.getCamera().createCaptureRequest(
                                    android.hardware.camera2.CameraDevice.TEMPLATE_PREVIEW);
                    captureBuilder.addTarget(texturedSurface);
                    CameraExtensionSession.ExtensionCaptureCallback captureCallbackMock =
                            mock(CameraExtensionSession.ExtensionCaptureCallback.class);
                    SimpleCaptureCallback simpleCaptureCallback =
                            new SimpleCaptureCallback(captureCallbackMock);
                    CaptureRequest request = captureBuilder.build();
                    int sequenceId = extensionSession.setRepeatingRequest(request,
                            new HandlerExecutor(mTestRule.getHandler()), simpleCaptureCallback);

                    verify(captureCallbackMock,
                            timeout(REPEATING_REQUEST_TIMEOUT_MS).atLeastOnce())
                            .onCaptureStarted(eq(extensionSession), eq(request), anyLong());
                    verify(captureCallbackMock,
                            timeout(REPEATING_REQUEST_TIMEOUT_MS).atLeastOnce())
                            .onCaptureProcessStarted(extensionSession, request);

                    extensionSession.stopRepeating();

                    verify(captureCallbackMock,
                            timeout(MULTI_FRAME_CAPTURE_IMAGE_TIMEOUT_MS).times(1))
                            .onCaptureSequenceCompleted(extensionSession, sequenceId);

                    verify(captureCallbackMock, times(0))
                            .onCaptureSequenceAborted(any(CameraExtensionSession.class),
                                    anyInt());

                    extensionSession.close();

                    sessionListener.getStateWaiter().waitForState(
                            BlockingExtensionSessionCallback.SESSION_CLOSED,
                            SESSION_CLOSE_TIMEOUT_MS);

                    assertTrue("The sum of onCaptureProcessStarted and onCaptureFailed" +
                                    " callbacks must be greater or equal than the number of calls" +
                                    " to onCaptureStarted!",
                            simpleCaptureCallback.getTotalFramesArrived() +
                                    simpleCaptureCallback.getTotalFramesFailed() >=
                                    simpleCaptureCallback.getTotalFramesStarted());
                    assertTrue(String.format("The last repeating request surface timestamp " +
                                    "%d must be less than or equal to the last " +
                                    "onCaptureStarted " +
                                    "timestamp %d", mSurfaceTexture.getTimestamp(),
                            simpleCaptureCallback.getLastTimestamp()),
                            mSurfaceTexture.getTimestamp() <=
                                    simpleCaptureCallback.getLastTimestamp());
                } finally {
                    mTestRule.closeDevice(id);
                    texturedSurface.release();
                }
            }
        }
    }

    // Test case for multi-frame only capture on all supported extensions and expected state
    // callbacks. Verify still frame output, measure the average capture latency and if possible
    // ensure that the value is within the reported range.
    @Test
    public void testMultiFrameCapture() throws Exception {
        final int IMAGE_COUNT = 10;
        final int SUPPORTED_CAPTURE_OUTPUT_FORMATS[] = {
                ImageFormat.YUV_420_888,
                ImageFormat.JPEG
        };
        for (String id : mCameraIdsUnderTest) {
            StaticMetadata staticMeta =
                    new StaticMetadata(mTestRule.getCameraManager().getCameraCharacteristics(id));
            if (!staticMeta.isColorOutputSupported()) {
                continue;
            }
            updatePreviewSurfaceTexture();
            CameraExtensionCharacteristics extensionChars =
                    mTestRule.getCameraManager().getCameraExtensionCharacteristics(id);
            List<Integer> supportedExtensions = extensionChars.getSupportedExtensions();
            for (Integer extension : supportedExtensions) {
                for (int captureFormat : SUPPORTED_CAPTURE_OUTPUT_FORMATS) {
                    List<Size> extensionSizes = extensionChars.getExtensionSupportedSizes(extension,
                            captureFormat);
                    if (extensionSizes.isEmpty()) {
                        continue;
                    }
                    Size maxSize = CameraTestUtils.getMaxSize(extensionSizes.toArray(new Size[0]));
                    SimpleImageReaderListener imageListener = new SimpleImageReaderListener(false,
                            1);
                    ImageReader extensionImageReader = CameraTestUtils.makeImageReader(maxSize,
                            captureFormat, /*maxImages*/ 1, imageListener,
                            mTestRule.getHandler());
                    Surface imageReaderSurface = extensionImageReader.getSurface();
                    OutputConfiguration readerOutput = new OutputConfiguration(
                            OutputConfiguration.SURFACE_GROUP_ID_NONE, imageReaderSurface);
                    List<OutputConfiguration> outputConfigs = new ArrayList<>();
                    outputConfigs.add(readerOutput);

                    BlockingExtensionSessionCallback sessionListener =
                            new BlockingExtensionSessionCallback(mock(
                                    CameraExtensionSession.StateCallback.class));
                    ExtensionSessionConfiguration configuration =
                            new ExtensionSessionConfiguration(extension, outputConfigs,
                                    new HandlerExecutor(mTestRule.getHandler()),
                                    sessionListener);
                    String streamName = "test_extension_capture";
                    mReportLog = new DeviceReportLog(REPORT_LOG_NAME, streamName);
                    mReportLog.addValue("camera_id", id, ResultType.NEUTRAL, ResultUnit.NONE);
                    mReportLog.addValue("extension_id", extension, ResultType.NEUTRAL,
                            ResultUnit.NONE);
                    double[] captureTimes = new double[IMAGE_COUNT];

                    try {
                        mTestRule.openDevice(id);
                        CameraDevice camera = mTestRule.getCamera();
                        camera.createExtensionSession(configuration);
                        CameraExtensionSession extensionSession =
                                sessionListener.waitAndGetSession(
                                        SESSION_CONFIGURE_TIMEOUT_MS);
                        assertNotNull(extensionSession);

                        CaptureRequest.Builder captureBuilder =
                                mTestRule.getCamera().createCaptureRequest(
                                        CameraDevice.TEMPLATE_STILL_CAPTURE);
                        captureBuilder.addTarget(imageReaderSurface);
                        CameraExtensionSession.ExtensionCaptureCallback captureCallback =
                                mock(CameraExtensionSession.ExtensionCaptureCallback.class);

                        for (int i = 0; i < IMAGE_COUNT; i++) {
                            int jpegOrientation = (i * 90) % 360; // degrees [0..270]
                            if (captureFormat == ImageFormat.JPEG) {
                                captureBuilder.set(CaptureRequest.JPEG_ORIENTATION,
                                        jpegOrientation);
                            }
                            CaptureRequest request = captureBuilder.build();
                            long startTimeMs = SystemClock.elapsedRealtime();
                            int sequenceId = extensionSession.capture(request,
                                    new HandlerExecutor(mTestRule.getHandler()), captureCallback);

                            Image img =
                                    imageListener.getImage(MULTI_FRAME_CAPTURE_IMAGE_TIMEOUT_MS);
                            captureTimes[i] = SystemClock.elapsedRealtime() - startTimeMs;
                            if (captureFormat == ImageFormat.JPEG) {
                                verifyJpegOrientation(img, maxSize, jpegOrientation);
                            } else {
                                validateImage(img, maxSize.getWidth(), maxSize.getHeight(),
                                        captureFormat, null);
                            }
                            img.close();

                            verify(captureCallback, times(1))
                                    .onCaptureStarted(eq(extensionSession), eq(request), anyLong());
                            verify(captureCallback,
                                    timeout(MULTI_FRAME_CAPTURE_IMAGE_TIMEOUT_MS).times(1))
                                    .onCaptureProcessStarted(extensionSession, request);
                            verify(captureCallback,
                                    timeout(MULTI_FRAME_CAPTURE_IMAGE_TIMEOUT_MS).times(1))
                                    .onCaptureSequenceCompleted(extensionSession, sequenceId);
                        }

                        long avgCaptureLatency = (long) Stat.getAverage(captureTimes);
                        String resultFormat = "avg_latency size: " + maxSize.toString() +
                                " image format: " + captureFormat;
                        mReportLog.addValue(resultFormat, avgCaptureLatency,
                                ResultType.LOWER_BETTER, ResultUnit.MS);

                        verify(captureCallback, times(0))
                                .onCaptureSequenceAborted(any(CameraExtensionSession.class),
                                        anyInt());
                        verify(captureCallback, times(0))
                                .onCaptureFailed(any(CameraExtensionSession.class),
                                        any(CaptureRequest.class));
                        Range<Long> latencyRange =
                                extensionChars.getEstimatedCaptureLatencyRangeMillis(extension,
                                        maxSize, captureFormat);
                        if (latencyRange != null) {
                            String msg = String.format("Camera [%s]: The measured average "
                                            + "capture latency of %d ms. for extension type %d  "
                                            + "with image format: %d and size: %dx%d must be "
                                            + "within the advertised range of [%d, %d] ms.",
                                    id, avgCaptureLatency, extension, captureFormat,
                                    maxSize.getWidth(), maxSize.getHeight(),
                                    latencyRange.getLower(), latencyRange.getUpper());
                            assertTrue(msg, latencyRange.contains(avgCaptureLatency));
                        }


                        extensionSession.close();

                        sessionListener.getStateWaiter().waitForState(
                                BlockingExtensionSessionCallback.SESSION_CLOSED,
                                SESSION_CLOSE_TIMEOUT_MS);
                    } finally {
                        mTestRule.closeDevice(id);
                        extensionImageReader.close();
                        mReportLog.submit(InstrumentationRegistry.getInstrumentation());
                    }
                }
            }
        }
    }

    // Verify concurrent extension sessions behavior
    @Test
    public void testConcurrentSessions() throws Exception {
        Set<Set<String>> concurrentCameraIdSet =
                mTestRule.getCameraManager().getConcurrentCameraIds();
        if (concurrentCameraIdSet.isEmpty()) {
            return;
        }

        for (String id : mCameraIdsUnderTest) {
            StaticMetadata staticMeta =
                    new StaticMetadata(mTestRule.getCameraManager().getCameraCharacteristics(id));
            if (!staticMeta.isColorOutputSupported()) {
                continue;
            }
            CameraExtensionCharacteristics extensionChars =
                    mTestRule.getCameraManager().getCameraExtensionCharacteristics(id);
            List<Integer> supportedExtensions = extensionChars.getSupportedExtensions();
            if (supportedExtensions.isEmpty()) {
                continue;
            }

            Set<String> concurrentCameraIds = null;
            for (Set<String> entry : concurrentCameraIdSet) {
                if (entry.contains(id)) {
                    concurrentCameraIds = entry;
                    break;
                }
            }
            if (concurrentCameraIds == null) {
                continue;
            }

            String concurrentCameraId = null;
            CameraExtensionCharacteristics concurrentExtensionChars = null;
            for (String entry : concurrentCameraIds) {
                if (entry.equals(id)) {
                    continue;
                }
                if (!(new StaticMetadata(mTestRule.getCameraManager().getCameraCharacteristics(
                        entry))).isColorOutputSupported()) {
                    continue;
                }
                CameraExtensionCharacteristics chars =
                        mTestRule.getCameraManager().getCameraExtensionCharacteristics(entry);
                if (chars.getSupportedExtensions().isEmpty()) {
                    continue;
                }
                concurrentCameraId = entry;
                concurrentExtensionChars = chars;
                break;
            }
            if ((concurrentCameraId == null) || (concurrentExtensionChars == null)) {
                continue;
            }

            updatePreviewSurfaceTexture();
            int extensionId = supportedExtensions.get(0);
            List<Size> extensionSizes = extensionChars.getExtensionSupportedSizes(extensionId,
                    mSurfaceTexture.getClass());
            Size maxSize = CameraTestUtils.getMaxSize(extensionSizes.toArray(new Size[0]));
            mSurfaceTexture.setDefaultBufferSize(maxSize.getWidth(), maxSize.getHeight());
            OutputConfiguration outputConfig = new OutputConfiguration(
                    OutputConfiguration.SURFACE_GROUP_ID_NONE,
                    new Surface(mSurfaceTexture));
            List<OutputConfiguration> outputConfigs = new ArrayList<>();
            outputConfigs.add(outputConfig);

            BlockingExtensionSessionCallback sessionListener =
                    new BlockingExtensionSessionCallback(
                            mock(CameraExtensionSession.StateCallback.class));
            ExtensionSessionConfiguration configuration =
                    new ExtensionSessionConfiguration(extensionId, outputConfigs,
                            new HandlerExecutor(mTestRule.getHandler()), sessionListener);

            CameraDevice concurrentCameraDevice = null;
            ImageReader extensionImageReader = null;
            try {
                mTestRule.openDevice(id);
                CameraDevice camera = mTestRule.getCamera();
                camera.createExtensionSession(configuration);
                CameraExtensionSession extensionSession = sessionListener.waitAndGetSession(
                        SESSION_CONFIGURE_TIMEOUT_MS);
                assertNotNull(extensionSession);

                concurrentCameraDevice = CameraTestUtils.openCamera(mTestRule.getCameraManager(),
                        concurrentCameraId, new BlockingStateCallback(), mTestRule.getHandler());
                assertNotNull(concurrentCameraDevice);
                int concurrentExtensionId =
                        concurrentExtensionChars.getSupportedExtensions().get(0);
                List<Size> captureSizes = concurrentExtensionChars.getExtensionSupportedSizes(
                        concurrentExtensionId, mSurfaceTexture.getClass());
                assertFalse("No SurfaceTexture output supported", captureSizes.isEmpty());
                Size captureMaxSize =
                        CameraTestUtils.getMaxSize(captureSizes.toArray(new Size[0]));

                extensionImageReader = ImageReader.newInstance(
                        captureMaxSize.getWidth(), captureMaxSize.getHeight(), ImageFormat.PRIVATE,
                        /*maxImages*/ 1, HardwareBuffer.USAGE_GPU_SAMPLED_IMAGE);
                Surface imageReaderSurface = extensionImageReader.getSurface();
                OutputConfiguration readerOutput = new OutputConfiguration(
                        OutputConfiguration.SURFACE_GROUP_ID_NONE, imageReaderSurface);
                outputConfigs = new ArrayList<>();
                outputConfigs.add(readerOutput);
                CameraExtensionSession.StateCallback mockSessionListener =
                        mock(CameraExtensionSession.StateCallback.class);
                ExtensionSessionConfiguration concurrentConfiguration =
                        new ExtensionSessionConfiguration(concurrentExtensionId, outputConfigs,
                                new HandlerExecutor(mTestRule.getHandler()),
                                mockSessionListener);
                concurrentCameraDevice.createExtensionSession(concurrentConfiguration);
                // Trying to initialize multiple concurrent extension sessions is expected to fail
                verify(mockSessionListener, timeout(SESSION_CONFIGURE_TIMEOUT_MS).times(1))
                        .onConfigureFailed(any(CameraExtensionSession.class));
                verify(mockSessionListener, times(0)).onConfigured(
                        any(CameraExtensionSession.class));

                extensionSession.close();
                sessionListener.getStateWaiter().waitForState(
                        BlockingExtensionSessionCallback.SESSION_CLOSED,
                        SESSION_CLOSE_TIMEOUT_MS);

                // Initialization of another extension session must now be possible
                BlockingExtensionSessionCallback concurrentSessionListener =
                        new BlockingExtensionSessionCallback(
                                mock(CameraExtensionSession.StateCallback.class));
                concurrentConfiguration = new ExtensionSessionConfiguration(concurrentExtensionId,
                        outputConfigs, new HandlerExecutor(mTestRule.getHandler()),
                        concurrentSessionListener);
                concurrentCameraDevice.createExtensionSession(concurrentConfiguration);
                extensionSession = concurrentSessionListener.waitAndGetSession(
                        SESSION_CONFIGURE_TIMEOUT_MS);
                assertNotNull(extensionSession);
                extensionSession.close();
                concurrentSessionListener.getStateWaiter().waitForState(
                        BlockingExtensionSessionCallback.SESSION_CLOSED,
                        SESSION_CLOSE_TIMEOUT_MS);
            } finally {
                mTestRule.closeDevice(id);
                if (concurrentCameraDevice != null) {
                    concurrentCameraDevice.close();
                }
                if (extensionImageReader != null) {
                    extensionImageReader.close();
                }
            }
        }
    }

    // Test case combined repeating with multi frame capture on all supported extensions.
    // Verify still frame output.
    @Test
    public void testRepeatingAndCaptureCombined() throws Exception {
        for (String id : mCameraIdsUnderTest) {
            StaticMetadata staticMeta =
                    new StaticMetadata(mTestRule.getCameraManager().getCameraCharacteristics(id));
            if (!staticMeta.isColorOutputSupported()) {
                continue;
            }
            updatePreviewSurfaceTexture();
            CameraExtensionCharacteristics extensionChars =
                    mTestRule.getCameraManager().getCameraExtensionCharacteristics(id);
            List<Integer> supportedExtensions = extensionChars.getSupportedExtensions();
            for (Integer extension : supportedExtensions) {
                int captureFormat = ImageFormat.JPEG;
                List<Size> captureSizes = extensionChars.getExtensionSupportedSizes(extension,
                        captureFormat);
                assertFalse("No Jpeg output supported", captureSizes.isEmpty());
                Size captureMaxSize =
                        CameraTestUtils.getMaxSize(captureSizes.toArray(new Size[0]));

                SimpleImageReaderListener imageListener = new SimpleImageReaderListener(false
                        , 1);
                ImageReader extensionImageReader = CameraTestUtils.makeImageReader(
                        captureMaxSize, captureFormat, /*maxImages*/ 1, imageListener,
                        mTestRule.getHandler());
                Surface imageReaderSurface = extensionImageReader.getSurface();
                OutputConfiguration readerOutput = new OutputConfiguration(
                        OutputConfiguration.SURFACE_GROUP_ID_NONE, imageReaderSurface);
                List<OutputConfiguration> outputConfigs = new ArrayList<>();
                outputConfigs.add(readerOutput);

                // Pick a supported preview/repeating size with aspect ratio close to the
                // multi-frame capture size
                List<Size> repeatingSizes = extensionChars.getExtensionSupportedSizes(extension,
                        mSurfaceTexture.getClass());
                Size maxRepeatingSize =
                        CameraTestUtils.getMaxSize(repeatingSizes.toArray(new Size[0]));
                List<Size> previewSizes = getSupportedPreviewSizes(id,
                        mTestRule.getCameraManager(),
                        getPreviewSizeBound(mTestRule.getWindowManager(), PREVIEW_SIZE_BOUND));
                List<Size> supportedPreviewSizes =
                        previewSizes.stream().filter(repeatingSizes::contains).collect(
                                Collectors.toList());
                if (!supportedPreviewSizes.isEmpty()) {
                    float targetAr =
                            ((float) captureMaxSize.getWidth()) / captureMaxSize.getHeight();
                    for (Size s : supportedPreviewSizes) {
                        float currentAr = ((float) s.getWidth()) / s.getHeight();
                        if (Math.abs(targetAr - currentAr) < 0.01) {
                            maxRepeatingSize = s;
                            break;
                        }
                    }
                }

                mSurfaceTexture.setDefaultBufferSize(maxRepeatingSize.getWidth(),
                        maxRepeatingSize.getHeight());
                Surface texturedSurface = new Surface(mSurfaceTexture);
                outputConfigs.add(new OutputConfiguration(
                        OutputConfiguration.SURFACE_GROUP_ID_NONE, texturedSurface));

                BlockingExtensionSessionCallback sessionListener =
                        new BlockingExtensionSessionCallback(mock(
                                CameraExtensionSession.StateCallback.class));
                ExtensionSessionConfiguration configuration =
                        new ExtensionSessionConfiguration(extension, outputConfigs,
                                new HandlerExecutor(mTestRule.getHandler()),
                                sessionListener);
                try {
                    mTestRule.openDevice(id);
                    CameraDevice camera = mTestRule.getCamera();
                    camera.createExtensionSession(configuration);
                    CameraExtensionSession extensionSession =
                            sessionListener.waitAndGetSession(
                                    SESSION_CONFIGURE_TIMEOUT_MS);
                    assertNotNull(extensionSession);

                    CaptureRequest.Builder captureBuilder =
                            mTestRule.getCamera().createCaptureRequest(
                                    android.hardware.camera2.CameraDevice.TEMPLATE_PREVIEW);
                    captureBuilder.addTarget(texturedSurface);
                    CameraExtensionSession.ExtensionCaptureCallback repeatingCallbackMock =
                            mock(CameraExtensionSession.ExtensionCaptureCallback.class);
                    SimpleCaptureCallback repeatingCaptureCallback =
                            new SimpleCaptureCallback(repeatingCallbackMock);
                    CaptureRequest repeatingRequest = captureBuilder.build();
                    int repeatingSequenceId =
                            extensionSession.setRepeatingRequest(repeatingRequest,
                                    new HandlerExecutor(mTestRule.getHandler()),
                                    repeatingCaptureCallback);

                    Thread.sleep(REPEATING_REQUEST_TIMEOUT_MS);

                    verify(repeatingCallbackMock, atLeastOnce())
                            .onCaptureStarted(eq(extensionSession), eq(repeatingRequest),
                                    anyLong());
                    verify(repeatingCallbackMock, atLeastOnce())
                            .onCaptureProcessStarted(extensionSession, repeatingRequest);

                    captureBuilder = mTestRule.getCamera().createCaptureRequest(
                            android.hardware.camera2.CameraDevice.TEMPLATE_STILL_CAPTURE);
                    captureBuilder.addTarget(imageReaderSurface);
                    CameraExtensionSession.ExtensionCaptureCallback captureCallback =
                            mock(CameraExtensionSession.ExtensionCaptureCallback.class);

                    CaptureRequest captureRequest = captureBuilder.build();
                    int captureSequenceId = extensionSession.capture(captureRequest,
                            new HandlerExecutor(mTestRule.getHandler()), captureCallback);

                    Image img =
                            imageListener.getImage(MULTI_FRAME_CAPTURE_IMAGE_TIMEOUT_MS);
                    validateImage(img, captureMaxSize.getWidth(),
                            captureMaxSize.getHeight(), captureFormat, null);
                    img.close();

                    verify(captureCallback, times(1))
                            .onCaptureStarted(eq(extensionSession), eq(captureRequest),
                                    anyLong());
                    verify(captureCallback, timeout(MULTI_FRAME_CAPTURE_IMAGE_TIMEOUT_MS).times(1))
                            .onCaptureProcessStarted(extensionSession, captureRequest);
                    verify(captureCallback, timeout(MULTI_FRAME_CAPTURE_IMAGE_TIMEOUT_MS).times(1))
                            .onCaptureSequenceCompleted(extensionSession,
                                    captureSequenceId);
                    verify(captureCallback, times(0))
                            .onCaptureSequenceAborted(any(CameraExtensionSession.class),
                                    anyInt());
                    verify(captureCallback, times(0))
                            .onCaptureFailed(any(CameraExtensionSession.class),
                                    any(CaptureRequest.class));

                    extensionSession.stopRepeating();

                    verify(repeatingCallbackMock,
                            timeout(MULTI_FRAME_CAPTURE_IMAGE_TIMEOUT_MS).times(1))
                            .onCaptureSequenceCompleted(extensionSession, repeatingSequenceId);

                    verify(repeatingCallbackMock, times(0))
                            .onCaptureSequenceAborted(any(CameraExtensionSession.class),
                                    anyInt());

                    extensionSession.close();

                    sessionListener.getStateWaiter().waitForState(
                            BlockingExtensionSessionCallback.SESSION_CLOSED,
                            SESSION_CLOSE_TIMEOUT_MS);

                    assertTrue("The sum of onCaptureProcessStarted and onCaptureFailed" +
                                    " callbacks must be greater or equal than the number of calls" +
                                    " to onCaptureStarted!",
                            repeatingCaptureCallback.getTotalFramesArrived() +
                                    repeatingCaptureCallback.getTotalFramesFailed() >=
                            repeatingCaptureCallback.getTotalFramesStarted());
                    assertTrue(String.format("The last repeating request surface timestamp " +
                                    "%d must be less than or equal to the last " +
                                    "onCaptureStarted " +
                                    "timestamp %d", mSurfaceTexture.getTimestamp(),
                            repeatingCaptureCallback.getLastTimestamp()),
                            mSurfaceTexture.getTimestamp() <=
                                    repeatingCaptureCallback.getLastTimestamp());

                } finally {
                    mTestRule.closeDevice(id);
                    texturedSurface.release();
                    extensionImageReader.close();
                }
            }
        }
    }

    private void verifyJpegOrientation(Image img, Size jpegSize, int requestedOrientation)
            throws IOException {
        byte[] blobBuffer = getDataFromImage(img);
        String blobFilename = mTestRule.getDebugFileNameBase() + "/verifyJpegKeys.jpeg";
        dumpFile(blobFilename, blobBuffer);
        ExifInterface exif = new ExifInterface(blobFilename);
        int exifWidth = exif.getAttributeInt(ExifInterface.TAG_IMAGE_WIDTH, /*defaultValue*/0);
        int exifHeight = exif.getAttributeInt(ExifInterface.TAG_IMAGE_LENGTH, /*defaultValue*/0);
        Size exifSize = new Size(exifWidth, exifHeight);
        int exifOrientation = exif.getAttributeInt(ExifInterface.TAG_ORIENTATION,
                /*defaultValue*/ ExifInterface.ORIENTATION_UNDEFINED);
        final int ORIENTATION_MIN = ExifInterface.ORIENTATION_UNDEFINED;
        final int ORIENTATION_MAX = ExifInterface.ORIENTATION_ROTATE_270;
        assertTrue(String.format("Exif orientation must be in range of [%d, %d]",
                ORIENTATION_MIN, ORIENTATION_MAX),
                exifOrientation >= ORIENTATION_MIN && exifOrientation <= ORIENTATION_MAX);

        /**
         * Device captured image doesn't respect the requested orientation,
         * which means it rotates the image buffer physically. Then we
         * should swap the exif width/height accordingly to compare.
         */
        boolean deviceRotatedImage = exifOrientation == ExifInterface.ORIENTATION_UNDEFINED;

        if (deviceRotatedImage) {
            // Case 1.
            boolean needSwap = (requestedOrientation % 180 == 90);
            if (needSwap) {
                exifSize = new Size(exifHeight, exifWidth);
            }
        } else {
            // Case 2.
            assertEquals("Exif orientation should match requested orientation",
                    requestedOrientation, getExifOrientationInDegree(exifOrientation));
        }

        assertEquals("Exif size should match jpeg capture size", jpegSize, exifSize);
    }

    private static int getExifOrientationInDegree(int exifOrientation) {
        switch (exifOrientation) {
            case ExifInterface.ORIENTATION_NORMAL:
                return 0;
            case ExifInterface.ORIENTATION_ROTATE_90:
                return 90;
            case ExifInterface.ORIENTATION_ROTATE_180:
                return 180;
            case ExifInterface.ORIENTATION_ROTATE_270:
                return 270;
            default:
                fail("It is impossible to get non 0, 90, 180, 270 degress exif" +
                        "info based on the request orientation range");
                return -1;
        }
    }

    public static class SimpleCaptureCallback
            extends CameraExtensionSession.ExtensionCaptureCallback {
        private long mLastTimestamp = -1;
        private int mNumFramesArrived = 0;
        private int mNumFramesStarted = 0;
        private int mNumFramesFailed = 0;
        private boolean mNonIncreasingTimestamps = false;
        private final CameraExtensionSession.ExtensionCaptureCallback mProxy;

        public SimpleCaptureCallback(CameraExtensionSession.ExtensionCaptureCallback proxy) {
            mProxy = proxy;
        }

        @Override
        public void onCaptureStarted(CameraExtensionSession session,
                                     CaptureRequest request, long timestamp) {

            if (timestamp < mLastTimestamp) {
                mNonIncreasingTimestamps = true;
            }
            mLastTimestamp = timestamp;
            mNumFramesStarted++;
            if (mProxy != null) {
                mProxy.onCaptureStarted(session, request, timestamp);
            }
        }

        @Override
        public void onCaptureProcessStarted(CameraExtensionSession session,
                                            CaptureRequest request) {
            mNumFramesArrived++;
            if (mProxy != null) {
                mProxy.onCaptureProcessStarted(session, request);
            }
        }

        @Override
        public void onCaptureFailed(CameraExtensionSession session,
                                    CaptureRequest request) {
            mNumFramesFailed++;
            if (mProxy != null) {
                mProxy.onCaptureFailed(session, request);
            }
        }

        @Override
        public void onCaptureSequenceAborted(CameraExtensionSession session,
                                             int sequenceId) {
            if (mProxy != null) {
                mProxy.onCaptureSequenceAborted(session, sequenceId);
            }
        }

        @Override
        public void onCaptureSequenceCompleted(CameraExtensionSession session,
                                               int sequenceId) {
            if (mProxy != null) {
                mProxy.onCaptureSequenceCompleted(session, sequenceId);
            }
        }

        public int getTotalFramesArrived() {
            return mNumFramesArrived;
        }

        public int getTotalFramesStarted() {
            return mNumFramesStarted;
        }

        public int getTotalFramesFailed() {
            return mNumFramesFailed;
        }

        public long getLastTimestamp() throws IllegalStateException {
            if (mNonIncreasingTimestamps) {
                throw new IllegalStateException("Non-monotonically increasing timestamps!");
            }
            return mLastTimestamp;
        }
    }

    @Test
    public void testIllegalArguments() throws Exception {
        for (String id : mCameraIdsUnderTest) {
            StaticMetadata staticMeta =
                    new StaticMetadata(mTestRule.getCameraManager().getCameraCharacteristics(id));
            if (!staticMeta.isColorOutputSupported()) {
                continue;
            }
            updatePreviewSurfaceTexture();
            CameraExtensionCharacteristics extensionChars =
                    mTestRule.getCameraManager().getCameraExtensionCharacteristics(id);
            List<Integer> supportedExtensions = extensionChars.getSupportedExtensions();
            for (Integer extension : supportedExtensions) {
                List<OutputConfiguration> outputConfigs = new ArrayList<>();
                BlockingExtensionSessionCallback sessionListener =
                        new BlockingExtensionSessionCallback(mock(
                                CameraExtensionSession.StateCallback.class));
                ExtensionSessionConfiguration configuration =
                        new ExtensionSessionConfiguration(extension, outputConfigs,
                                new HandlerExecutor(mTestRule.getHandler()),
                                sessionListener);

                try {
                    mTestRule.openDevice(id);
                    CameraDevice camera = mTestRule.getCamera();
                    try {
                        camera.createExtensionSession(configuration);
                        fail("should get IllegalArgumentException due to absent output surfaces");
                    } catch (IllegalArgumentException e) {
                        // Expected, we can proceed further
                    }

                    int captureFormat = ImageFormat.YUV_420_888;
                    List<Size> captureSizes = extensionChars.getExtensionSupportedSizes(extension,
                            captureFormat);
                    if (captureSizes.isEmpty()) {
                        captureFormat = ImageFormat.JPEG;
                        captureSizes = extensionChars.getExtensionSupportedSizes(extension,
                                captureFormat);
                    }
                    Size captureMaxSize =
                            CameraTestUtils.getMaxSize(captureSizes.toArray(new Size[0]));

                    mSurfaceTexture.setDefaultBufferSize(1, 1);
                    Surface texturedSurface = new Surface(mSurfaceTexture);
                    outputConfigs.add(new OutputConfiguration(
                            OutputConfiguration.SURFACE_GROUP_ID_NONE, texturedSurface));
                    configuration = new ExtensionSessionConfiguration(extension, outputConfigs,
                            new HandlerExecutor(mTestRule.getHandler()), sessionListener);

                    try {
                        camera.createExtensionSession(configuration);
                        fail("should get IllegalArgumentException due to illegal repeating request"
                                + " output surface");
                    } catch (IllegalArgumentException e) {
                        // Expected, we can proceed further
                    } finally {
                        outputConfigs.clear();
                    }

                    SimpleImageReaderListener imageListener = new SimpleImageReaderListener(false,
                            1);
                    Size invalidCaptureSize = new Size(1, 1);
                    ImageReader extensionImageReader = CameraTestUtils.makeImageReader(
                            invalidCaptureSize, captureFormat, /*maxImages*/ 1,
                            imageListener, mTestRule.getHandler());
                    Surface imageReaderSurface = extensionImageReader.getSurface();
                    OutputConfiguration readerOutput = new OutputConfiguration(
                            OutputConfiguration.SURFACE_GROUP_ID_NONE, imageReaderSurface);
                    outputConfigs.add(readerOutput);
                    configuration = new ExtensionSessionConfiguration(extension, outputConfigs,
                            new HandlerExecutor(mTestRule.getHandler()), sessionListener);

                    try{
                        camera.createExtensionSession(configuration);
                        fail("should get IllegalArgumentException due to illegal multi-frame"
                                + " request output surface");
                    } catch (IllegalArgumentException e) {
                        // Expected, we can proceed further
                    } finally {
                        outputConfigs.clear();
                        extensionImageReader.close();
                    }

                    // Pick a supported preview/repeating size with aspect ratio close to the
                    // multi-frame capture size
                    List<Size> repeatingSizes = extensionChars.getExtensionSupportedSizes(extension,
                            mSurfaceTexture.getClass());
                    Size maxRepeatingSize =
                            CameraTestUtils.getMaxSize(repeatingSizes.toArray(new Size[0]));
                    List<Size> previewSizes = getSupportedPreviewSizes(id,
                            mTestRule.getCameraManager(),
                            getPreviewSizeBound(mTestRule.getWindowManager(), PREVIEW_SIZE_BOUND));
                    List<Size> supportedPreviewSizes =
                            previewSizes.stream().filter(repeatingSizes::contains).collect(
                                    Collectors.toList());
                    if (!supportedPreviewSizes.isEmpty()) {
                        float targetAr =
                                ((float) captureMaxSize.getWidth()) / captureMaxSize.getHeight();
                        for (Size s : supportedPreviewSizes) {
                            float currentAr = ((float) s.getWidth()) / s.getHeight();
                            if (Math.abs(targetAr - currentAr) < 0.01) {
                                maxRepeatingSize = s;
                                break;
                            }
                        }
                    }

                    imageListener = new SimpleImageReaderListener(false, 1);
                    extensionImageReader = CameraTestUtils.makeImageReader(captureMaxSize,
                            captureFormat, /*maxImages*/ 1, imageListener, mTestRule.getHandler());
                    imageReaderSurface = extensionImageReader.getSurface();
                    readerOutput = new OutputConfiguration(OutputConfiguration.SURFACE_GROUP_ID_NONE,
                            imageReaderSurface);
                    outputConfigs.add(readerOutput);

                    mSurfaceTexture.setDefaultBufferSize(maxRepeatingSize.getWidth(),
                            maxRepeatingSize.getHeight());
                    texturedSurface = new Surface(mSurfaceTexture);
                    outputConfigs.add(new OutputConfiguration(
                            OutputConfiguration.SURFACE_GROUP_ID_NONE, texturedSurface));

                    configuration = new ExtensionSessionConfiguration(extension, outputConfigs,
                            new HandlerExecutor(mTestRule.getHandler()), sessionListener);
                    camera.createExtensionSession(configuration);
                    CameraExtensionSession extensionSession =
                            sessionListener.waitAndGetSession(
                                    SESSION_CONFIGURE_TIMEOUT_MS);
                    assertNotNull(extensionSession);

                    CaptureRequest.Builder captureBuilder =
                            mTestRule.getCamera().createCaptureRequest(
                                    android.hardware.camera2.CameraDevice.TEMPLATE_PREVIEW);
                    captureBuilder.addTarget(imageReaderSurface);
                    CameraExtensionSession.ExtensionCaptureCallback repeatingCallbackMock =
                            mock(CameraExtensionSession.ExtensionCaptureCallback.class);
                    SimpleCaptureCallback repeatingCaptureCallback =
                            new SimpleCaptureCallback(repeatingCallbackMock);
                    CaptureRequest repeatingRequest = captureBuilder.build();
                    try {
                        extensionSession.setRepeatingRequest(repeatingRequest,
                                new HandlerExecutor(mTestRule.getHandler()),
                                repeatingCaptureCallback);
                        fail("should get IllegalArgumentException due to illegal repeating request"
                                + " output target");
                    } catch (IllegalArgumentException e) {
                        // Expected, we can proceed further
                    }

                    captureBuilder = mTestRule.getCamera().createCaptureRequest(
                            android.hardware.camera2.CameraDevice.TEMPLATE_STILL_CAPTURE);
                    captureBuilder.addTarget(texturedSurface);
                    CameraExtensionSession.ExtensionCaptureCallback captureCallback =
                            mock(CameraExtensionSession.ExtensionCaptureCallback.class);

                    CaptureRequest captureRequest = captureBuilder.build();
                    try {
                        extensionSession.capture(captureRequest,
                                new HandlerExecutor(mTestRule.getHandler()), captureCallback);
                        fail("should get IllegalArgumentException due to illegal multi-frame"
                                + " request output target");
                    } catch (IllegalArgumentException e) {
                        // Expected, we can proceed further
                    }

                    extensionSession.close();

                    sessionListener.getStateWaiter().waitForState(
                            BlockingExtensionSessionCallback.SESSION_CLOSED,
                            SESSION_CLOSE_TIMEOUT_MS);

                    texturedSurface.release();
                    extensionImageReader.close();

                    try {
                        extensionSession.setRepeatingRequest(captureRequest,
                                new HandlerExecutor(mTestRule.getHandler()), captureCallback);
                        fail("should get IllegalStateException due to closed session");
                    } catch (IllegalStateException e) {
                        // Expected, we can proceed further
                    }

                    try {
                        extensionSession.stopRepeating();
                        fail("should get IllegalStateException due to closed session");
                    } catch (IllegalStateException e) {
                        // Expected, we can proceed further
                    }

                    try {
                        extensionSession.capture(captureRequest,
                                new HandlerExecutor(mTestRule.getHandler()), captureCallback);
                        fail("should get IllegalStateException due to closed session");
                    } catch (IllegalStateException e) {
                        // Expected, we can proceed further
                    }
                } finally {
                    mTestRule.closeDevice(id);
                }
            }
        }
    }
}
