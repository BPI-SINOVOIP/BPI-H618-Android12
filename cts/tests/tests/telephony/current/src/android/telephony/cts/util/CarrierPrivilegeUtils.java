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

package android.telephony.cts.util;

import static com.android.compatibility.common.util.SystemUtil.runWithShellPermissionIdentity;
import static com.android.internal.util.FunctionalUtils.ThrowingRunnable;
import static com.android.internal.util.FunctionalUtils.ThrowingSupplier;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.os.PersistableBundle;
import android.telephony.CarrierConfigManager;
import android.telephony.SubscriptionManager;
import android.telephony.TelephonyManager;
import android.util.Log;

import com.android.internal.telephony.uicc.IccUtils;

import java.security.MessageDigest;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;

/**
 * Utility to execute a code block with carrier privileges.
 *
 * <p>The utility methods contained in this class will release carrier privileges once the specified
 * task is completed.
 *
 * <p>Example:
 *
 * <pre>
 *   CarrierPrivilegeUtils.withCarrierPrivileges(c, subId, () -> telephonyManager.setFoo(bar));
 * </pre>
 */
public class CarrierPrivilegeUtils {
    private static final String TAG = CarrierPrivilegeUtils.class.getSimpleName();

    private static class CarrierPrivilegeReceiver extends BroadcastReceiver
            implements AutoCloseable {

        private final CountDownLatch mLatch = new CountDownLatch(1);
        private final Context mContext;
        private final int mSubId;
        private final boolean mGain;

        /**
         * Construct a listener that will wait for adding or removing carrier privileges.
         *
         * @param subId the subId to wait for.
         * @param hash the package hash that indicate carrier privileges.
         * @param gain if true, wait for the package to be added; if false, wait for the package to
         *     be removed.
         */
        CarrierPrivilegeReceiver(Context c, int subId, String hash, boolean gain) {
            mContext = c;
            mSubId = subId;
            mGain = gain;

            mContext.registerReceiver(
                    this, new IntentFilter(CarrierConfigManager.ACTION_CARRIER_CONFIG_CHANGED));
        }

        @Override
        public void close() {
            mContext.unregisterReceiver(this);
        }

        @Override
        public void onReceive(Context c, Intent intent) {
            if (!CarrierConfigManager.ACTION_CARRIER_CONFIG_CHANGED.equals(intent.getAction())) {
                return;
            }

            final int subId = intent.getIntExtra(
                    CarrierConfigManager.EXTRA_SUBSCRIPTION_INDEX,
                    SubscriptionManager.INVALID_SUBSCRIPTION_ID);
            if (mSubId != subId) {
                return;
            }

            final PersistableBundle carrierConfigs =
                    c.getSystemService(CarrierConfigManager.class).getConfigForSubId(mSubId);
            if (!CarrierConfigManager.isConfigForIdentifiedCarrier(carrierConfigs)) {
                return;
            }

            try {
                if (hasCarrierPrivileges(c, mSubId) == mGain) {
                    mLatch.countDown();
                }
            } catch (Exception e) {
            }
        }

        public void waitForCarrierPrivilegeChanged() throws Exception {
            if (!mLatch.await(5000 /* millis */, TimeUnit.MILLISECONDS)) {
                throw new IllegalStateException("Failed to update carrier privileges");
            }
        }
    }

    private static boolean hasCarrierPrivileges(Context c, int subId) {
        // Synchronously check for carrier privileges. Checking certificates MAY be incorrect if
        // broadcasts are delayed.
        return c.getSystemService(TelephonyManager.class)
                .createForSubscriptionId(subId)
                .hasCarrierPrivileges();
    }

    private static String getCertHashForThisPackage(final Context c) throws Exception {
        final PackageInfo pkgInfo = c.getPackageManager()
                .getPackageInfo(c.getOpPackageName(), PackageManager.GET_SIGNATURES);
        final MessageDigest md = MessageDigest.getInstance("SHA-256");
        final byte[] certHash = md.digest(pkgInfo.signatures[0].toByteArray());
        return IccUtils.bytesToHexString(certHash);
    }

    private static void changeCarrierPrivileges(Context c, int subId, boolean gain, boolean isShell)
            throws Exception {
        if (hasCarrierPrivileges(c, subId) == gain) {
            Log.w(TAG, "Carrier privileges already " + (gain ? "granted" : "revoked") + "; bug?");
            return;
        }

        final String certHash = getCertHashForThisPackage(c);
        final PersistableBundle carrierConfigs;

        if (gain) {
            carrierConfigs = new PersistableBundle();
            carrierConfigs.putStringArray(
                    CarrierConfigManager.KEY_CARRIER_CERTIFICATE_STRING_ARRAY,
                    new String[] {certHash});
        } else {
            carrierConfigs = null;
        }

        final CarrierConfigManager configManager = c.getSystemService(CarrierConfigManager.class);

        try (CarrierPrivilegeReceiver receiver =
                new CarrierPrivilegeReceiver(c, subId, certHash, gain)) {
            // If the caller is the shell, it's dangerous to adopt shell permission identity for
            // the CarrierConfig override (as it will override the existing shell permissions).
            if (isShell) {
                configManager.overrideConfig(subId, carrierConfigs);
                configManager.notifyConfigChangedForSubId(subId);
            } else {
                runWithShellPermissionIdentity(() -> {
                    configManager.overrideConfig(subId, carrierConfigs);
                    configManager.notifyConfigChangedForSubId(subId);
                }, android.Manifest.permission.MODIFY_PHONE_STATE);
            }

            receiver.waitForCarrierPrivilegeChanged();
        }
    }

    public static void withCarrierPrivileges(Context c, int subId, ThrowingRunnable action)
            throws Exception {
        try {
            changeCarrierPrivileges(c, subId, true /* gain */, false /* isShell */);
            action.runOrThrow();
        } finally {
            changeCarrierPrivileges(c, subId, false /* lose */, false /* isShell */);
        }
    }

    /** Completes the provided action while assuming the caller is the Shell. */
    public static void withCarrierPrivilegesForShell(Context c, int subId, ThrowingRunnable action)
            throws Exception {
        try {
            changeCarrierPrivileges(c, subId, true /* gain */, true /* isShell */);
            action.runOrThrow();
        } finally {
            changeCarrierPrivileges(c, subId, false /* lose */, true /* isShell */);
        }
    }

    public static <R> R withCarrierPrivileges(Context c, int subId, ThrowingSupplier<R> action)
            throws Exception {
        try {
            changeCarrierPrivileges(c, subId, true /* gain */, false /* isShell */);
            return action.getOrThrow();
        } finally {
            changeCarrierPrivileges(c, subId, false /* lose */, false /* isShell */);
        }
    }
}
