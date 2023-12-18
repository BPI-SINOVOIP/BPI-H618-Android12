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

package android.hardware.camera2.cts;

import android.graphics.ImageFormat;
import android.hardware.camera2.cts.CameraTestUtils;
import android.hardware.camera2.cts.helpers.StaticMetadata;
import android.hardware.camera2.cts.testcases.Camera2AndroidTestCase;
import android.hardware.camera2.params.OutputConfiguration;
import android.hardware.camera2.CameraDevice;
import android.hardware.camera2.CaptureRequest;
import android.hardware.camera2.CaptureResult;
import android.media.Image;
import android.media.ImageReader;
import android.os.Build;
import android.os.ConditionVariable;
import android.util.Log;
import android.util.Size;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

import com.android.compatibility.common.util.PropertyUtil;

import org.junit.runner.RunWith;
import org.junit.runners.Parameterized;
import org.junit.Test;

import static android.hardware.camera2.cts.CameraTestUtils.CAPTURE_RESULT_TIMEOUT_MS;
import static android.hardware.camera2.cts.CameraTestUtils.SimpleCaptureCallback;
import static junit.framework.Assert.*;

/**
 * <p>Basic test for image capture using CONTROL_ZOOM_RATIO. It uses CameraDevice as
 * producer, and camera sends image data to an imageReader. Image formats
 * being tested are JPEG and RAW.</p>
 */
@RunWith(Parameterized.class)
public class ZoomCaptureTest extends Camera2AndroidTestCase {
    private static final String TAG = "ZoomCaptureTest";
    private static final boolean VERBOSE = Log.isLoggable(TAG, Log.VERBOSE);

    private SimpleImageListener mListener;

    @Override
    public void setUp() throws Exception {
        super.setUp();
    }

    @Override
    public void tearDown() throws Exception {
        super.tearDown();
    }

    @Test
    public void testJpegZoomCapture() throws Exception {
        for (String id : mCameraIdsUnderTest) {
            try {
                Log.v(TAG, "Testing jpeg zoom capture for Camera " + id);
                openDevice(id);
                bufferFormatZoomTestByCamera(ImageFormat.JPEG);
            } finally {
                closeDevice(id);
            }
        }
    }

    @Test
    public void testRawZoomCapture() throws Exception {
        for (String id : mCameraIdsUnderTest) {
            try {
                Log.v(TAG, "Testing raw zoom capture for camera " + id);
                openDevice(id);

                bufferFormatZoomTestByCamera(ImageFormat.RAW_SENSOR);
            } finally {
                closeDevice(id);
            }
        }
    }

    private void bufferFormatZoomTestByCamera(int format) throws Exception {
        Size[] availableSizes = mStaticInfo.getAvailableSizesForFormatChecked(format,
                StaticMetadata.StreamDirection.Output);
        if (availableSizes.length == 0) {
            return;
        }

        List<Float> candidateZoomRatios = CameraTestUtils.getCandidateZoomRatios(mStaticInfo);
        Set<String> physicalCameraIds = null;
        if (mStaticInfo.isLogicalMultiCamera()) {
            physicalCameraIds = mStaticInfo.getCharacteristics().getPhysicalCameraIds();
        }
        try {
            mListener  = new SimpleImageListener();
            // Pick the largest image size:
            Size maxSize = CameraTestUtils.getMaxSize(availableSizes);
            createDefaultImageReader(maxSize, format, 1, mListener);

            checkImageReaderSessionConfiguration(
                    "Camera capture session validation for format: " + format + "failed");

            ArrayList<OutputConfiguration> outputConfigs = new ArrayList<>();
            OutputConfiguration config = new OutputConfiguration(mReader.getSurface());
            outputConfigs.add(config);

            CaptureRequest.Builder requestBuilder = prepareCaptureRequestForConfigs(
                    outputConfigs, CameraDevice.TEMPLATE_PREVIEW);

            Set<String> activePhysicalIdsSeen = new HashSet<String>();
            boolean checkActivePhysicalIdConsistency =
                    PropertyUtil.getFirstApiLevel() >= Build.VERSION_CODES.S;
            for (Float zoomRatio : candidateZoomRatios) {
                if (VERBOSE) {
                    Log.v(TAG, "Testing format " + format + " zoomRatio " + zoomRatio +
                            " for camera " + mCamera.getId());
                }

                requestBuilder.set(CaptureRequest.CONTROL_ZOOM_RATIO, zoomRatio);
                CaptureRequest request = requestBuilder.build();

                SimpleCaptureCallback listener = new SimpleCaptureCallback();
                startCapture(request, false /*repeating*/, listener, mHandler);

                // Validate images.
                mListener.waitForAnyImageAvailable(CAPTURE_WAIT_TIMEOUT_MS);
                Image img = mReader.acquireNextImage();
                assertNotNull("Unable to acquire the latest image", img);
                CameraTestUtils.validateImage(img, maxSize.getWidth(), maxSize.getHeight(), format,
                        mDebugFileNameBase);
                img.close();

                // Validate capture result.
                if (mStaticInfo.isActivePhysicalCameraIdSupported()) {
                    CaptureResult result = listener.getCaptureResult(CAPTURE_RESULT_TIMEOUT_MS);
                    String activePhysicalId = result.get(
                            CaptureResult.LOGICAL_MULTI_CAMERA_ACTIVE_PHYSICAL_ID);
                    if (checkActivePhysicalIdConsistency) {
                        assertNotNull("Camera " + mCamera.getId() +
                                " result metadata must contain ACTIVE_PHYSICAL_ID",
                                activePhysicalId);
                        assertTrue("Camera " + mCamera.getId() + " must be logical " +
                                "camera if activePhysicalId exists in capture result",
                                physicalCameraIds != null && physicalCameraIds.size() != 0);
                        mCollector.expectTrue("Camera " + mCamera.getId() + "  activePhysicalId " +
                                activePhysicalId + "must be among valid physical Ids "  +
                                physicalCameraIds.toString(),
                                physicalCameraIds.contains(activePhysicalId));

                        activePhysicalIdsSeen.add(activePhysicalId);
                    }
                }
            }
            // stop capture.
            stopCapture(/*fast*/false);

            if (activePhysicalIdsSeen.size() > 0 && format == ImageFormat.RAW_SENSOR) {
                mCollector.expectTrue("Logical camera's activePhysicalCamera should not " +
                        " change at different zoom levels.", activePhysicalIdsSeen.size() == 1);
            }
        } finally {
            closeDefaultImageReader();
        }
    }

    private final class SimpleImageListener implements ImageReader.OnImageAvailableListener {
        private final ConditionVariable imageAvailable = new ConditionVariable();
        @Override
        public void onImageAvailable(ImageReader reader) {
            imageAvailable.open();
        }

        public void waitForAnyImageAvailable(long timeout) {
            if (imageAvailable.block(timeout)) {
                imageAvailable.close();
            } else {
                fail("wait for image available timed out after " + timeout + "ms");
            }
        }
    }
}
