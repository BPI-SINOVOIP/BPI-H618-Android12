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

import com.android.queryable.Queryable;
import com.android.queryable.queries.StringQuery;
import com.android.queryable.queries.StringQueryHelper;

/** Builder for progressively building {@link TestApp} queries. */
public final class TestAppQueryBuilder implements Queryable {
    private final TestAppProvider mProvider;

    StringQueryHelper<TestAppQueryBuilder> mPackageName = new StringQueryHelper<>(this);

    TestAppQueryBuilder(TestAppProvider provider) {
        if (provider == null) {
            throw new NullPointerException();
        }
        mProvider = provider;
    }

    /**
     * Query for a {@link TestApp} with a given package name.
     *
     * <p>Only use this filter when you are relying specifically on the package name itself. If you
     * are relying on features you know the {@link TestApp} with that package name has, query for
     * those features directly.
     */
    public StringQuery<TestAppQueryBuilder> wherePackageName() {
        return mPackageName;
    }

    /**
     * Get the {@link TestApp} matching the query.
     *
     * @throws NotFoundException if there is no matching @{link TestApp}.
     */
    public TestApp get() {
        // TODO(scottjonathan): Provide instructions on adding the TestApp if the query fails
        return new TestApp(resolveQuery());
    }

    private TestAppDetails resolveQuery() {
        for (TestAppDetails details : mProvider.testApps()) {
            if (!matches(details)) {
                continue;
            }

            mProvider.markTestAppUsed(details);
            return details;
        }

        throw new NotFoundException(this);
    }

    private boolean matches(TestAppDetails details) {
        if (!StringQueryHelper.matches(mPackageName, details.mPackageName)) {
            return false;
        }

        return true;
    }
}
