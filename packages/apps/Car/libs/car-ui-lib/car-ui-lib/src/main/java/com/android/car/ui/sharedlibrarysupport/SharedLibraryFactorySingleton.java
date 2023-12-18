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
package com.android.car.ui.sharedlibrarysupport;

import static java.util.Objects.requireNonNull;

import android.content.Context;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.os.Build;
import android.os.Process;
import android.text.TextUtils;
import android.util.Log;

import androidx.annotation.NonNull;
import androidx.annotation.VisibleForTesting;

import com.android.car.ui.R;
import com.android.car.ui.utils.CarUiUtils;

import java.io.File;
import java.lang.reflect.Method;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.Set;

/**
 * This is a singleton that contains a {@link SharedLibraryFactory}. That SharedLibraryFactory
 * is used to create UI components that we want to be customizable by the OEM.
 */
@SuppressWarnings("AndroidJdkLibsChecker")
public final class SharedLibraryFactorySingleton {

    private static final String TAG = "carui";
    private static SharedLibraryFactory sInstance;

    /*
     ********************************************
     *               WARNING                    *
     ********************************************
     * The OEM APIs as they appear on this      *
     * branch of android are not finalized!     *
     * If a shared library is built using them, *
     * it will cause apps to crash!             *
     *                                          *
     * Please only use a shared library with    *
     * a later version of car-ui-lib.           *
     ********************************************
     */
    private static boolean sSharedLibEnabled = false;

    /**
     * Get the {@link SharedLibraryFactory}.
     *
     * If this is the first time the method is being called, it will initialize it using reflection
     * to check for the existence of a shared library, and resolving the appropriate version
     * of the shared library to use.
     */
    public static SharedLibraryFactory get(Context context) {
        if (sInstance != null) {
            return sInstance;
        }

        context = context.getApplicationContext();

        if (!sSharedLibEnabled) {
            sInstance = new SharedLibraryFactoryStub(context);
            return sInstance;
        }

        String sharedLibPackageName = CarUiUtils.getSystemProperty(context.getResources(),
                R.string.car_ui_shared_library_package_system_property_name);

        if (TextUtils.isEmpty(sharedLibPackageName)) {
            sInstance = new SharedLibraryFactoryStub(context);
            return sInstance;
        }

        PackageInfo sharedLibPackageInfo;
        try {
            sharedLibPackageInfo = context.getPackageManager()
                    .getPackageInfo(sharedLibPackageName, 0);
        } catch (PackageManager.NameNotFoundException e) {
            Log.e(TAG, "Could not load CarUi shared library, package "
                    + sharedLibPackageName + " was not found.");
            sInstance = new SharedLibraryFactoryStub(context);
            return sInstance;
        }

        Context applicationContext = context.getApplicationContext();
        if (applicationContext instanceof SharedLibraryConfigProvider) {
            Set<SharedLibrarySpecifier> deniedPackages =
                    ((SharedLibraryConfigProvider) applicationContext).getSharedLibraryDenyList();
            if (deniedPackages != null && deniedPackages.stream()
                    .anyMatch(specs -> specs.matches(sharedLibPackageInfo))) {
                Log.i(TAG, "Package " + context.getPackageName()
                        + " denied loading shared library " + sharedLibPackageName);
                sInstance = new SharedLibraryFactoryStub(context);
                return sInstance;
            }
        }

        Context sharedLibraryContext;
        try {
            sharedLibraryContext = context.createPackageContext(
                    sharedLibPackageName,
                    Context.CONTEXT_INCLUDE_CODE | Context.CONTEXT_IGNORE_SECURITY);
        } catch (PackageManager.NameNotFoundException e) {
            Log.e(TAG, "Could not load CarUi shared library, package "
                    + sharedLibPackageName + " was not found.");
            sInstance = new SharedLibraryFactoryStub(context);
            return sInstance;
        }

        AdapterClassLoader adapterClassLoader =
                instantiateClassLoader(context.getApplicationInfo(),
                        requireNonNull(SharedLibraryFactorySingleton.class.getClassLoader()),
                        sharedLibraryContext.getClassLoader());

        try {
            Class<?> oemApiUtilClass = adapterClassLoader
                    .loadClass("com.android.car.ui.sharedlibrarysupport.OemApiUtil");
            Method getSharedLibraryFactoryMethod = oemApiUtilClass.getDeclaredMethod(
                    "getSharedLibraryFactory", Context.class, String.class);
            getSharedLibraryFactoryMethod.setAccessible(true);
            sInstance = (SharedLibraryFactory) getSharedLibraryFactoryMethod
                    .invoke(null, sharedLibraryContext, context.getPackageName());
        } catch (ReflectiveOperationException e) {
            Log.e(TAG, "Could not load CarUi shared library", e);
            sInstance = new SharedLibraryFactoryStub(context);
            return sInstance;
        }

        if (sInstance == null) {
            Log.e(TAG, "Could not load CarUi shared library");
            sInstance = new SharedLibraryFactoryStub(context);
            return sInstance;
        }

        Log.i(TAG, "Loaded shared library " + sharedLibPackageName
                + " version " + sharedLibPackageInfo.getLongVersionCode()
                + " for package " + context.getPackageName());

        return sInstance;
    }

    /**
     * This method globally enables/disables the shared library. It only applies upon the next
     * call to {@link #get}, components that have already been created won't switch between
     * the shared/static library implementations.
     * <p>
     * This method is @VisibleForTesting so that unit tests can run both with and without
     * the shared library. Since it's tricky to use correctly, real apps shouldn't use it.
     * Instead, apps should use {@link SharedLibraryConfigProvider} to control if their
     * shared library is disabled.
     */
    @VisibleForTesting
    public static void setSharedLibEnabled(boolean sharedLibEnabled) {
        sSharedLibEnabled = sharedLibEnabled;
        // Cause the next call to get() to reinitialize the shared library
        sInstance = null;
    }

    private SharedLibraryFactorySingleton() {}

    @NonNull
    private static AdapterClassLoader instantiateClassLoader(@NonNull ApplicationInfo appInfo,
            @NonNull ClassLoader parent, @NonNull ClassLoader sharedlibraryClassLoader) {
        // All this apk loading code is copied from another Google app
        List<String> libraryPaths = new ArrayList<>(3);
        if (appInfo.nativeLibraryDir != null) {
            libraryPaths.add(appInfo.nativeLibraryDir);
        }
        if ((appInfo.flags & ApplicationInfo.FLAG_EXTRACT_NATIVE_LIBS) == 0) {
            for (String abi : getSupportedAbisForCurrentRuntime()) {
                libraryPaths.add(appInfo.sourceDir + "!/lib/" + abi);
            }
        }

        String flatLibraryPaths = (libraryPaths.size() == 0
                ? null : TextUtils.join(File.pathSeparator, libraryPaths));

        String apkPaths = appInfo.sourceDir;
        if (appInfo.sharedLibraryFiles != null && appInfo.sharedLibraryFiles.length > 0) {
            // Unless you pass PackageManager.GET_SHARED_LIBRARY_FILES this will always be null
            // HOWEVER, if you running on a device with F5 active, the module's dex files are
            // always listed in ApplicationInfo.sharedLibraryFiles and should be included in
            // the classpath.
            apkPaths +=
                    File.pathSeparator + TextUtils.join(File.pathSeparator,
                            appInfo.sharedLibraryFiles);
        }

        return new AdapterClassLoader(apkPaths, flatLibraryPaths, parent, sharedlibraryClassLoader);
    }

    private static List<String> getSupportedAbisForCurrentRuntime() {
        List<String> abis = new ArrayList<>();
        if (Process.is64Bit()) {
            Collections.addAll(abis, Build.SUPPORTED_64_BIT_ABIS);
        } else {
            Collections.addAll(abis, Build.SUPPORTED_32_BIT_ABIS);
        }
        return abis;
    }
}
