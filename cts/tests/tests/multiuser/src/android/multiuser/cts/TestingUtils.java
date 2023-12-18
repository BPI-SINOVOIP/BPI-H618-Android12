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
 * limitations under the License
 */
package android.multiuser.cts;

import static com.android.compatibility.common.util.SystemUtil.runShellCommand;

import android.app.Instrumentation;

import java.io.IOException;

final class TestingUtils {

    public static boolean getBooleanProperty(Instrumentation instrumentation, String property)
            throws IOException {
        String value = trim(runShellCommand(instrumentation, "getprop " + property));
        return "y".equals(value) || "yes".equals(value) || "1".equals(value) || "true".equals(value)
                || "on".equals(value);
    }

    private static String trim(String s) {
        return s == null ? null : s.trim();
    }

    private TestingUtils() {
        throw new UnsupportedOperationException("contains only static methods");
    }
}
