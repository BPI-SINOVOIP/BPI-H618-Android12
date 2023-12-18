/*
 * Copyright (C) 2021 The Android Open Source Project
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

package com.android.server.wifi.util;

import android.annotation.IntDef;
import android.util.SparseArray;

import java.io.PrintWriter;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;

/**
 * Manage multiple last caller info
 */
public class LastCallerInfoManager {
    /** @hide */
    @Retention(RetentionPolicy.SOURCE)
    @IntDef(value = {
            SCANNING_ENABLED,
            WIFI_ENABLED,
            SOFT_AP,
            TETHERED_HOTSPOT,
            AUTOJOIN_GLOBAL})
    public @interface ApiType {}
    public static final int SCANNING_ENABLED = 1;
    public static final int WIFI_ENABLED = 2;
    public static final int SOFT_AP = 3;
    public static final int TETHERED_HOTSPOT = 4;
    public static final int AUTOJOIN_GLOBAL = 5;

    private final SparseArray<LastCallerInfo> mLastCallerInfoMap = new SparseArray<>();

    /**
     * Store the last caller information for the API
     */
    public void put(@ApiType int apiName, int tid, int uid, int pid, String packageName,
            boolean toggleState) {
        synchronized (mLastCallerInfoMap) {
            LastCallerInfo callerInfo = new LastCallerInfo(tid, uid, pid, packageName, toggleState);
            mLastCallerInfoMap.put(apiName, callerInfo);
        }
    }

    /**
     * Convert int constant into API String name
     */
    private String convertApiName(@ApiType int key) {
        switch (key) {
            case SCANNING_ENABLED:
                return "ScanningEnabled";
            case WIFI_ENABLED:
                return "WifiEnabled";
            case SOFT_AP:
                return "SoftAp";
            case TETHERED_HOTSPOT:
                return "TetheredHotspot";
            case AUTOJOIN_GLOBAL:
                return "AutojoinGlobal";
            default:
                return "Unknown";
        }
    }

    /**
     * Print the last caller info for the APIs tracked
     */
    public void dump(PrintWriter pw) {
        pw.println("Dump of LastCallerInfoManager");
        for (int i = 0; i < mLastCallerInfoMap.size(); i++) {
            String apiName = convertApiName(mLastCallerInfoMap.keyAt(i));
            String callerInfo = mLastCallerInfoMap.valueAt(i).toString();
            pw.println(apiName + ": " + callerInfo);
        }
    }

    /**
     * Last caller info
     */
    public static class LastCallerInfo {
        private int mTid;
        private int mUid;
        private int mPid;
        private String mPackageName;
        private boolean mToggleState;

        public LastCallerInfo(int tid, int uid, int pid, String packageName, boolean toggleState) {
            mTid = tid;
            mUid = uid;
            mPid = pid;
            mPackageName = packageName;
            mToggleState = toggleState;
        }

        /**
         * Convert the last caller info into String format
         */
        public String toString() {
            StringBuilder sb = new StringBuilder();
            sb.append("tid=").append(mTid).append(" uid=").append(mUid)
                    .append(" pid=").append(mPid).append(" packageName=").append(mPackageName)
                    .append(" toggleState=").append(mToggleState);
            return sb.toString();
        }
    }
}
