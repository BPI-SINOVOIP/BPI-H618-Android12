//
// Copyright (C) 2021 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
package android.frameworks.stats;

import android.frameworks.stats.VendorAtomValue;

/*
 * Generic vendor atom that allows dynamically allocated atoms to be uploaded
 * through statsd.
 *
 * Here's an example that uses this struct:
 *     VendorAtom atom = {
 *         .atomId  = 100000,
 *         .values  = {2, 70000, 5.2, 4, "a"}
 *     };
 *
 * The resulting LogEvent must have the following fields:
 *     Index    Value
 *     0x1      2
 *     0x2      70000
 *     0x3      5.2
 *     0x4      4
 *     0x5      "a"
 */
@VintfStability
parcelable VendorAtom {
    /**
     * Vendor or OEM reverse domain name. Must be less than 50 characters.
     * Ex. "com.google.pixel"
     */
    String reverseDomainName;
    /*
     * Atom ID. Must be between 100,000 - 199,999 to indicate non-AOSP field.
     */
    int atomId;
    /*
     * Vector of fields in the order that the LogEvent should be filled.
     */
    VendorAtomValue[] values;
}

