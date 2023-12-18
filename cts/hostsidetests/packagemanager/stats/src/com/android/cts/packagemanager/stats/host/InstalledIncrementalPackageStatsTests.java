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

package com.android.cts.packagemanager.stats.host;

import static com.android.cts.packagemanager.stats.host.Utils.FEATURE_INCREMENTAL_DELIVERY;

import android.cts.statsdatom.lib.AtomTestUtils;
import android.cts.statsdatom.lib.ConfigUtils;
import android.cts.statsdatom.lib.DeviceUtils;
import android.cts.statsdatom.lib.ReportUtils;

import com.android.os.AtomsProto;

import java.util.ArrayList;
import java.util.List;

public class InstalledIncrementalPackageStatsTests extends PackageManagerStatsTestsBase {
    private static final String TEST_INSTALL_APK = "CtsStatsdAtomEmptyApp.apk";
    private static final String TEST_INSTALL_PACKAGE =
            "com.android.cts.packagemanager.stats.emptyapp";
    private static final String TEST_INSTALL_APK2 = "CtsStatsdAtomEmptyApp2.apk";
    private static final String TEST_INSTALL_PACKAGE2 =
            "com.android.cts.packagemanager.stats.emptyapp2";

    @Override
    protected void tearDown() throws Exception {
        getDevice().uninstallPackage(TEST_INSTALL_PACKAGE);
        getDevice().uninstallPackage(TEST_INSTALL_PACKAGE2);
        super.tearDown();
    }

    // Install 2 incremental packages and check if their UIDs are included in the pulled metrics
    public void testInstalledIncrementalMetricsReported() throws Throwable {
        if (!DeviceUtils.hasFeature(getDevice(), FEATURE_INCREMENTAL_DELIVERY)) {
            return;
        }
        ConfigUtils.uploadConfigForPulledAtom(getDevice(), DeviceUtils.STATSD_ATOM_TEST_PKG,
                AtomsProto.Atom.INSTALLED_INCREMENTAL_PACKAGE_FIELD_NUMBER);
        installPackageUsingIncremental(new String[]{TEST_INSTALL_APK});
        assertTrue(getDevice().isPackageInstalled(TEST_INSTALL_PACKAGE,
                String.valueOf(getDevice().getCurrentUser())));
        installPackageUsingIncremental(new String[]{TEST_INSTALL_APK2});
        assertTrue(getDevice().isPackageInstalled(TEST_INSTALL_PACKAGE2,
                String.valueOf(getDevice().getCurrentUser())));
        AtomTestUtils.sendAppBreadcrumbReportedAtom(getDevice());
        Thread.sleep(AtomTestUtils.WAIT_TIME_LONG);

        List<AtomsProto.Atom> data = ReportUtils.getGaugeMetricAtoms(getDevice());
        assertEquals(2, data.size());
        // The order of the UIDs in the metrics can be different from the order of the installations
        List<Integer> uidsReported = new ArrayList<>();
        for (AtomsProto.Atom atom : data) {
            uidsReported.add(atom.getInstalledIncrementalPackage().getUid());
        }
        assertTrue(uidsReported.contains(getAppUid(TEST_INSTALL_PACKAGE)));
        assertTrue(uidsReported.contains(getAppUid(TEST_INSTALL_PACKAGE2)));
    }
}
