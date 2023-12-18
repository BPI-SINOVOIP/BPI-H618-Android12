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

import java.io.Serializable;

@RunWith(JUnit4.class)
public class BundleKeyQueryHelperTest {

    private static final String KEY = "Key";
    private static final String KEY2 = "Key2";
    private static final String STRING_VALUE = "String";
    private static final String DIFFERENT_STRING_VALUE = "String2";

    private final Queryable mQuery = null;
    private final Bundle mBundle = new Bundle();
    private final Bundle mBundle2 = new Bundle();
    private final Serializable mSerializable = "SerializableString";
    private final Serializable mDifferentSerializable = "SerializableString2";

    @Test
    public void matches_noRestrictions_returnsTrue() {
        BundleKeyQueryHelper<Queryable> bundleKeyQueryHelper =
                new BundleKeyQueryHelper<>(mQuery);

        assertThat(bundleKeyQueryHelper.matches(mBundle, KEY)).isTrue();
    }

    @Test
    public void matches_stringValueRestriction_meetsRestriction_returnsTrue() {
        mBundle.putString(KEY, STRING_VALUE);
        BundleKeyQueryHelper<Queryable> bundleKeyQueryHelper =
                new BundleKeyQueryHelper<>(mQuery);

        bundleKeyQueryHelper.stringValue().isEqualTo(STRING_VALUE);

        assertThat(bundleKeyQueryHelper.matches(mBundle, KEY)).isTrue();
    }

    @Test
    public void matches_stringValueRestriction_doesNotMeetRestriction_returnsFalse() {
        mBundle.putString(KEY, STRING_VALUE);
        BundleKeyQueryHelper<Queryable> bundleKeyQueryHelper =
                new BundleKeyQueryHelper<>(mQuery);

        bundleKeyQueryHelper.stringValue().isEqualTo(DIFFERENT_STRING_VALUE);

        assertThat(bundleKeyQueryHelper.matches(mBundle, KEY)).isFalse();
    }

    @Test
    public void matches_bundleValueRestriction_meetsRestriction_returnsTrue() {
        mBundle.putBundle(KEY, mBundle2);
        mBundle2.putString(KEY2, STRING_VALUE);
        BundleKeyQueryHelper<Queryable> bundleKeyQueryHelper =
                new BundleKeyQueryHelper<>(mQuery);

        bundleKeyQueryHelper.bundleValue().key(KEY2).exists();

        assertThat(bundleKeyQueryHelper.matches(mBundle, KEY)).isTrue();
    }

    @Test
    public void matches_bundleValueRestriction_doesNotMeetRestriction_returnsFalse() {
        mBundle.putBundle(KEY, mBundle2);
        mBundle2.remove(KEY2);
        BundleKeyQueryHelper<Queryable> bundleKeyQueryHelper =
                new BundleKeyQueryHelper<>(mQuery);

        bundleKeyQueryHelper.bundleValue().key(KEY2).exists();

        assertThat(bundleKeyQueryHelper.matches(mBundle, KEY)).isFalse();
    }

    @Test
    public void matches_serializableValueRestriction_meetsRestriction_returnsTrue() {
        mBundle.putSerializable(KEY, mSerializable);
        BundleKeyQueryHelper<Queryable> bundleKeyQueryHelper =
                new BundleKeyQueryHelper<>(mQuery);

        bundleKeyQueryHelper.serializableValue().isEqualTo(mSerializable);

        assertThat(bundleKeyQueryHelper.matches(mBundle, KEY)).isTrue();
    }

    @Test
    public void matches_serializableValueRestriction_doesNotMeetRestriction_returnsFalse() {
        mBundle.putSerializable(KEY, mSerializable);
        BundleKeyQueryHelper<Queryable> bundleKeyQueryHelper =
                new BundleKeyQueryHelper<>(mQuery);

        bundleKeyQueryHelper.serializableValue().isEqualTo(mDifferentSerializable);

        assertThat(bundleKeyQueryHelper.matches(mBundle, KEY)).isFalse();
    }

    @Test
    public void matches_existsRestriction_meetsRestriction_returnsTrue() {
        mBundle.putString(KEY, STRING_VALUE);
        BundleKeyQueryHelper<Queryable> bundleKeyQueryHelper =
                new BundleKeyQueryHelper<>(mQuery);

        bundleKeyQueryHelper.exists();

        assertThat(bundleKeyQueryHelper.matches(mBundle, KEY)).isTrue();
    }

    @Test
    public void matches_existsRestriction_doesNotMeetRestriction_returnsFalse() {
        mBundle.remove(KEY);
        BundleKeyQueryHelper<Queryable> bundleKeyQueryHelper =
                new BundleKeyQueryHelper<>(mQuery);

        bundleKeyQueryHelper.exists();

        assertThat(bundleKeyQueryHelper.matches(mBundle, KEY)).isFalse();
    }

    @Test
    public void matches_doesNotExistRestriction_meetsRestriction_returnsTrue() {
        mBundle.remove(KEY);
        BundleKeyQueryHelper<Queryable> bundleKeyQueryHelper =
                new BundleKeyQueryHelper<>(mQuery);

        bundleKeyQueryHelper.doesNotExist();

        assertThat(bundleKeyQueryHelper.matches(mBundle, KEY)).isTrue();
    }

    @Test
    public void matches_doesNotExistRestriction_doesNotMeetRestriction_returnsFalse() {
        mBundle.putString(KEY, STRING_VALUE);
        BundleKeyQueryHelper<Queryable> bundleKeyQueryHelper =
                new BundleKeyQueryHelper<>(mQuery);

        bundleKeyQueryHelper.doesNotExist();

        assertThat(bundleKeyQueryHelper.matches(mBundle, KEY)).isFalse();
    }
}
