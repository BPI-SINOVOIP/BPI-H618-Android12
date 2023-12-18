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

import static org.testng.Assert.assertThrows;

import com.android.queryable.Queryable;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

@RunWith(JUnit4.class)
public class EnumQueryHelperTest {

    private enum TestEnum {
        VALUE1, VALUE2
    }

    private final Queryable mQuery = null;

    @Test
    public void matches_noRestrictions_returnsTrue() {
        EnumQueryHelper<Queryable, TestEnum> enumQueryHelper =
                new EnumQueryHelper<>(mQuery);

        assertThat(enumQueryHelper.matches(TestEnum.VALUE1)).isTrue();
    }

    @Test
    public void matches_isEqualTo_matches_returnsTrue() {
        EnumQueryHelper<Queryable, TestEnum> enumQueryHelper =
                new EnumQueryHelper<>(mQuery);

        enumQueryHelper.isEqualTo(TestEnum.VALUE1);

        assertThat(enumQueryHelper.matches(TestEnum.VALUE1)).isTrue();
    }

    @Test
    public void matches_isEqualTo_doesNotMatch_returnsFalse() {
        EnumQueryHelper<Queryable, TestEnum> enumQueryHelper =
                new EnumQueryHelper<>(mQuery);

        enumQueryHelper.isEqualTo(TestEnum.VALUE1);

        assertThat(enumQueryHelper.matches(TestEnum.VALUE2)).isFalse();
    }

    @Test
    public void matches_isNotEqualTo_matches_returnsTrue() {
        EnumQueryHelper<Queryable, TestEnum> enumQueryHelper =
                new EnumQueryHelper<>(mQuery);

        enumQueryHelper.isNotEqualTo(TestEnum.VALUE1);

        assertThat(enumQueryHelper.matches(TestEnum.VALUE2)).isTrue();
    }

    @Test
    public void matches_isNotEqualTo_doesNotMatch_returnsFalse() {
        EnumQueryHelper<Queryable, TestEnum> enumQueryHelper =
                new EnumQueryHelper<>(mQuery);

        enumQueryHelper.isNotEqualTo(TestEnum.VALUE1);

        assertThat(enumQueryHelper.matches(TestEnum.VALUE1)).isFalse();
    }

    @Test
    public void isEqualTo_alreadySpecifiedIsEqualTo_throwsException() {
        EnumQueryHelper<Queryable, TestEnum> enumQueryHelper =
                new EnumQueryHelper<>(mQuery);

        enumQueryHelper.isEqualTo(TestEnum.VALUE1);

        assertThrows(IllegalStateException.class, () -> enumQueryHelper.isEqualTo(TestEnum.VALUE1));
    }

    @Test
    public void isNotEqualTo_alreadySpecifiedIsNotEqualTo_throwsException() {
        EnumQueryHelper<Queryable, TestEnum> enumQueryHelper =
                new EnumQueryHelper<>(mQuery);

        enumQueryHelper.isNotEqualTo(TestEnum.VALUE1);

        assertThrows(IllegalStateException.class,
                () -> enumQueryHelper.isNotEqualTo(TestEnum.VALUE1));
    }

    @Test
    public void matches_isOneOf_matches_returnsTrue() {
        EnumQueryHelper<Queryable, TestEnum> enumQueryHelper =
                new EnumQueryHelper<>(mQuery);

        enumQueryHelper.isOneOf(TestEnum.VALUE1, TestEnum.VALUE2);

        assertThat(enumQueryHelper.matches(TestEnum.VALUE2)).isTrue();
    }

    @Test
    public void matches_isOneOf_doesNotMatch_returnsFalse() {
        EnumQueryHelper<Queryable, TestEnum> enumQueryHelper =
                new EnumQueryHelper<>(mQuery);

        enumQueryHelper.isOneOf(TestEnum.VALUE1, TestEnum.VALUE2);

        assertThat(enumQueryHelper.matches(null)).isFalse();
    }

    @Test
    public void matches_isNotOneOf_matches_returnsTrue() {
        EnumQueryHelper<Queryable, TestEnum> enumQueryHelper =
                new EnumQueryHelper<>(mQuery);

        enumQueryHelper.isNotOneOf(TestEnum.VALUE1, null);

        assertThat(enumQueryHelper.matches(TestEnum.VALUE2)).isTrue();
    }

    @Test
    public void matches_isNotOneOf_doesNotMatch_returnsFalse() {
        EnumQueryHelper<Queryable, TestEnum> enumQueryHelper =
                new EnumQueryHelper<>(mQuery);

        enumQueryHelper.isNotOneOf(TestEnum.VALUE1, TestEnum.VALUE2);

        assertThat(enumQueryHelper.matches(TestEnum.VALUE1)).isFalse();
    }

    @Test
    public void isOneOf_alreadySpecifiedIsEqualTo_throwsException() {
        EnumQueryHelper<Queryable, TestEnum> enumQueryHelper =
                new EnumQueryHelper<>(mQuery);

        enumQueryHelper.isEqualTo(TestEnum.VALUE1);

        assertThrows(IllegalStateException.class, () -> enumQueryHelper.isOneOf(TestEnum.VALUE1));
    }

    @Test
    public void isNotOneOf_alreadySpecifiedIsNotEqualTo_throwsException() {
        EnumQueryHelper<Queryable, TestEnum> enumQueryHelper =
                new EnumQueryHelper<>(mQuery);

        enumQueryHelper.isNotEqualTo(TestEnum.VALUE1);

        assertThrows(IllegalStateException.class,
                () -> enumQueryHelper.isNotOneOf(TestEnum.VALUE1));
    }
}
