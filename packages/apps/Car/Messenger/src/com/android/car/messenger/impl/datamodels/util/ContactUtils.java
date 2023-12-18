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
package com.android.car.messenger.impl.datamodels.util;

import static android.provider.BaseColumns._ID;
import static android.provider.ContactsContract.PhoneLookup.CONTENT_FILTER_URI;
import static android.provider.Telephony.ThreadsColumns.RECIPIENT_IDS;

import android.content.ContentUris;
import android.content.Context;
import android.content.res.AssetFileDescriptor;
import android.database.Cursor;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.net.Uri;
import android.provider.ContactsContract;
import android.provider.Telephony.MmsSms;
import android.text.TextUtils;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.core.app.Person;

import com.android.car.messenger.core.interfaces.AppFactory;
import com.android.car.messenger.core.util.L;

import java.io.IOException;
import java.io.InputStream;
import java.util.ArrayList;
import java.util.List;
import java.util.function.BiConsumer;

/** Contact Utils for getting information on a contact */
public class ContactUtils {
    @NonNull
    private static final Uri SINGLE_CANONICAL_ADDRESS_URI =
            MmsSms.CONTENT_URI.buildUpon().appendPath("canonical-address").build();

    @NonNull private static final String RECIPIENT_SPLIT_SEPARATOR = " ";
    @NonNull public static final String DRIVER_NAME = "Driver";

    @NonNull
    private static final String[] PROJECTION =
            new String[] {
                ContactsContract.PhoneLookup.DISPLAY_NAME,
                ContactsContract.PhoneLookup.CONTACT_ID,
                _ID,
                ContactsContract.PhoneLookup.PHOTO_ID,
                ContactsContract.PhoneLookup.PHOTO_THUMBNAIL_URI,
                ContactsContract.PhoneLookup.PHOTO_FILE_ID,
                ContactsContract.PhoneLookup.PHOTO_URI
            };

    private ContactUtils() {}
    /**
     * Get the list of recipients as {@link Person} for the given conversation id
     *
     * @param conversationId The conversation id to retrieve the list of participants
     * @param processParticipant A nullable method to further process an individual participant
     */
    public static List<Person> getRecipients(
            @NonNull String conversationId,
            @Nullable BiConsumer<String, Bitmap> processParticipant) {
        String[] recipientIds = getRecipientIds(conversationId);
        List<Person> participants = new ArrayList<>();
        Context context = AppFactory.get().getContext();
        for (String contactId : recipientIds) {
            long contactIdLong = Long.parseLong(contactId);
            String number = getCanonicalAddressesFromRecipientIds(context, contactIdLong);
            if (number == null) {
                L.e("No phone number found for contactId: " + contactId);
                continue;
            }
            Person person = getPerson(context, number, processParticipant);
            participants.add(person);
        }

        return participants;
    }

    private static String[] getRecipientIds(@NonNull String conversationId) {
        Cursor threadCursor = CursorUtils.getThreadCursor(conversationId);
        threadCursor.moveToFirst();
        return threadCursor
                .getString(threadCursor.getColumnIndex(RECIPIENT_IDS))
                .split(RECIPIENT_SPLIT_SEPARATOR);
    }

    /**
     * Get Profile information for the contact, including the contact name and the contact avatar if
     * available
     */
    @NonNull
    static Person getPerson(
            @NonNull Context context,
            @NonNull String phoneNo,
            @Nullable BiConsumer<String, Bitmap> processParticipant) {
        String name = phoneNo;
        Bitmap bitmap = null;
        Cursor cursor = null;
        try {
            Uri uri = CONTENT_FILTER_URI.buildUpon().appendEncodedPath(Uri.encode(phoneNo)).build();
            cursor = CursorUtils.simpleQueryWithProjection(context, uri, PROJECTION);
        } catch (IllegalArgumentException e) {
            L.w("Unable to retrieve PhoneLookup cursor");
            L.w(e.toString());
        }

        if (cursor != null && cursor.moveToFirst()) {
            name =
                    cursor.getString(
                            cursor.getColumnIndex(ContactsContract.PhoneLookup.DISPLAY_NAME));
            String thumbnailPath =
                    cursor.getString(cursor.getColumnIndex(ContactsContract.PhoneLookup.PHOTO_URI));

            if (thumbnailPath != null && processParticipant != null) {
                try {
                    Uri thumbnailUri = Uri.parse(thumbnailPath);
                    AssetFileDescriptor fd =
                            context.getContentResolver().openAssetFileDescriptor(thumbnailUri, "r");
                    if (fd != null) {
                        InputStream stream = fd.createInputStream();
                        bitmap = BitmapFactory.decodeStream(stream);
                        fd.close();
                    }
                } catch (IOException e) {
                    L.e(e.toString());
                }
            }
        }

        if (cursor != null && !cursor.isClosed()) {
            cursor.close();
        }

        // don't include icon when building out the Person class in order
        // to reduce the size of individual messages, instead pass it to the caller
        // to build out avatar for the entire conversation
        if (processParticipant != null) {
            bitmap = AvatarUtil.resolvePersonAvatar(context, bitmap, name);
            processParticipant.accept(name, bitmap);
        }

        return new Person.Builder().setUri(phoneNo).setName(name).build();
    }

    @Nullable
    private static String getCanonicalAddressesFromRecipientIds(
            @NonNull Context context, long contactId) {
        Cursor cursor =
                CursorUtils.simpleQuery(
                        context,
                        ContentUris.withAppendedId(SINGLE_CANONICAL_ADDRESS_URI, contactId));
        if (cursor != null) {
            try {
                if (cursor.moveToFirst()) {
                    String rawNumber = cursor.getString(0);
                    if (!TextUtils.isEmpty(rawNumber)) {
                        return rawNumber;
                    }
                }
            } finally {
                cursor.close();
            }
        }
        L.w("No canonical address found for recipient id");
        return null;
    }
}
