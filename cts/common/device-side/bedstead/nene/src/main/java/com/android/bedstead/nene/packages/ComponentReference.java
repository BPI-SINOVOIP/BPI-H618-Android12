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

import android.content.ComponentName;

import com.android.bedstead.nene.TestApis;
import com.android.bedstead.nene.annotations.Experimental;
import com.android.bedstead.nene.exceptions.AdbException;
import com.android.bedstead.nene.exceptions.NeneException;
import com.android.bedstead.nene.users.UserReference;
import com.android.bedstead.nene.utils.ShellCommand;

/**
 * A representation of a component on device which may or may not exist.
 */
@Experimental
public class ComponentReference {

    final TestApis mTestApis;
    final PackageReference mPackage;
    final String mClassName;

    public ComponentReference(TestApis testApis, PackageReference packageName, String className) {
        mTestApis = testApis;
        mPackage = packageName;
        mClassName = className;
    }

    public ComponentReference(TestApis testApis, ComponentName component) {
        this(testApis,
                new UnresolvedPackage(testApis, component.getPackageName()),
                component.getClassName());
    }

    /**
     * Get the {@link PackageReference} for this component.
     */
    public PackageReference packageName() {
        return mPackage;
    }

    /**
     * Get the class for this component.
     */
    public String className() {
        return mClassName;
    }

    /**
     * Get this component as a {@link ComponentName}.
     */
    public ComponentName componentName() {
        return new ComponentName(mPackage.packageName(), mClassName);
    }

    /**
     * Enable this component for the given {@link UserReference}.
     */
    public ComponentReference enable(UserReference user) {
        try {
            ShellCommand.builderForUser(user, "pm enable")
                    .addOperand(mPackage.packageName() + "/" + mClassName)
                    .validate(o -> o.contains("new state"))
                    .execute();
        } catch (AdbException e) {
            throw new NeneException("Error enabling component " + this + " for user " + user, e);
        }
        return this;
    }

    /**
     * Enable this component for the instrumented user.
     */
    public ComponentReference enable() {
        return enable(mTestApis.users().instrumented());
    }

    /**
     * Disable this component for the given {@link UserReference}.
     */
    public ComponentReference disable(UserReference user) {
        try {
            ShellCommand.builderForUser(user, "pm disable")
                    .addOperand(mPackage.packageName() + "/" + mClassName)
                    .validate(o -> o.contains("new state"))
                    .execute();
        } catch (AdbException e) {
            throw new NeneException("Error disabling component " + this + " for user " + user, e);
        }
        return this;
    }

    /**
     * Disable this component for the instrumented user.
     */
    public ComponentReference disable() {
        return disable(mTestApis.users().instrumented());
    }

    @Override
    public int hashCode() {
        return componentName().hashCode();
    }

    @Override
    public boolean equals(Object obj) {
        if (!(obj instanceof ComponentReference)) {
            return false;
        }

        ComponentReference other = (ComponentReference) obj;
        return other.componentName().equals(componentName());
    }

    @Override
    public String toString() {
        StringBuilder stringBuilder = new StringBuilder("ComponentReference{");
        stringBuilder.append("package=" + mPackage);
        stringBuilder.append(", component=" + mClassName);
        stringBuilder.append("}");
        return stringBuilder.toString();
    }
}
