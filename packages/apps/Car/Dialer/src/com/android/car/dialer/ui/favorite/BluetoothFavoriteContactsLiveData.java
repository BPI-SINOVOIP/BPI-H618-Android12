/*
 * Copyright (C) 2019 The Android Open Source Project
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

package com.android.car.dialer.ui.favorite;

import android.Manifest;
import android.content.Context;
import android.database.Cursor;
import android.provider.ContactsContract;
import android.text.TextUtils;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import com.android.car.telephony.common.AsyncQueryLiveData;
import com.android.car.telephony.common.Contact;
import com.android.car.telephony.common.QueryParam;

import java.util.ArrayList;
import java.util.List;

import dagger.assisted.Assisted;
import dagger.assisted.AssistedFactory;
import dagger.assisted.AssistedInject;
import dagger.hilt.android.qualifiers.ApplicationContext;

/** Presents the favorite contacts downloaded from phone. It reads the contacts provider. */
public class BluetoothFavoriteContactsLiveData extends AsyncQueryLiveData<List<Contact>> {
    private final Context mContext;

    @AssistedInject
    BluetoothFavoriteContactsLiveData(
            @ApplicationContext Context context, @Assisted @Nullable String accountName) {
        super(context, QueryParam.of(getFavoriteQueryParam(accountName)));
        mContext = context;
    }

    @Override
    protected List<Contact> convertToEntity(@NonNull Cursor cursor) {
        List<Contact> resultList = new ArrayList<>();

        while (cursor.moveToNext()) {
            Contact favoriteEntry = Contact.fromCursor(mContext, cursor);
            // If there is already a contact with the same phone number, don't add duplicate
            // entries.
            boolean alreadyExists = false;
            for (Contact contact : resultList) {
                if (favoriteEntry.equals(contact) && favoriteEntry.getNumbers().containsAll(
                        contact.getNumbers())) {
                    alreadyExists = true;
                    break;
                }
            }
            if (!alreadyExists) {
                resultList.add(favoriteEntry);
            }
        }
        return resultList;
    }

    private static QueryParam getFavoriteQueryParam(String accountName) {
        StringBuilder where = new StringBuilder();
        List<String> selectionArgs = new ArrayList<>();

        where.append(String.format("%s = ?", ContactsContract.Data.MIMETYPE));
        selectionArgs.add(ContactsContract.CommonDataKinds.Phone.CONTENT_ITEM_TYPE);
        where.append(String.format(" AND %s = 1", ContactsContract.CommonDataKinds.Phone.STARRED));
        if (TextUtils.isEmpty(accountName)) {
            where.append(
                    String.format(" AND %s IS NULL", ContactsContract.RawContacts.ACCOUNT_NAME));
        } else {
            where.append(String.format(" AND %s = ?", ContactsContract.RawContacts.ACCOUNT_NAME));
            selectionArgs.add(accountName);
        }
        return new QueryParam(ContactsContract.Data.CONTENT_URI,
                null,
                where.toString(),
                selectionArgs.toArray(new String[0]),
                ContactsContract.Contacts.DISPLAY_NAME + " ASC",
                Manifest.permission.READ_CONTACTS);
    }

    /**
     * Factory to create {@link BluetoothFavoriteContactsLiveData} instances via the {@link
     * AssistedInject} constructor.
     */
    @AssistedFactory
    public interface Factory {
        /** Creates a {@link BluetoothFavoriteContactsLiveData} instance. */
        BluetoothFavoriteContactsLiveData create(@Nullable String accountName);
    }
}
