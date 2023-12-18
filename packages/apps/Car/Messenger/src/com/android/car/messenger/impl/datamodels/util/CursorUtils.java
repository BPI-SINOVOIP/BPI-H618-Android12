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
import static android.provider.Telephony.BaseMmsColumns.CONTENT_TYPE;
import static android.provider.Telephony.BaseMmsColumns.MESSAGE_BOX;
import static android.provider.Telephony.MmsSms.CONTENT_CONVERSATIONS_URI;
import static android.provider.Telephony.TextBasedSmsColumns.ADDRESS;
import static android.provider.Telephony.TextBasedSmsColumns.BODY;
import static android.provider.Telephony.TextBasedSmsColumns.SUBSCRIPTION_ID;
import static android.provider.Telephony.TextBasedSmsColumns.THREAD_ID;
import static android.provider.Telephony.TextBasedSmsColumns.TYPE;
import static android.provider.Telephony.ThreadsColumns.DATE;
import static android.provider.Telephony.ThreadsColumns.READ;
import static android.provider.Telephony.ThreadsColumns.RECIPIENT_IDS;

import android.content.ContentResolver;
import android.content.Context;
import android.database.Cursor;
import android.net.Uri;
import android.provider.Telephony;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import com.android.car.messenger.core.interfaces.AppFactory;

/** Cursor Utils to get quick cursor or uri telephony information */
public class CursorUtils {
    private CursorUtils() {}
    // This URI provides all the metadata for a thread.
    // Appending simple=true is important to get the recipient_ids for the conversation.
    @NonNull
    public static final Uri THREAD_INFO_URI =
            CONTENT_CONVERSATIONS_URI.buildUpon().appendQueryParameter("simple", "true").build();

    @NonNull protected static final String[] THREAD_INFO_PROJECTION = {_ID, RECIPIENT_IDS, READ};

    @NonNull
    protected static final String[] CONTENT_CONVERSATION_PROJECTION = {
        _ID, TYPE, DATE, READ, CONTENT_TYPE, BODY, ADDRESS, THREAD_ID, SUBSCRIPTION_ID, MESSAGE_BOX
    };

    /** Provides the default sort order for items in database. Default is DESC order by Date. */
    @NonNull
    public static final String DEFAULT_SORT_ORDER = Telephony.TextBasedSmsColumns.DATE + " DESC";

    /**
     * Get simplified thread cursor with metadata information on the thread, such as recipient ids
     */
    @Nullable
    public static Cursor getThreadCursor(@NonNull String threadId) {
        Context context = AppFactory.get().getContext();
        ContentResolver contentResolver = context.getContentResolver();
        return contentResolver.query(
                THREAD_INFO_URI,
                THREAD_INFO_PROJECTION,
                _ID + "=" + threadId,
                /* selectionArgs= */ null,
                DEFAULT_SORT_ORDER);
    }

    /**
     * Get the message cursor in descending order for
     *
     * @param conversationId The conversation or thread id for the conversation
     * @param limit The maximum number of message rows to fetch
     * @param offset The starting point in timestamp in millisecond to fetch for data
     */
    @Nullable
    public static Cursor getMessagesCursor(@NonNull String conversationId, int limit, long offset) {
        Context context = AppFactory.get().getContext();
        ContentResolver contentResolver = context.getContentResolver();
        return contentResolver.query(
                getConversationUri(conversationId),
                CONTENT_CONVERSATION_PROJECTION,
                DATE + " > " + offset,
                /* selectionArgs= */ null,
                DEFAULT_SORT_ORDER + " LIMIT " + limit);
    }

    /** Gets the Conversation Uri for the Conversation with specified conversationId */
    @NonNull
    public static Uri getConversationUri(@NonNull String conversationId) {
        return CONTENT_CONVERSATIONS_URI.buildUpon().appendPath(conversationId).build();
    }

    /** Returns a cursor query with the uri provided, with no filtering or projection */
    @Nullable
    public static Cursor simpleQuery(@NonNull Context context, @NonNull Uri uri) {
        return context.getContentResolver().query(uri, null, null, null, null);
    }

    /** Returns a cursor query given a uri and projection */
    @Nullable
    public static Cursor simpleQueryWithProjection(
            @NonNull Context context, @NonNull Uri uri, @Nullable String[] projection) {
        return context.getContentResolver().query(uri, projection, null, null, null);
    }

    /** Returns a cursor query given a uri and selection */
    @Nullable
    public static Cursor simpleQueryWithSelection(
            @NonNull Context context, @NonNull Uri uri, @Nullable String selection) {
        return context.getContentResolver().query(uri, null, selection, null, null);
    }
}
