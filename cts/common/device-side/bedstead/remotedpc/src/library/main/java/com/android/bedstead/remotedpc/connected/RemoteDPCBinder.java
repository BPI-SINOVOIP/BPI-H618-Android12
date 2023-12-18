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

package com.android.bedstead.remotedpc.connected;

import static android.Manifest.permission.INTERACT_ACROSS_USERS_FULL;
import static android.content.Context.BIND_AUTO_CREATE;

import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.util.Log;

import com.android.bedstead.nene.TestApis;
import com.android.bedstead.nene.permissions.PermissionContext;
import com.android.bedstead.remotedpc.RemoteDpc;

import com.google.android.enterprise.connectedapps.ConnectionBinder;
import com.google.android.enterprise.connectedapps.annotations.AvailabilityRestrictions;

/** {@link ConnectionBinder} used by RemoteDPC. */
public final class RemoteDPCBinder implements ConnectionBinder {

    private static final String LOG_TAG = RemoteDPCBinder.class.getName();

    private static final TestApis sTestApis = new TestApis();

    private final RemoteDpc mRemoteDpc;

    public RemoteDPCBinder(RemoteDpc remoteDpc) {
        mRemoteDpc = remoteDpc;
    }

    @Override
    @SuppressWarnings({"MissingPermission", "NewApi"})
    public boolean tryBind(
            Context context,
            ComponentName bindToService,
            ServiceConnection connection,
            AvailabilityRestrictions availabilityRestrictions) {

        Intent bindIntent = new Intent();
        bindIntent.setComponent(new ComponentName(
                mRemoteDpc.devicePolicyController().componentName().getPackageName(),
                bindToService.getClassName()));

        Log.i(LOG_TAG, "Attempting to bind to " + bindIntent);

        if (mRemoteDpc.devicePolicyController().user().equals(sTestApis.users().instrumented())) {
            try {
                return context.bindService(bindIntent,
                        connection, /* flags= */ BIND_AUTO_CREATE);
            } catch (Exception e) {
                // TODO(scottjonathan): This should actually be communicated back...
                //  (catch the exception outside of the tryBind call)
                Log.e(LOG_TAG, "Error binding", e);
                return false;
            }
        } else {
            try (PermissionContext p =
                         sTestApis.permissions().withPermission(INTERACT_ACROSS_USERS_FULL)) {
                return context.bindServiceAsUser(bindIntent,
                        connection, /* flags= */ BIND_AUTO_CREATE,
                        mRemoteDpc.devicePolicyController().user().userHandle());
            } catch (Exception e) {
                // TODO(scottjonathan): This should actually be communicated back...
                //  (catch the exception outside of the tryBind call)
                Log.e(LOG_TAG, "Error binding", e);
                return false;
            }
        }
    }

    @Override
    public boolean bindingIsPossible(Context context,
            AvailabilityRestrictions availabilityRestrictions) {
        return true;
    }

    @Override
    public boolean hasPermissionToBind(Context context) {
        return true;
    }
}
