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

package com.android.car.messenger.impl.datamodels;

import androidx.lifecycle.MediatorLiveData;
import android.content.Context;
import android.database.ContentObserver;
import android.net.Uri;

import androidx.annotation.NonNull;

import com.android.car.messenger.core.interfaces.AppFactory;

/**
 * Abstract class for Content Provider live data implementations
 *
 * @param <T> the class type emitted from the live data to observers
 */
public abstract class ContentProviderLiveData<T> extends MediatorLiveData<T> {
    @NonNull
    private final ContentObserver mContentObserver =
            new ContentObserver(null) {
                @Override
                public void onChange(boolean selfChange) {
                    onDataChange();
                }
            };

    @NonNull private final Uri[] mUris;
    private boolean mIsRegistered = false;

    /** Constructor that takes in a list of content provider uris to observe */
    public ContentProviderLiveData(@NonNull Uri... uris) {
        mUris = uris;
    }

    @Override
    protected void onActive() {
        super.onActive();
        if (!mIsRegistered) {
            for (Uri uri : mUris) {
                getContext()
                        .getContentResolver()
                        .registerContentObserver(
                                uri, /* notifyForDescendants =*/ true, mContentObserver);
                mIsRegistered = true;
            }
        }
    }

    @Override
    protected void onInactive() {
        super.onInactive();
        getContext().getContentResolver().unregisterContentObserver(mContentObserver);
        mIsRegistered = false;
    }

    /** Get Context for use in Live Data */
    @NonNull
    public Context getContext() {
        return AppFactory.get().getContext();
    }

    /** Abstract method called on data change */
    public abstract void onDataChange();
}
