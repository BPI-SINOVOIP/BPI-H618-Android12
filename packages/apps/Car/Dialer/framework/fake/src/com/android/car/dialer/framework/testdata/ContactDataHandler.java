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

package com.android.car.dialer.framework.testdata;

import android.content.ContentProviderOperation;
import android.content.Context;
import android.content.OperationApplicationException;
import android.os.RemoteException;
import android.provider.ContactsContract;
import android.text.TextUtils;

import com.android.car.apps.common.log.L;

import java.util.ArrayList;
import java.util.List;

import javax.inject.Inject;
import javax.inject.Singleton;

import dagger.hilt.android.qualifiers.ApplicationContext;

/**
 * A handler for adding contact data
 */
@Singleton
public class ContactDataHandler {
    private static final String TAG = "CD.ContactDataHandler";

    private final Context mContext;
    private final WorkerExecutor mWorkerExecutor;
    private final DataParser mDataParser;

    @Inject
    ContactDataHandler(@ApplicationContext Context context, WorkerExecutor workerExecutor,
            DataParser dataParser) {
        mContext = context;
        mWorkerExecutor = workerExecutor;
        mDataParser = dataParser;
    }

    /**
     * Adds contact data in a json file to contact database contacts2.db in Contacts Provider.
     */
    public void addContactsAsync(String file, String accountName, String accountType) {
        List<ContactRawData> list = mDataParser.parseContactData(mContext, file);
        addContactsAsync(list, accountName, accountType);
    }

    /**
     * Adds a list of {@link ContactRawData} that contains contact data to database contacts2.db in
     * Contacts Provider.
     */
    public void addContactsAsync(List<ContactRawData> list, String accountName,
            String accountType) {

        Runnable runnable = () -> {
            for (ContactRawData rawData : list) {
                addOneContact(rawData, accountName, accountType);
            }
        };

        mWorkerExecutor.run(runnable);
    }

    /**
     * Add a single contact to the database.
     */
    public void addOneContact(ContactRawData contactRawData, String accountName,
            String accountType) {
        ArrayList<ContentProviderOperation> ops = new ArrayList<>();

        String id = contactRawData.getId();
        Integer starred = contactRawData.getStarred();
        String displayName = contactRawData.getDisplayName();
        String number = contactRawData.getNumber();
        Integer type = contactRawData.getNumberType();
        String label = contactRawData.getNumberLabel();
        String address = contactRawData.getAddress();

        //TODO: reduce code reuse for this part and add randomly generated data.
        ops.add(ContentProviderOperation.newInsert(ContactsContract.RawContacts.CONTENT_URI)
                .withValue(ContactsContract.RawContacts.ACCOUNT_NAME, accountName)
                .withValue(ContactsContract.RawContacts.ACCOUNT_TYPE, accountType)
                .withValue(ContactsContract.RawContacts._ID, id)
                .withValue(ContactsContract.RawContacts.STARRED, starred)
                .build());

        ops.add(ContentProviderOperation.newInsert(ContactsContract.Data.CONTENT_URI)
                .withValue(ContactsContract.Data.RAW_CONTACT_ID, id)
                .withValue(ContactsContract.Data.MIMETYPE,
                        ContactsContract.CommonDataKinds.StructuredName.CONTENT_ITEM_TYPE)
                .withValue(ContactsContract.CommonDataKinds.StructuredName.DISPLAY_NAME,
                        displayName)
                .build());

        ops.add(ContentProviderOperation.newInsert(ContactsContract.Data.CONTENT_URI)
                .withValue(ContactsContract.Data.RAW_CONTACT_ID, id)
                .withValue(ContactsContract.Data.MIMETYPE,
                        ContactsContract.CommonDataKinds.Phone.CONTENT_ITEM_TYPE)
                .withValue(ContactsContract.CommonDataKinds.Phone.NUMBER, number)
                .withValue(ContactsContract.CommonDataKinds.Phone.TYPE, type)
                .withValue(ContactsContract.CommonDataKinds.Phone.LABEL, label)
                .build());

        if (!TextUtils.isEmpty(address)) {
            ops.add(ContentProviderOperation.newInsert(ContactsContract.Data.CONTENT_URI)
                    .withValue(ContactsContract.Data.RAW_CONTACT_ID, id)
                    .withValue(ContactsContract.Data.MIMETYPE,
                            ContactsContract.CommonDataKinds.StructuredPostal.CONTENT_ITEM_TYPE)
                    .withValue(ContactsContract.CommonDataKinds.StructuredPostal.FORMATTED_ADDRESS,
                            address)
                    .build());
        }

        try {
            mContext.getContentResolver().applyBatch(ContactsContract.AUTHORITY, ops);
        } catch (RemoteException e) {
            L.e(TAG,
                    "thrown if a RemoteException is encountered while attempting to communicate "
                            + "with a remote provider.");
        } catch (OperationApplicationException e) {
            L.e(TAG,
                    "thrown if a OperationApplicationException is encountered when an operation "
                            + "fails.");
        }
    }

    /**
     * Tears down the instance.
     */
    public void tearDown() {
        removeAddedContactsAsync();
    }

    /**
     * Removes contacts that are added to contacts2 database after this instance is created.
     */
    public void removeAddedContactsAsync() {
        // to be implemented.
    }

    /**
     * Remove every contact data piece in the contact database.
     */
    public void removeAllContacts() {
        // to be implemented.
    }

    /**
     * Removes the contacts.
     *
     * @param id is a unique identifier for a raw contact.
     */
    public void removeContact(String id) {
        Runnable runnable = () -> {
            ArrayList<ContentProviderOperation> ops = new ArrayList<>();

            ops.add(ContentProviderOperation.newDelete(ContactsContract.RawContacts.CONTENT_URI)
                    .withSelection(ContactsContract.RawContacts._ID + "=?", new String[]{id})
                    .build());

            try {
                mContext.getContentResolver().applyBatch(ContactsContract.AUTHORITY, ops);
            } catch (RemoteException e) {
                L.e(TAG,
                        "thrown if a RemoteException is encountered while attempting to "
                                + "communicate with a remote provider.");
            } catch (OperationApplicationException e) {
                L.e(TAG,
                        "thrown if a OperationApplicationException is encountered when a delete "
                                + "operation fails.");
            }
        };

        mWorkerExecutor.run(runnable);
    }
}
