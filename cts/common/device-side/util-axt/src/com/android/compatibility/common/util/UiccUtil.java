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

package com.android.compatibility.common.util;

import android.Manifest;
import android.telephony.TelephonyManager;

import androidx.annotation.StringDef;
import androidx.test.InstrumentationRegistry;

import java.util.List;

/** Utility class for common UICC- and SIM-related operations. */
public final class UiccUtil {
    /** The hashes of all supported CTS UICC test keys and their corresponding specification. */
    @StringDef({UiccCertificate.CTS_UICC_LEGACY, UiccCertificate.CTS_UICC_2021})
    public @interface UiccCertificate {

        /**
         * Indicates compliance with the "legacy" CTS UICC specification (prior to 2021).
         *
         * <p>Deprecated as of 2021, support to be removed in 2022.
         *
         * <p>Corresponding certificate: {@code aosp-testkey}.
         */
        String CTS_UICC_LEGACY = "61ED377E85D386A8DFEE6B864BD85B0BFAA5AF81";

        /**
         * Indicates compliance with the 2021 CTS UICC specification.
         *
         * <p>Strongly recommended as of 2021, required as of 2022.
         *
         * <p>Corresponding certificate: {@code cts-uicc-2021-testkey}.
         */
        String CTS_UICC_2021 = "CE7B2B47AE2B7552C8F92CC29124279883041FB623A5F194A82C9BF15D492AA0";
    }

    /**
     * A simple check for use with {@link org.junit.Assume#assumeTrue}. Checks the carrier privilege
     * certificates stored on the SIM and returns {@code true} if {@code requiredCert} is present.
     *
     * <p>Can be used either in the {@code #setUp} method if an entire class requires a particular
     * UICC, or at the top of a specific {@code @Test} method.
     *
     * <p>If we had JUnit 5, we could create a much cooler {@code @RequiresUiccCertificate}
     * annotation using {@code ExtendWith} and {@code ExecutionCondition}, but that isn't available
     * to us yet.
     */
    public static boolean uiccHasCertificate(@UiccCertificate String requiredCert) {
        TelephonyManager tm =
                InstrumentationRegistry.getInstrumentation()
                        .getTargetContext()
                        .getSystemService(TelephonyManager.class);
        List<String> uiccCerts =
                ShellIdentityUtils.invokeMethodWithShellPermissions(
                        tm,
                        TelephonyManager::getCertsFromCarrierPrivilegeAccessRules,
                        Manifest.permission.READ_PRIVILEGED_PHONE_STATE);
        return uiccCerts == null ? false : uiccCerts.contains(requiredCert);
    }
}
