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

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import dalvik.system.PathClassLoader;

import java.util.regex.Pattern;

/**
 * This is a {@link PathClassLoader} that you can pass an additional classloaders to the
 * constructor. That will be searched after the classes in this classloader have been searched,
 * but before the parent classloader is searched. The first classloader in the list of
 * classloaders is set as the parent classloader.
 *
 * Much of the code is copied from {@link dalvik.system.DelegateLastClassLoader}. However,
 * note that resource loading is not implemented the same way just because we didn't need it.
 */
class AdapterClassLoader extends PathClassLoader {

    @Nullable
    private final ClassLoader mSharedLibraryClassLoader;

    private static final Pattern PATTERN = Pattern.compile(
            "^com\\.android\\.car\\.ui\\..*AdapterV[0-9]+(\\$.*)?$"
            + "|Lambda"
            + "|^" + Pattern.quote(OemApiUtil.class.getName()) + "$");

    private static final Pattern SHARED_LIBRARY_PATTERN =
            Pattern.compile("^com\\.android\\.car\\.ui\\.sharedlibrary\\."
                    + "(oemapis\\..*|SharedLibraryVersionProviderImpl)$");

    /**
     * Equivalent to calling {@link #AdapterClassLoader(String, String, ClassLoader, ClassLoader)}
     * with {@code librarySearchPath = null, delegateResourceLoading = true}.
     */
    AdapterClassLoader(String dexPath, @Nullable ClassLoader parent,
            @Nullable ClassLoader additionalClassloader) {
        this(dexPath, null, parent, additionalClassloader);
    }

    /**
     * Equivalent to calling
     * {@link #AdapterClassLoader(String, String, ClassLoader, ClassLoader, boolean)}
     * with {@code delegateResourceLoading = true}.
     */
    AdapterClassLoader(String dexPath, String librarySearchPath, @Nullable ClassLoader parent,
            @Nullable ClassLoader additionalClassloader) {
        this(dexPath, librarySearchPath, parent, additionalClassloader, true);
    }

    /**
     * See {@link dalvik.system.DelegateLastClassLoader#DelegateLastClassLoader
     * (String, String, List<ClassLoader>, boolean)}.
     */
    AdapterClassLoader(@NonNull String dexPath, @Nullable String librarySearchPath,
            @Nullable ClassLoader parent, @Nullable ClassLoader additionalClassloader,
                boolean delegateResourceLoading) {
        super(dexPath, librarySearchPath, parent);
        mSharedLibraryClassLoader = additionalClassloader;
    }

    /**
     * A copy from {@link dalvik.system.DelegateLastClassLoader}, but uses both {@code parent}
     * and {@code additionalClassLoader} from the constructor as parent classloaders.
     *
     * If AdapterClassLoader are loaded, loading them
     * from this classloader will be skipped and instead they'll be loaded from the parent
     * classloader, so that they are not duplicated.
     */
    @Override
    protected Class<?> loadClass(String name, boolean resolve) throws ClassNotFoundException {
        // First, check whether the class has already been loaded. Return it if that's the case.
        Class<?> cl = findLoadedClass(name);
        if (cl != null) {
            return cl;
        }

        // Next, check whether the class in question is present in the boot classpath.
        try {
            return Object.class.getClassLoader().loadClass(name);
        } catch (ClassNotFoundException ignored) {
            // It's fine, just continue
        }

        ClassNotFoundException fromSuper = null;

        // Only load adapter classes and certain util classes in this classloader.
        if (name != null && PATTERN.matcher(name).find()) {
            // Next, check whether the class in question is present in the dexPath that this
            // classloader operates on, or its shared libraries.
            try {
                return findClass(name);
            } catch (ClassNotFoundException ex) {
                fromSuper = ex;
            }
        }

        // Loading OEM-APIs
        // Next, check any additional classloaders.
        if (mSharedLibraryClassLoader != null && SHARED_LIBRARY_PATTERN.matcher(name).matches()) {
            try {
                return mSharedLibraryClassLoader.loadClass(name);
            } catch (ClassNotFoundException ignored) {
                // It's fine, just continue
            }
        }

        // Finally, check whether the class in question is present in the parent classloader.
        try {
            return getParent().loadClass(name);
        } catch (ClassNotFoundException cnfe) {
            // The exception we're catching here is the CNFE thrown by the parent of this
            // classloader. However, we would like to throw a CNFE that provides details about
            // the class path / list of dex files associated with *this* classloader, so we choose
            // to throw the exception thrown from that lookup.
            if (fromSuper == null) {
                throw cnfe;
            }
            throw fromSuper;
        }
    }
}
