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

package com.android.bedstead.metricsrecorder;

import com.android.os.nano.AtomsProto;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

public final class EnterpriseMetricInfo {
    private final String mAdminPackageName;
    private final boolean mBoolean;
    private final List<String> mStrings;

    EnterpriseMetricInfo(AtomsProto.DevicePolicyEvent event) {
        mAdminPackageName = event.adminPackageName;
        mBoolean = event.booleanValue;
        mStrings = (event.stringListValue == null) ? new ArrayList<>() : Arrays.asList(
                event.stringListValue.stringValue);
    }

    public String adminPackageName() {
        return mAdminPackageName;
    }

    public boolean Boolean() {
        return mBoolean;
    }

    public List<String> strings() {
        return mStrings;
    }

    @Override
    public String toString() {
        return "EnterpriseMetricInfo{"
                + "adminPackageName=" + mAdminPackageName
                + ", boolean=" + mBoolean
                + ", strings=" + mStrings
                + "}";
    }
}
