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

package android.app.cts.android.app.cts.tools;

import android.app.NotificationManager;
import android.app.PendingIntent.CanceledException;
import android.app.stubs.TestNotificationListener;
import android.content.Context;
import android.service.notification.StatusBarNotification;

import java.util.function.Supplier;

public class NotificationHelper {
    public static final long SHORT_WAIT_TIME = 100;
    public static final long MAX_WAIT_TIME = 2000;

    private final Context mContext;
    private final NotificationManager mNotificationManager;
    private Supplier<TestNotificationListener> mNotificationListener;

    public NotificationHelper(Context context, Supplier<TestNotificationListener> listener) {
        mContext = context;
        mNotificationManager = mContext.getSystemService(NotificationManager.class);
        mNotificationListener = listener;
    }

    public void clickNotification(int notificationId, boolean searchAll) throws CanceledException {
        findPostedNotification(notificationId, searchAll).getNotification().contentIntent.send();
    }

    public StatusBarNotification findPostedNotification(int id, boolean all) {
        // notification is a bit asynchronous so it may take a few ms to appear in
        // getActiveNotifications()
        // we will check for it for up to 1000ms before giving up
        for (long totalWait = 0; totalWait < MAX_WAIT_TIME; totalWait += SHORT_WAIT_TIME) {
            StatusBarNotification n = findNotificationNoWait(id, all);
            if (n != null) {
                return n;
            }
            try {
                Thread.sleep(SHORT_WAIT_TIME);
            } catch (InterruptedException ex) {
                // pass
            }
        }
        return findNotificationNoWait(id, all);
    }

    public StatusBarNotification findNotificationNoWait(int id, boolean all) {
        for (StatusBarNotification sbn : getActiveNotifications(all)) {
            if (sbn.getId() == id) {
                return sbn;
            }
        }
        return null;
    }

    public StatusBarNotification[] getActiveNotifications(boolean all) {
        if (all) {
            return mNotificationListener.get().getActiveNotifications();
        } else {
            return mNotificationManager.getActiveNotifications();
        }
    }
}
