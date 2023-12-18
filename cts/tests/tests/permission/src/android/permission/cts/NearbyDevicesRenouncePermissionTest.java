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

package android.permission.cts;

import static android.Manifest.permission.ACCESS_FINE_LOCATION;
import static android.app.AppOpsManager.OPSTR_FINE_LOCATION;

import static androidx.test.core.app.ApplicationProvider.getApplicationContext;

import static com.android.compatibility.common.util.SystemUtil.runShellCommandOrThrow;

import static com.google.common.truth.Truth.assertThat;

import static org.junit.Assert.assertTrue;
import static org.junit.Assume.assumeTrue;

import android.app.AppOpsManager;
import android.app.AsyncNotedAppOp;
import android.app.SyncNotedAppOp;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothManager;
import android.bluetooth.cts.BTAdapterUtils;
import android.bluetooth.le.BluetoothLeScanner;
import android.bluetooth.le.ScanCallback;
import android.bluetooth.le.ScanResult;
import android.content.AttributionSource;
import android.content.Context;
import android.content.ContextParams;
import android.content.pm.PackageManager;
import android.os.Process;
import android.os.SystemClock;
import android.platform.test.annotations.AppModeFull;
import android.util.ArraySet;
import android.util.Base64;
import android.util.Log;

import androidx.test.InstrumentationRegistry;

import com.android.compatibility.common.util.SystemUtil;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;

import java.util.HashSet;
import java.util.List;
import java.util.Set;

/**
 * Tests behaviour when performing bluetooth scans with renounced location permission.
 */
public class NearbyDevicesRenouncePermissionTest {

    private static final String TAG = "NearbyDevicesRenouncePermissionTest";
    private static final String OPSTR_BLUETOOTH_SCAN = "android:bluetooth_scan";

    private AppOpsManager mAppOpsManager;
    private int mLocationNoteCount;
    private int mScanNoteCount;
    private Context mContext = InstrumentationRegistry.getInstrumentation().getContext();
    private BluetoothAdapter mBluetoothAdapter;
    private boolean mBluetoothAdapterWasEnabled;

    private enum Result {
        UNKNOWN, EXCEPTION, EMPTY, FILTERED, FULL
    }

    private enum Scenario {
        DEFAULT, RENOUNCE, RENOUNCE_MIDDLE, RENOUNCE_END
    }

    @Before
    public void enableBluetooth() {
        assumeTrue(supportsBluetooth());
        mBluetoothAdapter = mContext.getSystemService(BluetoothManager.class).getAdapter();
        mBluetoothAdapterWasEnabled = mBluetoothAdapter.isEnabled();
        assertTrue(BTAdapterUtils.enableAdapter(mBluetoothAdapter, mContext));
        enableTestMode();
    }

    @After
    public void disableBluetooth() {
        assumeTrue(supportsBluetooth());
        disableTestMode();
        if (!mBluetoothAdapterWasEnabled) {
            assertTrue(BTAdapterUtils.disableAdapter(mBluetoothAdapter, mContext));
        }
    }

    @Before
    public void setUp() {
        mAppOpsManager = getApplicationContext().getSystemService(AppOpsManager.class);
        mAppOpsManager.setOnOpNotedCallback(getApplicationContext().getMainExecutor(),
                new AppOpsManager.OnOpNotedCallback() {
                    @Override
                    public void onNoted(SyncNotedAppOp op) {
                        switch (op.getOp()) {
                            case OPSTR_FINE_LOCATION:
                                mLocationNoteCount++;
                                break;
                            case OPSTR_BLUETOOTH_SCAN:
                                mScanNoteCount++;
                                break;
                            default:
                        }
                    }

                    @Override
                    public void onSelfNoted(SyncNotedAppOp op) {
                    }

                    @Override
                    public void onAsyncNoted(AsyncNotedAppOp asyncOp) {
                        switch (asyncOp.getOp()) {
                            case OPSTR_FINE_LOCATION:
                                mLocationNoteCount++;
                                break;
                            case OPSTR_BLUETOOTH_SCAN:
                                mScanNoteCount++;
                                break;
                            default:
                        }
                    }
                });
    }

    @After
    public void tearDown() {
        mAppOpsManager.setOnOpNotedCallback(null, null);
    }

    private void clearNoteCounts() {
        mLocationNoteCount = 0;
        mScanNoteCount = 0;
    }

    @AppModeFull
    @Test
    public void scanWithoutRenouncingNotesBluetoothAndLocation() throws Exception {
        clearNoteCounts();
        assertThat(performScan(Scenario.DEFAULT)).isEqualTo(Result.FULL);
        SystemUtil.eventually(() -> {
            assertThat(mLocationNoteCount).isGreaterThan(0);
            assertThat(mScanNoteCount).isGreaterThan(0);
        });
    }

    @AppModeFull
    @Test
    public void scanRenouncingLocationNotesBluetoothButNotLocation() throws Exception {
        clearNoteCounts();
        assertThat(performScan(Scenario.RENOUNCE)).isEqualTo(Result.FILTERED);
        SystemUtil.eventually(() -> {
            assertThat(mLocationNoteCount).isEqualTo(0);
            assertThat(mScanNoteCount).isGreaterThan(0);
        });
    }

    @AppModeFull
    @Test
    public void scanRenouncingInMiddleOfChainNotesBluetoothButNotLocation() throws Exception {
        clearNoteCounts();
        assertThat(performScan(Scenario.RENOUNCE_MIDDLE)).isEqualTo(Result.FILTERED);
        SystemUtil.eventually(() -> {
            assertThat(mLocationNoteCount).isEqualTo(0);
            assertThat(mScanNoteCount).isGreaterThan(0);
        });
    }

    @AppModeFull
    @Test
    public void scanRenouncingAtEndOfChainNotesBluetoothButNotLocation() throws Exception {
        clearNoteCounts();
        assertThat(performScan(Scenario.RENOUNCE_END)).isEqualTo(Result.FILTERED);
        SystemUtil.eventually(() -> {
            assertThat(mLocationNoteCount).isEqualTo(0);
            assertThat(mScanNoteCount).isGreaterThan(0);
        });
    }

    private Result performScan(Scenario scenario) {
        try {
            Context context = createContext(scenario, getApplicationContext());

            final BluetoothManager bm = context.getSystemService(BluetoothManager.class);
            final BluetoothLeScanner scanner = bm.getAdapter().getBluetoothLeScanner();

            final HashSet<String> observed = new HashSet<>();

            ScanCallback callback = new ScanCallback() {
                public void onScanResult(int callbackType, ScanResult result) {
                    Log.v(TAG, String.valueOf(result));
                    observed.add(Base64.encodeToString(result.getScanRecord().getBytes(), 0));
                }

                public void onBatchScanResults(List<ScanResult> results) {
                    for (ScanResult result : results) {
                        onScanResult(0, result);
                    }
                }
            };
            scanner.startScan(callback);

            // Wait a few seconds to figure out what we actually observed
            SystemClock.sleep(3000);
            scanner.stopScan(callback);
            switch (observed.size()) {
                case 0: return Result.EMPTY;
                case 1: return Result.FILTERED;
                case 5: return Result.FULL;
                default: return Result.UNKNOWN;
            }
        } catch (Throwable t) {
            Log.v(TAG, "Failed to scan", t);
            return Result.EXCEPTION;
        }
    }

    private Context createContext(Scenario scenario, Context context) throws Exception {
        if (scenario == Scenario.DEFAULT) {
            return context;
        }

        Set<String> renouncedPermissions = new ArraySet<>();
        renouncedPermissions.add(ACCESS_FINE_LOCATION);

        switch (scenario) {
            case RENOUNCE:
                return SystemUtil.callWithShellPermissionIdentity(() ->
                        context.createContext(
                                new ContextParams.Builder()
                                        .setRenouncedPermissions(renouncedPermissions)
                                        .setAttributionTag(context.getAttributionTag())
                                        .build())
                );
            case RENOUNCE_MIDDLE:
                AttributionSource nextAttrib = new AttributionSource(
                        Process.SHELL_UID, "com.android.shell", null, (Set<String>) null, null);
                return SystemUtil.callWithShellPermissionIdentity(() ->
                        context.createContext(
                                new ContextParams.Builder()
                                        .setRenouncedPermissions(renouncedPermissions)
                                        .setAttributionTag(context.getAttributionTag())
                                        .setNextAttributionSource(nextAttrib)
                                        .build())
                );
            case RENOUNCE_END:
                nextAttrib = new AttributionSource(
                        Process.SHELL_UID, "com.android.shell", null, renouncedPermissions, null);
                return SystemUtil.callWithShellPermissionIdentity(() ->
                        context.createContext(
                                new ContextParams.Builder()
                                        .setAttributionTag(context.getAttributionTag())
                                        .setNextAttributionSource(nextAttrib)
                                        .build())
                );
            default:
                throw new IllegalStateException();
        }
    }


    private boolean supportsBluetooth() {
        return mContext.getPackageManager().hasSystemFeature(PackageManager.FEATURE_BLUETOOTH);
    }

    private void enableTestMode() {
        runShellCommandOrThrow("dumpsys activity service"
                + " com.android.bluetooth/.btservice.AdapterService set-test-mode enabled");
    }

    private void disableTestMode() {
        runShellCommandOrThrow("dumpsys activity service"
                + " com.android.bluetooth/.btservice.AdapterService set-test-mode disabled");
    }

}
