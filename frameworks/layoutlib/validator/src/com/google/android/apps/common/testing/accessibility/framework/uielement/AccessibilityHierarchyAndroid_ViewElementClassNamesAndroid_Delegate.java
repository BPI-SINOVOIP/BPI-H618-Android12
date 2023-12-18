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

package com.google.android.apps.common.testing.accessibility.framework.uielement;

import com.android.ide.common.rendering.api.LayoutlibCallback;
import com.android.tools.layoutlib.annotations.LayoutlibDelegate;

public class AccessibilityHierarchyAndroid_ViewElementClassNamesAndroid_Delegate {

    public static LayoutlibCallback sLayoutlibCallback;

    @LayoutlibDelegate
    public static Class<?> getClassByName(ViewHierarchyElementAndroid view, String className) {
        Class toReturn = AccessibilityHierarchyAndroid
                .ViewElementClassNamesAndroid.getClassByName_Original(view, className);
        if (toReturn == null && sLayoutlibCallback != null) {
            try {
                return sLayoutlibCallback.findClass(className);
            } catch (ClassNotFoundException ignore) {
            }
        }

        return toReturn;
    }
}
