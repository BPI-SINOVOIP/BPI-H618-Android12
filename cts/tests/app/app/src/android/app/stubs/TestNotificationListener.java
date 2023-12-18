/*
 * Copyright (C) 2018 The Android Open Source Project
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
package android.app.stubs;

import android.content.ComponentName;
import android.os.ConditionVariable;
import android.service.notification.NotificationListenerService;
import android.service.notification.StatusBarNotification;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.Map;

public class TestNotificationListener extends NotificationListenerService {
    public static final String TAG = "TestNotificationListener";
    public static final String PKG = "android.app.stubs";
    private static final long CONNECTION_TIMEOUT_MS = 1000;

    private ArrayList<String> mTestPackages = new ArrayList<>();

    public ArrayList<StatusBarNotification> mPosted = new ArrayList<>();
    public Map<String, Integer> mRemoved = new HashMap<>();
    public RankingMap mRankingMap;

    /**
     * This controls whether there is a listener connected or not. Depending on the method, if the
     * caller tries to use a listener after it has disconnected, NMS can throw a SecurityException.
     *
     * There is no race between onListenerConnected() and onListenerDisconnected() because they are
     * called in the same thread. The value that getInstance() sees is guaranteed to be the value
     * that was set by onListenerConnected() because of the happens-before established by the
     * condition variable.
     */
    private static final ConditionVariable INSTANCE_AVAILABLE = new ConditionVariable(false);
    private static TestNotificationListener sNotificationListenerInstance = null;
    boolean isConnected;

    public static String getId() {
        return String.format("%s/%s", TestNotificationListener.class.getPackage().getName(),
                TestNotificationListener.class.getName());
    }

    public static ComponentName getComponentName() {
        return new ComponentName(TestNotificationListener.class.getPackage().getName(),
                TestNotificationListener.class.getName());
    }

    @Override
    public void onCreate() {
        super.onCreate();
        mTestPackages.add(PKG);
    }

    @Override
    public void onListenerConnected() {
        super.onListenerConnected();
        sNotificationListenerInstance = this;
        INSTANCE_AVAILABLE.open();
        isConnected = true;
    }

    @Override
    public void onListenerDisconnected() {
        INSTANCE_AVAILABLE.close();
        sNotificationListenerInstance = null;
        isConnected = false;
    }

    public static TestNotificationListener getInstance() {
        if (INSTANCE_AVAILABLE.block(CONNECTION_TIMEOUT_MS)) {
            return sNotificationListenerInstance;
        }
        return null;
    }

    public void resetData() {
        mPosted.clear();
        mRemoved.clear();
    }

    public void addTestPackage(String packageName) {
        mTestPackages.add(packageName);
    }

    public void removeTestPackage(String packageName) {
        mTestPackages.remove(packageName);
    }

    @Override
    public void onNotificationPosted(StatusBarNotification sbn, RankingMap rankingMap) {
        if (sbn == null || !mTestPackages.contains(sbn.getPackageName())) { return; }
        mRankingMap = rankingMap;
        mPosted.add(sbn);
    }

    @Override
    public void onNotificationRemoved(StatusBarNotification sbn, RankingMap rankingMap,
            int reason) {
        if (sbn == null || !mTestPackages.contains(sbn.getPackageName())) { return; }
        mRankingMap = rankingMap;
        mRemoved.put(sbn.getKey(), reason);
    }

    @Override
    public void onNotificationRankingUpdate(RankingMap rankingMap) {
        mRankingMap = rankingMap;
    }
}
