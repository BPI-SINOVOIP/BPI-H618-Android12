/*
 * Copyright 2021 The Android Open Source Project
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

package android.camera.cts.api31test;

import static android.hardware.camera2.cts.CameraTestUtils.*;

import android.content.Context;
import android.graphics.ImageFormat;
import android.hardware.camera2.CameraDevice;
import android.hardware.camera2.CameraManager;
import android.hardware.camera2.CameraCaptureSession;
import android.hardware.camera2.CameraCharacteristics;
import android.hardware.camera2.CaptureFailure;
import android.hardware.camera2.CaptureRequest;
import android.hardware.camera2.cts.CameraTestUtils;
import android.hardware.camera2.cts.CameraTestUtils.SimpleImageReaderListener;
import android.hardware.camera2.cts.helpers.CameraErrorCollector;
import android.hardware.camera2.cts.helpers.StaticMetadata;
import android.hardware.camera2.cts.helpers.StaticMetadata.CheckLevel;
import android.hardware.camera2.params.OutputConfiguration;
import android.hardware.camera2.params.SessionConfiguration;
import android.hardware.camera2.TotalCaptureResult;
import android.media.Image;
import android.media.ImageReader;
import android.os.Handler;
import android.os.HandlerThread;
import android.test.AndroidTestCase;
import android.util.Log;
import android.util.Size;
import android.view.Surface;

import com.android.compatibility.common.util.CddTest;
import com.android.ex.camera2.blocking.BlockingSessionCallback;

import java.util.ArrayList;
import java.util.List;

import org.junit.Test;

import static junit.framework.Assert.*;
import static org.mockito.Mockito.*;

public class SPerfClassTest extends AndroidTestCase {
    private static final String TAG = "SPerfClassTest";
    private static final boolean VERBOSE = Log.isLoggable(TAG, Log.VERBOSE);

    private static final Size FULLHD = new Size(1920, 1080);
    private static final Size VGA = new Size(640, 480);
    private static final int CONFIGURE_TIMEOUT = 5000; //ms
    private static final int CAPTURE_TIMEOUT = 1500; //ms

    private CameraManager mCameraManager;
    private String[] mCameraIds;
    private Handler mHandler;
    private HandlerThread mHandlerThread;
    private CameraErrorCollector mCollector;

    @Override
    public void setContext(Context context) {
        super.setContext(context);
        mCameraManager = context.getSystemService(CameraManager.class);
        assertNotNull("Can't connect to camera manager!", mCameraManager);
    }

    @Override
    protected void setUp() throws Exception {
        super.setUp();

        mCameraIds = mCameraManager.getCameraIdList();
        assertNotNull("Camera ids shouldn't be null", mCameraIds);

        mHandlerThread = new HandlerThread(TAG);
        mHandlerThread.start();
        mHandler = new Handler(mHandlerThread.getLooper());

        mCollector = new CameraErrorCollector();
    }

    @Override
    protected void tearDown() throws Exception {
        mHandlerThread.quitSafely();
        mHandler = null;

        try {
            mCollector.verify();
        } catch (Throwable e) {
            // When new Exception(e) is used, exception info will be printed twice.
            throw new Exception(e.getMessage());
        } finally {
            super.tearDown();
        }
    }

    // Verify primary camera devices's supported JPEG sizes are at least 1080p.
    private void testSPerfClassJpegSizesByCamera(String cameraId) throws Exception {
        boolean isPrimaryRear = CameraTestUtils.isPrimaryRearFacingCamera(
                mCameraManager, cameraId);
        boolean isPrimaryFront = CameraTestUtils.isPrimaryFrontFacingCamera(
                mCameraManager, cameraId);
        if (!isPrimaryRear && !isPrimaryFront) {
            return;
        }

        CameraCharacteristics c = mCameraManager.getCameraCharacteristics(cameraId);
        StaticMetadata staticInfo = new StaticMetadata(c, CheckLevel.ASSERT, mCollector);

        Size[] jpegSizes = staticInfo.getJpegOutputSizesChecked();
        assertTrue("Primary cameras must support JPEG formats",
                jpegSizes != null && jpegSizes.length > 0);
        for (Size jpegSize : jpegSizes) {
            mCollector.expectTrue(
                    "Primary camera's JPEG size must be at least 1080p, but is " +
                    jpegSize,
                    jpegSize.getWidth() >= FULLHD.getWidth() &&
                    jpegSize.getHeight() >= FULLHD.getHeight());
        }

        CameraDevice camera = null;
        ImageReader jpegTarget = null;
        Image image = null;
        try {
            camera = CameraTestUtils.openCamera(mCameraManager, cameraId,
                    /*listener*/null, mHandler);

            List<OutputConfiguration> outputConfigs = new ArrayList<>();
            SimpleImageReaderListener imageListener = new SimpleImageReaderListener();
            jpegTarget = CameraTestUtils.makeImageReader(VGA,
                    ImageFormat.JPEG, 1 /*maxNumImages*/, imageListener, mHandler);
            Surface jpegSurface = jpegTarget.getSurface();
            outputConfigs.add(new OutputConfiguration(jpegSurface));

            // isSessionConfigurationSupported will return true for JPEG sizes smaller
            // than 1080P, due to framework rouding up to closest supported size (1080p).
            SessionConfigSupport sessionConfigSupport = isSessionConfigSupported(
                    camera, mHandler, outputConfigs, /*inputConfig*/ null,
                    SessionConfiguration.SESSION_REGULAR, true/*defaultSupport*/);
            mCollector.expectTrue("isSessionConfiguration fails with error",
                    !sessionConfigSupport.error);
            mCollector.expectTrue("isSessionConfiguration returns false for JPEG < 1080p",
                    sessionConfigSupport.configSupported);

            // Session creation for JPEG sizes smaller than 1080p will succeed, and the
            // result JPEG image dimension is rounded up to closest supported size (1080p).
            CaptureRequest.Builder request =
                    camera.createCaptureRequest(CameraDevice.TEMPLATE_STILL_CAPTURE);
            request.addTarget(jpegSurface);


            CameraCaptureSession.StateCallback sessionListener =
                    mock(CameraCaptureSession.StateCallback.class);
            CameraCaptureSession session = configureCameraSessionWithConfig(
                    camera, outputConfigs, sessionListener, mHandler);

            verify(sessionListener, timeout(CONFIGURE_TIMEOUT).atLeastOnce()).
                    onConfigured(any(CameraCaptureSession.class));
            verify(sessionListener, timeout(CONFIGURE_TIMEOUT).atLeastOnce()).
                    onReady(any(CameraCaptureSession.class));
            verify(sessionListener, never()).onConfigureFailed(any(CameraCaptureSession.class));
            verify(sessionListener, never()).onActive(any(CameraCaptureSession.class));
            verify(sessionListener, never()).onClosed(any(CameraCaptureSession.class));

            CameraCaptureSession.CaptureCallback captureListener =
                    mock(CameraCaptureSession.CaptureCallback.class);
            session.capture(request.build(), captureListener, mHandler);

            verify(captureListener, timeout(CAPTURE_TIMEOUT).atLeastOnce()).
                    onCaptureCompleted(any(CameraCaptureSession.class),
                            any(CaptureRequest.class), any(TotalCaptureResult.class));
            verify(captureListener, never()).onCaptureFailed(any(CameraCaptureSession.class),
                    any(CaptureRequest.class), any(CaptureFailure.class));

            image = imageListener.getImage(CAPTURE_TIMEOUT);
            assertNotNull("Image must be valid", image);
            assertEquals("Image format isn't JPEG", image.getFormat(), ImageFormat.JPEG);

            byte[] data = CameraTestUtils.getDataFromImage(image);
            assertTrue("Invalid image data", data != null && data.length > 0);

            CameraTestUtils.validateJpegData(data, FULLHD.getWidth(), FULLHD.getHeight(),
                    null /*filePath*/);
        } finally {
            if (camera != null) {
                camera.close();
            }
            if (jpegTarget != null) {
                jpegTarget.close();
            }
            if (image != null) {
                image.close();
            }
        }
    }

    /**
     * Check camera S Performance class requirement for JPEG sizes.
     */
    @CddTest(requirement="7.5/H-1-8")
    public void testSPerfClassJpegSizes() throws Exception {
        boolean isSPerfClass = CameraTestUtils.isSPerfClass();
        if (!isSPerfClass) {
            return;
        }

        for (int i = 0; i < mCameraIds.length; i++) {
            testSPerfClassJpegSizesByCamera(mCameraIds[i]);
        }
    }
}
