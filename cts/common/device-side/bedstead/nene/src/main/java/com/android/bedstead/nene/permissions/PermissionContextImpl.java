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

package com.android.bedstead.nene.permissions;

import com.android.bedstead.nene.exceptions.NeneException;

import java.util.Arrays;
import java.util.HashSet;
import java.util.Set;

/**
 * Default implementation of {@link PermissionContext}
 */
public final class PermissionContextImpl implements PermissionContext {
    private final Permissions mPermissions;
    private final Set<String> mGrantedPermissions = new HashSet<>();
    private final Set<String> mDeniedPermissions = new HashSet<>();

    PermissionContextImpl(Permissions permissions) {
        mPermissions = permissions;
    }

    Set<String> grantedPermissions() {
        return mGrantedPermissions;
    }

    Set<String> deniedPermissions() {
        return mDeniedPermissions;
    }

    /**
     * See {@link Permissions#withPermission(String...)}
     */
    public PermissionContextImpl withPermission(String... permissions) {
        for (String permission : permissions) {
            if (mDeniedPermissions.contains(permission)) {
                mPermissions.clearPermissions();
                throw new NeneException(
                        permission + " cannot be required to be both granted and denied");
            }
        }

        mGrantedPermissions.addAll(Arrays.asList(permissions));

        mPermissions.applyPermissions();

        return this;
    }

    /**
     * See {@link Permissions#withoutPermission(String...)}
     */
    public PermissionContextImpl withoutPermission(String... permissions) {
        for (String permission : permissions) {
            if (mGrantedPermissions.contains(permission)) {
                mPermissions.clearPermissions();
                throw new NeneException(
                        permission + " cannot be required to be both granted and denied");
            }
        }

        mDeniedPermissions.addAll(Arrays.asList(permissions));

        mPermissions.applyPermissions();

        return this;
    }

    @Override
    public void close() {
        Permissions.sInstance.undoPermission(this);
    }
}
