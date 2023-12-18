/*
 * Copyright (C) 2021 The Android Open Source Project
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

package android.scopedstorage.cts.device;

import static android.scopedstorage.cts.lib.TestUtils.getExternalFilesDir;
import static android.scopedstorage.cts.lib.TestUtils.pollForExternalStorageState;
import static android.scopedstorage.cts.lib.TestUtils.resetDefaultExternalStorageVolume;
import static android.scopedstorage.cts.lib.TestUtils.setExternalStorageVolume;
import static android.scopedstorage.cts.lib.TestUtils.setupDefaultDirectories;

import static androidx.test.InstrumentationRegistry.getContext;

import static com.google.common.truth.Truth.assertThat;
import static com.google.common.truth.Truth.assertWithMessage;

import android.provider.MediaStore;
import android.scopedstorage.cts.lib.TestUtils;

import org.junit.BeforeClass;

import java.util.Arrays;
import java.util.List;

class ScopedStorageBaseDeviceTest {
    private static final String VOLUME_PUBLIC = "volume_public";

    @BeforeClass
    public static void setup() throws Exception {
        createPublicVolume();
        setupStorage();
    }

    private static void createPublicVolume() throws Exception {
        if (TestUtils.getCurrentPublicVolumeName() == null) {
            TestUtils.createNewPublicVolume();
            assertWithMessage("Expected newly created public volume name to be not null")
                    .that(TestUtils.getCurrentPublicVolumeName())
                    .isNotNull();
        }
    }
    private static void setupStorage() throws Exception {
        if (!getContext().getPackageManager().isInstantApp()) {
            pollForExternalStorageState();
            getExternalFilesDir().mkdirs();
        }
    }

    void setupExternalStorage(String volumeName) {
        assertThat(volumeName).isNotNull();
        if (volumeName.equals(MediaStore.VOLUME_EXTERNAL)) {
            resetDefaultExternalStorageVolume();
            TestUtils.assertDefaultVolumeIsPrimary();
        } else {
            final String publicVolumeName = TestUtils.getCurrentPublicVolumeName();
            assertWithMessage("Expected public volume name to be not null")
                    .that(publicVolumeName)
                    .isNotNull();
            setExternalStorageVolume(publicVolumeName);
            TestUtils.assertDefaultVolumeIsPublic();
        }
        setupDefaultDirectories();
    }

    static List<String> getTestParameters() {
        return Arrays.asList(
                MediaStore.VOLUME_EXTERNAL,
                VOLUME_PUBLIC
        );
    }
}
