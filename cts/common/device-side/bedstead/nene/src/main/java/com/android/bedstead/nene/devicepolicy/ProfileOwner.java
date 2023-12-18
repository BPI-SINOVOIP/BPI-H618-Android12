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

package com.android.bedstead.nene.devicepolicy;

import android.content.ComponentName;

import com.android.bedstead.nene.TestApis;
import com.android.bedstead.nene.exceptions.AdbException;
import com.android.bedstead.nene.exceptions.NeneException;
import com.android.bedstead.nene.packages.PackageReference;
import com.android.bedstead.nene.users.UserReference;
import com.android.bedstead.nene.utils.ShellCommand;
import com.android.bedstead.nene.utils.ShellCommandUtils;

import java.util.Objects;

/**
 * A reference to a Profile Owner.
 */
public final class ProfileOwner extends DevicePolicyController {

    ProfileOwner(TestApis testApis,
            UserReference user,
            PackageReference pkg,
            ComponentName componentName) {
        super(testApis, user, pkg, componentName);
    }

    @Override
    public void remove() {
        // TODO(scottjonathan): use DevicePolicyManager#forceRemoveActiveAdmin on S+

        try {
            ShellCommand.builderForUser(mUser, "dpm remove-active-admin")
                    .addOperand(componentName().flattenToShortString())
                    .validate(ShellCommandUtils::startsWithSuccess)
                    .execute();
        } catch (AdbException e) {
            throw new NeneException("Error removing profile owner " + this, e);
        }
    }

    @Override
    public String toString() {
        StringBuilder stringBuilder = new StringBuilder("ProfileOwner{");
        stringBuilder.append("user=").append(user());
        stringBuilder.append(", package=").append(pkg());
        stringBuilder.append(", componentName=").append(componentName());
        stringBuilder.append("}");

        return stringBuilder.toString();
    }

    @Override
    public boolean equals(Object obj) {
        if (!(obj instanceof ProfileOwner)) {
            return false;
        }

        ProfileOwner other = (ProfileOwner) obj;

        return Objects.equals(other.mUser, mUser)
                && Objects.equals(other.mPackage, mPackage)
                && Objects.equals(other.mComponentName, mComponentName);
    }
}
