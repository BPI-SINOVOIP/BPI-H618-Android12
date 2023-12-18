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

package com.android.bedstead.nene.packages;

import android.util.Log;

import com.android.bedstead.nene.annotations.Experimental;
import com.android.bedstead.nene.exceptions.AdbException;
import com.android.bedstead.nene.exceptions.NeneException;
import com.android.bedstead.nene.users.UserReference;
import com.android.bedstead.nene.utils.ShellCommand;

@Experimental
public final class ProcessReference {

    private final PackageReference mPackage;
    private final int mProcessId;
    private final UserReference mUser;

    ProcessReference(PackageReference pkg, int processId, UserReference user) {
        if (pkg == null) {
            throw new NullPointerException();
        }
        mPackage = pkg;
        mProcessId = processId;
        mUser = user;
    }

    public PackageReference pkg() {
        return mPackage;
    }

    public int pid() {
        return mProcessId;
    }

    public UserReference user() {
        return mUser;
    }

    public void kill() {
        try {
            ShellCommand.builder("kill")
                    .addOperand(mProcessId)
                    .validate(String::isEmpty)
                    .asRoot()
                    .execute();
        } catch (AdbException e) {
            throw new NeneException("Error killing process", e);
        }
    }

    @Override
    public int hashCode() {
        return mPackage.hashCode() + mProcessId + mUser.hashCode();
    }

    @Override
    public boolean equals(Object obj) {
        if (!(obj instanceof ProcessReference)) {
            return false;
        }

        ProcessReference other = (ProcessReference) obj;
        return other.mUser.equals(mUser)
                && other.mProcessId == mProcessId
                && other.mPackage.equals(mPackage);
    }

    @Override
    public String toString() {
        return "ProcessReference{package=" + mPackage + ", processId=" + mProcessId + ", user=" + mUser + "}";
    }
}
