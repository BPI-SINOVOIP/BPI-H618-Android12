/*
 * Copyright (C) 2020 The Android Open Source Project
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

package com.android.eventlib;

import android.os.UserHandle;

import com.android.bedstead.nene.users.UserReference;
import com.android.queryable.Queryable;

import java.util.HashSet;
import java.util.Set;
import java.util.function.Function;

/**
 * Interface to provide additional restrictions on an {@link Event} query.
 */
public abstract class EventLogsQuery<E extends Event, F extends EventLogsQuery>
        extends EventLogs<E> implements Queryable {

    /**
     * Default implementation of {@link EventLogsQuery} used when there are no additional query
     * options to add.
     */
    public static class Default<E extends Event> extends EventLogsQuery<E, Default> {
        public Default(Class<E> eventClass, String packageName) {
            super(eventClass, packageName);
        }

        @Override
        protected boolean filter(E event) {
            return getPackageName().equals(event.packageName());
        }
    }

    private final Class<E> mEventClass;
    private final String mPackageName;
    private final transient Set<Function<E, Boolean>> filters = new HashSet<>();
    private transient UserHandle mUserHandle = null; // null is default, meaning current user

    protected EventLogsQuery(Class<E> eventClass, String packageName) {
        if (eventClass == null || packageName == null) {
            throw new NullPointerException();
        }
        mQuerier = new RemoteEventQuerier<>(packageName, this);
        mEventClass = eventClass;
        mPackageName = packageName;
    }

    /** Get the package name being filtered for. */
    protected String getPackageName() {
        return mPackageName;
    }

    protected Class<E> eventClass() {
        return mEventClass;
    }

    private final transient EventQuerier<E> mQuerier;

    @Override
    protected EventQuerier<E> getQuerier() {
        return mQuerier;
    }

    /** Apply a lambda filter to the results. */
    public F filter(Function<E, Boolean> filter) {
        filters.add(filter);
        return (F) this;
    }

    /**
     * Returns true if {@code E} matches custom and default filters for this {@link Event} subclass.
     */
    protected final boolean filterAll(E event) {
        if (filters != null) {
            // Filters will be null when called remotely
            for (Function<E, Boolean> filter : filters) {
                if (!filter.apply(event)) {
                    return false;
                }
            }
        }
        return filter(event);
    }

    /** Returns true if {@code E} matches the custom filters for this {@link Event} subclass. */
    protected abstract boolean filter(E event);

    /** Query a package running on another user. */
    public F onUser(UserHandle userHandle) {
        if (userHandle == null) {
            throw new NullPointerException();
        }
        mUserHandle = userHandle;
        return (F) this;
    }

    /** Query a package running on another user. */
    public F onUser(UserReference userReference) {
        return onUser(userReference.userHandle());
    }

    UserHandle getUserHandle() {
        return mUserHandle;
    }
}
