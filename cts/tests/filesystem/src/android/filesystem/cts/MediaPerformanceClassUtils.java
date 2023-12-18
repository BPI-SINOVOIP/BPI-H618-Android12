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

package android.filesystem.cts;

import android.os.Build;
import android.os.Build;
import android.os.SystemProperties;
import android.util.Log;

/**
 * Test utilities.
 */
/* package private */ class MediaPerformanceClassUtils {
    private static final int sPc = SystemProperties.getInt(
        "ro.odm.build.media_performance_class", 0);

    private static final String TAG = "PerformanceClassTestUtils";

    /**
     * First defined media performance class.
     */
    private static final int FIRST_PERFORMANCE_CLASS = Build.VERSION_CODES.R;

    /**
     * Latest defined media performance class.
     */
    /* TODO: make this S */
    private static final int LAST_PERFORMANCE_CLASS = Build.VERSION_CODES.R + 1;

    static {
        if (sPc > 0) {
            Log.d(TAG, "performance class is "  + sPc);
        }
    }

    public static boolean isRPerfClass() {
        return sPc == Build.VERSION_CODES.R;
    }

    public static boolean isSPerfClass() {
        return sPc == Build.VERSION_CODES.R + 1; /* TODO: make this S */
    }

    public static int getPerfClass() {
        return sPc;
    }

    public static boolean isPerfClass() {
        return sPc >= FIRST_PERFORMANCE_CLASS &&
               sPc <= LAST_PERFORMANCE_CLASS;
    }
}
