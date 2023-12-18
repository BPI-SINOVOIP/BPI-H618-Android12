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

package com.android.bedstead.testapp;

import static com.google.common.truth.Truth.assertThat;

import static org.testng.Assert.assertThrows;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

@RunWith(JUnit4.class)
public class TestAppProviderTest {

    // Expects that this package name matches an actual test app
    private static final String EXISTING_PACKAGENAME = "android.EmptyTestApp";

    // Expects that this package name does not match an actual test app
    private static final String NOT_EXISTING_PACKAGENAME = "not.existing.test.app";

    private TestAppProvider mTestAppProvider;

    @Before
    public void setup() {
        mTestAppProvider = new TestAppProvider();
    }

    @Test
    public void get_queryMatches_returnsTestApp() {
        TestAppQueryBuilder query = mTestAppProvider.query()
                .wherePackageName().isEqualTo(EXISTING_PACKAGENAME);

        assertThat(query.get()).isNotNull();
    }

    @Test
    public void get_queryMatches_packageNameIsSet() {
        TestAppQueryBuilder query = mTestAppProvider.query()
                .wherePackageName().isEqualTo(EXISTING_PACKAGENAME);

        assertThat(query.get().packageName()).isEqualTo(EXISTING_PACKAGENAME);
    }

    @Test
    public void get_queryDoesNotMatch_throwsException() {
        TestAppQueryBuilder query = mTestAppProvider.query()
                .wherePackageName().isEqualTo(NOT_EXISTING_PACKAGENAME);

        assertThrows(NotFoundException.class, query::get);
    }

    @Test
    public void any_returnsTestApp() {
        assertThat(mTestAppProvider.any()).isNotNull();
    }

    @Test
    public void any_returnsDifferentTestApps() {
        assertThat(mTestAppProvider.any()).isNotEqualTo(mTestAppProvider.any());
    }

    @Test
    public void query_onlyReturnsTestAppOnce() {
        mTestAppProvider.query().wherePackageName().isEqualTo(EXISTING_PACKAGENAME).get();

        TestAppQueryBuilder query = mTestAppProvider.query().wherePackageName().isEqualTo(EXISTING_PACKAGENAME);

        assertThrows(NotFoundException.class, query::get);
    }

    // TODO(scottjonathan): Once we support features other than package name, test that we can get
    //  different test apps by querying for the same thing
}
