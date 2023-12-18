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

import static java.lang.Math.min;

import android.app.job.JobParameters;
import android.app.job.JobService;
import android.content.Context;
import android.net.ConnectivityManager;
import android.os.RemoteException;
import android.os.ServiceManager;
import android.security.remoteprovisioning.AttestationPoolStatus;
import android.security.remoteprovisioning.ImplInfo;
import android.security.remoteprovisioning.IRemoteProvisioning;
import android.util.Log;

import java.time.Duration;

/**
 * A class that extends JobService in order to be scheduled to check the status of the attestation
 * key pool at regular intervals. If the job determines that more keys need to be generated and
 * signed, it drives that process.
 */
public class PeriodicProvisioner extends JobService {

    private static final int FAILURE_MAXIMUM = 5;
    private static final int SAFE_CSR_BATCH_SIZE = 20;

    // How long to wait in between key pair generations to avoid flooding keystore with requests.
    private static final Duration KEY_GENERATION_PAUSE = Duration.ofMillis(1000);

    // If the connection is metered when the job service is started, try to avoid provisioning.
    private static final long METERED_CONNECTION_EXPIRATION_CHECK = Duration.ofDays(1).toMillis();

    private static final String SERVICE = "android.security.remoteprovisioning";
    private static final String TAG = "RemoteProvisioningService";
    private ProvisionerThread mProvisionerThread;

    /**
     * Starts the periodic provisioning job, which will check the attestation key pool
     * and provision it as necessary.
     */
    public boolean onStartJob(JobParameters params) {
        Log.i(TAG, "Starting provisioning job");
        mProvisionerThread = new ProvisionerThread(params, this);
        mProvisionerThread.start();
        return true;
    }

    /**
     * Allows the job to be stopped if need be.
     */
    public boolean onStopJob(JobParameters params) {
        return false;
    }

    private class ProvisionerThread extends Thread {
        private Context mContext;
        private JobParameters mParams;

        ProvisionerThread(JobParameters params, Context context) {
            mParams = params;
            mContext = context;
        }

        public void run() {
            try {
                IRemoteProvisioning binder =
                        IRemoteProvisioning.Stub.asInterface(ServiceManager.getService(SERVICE));
                if (binder == null) {
                    Log.e(TAG, "Binder returned null pointer to RemoteProvisioning service.");
                    jobFinished(mParams, false /* wantsReschedule */);
                    return;
                }

                ConnectivityManager cm = (ConnectivityManager) mContext.getSystemService(
                        Context.CONNECTIVITY_SERVICE);
                boolean isMetered = cm.isActiveNetworkMetered();
                long expiringBy;
                if (isMetered) {
                    // Check a shortened duration to attempt to avoid metered connection
                    // provisioning.
                    expiringBy = System.currentTimeMillis() + METERED_CONNECTION_EXPIRATION_CHECK;
                } else {
                    expiringBy = SettingsManager.getExpiringBy(mContext)
                                                      .plusMillis(System.currentTimeMillis())
                                                      .toMillis();
                }
                ImplInfo[] implInfos = binder.getImplementationInfo();
                if (implInfos == null) {
                    Log.e(TAG, "No instances of IRemotelyProvisionedComponent registered in "
                               + SERVICE);
                    jobFinished(mParams, false /* wantsReschedule */);
                    return;
                }
                int[] keysNeededForSecLevel = new int[implInfos.length];
                boolean provisioningNeeded =
                        isProvisioningNeeded(binder, expiringBy, implInfos, keysNeededForSecLevel);
                GeekResponse resp = null;
                if (!provisioningNeeded) {
                    if (!isMetered) {
                        // So long as the connection is unmetered, go ahead and grab an updated
                        // device configuration file.
                        resp = ServerInterface.fetchGeek(mContext);
                        if (!checkGeekResp(resp)) {
                            jobFinished(mParams, false /* wantsReschedule */);
                            return;
                        }
                        SettingsManager.setDeviceConfig(mContext,
                                resp.numExtraAttestationKeys,
                                resp.timeToRefresh,
                                resp.provisioningUrl);
                        if (resp.numExtraAttestationKeys == 0) {
                            binder.deleteAllKeys();
                        }
                    }
                    jobFinished(mParams, false /* wantsReschedule */);
                    return;
                }
                resp = ServerInterface.fetchGeek(mContext);
                if (!checkGeekResp(resp)) {
                    jobFinished(mParams, false /* wantsReschedule */);
                    return;
                }
                SettingsManager.setDeviceConfig(mContext,
                            resp.numExtraAttestationKeys,
                            resp.timeToRefresh,
                            resp.provisioningUrl);

                if (resp.numExtraAttestationKeys == 0) {
                    // Provisioning is disabled. Check with the server if it's time to turn it back
                    // on. If not, quit. Avoid checking if the connection is metered. Opt instead
                    // to just continue using the fallback factory provisioned key.
                    binder.deleteAllKeys();
                    jobFinished(mParams, false /* wantsReschedule */);
                    return;
                }
                for (int i = 0; i < implInfos.length; i++) {
                    // Break very large CSR requests into chunks, so as not to overwhelm the
                    // backend.
                    int keysToCertify = keysNeededForSecLevel[i];
                    while (keysToCertify != 0) {
                        int batchSize = min(keysToCertify, SAFE_CSR_BATCH_SIZE);
                        Provisioner.provisionCerts(batchSize,
                                                   implInfos[i].secLevel,
                                                   resp.getGeekChain(implInfos[i].supportedCurve),
                                                   resp.getChallenge(),
                                                   binder,
                                                   mContext);
                        keysToCertify -= batchSize;
                    }
                }
                jobFinished(mParams, false /* wantsReschedule */);
            } catch (RemoteException e) {
                jobFinished(mParams, false /* wantsReschedule */);
                Log.e(TAG, "Error on the binder side during provisioning.", e);
            } catch (InterruptedException e) {
                jobFinished(mParams, false /* wantsReschedule */);
                Log.e(TAG, "Provisioner thread interrupted.", e);
            }
        }

        private boolean checkGeekResp(GeekResponse resp) {
            if (resp == null) {
                Log.e(TAG, "Failed to get a response from the server.");
                if (SettingsManager.getFailureCounter(mContext) > FAILURE_MAXIMUM) {
                    Log.e(TAG, "Too many failures, resetting defaults.");
                    SettingsManager.clearPreferences(mContext);
                }
                jobFinished(mParams, false /* wantsReschedule */);
                return false;
            }
            return true;
        }

        private boolean isProvisioningNeeded(
                IRemoteProvisioning binder, long expiringBy, ImplInfo[] implInfos,
                int[] keysNeededForSecLevel)
                throws InterruptedException, RemoteException {
            if (implInfos == null || keysNeededForSecLevel == null
                || keysNeededForSecLevel.length != implInfos.length) {
                Log.e(TAG, "Invalid argument.");
                return false;
            }
            boolean provisioningNeeded = false;
            for (int i = 0; i < implInfos.length; i++) {
                keysNeededForSecLevel[i] =
                        generateNumKeysNeeded(binder,
                                   expiringBy,
                                   implInfos[i].secLevel);
                if (keysNeededForSecLevel[i] > 0) {
                    provisioningNeeded = true;
                }
            }
            return provisioningNeeded;
        }

        /**
         * This method will generate and bundle up keys for signing to make sure that there will be
         * enough keys available for use by the system when current keys expire.
         *
         * Enough keys is defined by checking how many keys are currently assigned to apps and
         * generating enough keys to cover any expiring certificates plus a bit of buffer room
         * defined by {@code sExtraSignedKeysAvailable}.
         *
         * This allows devices to dynamically resize their key pools as the user downloads and
         * removes apps that may also use attestation.
         */
        private int generateNumKeysNeeded(IRemoteProvisioning binder, long expiringBy, int secLevel)
                throws InterruptedException, RemoteException {
            AttestationPoolStatus pool = binder.getPoolStatus(expiringBy, secLevel);
            int unattestedKeys = pool.total - pool.attested;
            int keysInUse = pool.attested - pool.unassigned;
            int totalSignedKeys = keysInUse + SettingsManager.getExtraSignedKeysAvailable(mContext);
            int generated;
            // If nothing is expiring, and the amount of available unassigned keys is sufficient,
            // then do nothing. Otherwise, generate the complete amount of totalSignedKeys. It will
            // reduce network usage if the app just provisions an entire new batch in one go, rather
            // than consistently grabbing just a few at a time as the expiration dates become
            // misaligned.
            if (pool.expiring > pool.unassigned && pool.attested == totalSignedKeys) {
                return 0;
            }
            for (generated = 0;
                    generated + unattestedKeys < totalSignedKeys; generated++) {
                binder.generateKeyPair(false /* isTestMode */, secLevel);
                // Prioritize provisioning if there are no keys available. No keys being available
                // indicates that this is the first time a device is being brought online.
                if (pool.total != 0) {
                    Thread.sleep(KEY_GENERATION_PAUSE.toMillis());
                }
            }
            if (totalSignedKeys > 0) {
                return generated + unattestedKeys;
            }
            return 0;
        }
    }
}
