/*
 * Copyright (C) 2019 The Android Open Source Project
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

package android.os.ext;

import android.annotation.IntDef;
import android.annotation.NonNull;
import android.annotation.SystemApi;
import android.os.Build.VERSION_CODES;
import android.os.SystemProperties;

import com.android.modules.utils.build.SdkLevel;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.util.Collections;
import java.util.HashMap;
import java.util.Map;

/**
 * Methods for interacting with the extension SDK.
 *
 * This class provides information about the extension SDK version present
 * on this device. Use the {@link #getExtensionVersion(int) getExtension} to
 * query for the extension version for the given SDK version.

 * @hide
 */
@SystemApi
public class SdkExtensions {

    // S_VERSION_CODE is a separate field to simplify management across branches.
    private static final int VERSION_CODE_S = VERSION_CODES.S;
    private static final int R_EXTENSION_INT;
    private static final int S_EXTENSION_INT;
    private static final Map<Integer, Integer> ALL_EXTENSION_INTS;
    static {
        // Note: when adding more extension versions, the logic that records current
        // extension versions when saving a rollback must also be updated.
        // At the time of writing this is in RollbackManagerServiceImpl#getExtensionVersions()
        R_EXTENSION_INT = SystemProperties.getInt("build.version.extensions.r", 0);
        S_EXTENSION_INT = SystemProperties.getInt("build.version.extensions.s", 0);
        Map<Integer, Integer> extensions = new HashMap<Integer, Integer>();
        extensions.put(VERSION_CODES.R, R_EXTENSION_INT);
        if (SdkLevel.isAtLeastS()) {
            extensions.put(VERSION_CODE_S, S_EXTENSION_INT);
        }
        ALL_EXTENSION_INTS = Collections.unmodifiableMap(extensions);
    }

    /**
     * Values suitable as parameters for {@link #getExtensionVersion(int)}.
     * @hide
     */
    @IntDef(value = { VERSION_CODES.R, VERSION_CODE_S })
    @Retention(RetentionPolicy.SOURCE)
    public @interface Extension {}

    private SdkExtensions() { }

    /**
     * Return the version of the specified extensions.
     *
     * @param extension the extension to get the version of.
     * @throws IllegalArgumentException if extension is not a valid extension
     */
    public static int getExtensionVersion(@Extension int extension) {
        if (extension < VERSION_CODES.R) {
            throw new IllegalArgumentException("not a valid extension: " + extension);
        }

        if (extension == VERSION_CODES.R) {
            return R_EXTENSION_INT;
        }
        if (extension == VERSION_CODE_S) {
            return S_EXTENSION_INT;
        }
        return 0;
    }

    /**
     * Return all extension versions that exist on this device.
     *
     * @return a map from extension to extension version.
     */
    @NonNull
    public static Map<Integer, Integer> getAllExtensionVersions() {
        return ALL_EXTENSION_INTS;
    }

}
