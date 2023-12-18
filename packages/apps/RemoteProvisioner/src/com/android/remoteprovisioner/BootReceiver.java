/**
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

package com.android.remoteprovisioner;

import static java.lang.Math.max;

import android.app.job.JobInfo;
import android.app.job.JobScheduler;
import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.os.RemoteException;
import android.os.ServiceManager;
import android.security.remoteprovisioning.AttestationPoolStatus;
import android.security.remoteprovisioning.ImplInfo;
import android.security.remoteprovisioning.IRemoteProvisioning;
import android.util.Log;

import java.time.Duration;

/**
 * A receiver class that listens for boot to be completed and then starts a recurring job that will
 * monitor the status of the attestation key pool on device, purging old certificates and requesting
 * new ones as needed.
 */
public class BootReceiver extends BroadcastReceiver {
    private static final String TAG = "RemoteProvisioningBootReceiver";
    private static final String SERVICE = "android.security.remoteprovisioning";

    private static final Duration SCHEDULER_PERIOD = Duration.ofDays(1);

    private static final int ESTIMATED_DOWNLOAD_BYTES_STATIC = 2300;
    private static final int ESTIMATED_X509_CERT_BYTES = 540;
    private static final int ESTIMATED_UPLOAD_BYTES_STATIC = 600;
    private static final int ESTIMATED_CSR_KEY_BYTES = 44;

    @Override
    public void onReceive(Context context, Intent intent) {
        Log.i(TAG, "Caught boot intent, waking up.");
        SettingsManager.generateAndSetId(context);
        // An average call transmits about 500 bytes total. These calculations are for the
        // once a month wake-up where provisioning occurs, where the expected bytes sent is closer
        // to 8-10KB.
        int numKeysNeeded = max(SettingsManager.getExtraSignedKeysAvailable(context),
                                calcNumPotentialKeysToDownload());
        int estimatedDlBytes =
                ESTIMATED_DOWNLOAD_BYTES_STATIC + (ESTIMATED_X509_CERT_BYTES * numKeysNeeded);
        int estimatedUploadBytes =
                ESTIMATED_UPLOAD_BYTES_STATIC + (ESTIMATED_CSR_KEY_BYTES * numKeysNeeded);

        JobInfo info = new JobInfo
                .Builder(1, new ComponentName(context, PeriodicProvisioner.class))
                .setRequiredNetworkType(JobInfo.NETWORK_TYPE_ANY)
                .setEstimatedNetworkBytes(estimatedDlBytes, estimatedUploadBytes)
                .setPeriodic(SCHEDULER_PERIOD.toMillis())
                .build();
        if (((JobScheduler) context.getSystemService(Context.JOB_SCHEDULER_SERVICE)).schedule(info)
                != JobScheduler.RESULT_SUCCESS) {
            Log.e(TAG, "Could not start the job scheduler for provisioning");
        }
    }

    private int calcNumPotentialKeysToDownload() {
        try {
            IRemoteProvisioning binder =
                IRemoteProvisioning.Stub.asInterface(ServiceManager.getService(SERVICE));
            int totalKeysAssigned = 0;
            if (binder == null) {
                Log.e(TAG, "Binder returned null pointer to RemoteProvisioning service.");
                return totalKeysAssigned;
            }
            ImplInfo[] implInfos = binder.getImplementationInfo();
            if (implInfos == null) {
                Log.e(TAG, "No instances of IRemotelyProvisionedComponent registered in "
                           + SERVICE);
                return totalKeysAssigned;
            }
            for (int i = 0; i < implInfos.length; i++) {
                AttestationPoolStatus pool = binder.getPoolStatus(0, implInfos[i].secLevel);
                if (pool != null) {
                    totalKeysAssigned += pool.attested - pool.unassigned;
                }
            }
            return totalKeysAssigned;
        } catch (RemoteException e) {
            Log.e(TAG, "Failure on the RemoteProvisioning backend.", e);
            return 0;
        }
    }
}
