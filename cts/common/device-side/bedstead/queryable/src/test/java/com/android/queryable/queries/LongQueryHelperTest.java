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

import com.android.queryable.Queryable;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

@RunWith(JUnit4.class)
public class LongQueryHelperTest {

    private final Queryable mQuery = null;
    private static final long LONG_VALUE = 100L;
    private static final long GREATER_VALUE = 200L;
    private static final long LESS_VALUE = 50L;

    @Test
    public void matches_noRestrictions_returnsTrue() {
        LongQueryHelper<Queryable> longQueryHelper = new LongQueryHelper<>(mQuery);

        assertThat(longQueryHelper.matches(LONG_VALUE)).isTrue();
    }

    @Test
    public void matches_isEqualTo_meetsRestriction_returnsTrue() {
        LongQueryHelper<Queryable> longQueryHelper =
                new LongQueryHelper<>(mQuery);

        longQueryHelper.isEqualTo(LONG_VALUE);

        assertThat(longQueryHelper.matches(LONG_VALUE)).isTrue();
    }

    @Test
    public void matches_isEqualTo_doesNotMeetRestriction_returnsFalse() {
        LongQueryHelper<Queryable> longQueryHelper =
                new LongQueryHelper<>(mQuery);

        longQueryHelper.isEqualTo(LONG_VALUE);

        assertThat(longQueryHelper.matches(GREATER_VALUE)).isFalse();
    }

    @Test
    public void matches_isGreaterThan_meetsRestriction_returnsTrue() {
        LongQueryHelper<Queryable> longQueryHelper =
                new LongQueryHelper<>(mQuery);

        longQueryHelper.isGreaterThan(LONG_VALUE);

        assertThat(longQueryHelper.matches(GREATER_VALUE)).isTrue();
    }

    @Test
    public void matches_isGreaterThan_doesNotMeetRestriction_returnsFalse() {
        LongQueryHelper<Queryable> longQueryHelper =
                new LongQueryHelper<>(mQuery);

        longQueryHelper.isGreaterThan(LONG_VALUE);

        assertThat(longQueryHelper.matches(LONG_VALUE)).isFalse();
    }

    @Test
    public void matches_isGreaterThanOrEqualTo_greaterThan_returnsTrue() {
        LongQueryHelper<Queryable> longQueryHelper =
                new LongQueryHelper<>(mQuery);

        longQueryHelper.isGreaterThanOrEqualTo(LONG_VALUE);

        assertThat(longQueryHelper.matches(GREATER_VALUE)).isTrue();
    }

    @Test
    public void matches_isGreaterThanOrEqualTo_equalTo_returnsTrue() {
        LongQueryHelper<Queryable> longQueryHelper =
                new LongQueryHelper<>(mQuery);

        longQueryHelper.isGreaterThanOrEqualTo(LONG_VALUE);

        assertThat(longQueryHelper.matches(LONG_VALUE)).isTrue();
    }

    @Test
    public void matches_isGreaterThanOrEqualTo_doesNotMeetRestriction_returnsFalse() {
        LongQueryHelper<Queryable> longQueryHelper =
                new LongQueryHelper<>(mQuery);

        longQueryHelper.isGreaterThanOrEqualTo(LONG_VALUE);

        assertThat(longQueryHelper.matches(LESS_VALUE)).isFalse();
    }

    @Test
    public void matches_isLessThan_meetsRestriction_returnsTrue() {
        LongQueryHelper<Queryable> longQueryHelper =
                new LongQueryHelper<>(mQuery);

        longQueryHelper.isLessThan(LONG_VALUE);

        assertThat(longQueryHelper.matches(LESS_VALUE)).isTrue();
    }

    @Test
    public void matches_isLessThan_doesNotMeetRestriction_returnsFalse() {
        LongQueryHelper<Queryable> longQueryHelper =
                new LongQueryHelper<>(mQuery);

        longQueryHelper.isLessThan(LONG_VALUE);

        assertThat(longQueryHelper.matches(LONG_VALUE)).isFalse();
    }

    @Test
    public void matches_isLessThanOrEqualTo_lessThan_returnsTrue() {
        LongQueryHelper<Queryable> longQueryHelper =
                new LongQueryHelper<>(mQuery);

        longQueryHelper.isLessThanOrEqualTo(LONG_VALUE);

        assertThat(longQueryHelper.matches(LESS_VALUE)).isTrue();
    }

    @Test
    public void matches_isLessThanOrEqualTo_equalTo_returnsTrue() {
        LongQueryHelper<Queryable> longQueryHelper =
                new LongQueryHelper<>(mQuery);

        longQueryHelper.isLessThanOrEqualTo(LONG_VALUE);

        assertThat(longQueryHelper.matches(LONG_VALUE)).isTrue();
    }

    @Test
    public void matches_isLessThanOrEqualTo_doesNotMeetRestriction_returnsFalse() {
        LongQueryHelper<Queryable> longQueryHelper =
                new LongQueryHelper<>(mQuery);

        longQueryHelper.isLessThanOrEqualTo(LONG_VALUE);

        assertThat(longQueryHelper.matches(GREATER_VALUE)).isFalse();
    }
}
