/*
 * Copyright (C) 2017 The Android Open Source Project
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
package android.edi.cts;

import static android.compat.testing.Classpaths.ClasspathType.BOOTCLASSPATH;
import static android.compat.testing.Classpaths.ClasspathType.SYSTEMSERVERCLASSPATH;

import static com.google.common.truth.Truth.assertThat;
import static com.google.common.truth.Truth.assertWithMessage;

import android.compat.testing.Classpaths;
import android.compat.testing.Classpaths.ClasspathType;

import com.android.compatibility.common.util.DeviceInfo;
import com.android.compatibility.common.util.HostInfoStore;
import com.android.modules.utils.build.testing.DeviceSdkLevel;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.log.LogUtil.CLog;

import com.google.common.collect.ImmutableList;
import com.google.common.collect.ImmutableSet;

import org.jf.dexlib2.iface.ClassDef;

/**
 * Collects information about Java classes present in *CLASSPATH variables and Java shared libraries
 * from the device.
 */
public class ClasspathDeviceInfo extends DeviceInfo {

    private static final String HELPER_APP_PACKAGE = "android.edi.cts.app";
    private static final String HELPER_APP_CLASS = HELPER_APP_PACKAGE + ".ClasspathDeviceTest";

    private ITestDevice mDevice;
    private DeviceSdkLevel deviceSdkLevel;

    @Override
    protected void collectDeviceInfo(HostInfoStore store) throws Exception {
        mDevice = getDevice();
        deviceSdkLevel = new DeviceSdkLevel(mDevice);

        store.startArray("jars");
        collectClasspathsJars(store);
        collectSharedLibraryJars(store);
        store.endArray();
    }

    private void collectClasspathsJars(HostInfoStore store) throws Exception {
        collectClasspathJarInfo(store, BOOTCLASSPATH);
        collectClasspathJarInfo(store, SYSTEMSERVERCLASSPATH);
        // No need to collect DEX2OATBOOTCLASSPATH, as it is just a subset of BOOTCLASSPATH
    }

    private void collectClasspathJarInfo(HostInfoStore store, ClasspathType classpath)
            throws Exception {
        ImmutableList<String> paths = Classpaths.getJarsOnClasspath(mDevice, classpath);
        for (int i = 0; i < paths.size(); i++) {
            store.startGroup();
            store.addResult("classpath", classpath.name());
            store.addResult("path", paths.get(i));
            store.addResult("index", i);
            collectClassInfo(store, paths.get(i));
            store.endGroup();
        }
    }

    private void collectSharedLibraryJars(HostInfoStore store) throws Exception {
        if (!deviceSdkLevel.isDeviceAtLeastS()) {
            return;
        }

        // Trigger helper app to collect and write info about shared libraries on the device.
        assertThat(runDeviceTests(HELPER_APP_PACKAGE, HELPER_APP_CLASS)).isTrue();

        String remoteFile = "/sdcard/shared-libs.txt";
        String content;
        try {
            content = mDevice.pullFileContents(remoteFile);
        } finally {
            mDevice.deleteFile(remoteFile);
        }

        for (String line : content.split("\n")) {
            String[] words = line.split(" ");
            assertWithMessage(
                    "expected each line to be in the format: <name> <type> <version> <path>...")
                    .that(words.length)
                    .isAtLeast(4);
            String libraryName = words[0];
            String libraryType = words[1];
            String libraryVersion = words[2];
            for (int i = 3; i < words.length; i++) {
                String path = words[i];
                if (!mDevice.doesFileExist(path)) {
                    CLog.w("Shared library is not present on device " + path);
                    continue;
                }

                store.startGroup();
                store.startGroup("shared_library");
                store.addResult("name", libraryName);
                store.addResult("type", libraryType);
                store.addResult("version", libraryVersion);
                store.endGroup(); // shared_library
                store.addResult("path", path);
                store.addResult("index", i - 3); // minus <name> <type> <version>
                collectClassInfo(store, path);
                store.endGroup();
            }
        }
    }

    private void collectClassInfo(HostInfoStore store, String path) throws Exception {
        store.startArray("classes");
        for (ClassDef classDef : Classpaths.getClassDefsFromJar(mDevice, path)) {
            store.startGroup();
            store.addResult("name", classDef.getType());
            store.endGroup();
        }
        store.endArray();
    }
}
