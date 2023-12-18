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
package com.android.car.ui.sharedlibrarysupport;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import android.content.pm.PackageInfo;

import androidx.test.core.content.pm.PackageInfoBuilder;

import org.junit.Test;

public class SharedLibrarySpecifierTest {

    @Test
    public void test_empty_sharedlibraryspecifier_matches_anything() {
        SharedLibrarySpecifier sharedLibrarySpecifier = SharedLibrarySpecifier.builder()
                .build();

        PackageInfo packageInfo = PackageInfoBuilder.newBuilder()
                .setPackageName("com.android.car.testsharedlib").build();
        packageInfo.setLongVersionCode(100);

        assertTrue(sharedLibrarySpecifier.matches(packageInfo));
    }

    @Test
    public void test_sharedlibraryspecifier_doesnt_match_different_package_name() {
        SharedLibrarySpecifier sharedLibrarySpecifier = SharedLibrarySpecifier.builder()
                .setPackageName("com.android.car.testsharedlib")
                .build();

        PackageInfo packageInfo = PackageInfoBuilder.newBuilder()
                .setPackageName("com.android.car.testsharedlib2").build();

        assertFalse(sharedLibrarySpecifier.matches(packageInfo));
    }

    @Test
    public void test_sharedlibraryspecifier_matches_same_package_name() {
        SharedLibrarySpecifier sharedLibrarySpecifier = SharedLibrarySpecifier.builder()
                .setPackageName("com.android.car.testsharedlib")
                .build();

        PackageInfo packageInfo = PackageInfoBuilder.newBuilder()
                .setPackageName("com.android.car.testsharedlib").build();

        assertTrue(sharedLibrarySpecifier.matches(packageInfo));
    }

    @Test
    public void test_sharedlibraryspecifier_doesnt_match_versioncode() {
        SharedLibrarySpecifier sharedLibrarySpecifier = SharedLibrarySpecifier.builder()
                .setPackageName("com.android.car.testsharedlib")
                .setMaxVersion(5)
                .build();

        PackageInfo packageInfo = PackageInfoBuilder.newBuilder()
                .setPackageName("com.android.car.testsharedlib").build();
        packageInfo.setLongVersionCode(6);

        assertFalse(sharedLibrarySpecifier.matches(packageInfo));
    }

    @Test
    public void test_sharedlibraryspecifier_matches_versioncode() {
        SharedLibrarySpecifier sharedLibrarySpecifier = SharedLibrarySpecifier.builder()
                .setPackageName("com.android.car.testsharedlib")
                .setMaxVersion(5)
                .build();

        PackageInfo packageInfo = PackageInfoBuilder.newBuilder()
                .setPackageName("com.android.car.testsharedlib").build();
        packageInfo.setLongVersionCode(4);

        assertTrue(sharedLibrarySpecifier.matches(packageInfo));
    }
}
