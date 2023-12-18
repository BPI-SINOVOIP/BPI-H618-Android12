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

import static com.android.queryable.queries.BundleQuery.bundle;

import static com.google.common.truth.Truth.assertThat;

import android.os.Bundle;

import com.android.queryable.Queryable;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

import java.util.Set;

@RunWith(JUnit4.class)
public class SetQueryHelperTest {

    private final Queryable mQuery = null;
    private static final String BUNDLE_KEY = "key";
    private static final Bundle BUNDLE_CONTAINING_KEY = new Bundle();
    private static final Bundle BUNDLE_NOT_CONTAINING_KEY = new Bundle();
    static {
        BUNDLE_CONTAINING_KEY.putString(BUNDLE_KEY, "value");
    }

    @Test
    public void matches_size_matches_returnsTrue() {
        SetQueryHelper<Queryable, Bundle, BundleQuery<Queryable>> setQueryHelper = new SetQueryHelper<>(mQuery);

        setQueryHelper.size().isEqualTo(1);

        assertThat(setQueryHelper.matches(Set.of(new Bundle()))).isTrue();
    }

    @Test
    public void matches_size_doesNotMatch_returnsFalse() {
        SetQueryHelper<Queryable, Bundle, BundleQuery<Queryable>> setQueryHelper = new SetQueryHelper<>(mQuery);

        setQueryHelper.size().isEqualTo(1);

        assertThat(setQueryHelper.matches(Set.of())).isFalse();
    }

    @Test
    public void matches_contains_doesContain_returnsTrue() {
        SetQueryHelper<Queryable, Bundle, BundleQuery<Queryable>> setQueryHelper = new SetQueryHelper<>(mQuery);

        setQueryHelper.contains(
                bundle().key(BUNDLE_KEY).exists()
        );

        assertThat(setQueryHelper.matches(Set.of(BUNDLE_CONTAINING_KEY, BUNDLE_NOT_CONTAINING_KEY))).isTrue();
    }

    @Test
    public void matches_contains_doesNotContain_returnsFalse() {
        SetQueryHelper<Queryable, Bundle, BundleQuery<Queryable>> setQueryHelper = new SetQueryHelper<>(mQuery);

        setQueryHelper.contains(
                bundle().key(BUNDLE_KEY).exists()
        );

        assertThat(setQueryHelper.matches(Set.of(BUNDLE_NOT_CONTAINING_KEY))).isFalse();
    }

    @Test
    public void matches_doesNotContain_doesContain_returnsFalse() {
        SetQueryHelper<Queryable, Bundle, BundleQuery<Queryable>> setQueryHelper = new SetQueryHelper<>(mQuery);

        setQueryHelper.doesNotContain(
                bundle().key(BUNDLE_KEY).exists()
        );

        assertThat(setQueryHelper.matches(Set.of(BUNDLE_CONTAINING_KEY, BUNDLE_NOT_CONTAINING_KEY))).isFalse();
    }

    @Test
    public void matches_doesNotContain_doesNotContain_returnsTrue() {
        SetQueryHelper<Queryable, Bundle, BundleQuery<Queryable>> setQueryHelper = new SetQueryHelper<>(mQuery);

        setQueryHelper.doesNotContain(
                bundle().key(BUNDLE_KEY).exists()
        );

        assertThat(setQueryHelper.matches(Set.of(BUNDLE_NOT_CONTAINING_KEY))).isTrue();
    }

}
