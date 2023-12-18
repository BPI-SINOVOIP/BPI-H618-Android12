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

package com.android.queryable.queries;

import static com.google.common.truth.Truth.assertThat;

import android.os.Bundle;

import com.android.queryable.Queryable;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

@RunWith(JUnit4.class)
public class BundleQueryHelperTest {

    private static final String KEY = "Key";
    private static final String KEY2 = "Key2";
    private static final String STRING_VALUE = "value";

    private final Queryable mQuery = null;
    private final Bundle mBundle = new Bundle();

    @Test
    public void matches_noRestrictions_returnsTrue() {
        BundleQueryHelper<Queryable> bundleQueryHelper =
                new BundleQueryHelper<>(mQuery);

        assertThat(bundleQueryHelper.matches(mBundle)).isTrue();
    }

    @Test
    public void matches_restrictionOnOneKey_restrictionIsMet_returnsTrue() {
        mBundle.putString(KEY, STRING_VALUE);
        BundleQueryHelper<Queryable> bundleQueryHelper =
                new BundleQueryHelper<>(mQuery);

        bundleQueryHelper.key(KEY).exists();

        assertThat(bundleQueryHelper.matches(mBundle)).isTrue();
    }

    @Test
    public void matches_restrictionOnOneKey_restrictionIsNotMet_returnsFalse() {
        mBundle.putString(KEY, STRING_VALUE);
        BundleQueryHelper<Queryable> bundleQueryHelper =
                new BundleQueryHelper<>(mQuery);

        bundleQueryHelper.key(KEY).doesNotExist();

        assertThat(bundleQueryHelper.matches(mBundle)).isFalse();
    }

    @Test
    public void matches_restrictionOnMultipleKeys_oneRestrictionIsNotMet_returnsFalse() {
        mBundle.putString(KEY, STRING_VALUE);
        mBundle.remove(KEY2);
        BundleQueryHelper<Queryable> bundleQueryHelper =
                new BundleQueryHelper<>(mQuery);

        bundleQueryHelper.key(KEY).exists();
        bundleQueryHelper.key(KEY2).exists();

        assertThat(bundleQueryHelper.matches(mBundle)).isFalse();
    }

    @Test
    public void matches_restrictionOnNonExistingKey_returnsFalse() {
        mBundle.remove(KEY);
        BundleQueryHelper<Queryable> bundleQueryHelper =
                new BundleQueryHelper<>(mQuery);

        bundleQueryHelper.key(KEY).stringValue().isEqualTo(STRING_VALUE);

        assertThat(bundleQueryHelper.matches(mBundle)).isFalse();
    }
}
