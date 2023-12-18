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

import android.content.Context;
import android.graphics.ImageFormat;
import android.graphics.SurfaceTexture;
import android.hardware.camera2.CameraExtensionCharacteristics;
import android.hardware.camera2.cts.helpers.StaticMetadata;
import android.hardware.camera2.cts.testcases.Camera2AndroidTestRule;
import android.renderscript.Allocation;
import android.util.Log;
import android.util.Range;
import android.util.Size;

import androidx.test.InstrumentationRegistry;

import com.android.compatibility.common.util.PropertyUtil;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

import static org.junit.Assert.*;

import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

@RunWith(JUnit4.class)
public class CameraExtensionCharacteristicsTest {
    private static final String TAG = "CameraExtensionManagerTest";
    private static final boolean VERBOSE = Log.isLoggable(TAG, Log.VERBOSE);
    private static final List<Integer> EXTENSIONS = Arrays.asList(
            CameraExtensionCharacteristics.EXTENSION_AUTOMATIC,
            CameraExtensionCharacteristics.EXTENSION_BEAUTY,
            CameraExtensionCharacteristics.EXTENSION_BOKEH,
            CameraExtensionCharacteristics.EXTENSION_HDR,
            CameraExtensionCharacteristics.EXTENSION_NIGHT);

    private final Context mContext = InstrumentationRegistry.getTargetContext();

    @Rule
    public final Camera2AndroidTestRule mTestRule = new Camera2AndroidTestRule(mContext);

    private void openDevice(String cameraId) throws Exception {
        mTestRule.setCamera(CameraTestUtils.openCamera(
                mTestRule.getCameraManager(), cameraId,
                mTestRule.getCameraListener(), mTestRule.getHandler()));
        mTestRule.getCollector().setCameraId(cameraId);
        mTestRule.setStaticInfo(new StaticMetadata(
                mTestRule.getCameraManager().getCameraCharacteristics(cameraId),
                StaticMetadata.CheckLevel.ASSERT, /*collector*/null));
    }

    private <T> void verifySupportedExtension(CameraExtensionCharacteristics chars, String cameraId,
            Integer extension, Class<T> klass) {
        List<Size> availableSizes = chars.getExtensionSupportedSizes(extension, klass);
        assertTrue(String.format("Supported extension %d on camera id: %s doesn't " +
                        "include any valid resolutions!", extension, cameraId),
                (availableSizes != null) && (!availableSizes.isEmpty()));
    }

    private <T> void verifySupportedSizes(CameraExtensionCharacteristics chars, String cameraId,
            Integer extension, Class<T> klass) throws Exception {
        verifySupportedExtension(chars, cameraId, extension, klass);
        try {
            openDevice(cameraId);
            List<Size> extensionSizes = chars.getExtensionSupportedSizes(extension, klass);
            List<Size> cameraSizes = Arrays.asList(
                    mTestRule.getStaticInfo().getAvailableSizesForFormatChecked(ImageFormat.PRIVATE,
                            StaticMetadata.StreamDirection.Output));
            for (Size extensionSize : extensionSizes) {
                assertTrue(String.format("Supported extension %d on camera id: %s advertises " +
                                " resolution %s unsupported by camera", extension, cameraId,
                        extensionSize), cameraSizes.contains(extensionSize));
            }
        } finally {
            mTestRule.closeDevice(cameraId);
        }
    }

    private void verifySupportedSizes(CameraExtensionCharacteristics chars, String cameraId,
            Integer extension, int format) throws Exception {
        List<Size> extensionSizes = chars.getExtensionSupportedSizes(extension, format);
        assertFalse(String.format("No available sizes for extension %d on camera id: %s " +
                "using format: %x", extension, cameraId, format), extensionSizes.isEmpty());
        try {
            openDevice(cameraId);
            List<Size> cameraSizes = Arrays.asList(
                    mTestRule.getStaticInfo().getAvailableSizesForFormatChecked(format,
                            StaticMetadata.StreamDirection.Output));
            for (Size extensionSize : extensionSizes) {
                assertTrue(String.format("Supported extension %d on camera id: %s advertises " +
                                " resolution %s unsupported by camera", extension, cameraId,
                        extensionSize), cameraSizes.contains(extensionSize));
            }
        } finally {
            mTestRule.closeDevice(cameraId);
        }
    }

    private <T> void verifyUnsupportedExtension(CameraExtensionCharacteristics chars,
            Integer extension, Class<T> klass) {
        try {
            chars.getExtensionSupportedSizes(extension, klass);
            fail("should get IllegalArgumentException due to unsupported extension");
        } catch (IllegalArgumentException e) {
            // Expected
        }
    }

    @Test
    public void testExtensionAvailability() throws Exception {
        boolean extensionsAdvertised = false;
        for (String id : mTestRule.getCameraIdsUnderTest()) {
            StaticMetadata staticMeta =
                    new StaticMetadata(mTestRule.getCameraManager().getCameraCharacteristics(id));
            if (!staticMeta.isColorOutputSupported()) {
                continue;
            }
            CameraExtensionCharacteristics extensionChars =
                    mTestRule.getCameraManager().getCameraExtensionCharacteristics(id);
            ArrayList<Integer> unsupportedExtensions = new ArrayList<>(EXTENSIONS);
            List<Integer> supportedExtensions = extensionChars.getSupportedExtensions();
            if (!extensionsAdvertised && !supportedExtensions.isEmpty()) {
                extensionsAdvertised = true;
            }
            for (Integer extension : supportedExtensions) {
                verifySupportedExtension(extensionChars, id, extension, SurfaceTexture.class);
                unsupportedExtensions.remove(extension);
            }

            // Unsupported extension size queries must throw corresponding exception.
            for (Integer extension : unsupportedExtensions) {
                verifyUnsupportedExtension(extensionChars, extension, SurfaceTexture.class);
            }
        }
        boolean extensionsEnabledProp = PropertyUtil.areCameraXExtensionsEnabled();
        assertEquals("Extensions system property : " + extensionsEnabledProp + " does not match " +
                "with the advertised extensions: " + extensionsAdvertised, extensionsEnabledProp,
                extensionsAdvertised);
    }

    @Test
    public void testExtensionSizes() throws Exception {
        for (String id : mTestRule.getCameraIdsUnderTest()) {
            StaticMetadata staticMeta =
                    new StaticMetadata(mTestRule.getCameraManager().getCameraCharacteristics(id));
            if (!staticMeta.isColorOutputSupported()) {
                continue;
            }
            CameraExtensionCharacteristics extensionChars =
                    mTestRule.getCameraManager().getCameraExtensionCharacteristics(id);
            List<Integer> supportedExtensions = extensionChars.getSupportedExtensions();
            for (Integer extension : supportedExtensions) {
                verifySupportedSizes(extensionChars, id, extension, SurfaceTexture.class);
                verifySupportedSizes(extensionChars, id, extension, ImageFormat.JPEG);
            }
        }
    }

    @Test
    public void testIllegalArguments() throws Exception {
        try {
            mTestRule.getCameraManager().getCameraExtensionCharacteristics("InvalidCameraId!");
            fail("should get IllegalArgumentException due to invalid camera id");
        } catch (IllegalArgumentException e) {
            // Expected
        }

        for (String id : mTestRule.getCameraIdsUnderTest()) {
            CameraExtensionCharacteristics extensionChars =
                    mTestRule.getCameraManager().getCameraExtensionCharacteristics(id);
            List<Integer> supportedExtensions = extensionChars.getSupportedExtensions();
            for (Integer extension : supportedExtensions) {
                try {
                    extensionChars.getExtensionSupportedSizes(extension, ImageFormat.UNKNOWN);
                    fail("should get IllegalArgumentException due to invalid pixel format");
                } catch (IllegalArgumentException e) {
                    // Expected
                }

                try {
                    List<Size> ret = extensionChars.getExtensionSupportedSizes(extension,
                            Allocation.class);
                    assertTrue("should get empty resolution list for unsupported " +
                            "surface type", ret.isEmpty());
                } catch (IllegalArgumentException e) {
                    fail("should not get IllegalArgumentException due to unsupported surface " +
                            "type");
                }
            }
        }
    }

    @Test
    public void testExtensionLatencyRanges() throws Exception {
        final int testFormat = ImageFormat.JPEG;
        for (String id : mTestRule.getCameraIdsUnderTest()) {
            StaticMetadata staticMeta =
                    new StaticMetadata(mTestRule.getCameraManager().getCameraCharacteristics(id));
            if (!staticMeta.isColorOutputSupported()) {
                continue;
            }

            CameraExtensionCharacteristics chars =
                    mTestRule.getCameraManager().getCameraExtensionCharacteristics(id);
            List<Integer> supportedExtensions = chars.getSupportedExtensions();
            for (Integer extension : supportedExtensions) {
                List<Size> extensionSizes = chars.getExtensionSupportedSizes(extension, testFormat);
                for (Size sz : extensionSizes) {
                    Range<Long> latencyRange = chars.getEstimatedCaptureLatencyRangeMillis(
                            extension, sz, testFormat);
                    if (latencyRange != null) {
                        assertTrue("Negative range surface type", (latencyRange.getLower() > 0) &&
                                (latencyRange.getUpper() > 0));
                        assertTrue("Lower range value must be smaller compared to the upper",
                                (latencyRange.getLower() < latencyRange.getUpper()));
                    }
                }
            }
        }
    }
}
