/*
 * Copyright (C) 2014 The Android Open Source Project
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

package com.android.tv.launcher.device.apps;

import android.app.Application;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.ApplicationInfo;
import android.content.pm.IPackageStatsObserver;
import android.content.pm.PackageManager;
import android.content.pm.PackageStats;
import android.content.pm.PackageManager.NameNotFoundException;
import android.graphics.drawable.Drawable;
import android.net.Uri;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;
import android.os.Message;
import android.os.Process;
import android.os.SystemClock;
import android.os.UserHandle;
import android.text.format.Formatter;
import android.os.storage.StorageManager;
import android.app.usage.StorageStats;
import android.app.usage.StorageStatsManager;
import com.android.settingslib.applications.StorageStatsSource;
import com.android.settingslib.applications.StorageStatsSource.AppStorageStats;
import android.os.RemoteException;
import android.util.Log;

import java.io.IOException;
import java.io.File;
import java.text.Collator;
import java.text.Normalizer;
import java.text.Normalizer.Form;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.HashMap;
import java.util.List;
import java.util.regex.Pattern;
import java.util.Locale;
import java.util.Timer;
import java.util.TimerTask;

/**
 * Keeps track of information about all installed applications, lazy-loading as
 * needed.
 */
public class ApplicationsState {
    private static final String TAG = "ApplicationsState";
    private static final boolean DEBUG = true;

    public static interface Callbacks {
        public void onRunningStateChanged(boolean running);
        public void onPackageListChanged();
        public void onRebuildComplete();
        public void onPackageIconChanged();
        public void onPackageSizeChanged(String packageName);
        public void onAllSizesComputed();
    }

    static final int SIZE_UNKNOWN = -1;
    static final int SIZE_INVALID = -2;

    static final Pattern REMOVE_DIACRITICALS_PATTERN = Pattern.compile(
            "\\p{InCombiningDiacriticalMarks}+");

    public static String normalize(String str) {
        String tmp = Normalizer.normalize(str, Form.NFD);
        return REMOVE_DIACRITICALS_PATTERN.matcher(tmp).replaceAll("").toLowerCase();
    }

    public static class AppEntry {
        long cacheSize; //缓存大小
        long codeSize; //程序大小
        long dataSize; //数据大小
        long externalCodeSize;
        long externalDataSize;
        long externalCacheSize;

        final File apkFile;
        boolean mounted; //标示apk文件是否存在

        final long id; //AppEntry的id值，好像是没什么用
        String label; //包名
        String normalizedLabel;//序列化后的包名
        long internalSize;  //dataSize+codeSize
        long externalSize;  //externalDataSize+externalCodeSize

        ApplicationInfo info; //当前AppEntry的主要信息
        long size;  //internalSize+externalSize
        String sizeStr; //size的K或B或M的表示方式
        long sizeLoadStart; //开始加载的时间
        boolean sizeStale; //标示是否需要去获取包大小

        AppEntry(Context context, ApplicationInfo info, long id) {
            apkFile = new File(info.sourceDir);

            this.id = id;
            this.info = info;
            this.size = SIZE_UNKNOWN;
            this.sizeStale = true;
            ensureLabel(context);
        }

        void ensureLabel(Context context) {
            if (this.label == null || !this.mounted) {
                if (!this.apkFile.exists()) {
                    this.mounted = false;
                    this.label = info.packageName;
                } else {
                    this.mounted = true;
                    CharSequence label = info.loadLabel(context.getPackageManager());
                    this.label = label != null ? label.toString() : info.packageName;
                }
            }

        }

        String getNormalizedLabel() {

            if (normalizedLabel != null) {
                return normalizedLabel;
            }
            normalizedLabel = normalize(label);

            return normalizedLabel;
        }

        String getVersion(Context context) {

            try {
                String tmp = context.getPackageManager().getPackageInfo(info.packageName, 0).versionName;

                return tmp;
            } catch (NameNotFoundException e) {
                return "";
            }
        }
    }//AppEntry

    // Information about all applications. Synchronize on mEntriesMap
    // to protect access to these.
    private final ArrayList<Session> mSessions = new ArrayList<Session>();
    private final InterestingConfigChanges mInterestingConfigChanges = new InterestingConfigChanges();
    public final HashMap<String, AppEntry> mEntriesMap = new HashMap<String, AppEntry>(); //存放包名和AppEntry的map
    public final ArrayList<AppEntry> mAppEntries = new ArrayList<AppEntry>(); //存放AppEntry的list
    private List<ApplicationInfo> mApplications = new ArrayList<ApplicationInfo>();
    private long mCurId = 1; //创建AppEntry时需要的第三个参数id，逐渐增大
    private String mCurComputingSizePkg; //当前正在计算大小的 包名
    private boolean mSessionsChanged;

    // Temporary for dispatching session callbacks. Only touched by main thread.
    private final ArrayList<Session> mActiveSessions = new ArrayList<Session>();

    /**
     * Receives notifications when applications are added/removed.
     * 接收应用添加和去除的通知
     */
    private class PackageIntentReceiver extends BroadcastReceiver {
        void registerReceiver() {

            IntentFilter filter = new IntentFilter(Intent.ACTION_PACKAGE_ADDED);
            filter.addAction(Intent.ACTION_PACKAGE_REMOVED);
            filter.addAction(Intent.ACTION_PACKAGE_CHANGED);
            filter.addDataScheme("package");
            mContext.registerReceiver(this, filter);

            IntentFilter sdFilter = new IntentFilter();// Register for events related to sdcard installation.
            sdFilter.addAction(Intent.ACTION_EXTERNAL_APPLICATIONS_AVAILABLE);
            sdFilter.addAction(Intent.ACTION_EXTERNAL_APPLICATIONS_UNAVAILABLE);
            mContext.registerReceiver(this, sdFilter);
        }

        void unregisterReceiver() {

            mContext.unregisterReceiver(this);
        }

        @Override
        public void onReceive(Context context, Intent intent) {
            String actionStr = intent.getAction();

            if (Intent.ACTION_PACKAGE_ADDED.equals(actionStr)) {
                Uri data = intent.getData();
                String pkgName = data.getEncodedSchemeSpecificPart();

                addPackage(pkgName);
            } else if (Intent.ACTION_PACKAGE_REMOVED.equals(actionStr)) {
                Uri data = intent.getData();
                String pkgName = data.getEncodedSchemeSpecificPart();
                removePackage(pkgName);
            } else if (Intent.ACTION_PACKAGE_CHANGED.equals(actionStr)) {
                Uri data = intent.getData();
                String pkgName = data.getEncodedSchemeSpecificPart();
                invalidatePackage(pkgName);
            } else if (Intent.ACTION_EXTERNAL_APPLICATIONS_AVAILABLE.equals(actionStr)
                    || Intent.ACTION_EXTERNAL_APPLICATIONS_UNAVAILABLE.equals(actionStr)) {
            	//当插拔SD卡时的消息
                String pkgList[] = intent.getStringArrayExtra(Intent.EXTRA_CHANGED_PACKAGE_LIST);
                if (pkgList == null || pkgList.length == 0) {
                    return;
                }
                boolean avail = Intent.ACTION_EXTERNAL_APPLICATIONS_AVAILABLE.equals(actionStr);
                if (avail) {
                    for (String pkgName : pkgList) {
                        invalidatePackage(pkgName);
                    }
                }
            }
        }
    }//PackageIntentReceiver

    void rebuildActiveSessions() {
        if (!mSessionsChanged) {
            return;
        }
        mActiveSessions.clear();
        for (int i = 0; i < mSessions.size(); i++) {
            Session s = mSessions.get(i);
            if (s.mResumed) {
                mActiveSessions.add(s);
            }
        }
    }

    class MainHandler extends Handler {
        static final int MSG_REBUILD_COMPLETE = 1;
        static final int MSG_PACKAGE_LIST_CHANGED = 2;
        static final int MSG_PACKAGE_ICON_CHANGED = 3;
        static final int MSG_PACKAGE_SIZE_CHANGED = 4;
        static final int MSG_ALL_SIZES_COMPUTED = 5;
        static final int MSG_RUNNING_STATE_CHANGED = 6;
        @Override
        public void handleMessage(Message msg) {

            switch (msg.what) {
                case MSG_REBUILD_COMPLETE:
                    {
                        Session s = (Session) msg.obj;
                        if (mActiveSessions.contains(s)) {
                            s.mCallbacks.onRebuildComplete();
                        }
                    }
                    break;
                case MSG_PACKAGE_LIST_CHANGED:
                    for (int i = 0; i < mActiveSessions.size(); i++) {
                        mActiveSessions.get(i).mCallbacks.onPackageListChanged();
                    }
                    break;
                case MSG_PACKAGE_ICON_CHANGED:
                    for (int i = 0; i < mActiveSessions.size(); i++) {
                        mActiveSessions.get(i).mCallbacks.onPackageIconChanged();
                    }
                    break;
                case MSG_PACKAGE_SIZE_CHANGED:
                    for (int i = 0; i < mActiveSessions.size(); i++) {
                        mActiveSessions.get(i).mCallbacks.onPackageSizeChanged((String) msg.obj);
                    }
                    break;
                case MSG_ALL_SIZES_COMPUTED:
                    for (int i = 0; i < mActiveSessions.size(); i++) {
                        mActiveSessions.get(i).mCallbacks.onAllSizesComputed();
                    }
                    break;
                case MSG_RUNNING_STATE_CHANGED:
                    for (int i = 0; i < mActiveSessions.size(); i++) {
                        mActiveSessions.get(i).mCallbacks.onRunningStateChanged(msg.arg1 != 0);
                    }
                    break;
            }
        }
    }

    private final MainHandler mMainHandler = new MainHandler();

    private final Context mContext;
    private final PackageManager mPackManager;
    private final StorageStatsSource mSource;
    private final int mRetrieveFlags;
    private PackageIntentReceiver mPackageIntentReceiver;
    private final HandlerThread mThread;
    private final BackgroundHandler mBackgroundHandler;
    private boolean mResumed;
    private static ApplicationsState sInstance;
    private static final Object sLock = new Object();
    final StorageStatsManager mStats;
    private Locale mCurrentLocale;
    private Intent mLoadMainIntent = new Intent(Intent.ACTION_MAIN, null);
    //单例
    public static ApplicationsState getInstance(Context app) {

        synchronized (sLock) {

            if (sInstance == null) {
                sInstance = new ApplicationsState(app);
            }
            return sInstance;
        }
    }

    private Timer mLoadTimer = new Timer();

    private ApplicationsState(Context app) {

        mContext = app;
        mPackManager = mContext.getPackageManager();
        mSource = new StorageStatsSource(mContext);
        mThread = new HandlerThread("ApplicationsState.Loader", Process.THREAD_PRIORITY_BACKGROUND);
        mStats = mContext.getSystemService(StorageStatsManager.class);
        mCurrentLocale = Locale.getDefault();
        mThread.start();
        mBackgroundHandler = new BackgroundHandler(mThread.getLooper());

        // Only the owner can see all apps.
        if (UserHandle.myUserId() == 0) {
            mRetrieveFlags = PackageManager.GET_UNINSTALLED_PACKAGES | PackageManager.GET_DISABLED_COMPONENTS;
        } else {
            mRetrieveFlags = PackageManager.GET_DISABLED_COMPONENTS;
        }

        /**
         * This is a trick to prevent the foreground thread from being delayed.
         * The problem is that Dalvik monitors are initially spin locks, to keep
         * them lightweight. This leads to unfair contention -- Even though the
         * background thread only holds the lock for a short amount of time, if
         * it keeps running and locking again it can prevent the main thread
         * from acquiring its lock for a long time... sometimes even > 5 seconds
         * (leading to an ANR). Dalvik will promote a monitor to a "real" lock
         * if it detects enough contention on it. It doesn't figure this out
         * fast enough for us here, though, so this little trick will force it
         * to turn into a real lock immediately.
         */
        synchronized (mEntriesMap) {
            try {
                mEntriesMap.wait(1);
            } catch (InterruptedException e) {
            }
        }
        mLoadMainIntent.addCategory(Intent.CATEGORY_LAUNCHER);
        TimerTask timerTask = new TimerTask() {
            @Override
            public void run() {
                Log.w(TAG, "queryIntent size: " + mPackManager.queryIntentActivities(mLoadMainIntent, 0).size());
                if (mPackManager.queryIntentActivities(mLoadMainIntent, 0).size() > 0
                    && !mBackgroundHandler.hasMessages(BackgroundHandler.MSG_LOAD_ENTRIES)) {
                    mBackgroundHandler.sendEmptyMessage(BackgroundHandler.MSG_LOAD_ENTRIES);
                    cancel();
                }
            }
        };
        mLoadTimer.schedule(timerTask, 0, 1000L);
    }

    public class Session {
        private final Callbacks mCallbacks;
        private boolean mResumed;

        private Session(Callbacks callbacks) {

            mCallbacks = callbacks;
        }
        public void resume() {
            synchronized (mEntriesMap) {

                if (!mResumed) {
                    mResumed = true;
                    mSessionsChanged = true;
                    doResumeIfNeededLocked();
                    rebuildActiveSessions();
                }
            }
        }
        public void pause() {
            synchronized (mEntriesMap) {

                if (mResumed) {
                    mResumed = false;
                    mSessionsChanged = true;
                    mBackgroundHandler.removeMessages(BackgroundHandler.MSG_REBUILD_LIST, this);
                    doPauseIfNeededLocked();
                }
            }
        }
        public void release() {
            pause();
            synchronized (mEntriesMap) {
                mSessions.remove(this);
                rebuildActiveSessions();
            }
        }
    }//class Session

    public Session newSession(Callbacks callbacks) {
        Session s = new Session(callbacks);
        synchronized (mEntriesMap) {

            mSessions.add(s);
        }
        return s;
    }

    private void doResumeIfNeededLocked() {
        boolean localeHasChanged = !mCurrentLocale.equals(Locale.getDefault());
        if (mResumed && !localeHasChanged) {
            return;
        }
        else if(localeHasChanged) {
            mCurrentLocale = Locale.getDefault();
        }
        mResumed = true;
        if (mPackageIntentReceiver == null) {
            mPackageIntentReceiver = new PackageIntentReceiver();
            mPackageIntentReceiver.registerReceiver();
        }
        //获取已经安装了的应用列表
        mApplications = mPackManager.getInstalledApplications(mRetrieveFlags);
        if (mApplications == null) {
            mApplications = new ArrayList<ApplicationInfo>();
        }
        if (mInterestingConfigChanges.applyNewConfig(mContext.getResources())) {
            // If an interesting part of the configuration has changed, we
            // should completely reload the app entries.

            mEntriesMap.clear();
            mAppEntries.clear();
        } else {

            for (int i = 0; i < mAppEntries.size(); i++) {
                mAppEntries.get(i).sizeStale = true;
            }
        }

        for (int i = 0; i < mApplications.size(); i++) {
            final ApplicationInfo info = mApplications.get(i);

            // Need to trim out any applications that are disabled by
            // something different than the user.
            if (!info.enabled && info.enabledSetting != PackageManager.COMPONENT_ENABLED_STATE_DISABLED_USER) {
                mApplications.remove(i);  //移除不可用的应用
                i--;
                continue;
            }
            final AppEntry entry = mEntriesMap.get(info.packageName);
            if (entry != null) {  //更新对应的entry
                entry.info = info;
            }
        }
        mCurComputingSizePkg = null;
        if (!mBackgroundHandler.hasMessages(BackgroundHandler.MSG_LOAD_ENTRIES)) {
            mBackgroundHandler.sendEmptyMessage(BackgroundHandler.MSG_LOAD_ENTRIES);
        }
    }

    private void doPauseIfNeededLocked() {

        if (!mResumed) {

            return;
        }
        for (int i = 0; i < mSessions.size(); i++) {
            if (mSessions.get(i).mResumed) {  //检查是否所有的session都pause了
                return;
            }
        }
        mResumed = false;
        if (mPackageIntentReceiver != null) {
            mPackageIntentReceiver.unregisterReceiver();
            mPackageIntentReceiver = null;
        }
    }

    AppEntry getEntry(String packageName) {
        synchronized (mEntriesMap) {

            AppEntry entry = mEntriesMap.get(packageName);
            if (entry == null) {
                for (int i = 0; i < mApplications.size(); i++) {
                    ApplicationInfo info = mApplications.get(i);
                    if (packageName.equals(info.packageName)) {
                        entry = getEntryLocked(info); //获取一个新创建的entry
                        break;
                    }
                }
            }
            return entry;
        }
    }

    private String getSizeStr(long size) {
        if (size >= 0) {
            return Formatter.formatFileSize(mContext, size);
        }
        return null;
    }

    void requestSize(String packageName) {
        synchronized (mEntriesMap) {
            AppEntry entry = mEntriesMap.get(packageName);
            int userId = UserHandle.myUserId();
            if (entry != null && hasFlag(entry.info.flags, ApplicationInfo.FLAG_INSTALLED)) {
                mBackgroundHandler.postDelayed(
                        () -> {
                            try {
                                final StorageStats stats =
                                        mStats.queryStatsForPackage(
                                                entry.info.storageUuid,
                                                packageName,
                                                UserHandle.of(userId));
                                final long cacheQuota =
                                        mStats.getCacheQuotaBytes(
                                                entry.info.storageUuid.toString(), entry.info.uid);
                                final PackageStats legacy = new PackageStats(packageName, userId);
                                legacy.codeSize = stats.getAppBytes();
                                legacy.dataSize = stats.getDataBytes();
                                legacy.cacheSize = Math.min(stats.getCacheBytes(), cacheQuota);
                                try {
                                    mBackgroundHandler.mStatsObserver.onGetStatsCompleted(
                                            legacy, true);
                                } catch (RemoteException ignored) {
                                }
                            } catch (NameNotFoundException | IOException e) {
                                Log.w(TAG, "Failed to query stats: " + e);
                                try {
                                    mBackgroundHandler.mStatsObserver.onGetStatsCompleted(
                                            null, false);
                                } catch (RemoteException ignored) {
                                }
                            }
                        }, 1000);
            }
        }

    }

    private static boolean hasFlag(int flags, int flag) {
        return (flags & flag) != 0;
    }


    //返回pkgName在mApplications中的下标位置,其实就是来表明是否有在mApplications列表中
    private int indexOfApplicationInfoLocked(String pkgName) {

        for (int i = mApplications.size() - 1; i >= 0; i--) {
            if (mApplications.get(i).packageName.equals(pkgName)) {
                return i;
            }
        }
        return -1;
    }

    //当新安装了之后，这里要将安装的放入mApplications中
    private void addPackage(String pkgName) {
        try {
            synchronized (mEntriesMap) {

                if (!mResumed) {
                    // If we are not resumed, we will do a full query the
                    // next time we resume, so there is no reason to do work
                    // here.
                    return;
                }
                if (indexOfApplicationInfoLocked(pkgName) >= 0) {
                    return;
                }
                ApplicationInfo info = mPackManager.getApplicationInfo(pkgName, mRetrieveFlags);
                mApplications.add(info);
                if (!mBackgroundHandler.hasMessages(BackgroundHandler.MSG_LOAD_ENTRIES)) { //计算新安装的包的大小
                    mBackgroundHandler.sendEmptyMessage(BackgroundHandler.MSG_LOAD_ENTRIES);
                }
            }
        } catch (NameNotFoundException e) {
        }
    }

    void removePackage(String pkgName) {
        synchronized (mEntriesMap) {

            int idx = indexOfApplicationInfoLocked(pkgName);

            if (idx >= 0) {
                AppEntry entry = mEntriesMap.get(pkgName);
                if (entry != null) {
                    mEntriesMap.remove(pkgName);
                    mAppEntries.remove(entry);
                }
                mApplications.remove(idx);
                if (!mBackgroundHandler.hasMessages(BackgroundHandler.MSG_LOAD_ENTRIES)) { //计算新安装的包的大小
                    mBackgroundHandler.sendEmptyMessage(BackgroundHandler.MSG_LOAD_ENTRIES);
                }
            }
        }
    }
    //相当于刷新一下pkgName
    void invalidatePackage(String pkgName) {
        removePackage(pkgName);
        addPackage(pkgName);
    }

    //返回info对应的mEntriesMap中,如果没有，则新建一个APPEntry并加入之
    private AppEntry getEntryLocked(ApplicationInfo info) {
        AppEntry entry = mEntriesMap.get(info.packageName);

        if (entry == null) {

            entry = new AppEntry(mContext, info, mCurId++);
            mEntriesMap.put(info.packageName, entry);
            mAppEntries.add(entry);
        } else if (entry.info != info) {
            entry.info = info;
        }
        return entry;
    }

    // --------------------------------------------------------------

    private class BackgroundHandler extends Handler {
        static final int MSG_REBUILD_LIST = 1;
        static final int MSG_LOAD_ENTRIES = 2;
        static final int MSG_LOAD_SIZES = 3;
        boolean mRunning;
        BackgroundHandler(Looper looper) {
            super(looper);
        }
        @Override
        public void handleMessage(Message msg) {

            switch (msg.what) {
                case MSG_REBUILD_LIST:
                    break;
                case MSG_LOAD_ENTRIES: //将mApplications中的构建AppEntry装入mEntriesMap中去
                    {
                        synchronized (mEntriesMap) {
                            for (int i = 0; i < mApplications.size(); i++) {
                                if (!mRunning) {
                                    mRunning = true;
                                    Message m = mMainHandler.obtainMessage(
                                            MainHandler.MSG_RUNNING_STATE_CHANGED, 1);
                                    mMainHandler.sendMessage(m);
                                }
                                ApplicationInfo info = mApplications.get(i);
                                if (mEntriesMap.get(info.packageName) == null && info.sourceDir != null) {  //构造一个新的AppEntry
                                    getEntryLocked(info);
                                }
                            }
                        }
                        mMainHandler.sendEmptyMessage(MainHandler.MSG_PACKAGE_LIST_CHANGED);
                        sendEmptyMessage(MSG_LOAD_SIZES);
                    }
                    break;
                case MSG_LOAD_SIZES:
                    synchronized (mEntriesMap) {
                        if (mCurComputingSizePkg != null) {
                            return;
                        }
                        long now = SystemClock.uptimeMillis();//从开机到现在的毫秒数
                        for (int i = 0; i < mAppEntries.size(); i++) {
                            AppEntry entry = mAppEntries.get(i);
                            if (entry.size == SIZE_UNKNOWN || entry.sizeStale) {
                                if (entry.sizeLoadStart == 0 || (entry.sizeLoadStart < (now - 20 * 1000))) {
                                    if (!mRunning) {
                                        mRunning = true;
                                        Message m = mMainHandler.obtainMessage(
                                                MainHandler.MSG_RUNNING_STATE_CHANGED, 1);
                                        mMainHandler.sendMessage(m);
                                    }
                                    entry.sizeLoadStart = now;
                                    mCurComputingSizePkg = entry.info.packageName;
                                    //mPackManager.getPackageSizeInfo(mCurComputingSizePkg, mStatsObserver);
                                }
                                return;
                            }
                        }
                        //通知所有的大小都计算完了
                        if (!mMainHandler.hasMessages(MainHandler.MSG_ALL_SIZES_COMPUTED)) {
                            mMainHandler.sendEmptyMessage(MainHandler.MSG_ALL_SIZES_COMPUTED);
                            mRunning = false;
                            Message m = mMainHandler.obtainMessage(
                                    MainHandler.MSG_RUNNING_STATE_CHANGED, 0);
                            mMainHandler.sendMessage(m);
                        }
                    }
                    break;
            }
        }
        final IPackageStatsObserver.Stub mStatsObserver = new IPackageStatsObserver.Stub() {
        	//当查询包信息的大小完成之后的回调,PackageStats包含了所需要的信息
        	@Override
            public void onGetStatsCompleted(PackageStats stats, boolean succeeded) {
                boolean sizeChanged = false;
                synchronized (mEntriesMap) {
                    AppEntry entry = mEntriesMap.get(stats.packageName);
                    if (entry != null) {
                        synchronized (entry) {
                            entry.sizeStale = false;
                            entry.sizeLoadStart = 0;
                            long externalCodeSize = stats.externalCodeSize + stats.externalObbSize;
                            long externalDataSize = stats.externalDataSize
                                    + stats.externalMediaSize;
                            long newSize = externalCodeSize + externalDataSize
                                    + getTotalInternalSize(stats);
                            if (entry.size != newSize || entry.cacheSize != stats.cacheSize
                                    || entry.codeSize != stats.codeSize
                                    || entry.dataSize != stats.dataSize
                                    || entry.externalCodeSize != externalCodeSize
                                    || entry.externalDataSize != externalDataSize
                                    || entry.externalCacheSize != stats.externalCacheSize) {
                                entry.size = newSize;
                                entry.cacheSize = stats.cacheSize;
                                entry.codeSize = stats.codeSize;
                                entry.dataSize = stats.dataSize;
                                entry.externalCodeSize = externalCodeSize;
                                entry.externalDataSize = externalDataSize;
                                entry.externalCacheSize = stats.externalCacheSize;
                                entry.sizeStr = getSizeStr(entry.size);
                                entry.internalSize = getTotalInternalSize(stats);
                                entry.externalSize = getTotalExternalSize(stats);
                                sizeChanged = true;
                            }
                        }
                        if (sizeChanged) {
                            Message msg = mMainHandler.obtainMessage(
                                    MainHandler.MSG_PACKAGE_SIZE_CHANGED, stats.packageName);
                            mMainHandler.sendMessage(msg);
                        }
                    }
                    if (mCurComputingSizePkg == null
                            || mCurComputingSizePkg.equals(stats.packageName)) {
                        mCurComputingSizePkg = null;
                        sendEmptyMessage(MSG_LOAD_SIZES);
                    }
                }
            }
        };

        private long getTotalInternalSize(PackageStats ps) {

            if (ps != null) {
                return ps.codeSize + ps.dataSize;
            }
            return SIZE_INVALID;
        }

        private long getTotalExternalSize(PackageStats ps) {

            if (ps != null) {
                return ps.externalCodeSize + ps.externalDataSize + ps.externalCacheSize
                        + ps.externalMediaSize + ps.externalObbSize;
            }
            return SIZE_INVALID;
        }

    } //BackGroundHandler

}//Class ApplicationsState
