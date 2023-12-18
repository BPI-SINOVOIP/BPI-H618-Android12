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

package com.android.car.messenger.core.models;

import android.os.Parcel;
import android.os.Parcelable;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import java.time.Instant;

/**
 * A user account can represent a bluetooth connection to a phone sim or a user account. There can
 * be more than one user accounts during a drive. The user id can be used to retrieve data for that
 * specific account. The user name can be used to display the user the name of the account/account.
 */
public class UserAccount implements Parcelable {
    private final int mId;
    @Nullable private final String mIccId;
    @NonNull private final String mName;
    @NonNull private final Instant mConnectionTime;

    public UserAccount(
            int id, @NonNull String name, @Nullable String iccId, @NonNull Instant connectionTime) {
        mId = id;
        mIccId = iccId;
        mName = name;
        mConnectionTime = connectionTime;
    }

    protected UserAccount(Parcel in) {
        this.mId = in.readInt();
        this.mIccId = in.readString();
        this.mName = in.readString();
        this.mConnectionTime = (Instant) in.readSerializable();
    }

    @NonNull
    public static final Creator<UserAccount> CREATOR =
            new Creator<UserAccount>() {
                @Override
                public UserAccount createFromParcel(@NonNull Parcel source) {
                    return new UserAccount(source);
                }

                @Override
                public UserAccount[] newArray(int size) {
                    return new UserAccount[size];
                }
            };

    /**
     * The user id can be used to retrieve data for that specific account.
     *
     * @return the unique identifier for the user account
     */
    public int getId() {
        return mId;
    }

    /**
     * The IccId is a globally unique serial number—a one-of-a-kind signature that identifies the
     * SIM card itself or bluetooth address of the account.
     *
     * <p>For device/account disambiguation for Contact db queries, this field maps to {@link
     * android.provider.ContactsContract.RawContacts#ACCOUNT_NAME} in the Contacts database.
     *
     * @return The id or null, if not set
     */
    @Nullable
    public String getIccId() {
        return mIccId;
    }

    /** The display name for the account or account. */
    @NonNull
    public String getName() {
        return mName;
    }

    /** Returns the {@link Instant} the car was connected to this {@link UserAccount} */
    @NonNull
    public Instant getConnectionTime() {
        return mConnectionTime;
    }

    @Override
    public int describeContents() {
        return 0;
    }

    @Override
    public void writeToParcel(Parcel dest, int flags) {
        dest.writeInt(this.mId);
        dest.writeString(mIccId);
        dest.writeString(this.mName);
        dest.writeSerializable(this.mConnectionTime);
    }
}
