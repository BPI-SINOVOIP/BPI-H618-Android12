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
public class BooleanQueryHelperTest {

    private final Queryable mQuery = null;

    @Test
    public void matches_noRestrictions_returnsTrue() {
        BooleanQueryHelper<Queryable> booleanQueryHelper = new BooleanQueryHelper<>(mQuery);

        assertThat(booleanQueryHelper.matches(true)).isTrue();
    }

    @Test
    public void matches_isTrue_valueIsTrue_returnsTrue() {
        BooleanQueryHelper<Queryable> booleanQueryHelper = new BooleanQueryHelper<>(mQuery);
        booleanQueryHelper.isTrue();

        assertThat(booleanQueryHelper.matches(true)).isTrue();
    }

    @Test
    public void matches_isTrue_valueIsFalse_returnsFalse() {
        BooleanQueryHelper<Queryable> booleanQueryHelper = new BooleanQueryHelper<>(mQuery);
        booleanQueryHelper.isTrue();

        assertThat(booleanQueryHelper.matches(false)).isFalse();
    }

    @Test
    public void matches_isFalse_valueIsTrue_returnsFalse() {
        BooleanQueryHelper<Queryable> booleanQueryHelper = new BooleanQueryHelper<>(mQuery);
        booleanQueryHelper.isFalse();

        assertThat(booleanQueryHelper.matches(true)).isFalse();
    }

    @Test
    public void matches_isFalse_valueIsFalse_returnsTrue() {
        BooleanQueryHelper<Queryable> booleanQueryHelper = new BooleanQueryHelper<>(mQuery);
        booleanQueryHelper.isFalse();

        assertThat(booleanQueryHelper.matches(false)).isTrue();
    }

    @Test
    public void matches_equalsTrue_valueIsTrue_returnsTrue() {
        BooleanQueryHelper<Queryable> booleanQueryHelper = new BooleanQueryHelper<>(mQuery);
        booleanQueryHelper.equals(true);

        assertThat(booleanQueryHelper.matches(true)).isTrue();
    }

    @Test
    public void matches_equalsTrue_valueIsFalse_returnsFalse() {
        BooleanQueryHelper<Queryable> booleanQueryHelper = new BooleanQueryHelper<>(mQuery);
        booleanQueryHelper.equals(true);

        assertThat(booleanQueryHelper.matches(false)).isFalse();
    }

    @Test
    public void matches_equalsFalse_valueIsTrue_returnsFalse() {
        BooleanQueryHelper<Queryable> booleanQueryHelper = new BooleanQueryHelper<>(mQuery);
        booleanQueryHelper.equals(false);

        assertThat(booleanQueryHelper.matches(true)).isFalse();
    }

    @Test
    public void matches_equalsFalse_valueIsFalse_returnsTrue() {
        BooleanQueryHelper<Queryable> booleanQueryHelper = new BooleanQueryHelper<>(mQuery);
        booleanQueryHelper.equals(false);

        assertThat(booleanQueryHelper.matches(false)).isTrue();
    }

}
