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

import android.database.Cursor;
import android.provider.Telephony.Sms;

import androidx.annotation.NonNull;

import java.time.Instant;

/** SMS Utils for parsing SMS Telephony Content */
class SmsUtils {

    SmsUtils() {}

    /**
     * Returns the parsed sms result as a {@link MmsSmsMessage}
     *
     * @throws IllegalArgumentException if desired columns are missing.
     * @see CursorUtils#CONTENT_CONVERSATION_PROJECTION
     */
    @NonNull
    static MmsSmsMessage parseSms(@NonNull Cursor cursor) {
        int threadIdIndex = cursor.getColumnIndex(Sms.THREAD_ID);
        int recipientsIndex = cursor.getColumnIndex(Sms.ADDRESS);
        int bodyIndex = cursor.getColumnIndex(Sms.BODY);
        int subscriptionIdIndex = cursor.getColumnIndex(Sms.SUBSCRIPTION_ID);
        int dateIndex = cursor.getColumnIndex(Sms.DATE);
        int typeIndex = cursor.getColumnIndex(Sms.TYPE);
        int readIndex = cursor.getColumnIndex(Sms.READ);

        MmsSmsMessage message = new MmsSmsMessage();
        message.mThreadId = cursor.getInt(threadIdIndex);
        message.mPhoneNumber = cursor.getString(recipientsIndex);
        message.mBody = cursor.getString(bodyIndex);
        message.mSubscriptionId = cursor.getInt(subscriptionIdIndex);
        message.mType = cursor.getInt(typeIndex);
        message.mDate = Instant.ofEpochMilli(cursor.getLong(dateIndex));
        message.mRead = cursor.getInt(readIndex) == 1;
        message.mId = cursor.getString(cursor.getColumnIndex(_ID));
        return message;
    }
}
