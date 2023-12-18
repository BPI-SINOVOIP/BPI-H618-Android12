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
package com.android.car.am;

import static android.app.ActivityTaskManager.INVALID_TASK_ID;
import static android.os.Process.INVALID_UID;
import static android.view.WindowManager.LayoutParams.TYPE_KEYGUARD_DIALOG;

import static com.android.car.CarLog.TAG_AM;

import android.annotation.NonNull;
import android.annotation.Nullable;
import android.annotation.UserIdInt;
import android.app.ActivityManager;
import android.app.ActivityOptions;
import android.app.ActivityTaskManager.RootTaskInfo;
import android.app.IActivityManager;
import android.app.IProcessObserver;
import android.app.Presentation;
import android.app.TaskStackListener;
import android.car.hardware.power.CarPowerManager;
import android.car.user.CarUserManager;
import android.car.user.CarUserManager.UserLifecycleListener;
import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.ActivityInfo;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.hardware.display.DisplayManager;
import android.net.Uri;
import android.os.HandlerThread;
import android.os.RemoteException;
import android.os.SystemClock;
import android.os.UserHandle;
import android.os.UserManager;
import android.util.IndentingPrintWriter;
import android.util.Log;
import android.util.Slog;
import android.util.SparseArray;
import android.view.Display;

import com.android.car.CarLocalServices;
import com.android.car.CarServiceBase;
import com.android.car.CarServiceUtils;
import com.android.car.R;
import com.android.car.user.CarUserService;
import com.android.internal.annotations.GuardedBy;

import java.util.List;

/**
 * Monitors top activity for a display and guarantee activity in fixed mode is re-launched if it has
 * crashed or gone to background for whatever reason.
 *
 * <p>This component also monitors the upddate of the target package and re-launch it once
 * update is complete.</p>
 */
public final class FixedActivityService implements CarServiceBase {

    private static final boolean DBG = false;

    private static final long RECHECK_INTERVAL_MS = 500;
    private static final int MAX_NUMBER_OF_CONSECUTIVE_CRASH_RETRY = 5;
    // If process keep running without crashing, will reset consecutive crash counts.
    private static final long CRASH_FORGET_INTERVAL_MS = 2 * 60 * 1000; // 2 mins

    private static class RunningActivityInfo {
        @NonNull
        public final Intent intent;

        @NonNull
        public final ActivityOptions activityOptions;

        @UserIdInt
        public final int userId;

        @GuardedBy("mLock")
        public boolean isVisible;
        @GuardedBy("mLock")
        public long lastLaunchTimeMs = 0;
        @GuardedBy("mLock")
        public int consecutiveRetries = 0;
        @GuardedBy("mLock")
        public int taskId = INVALID_TASK_ID;
        @GuardedBy("mLock")
        public int previousTaskId = INVALID_TASK_ID;
        @GuardedBy("mLock")
        public boolean inBackground;
        @GuardedBy("mLock")
        public boolean failureLogged;

        RunningActivityInfo(@NonNull Intent intent, @NonNull ActivityOptions activityOptions,
                @UserIdInt int userId) {
            this.intent = intent;
            this.activityOptions = activityOptions;
            this.userId = userId;
        }

        private void resetCrashCounterLocked() {
            consecutiveRetries = 0;
            failureLogged = false;
        }

        @Override
        public String toString() {
            return "RunningActivityInfo{intent:" + intent + ",activityOptions:" + activityOptions
                    + ",userId:" + userId + ",isVisible:" + isVisible
                    + ",lastLaunchTimeMs:" + lastLaunchTimeMs
                    + ",consecutiveRetries:" + consecutiveRetries + ",taskId:" + taskId + "}";
        }
    }

    private final Context mContext;

    private final IActivityManager mAm;

    private final DisplayManager mDm;

    private final UserManager mUm;

    private final UserLifecycleListener mUserLifecycleListener = event -> {
        if (Log.isLoggable(TAG_AM, Log.DEBUG)) {
            Slog.d(TAG_AM, "onEvent(" + event + ")");
        }
        if (CarUserManager.USER_LIFECYCLE_EVENT_TYPE_SWITCHING == event.getEventType()) {
            synchronized (FixedActivityService.this.mLock) {
                clearRunningActivitiesLocked();
            }
        }
    };

    private final BroadcastReceiver mBroadcastReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            if (Intent.ACTION_PACKAGE_CHANGED.equals(action)
                    || Intent.ACTION_PACKAGE_REPLACED.equals(action)
                    || Intent.ACTION_PACKAGE_ADDED.equals(action)
                    || Intent.ACTION_PACKAGE_REMOVED.equals(action)) {
                Uri packageData = intent.getData();
                if (packageData == null) {
                    Slog.w(TAG_AM, "null packageData");
                    return;
                }
                String packageName = packageData.getSchemeSpecificPart();
                if (packageName == null) {
                    Slog.w(TAG_AM, "null packageName");
                    return;
                }
                int uid = intent.getIntExtra(Intent.EXTRA_UID, INVALID_UID);
                int userId = UserHandle.getUserId(uid);
                boolean tryLaunch = false;
                synchronized (mLock) {
                    for (int i = 0; i < mRunningActivities.size(); i++) {
                        RunningActivityInfo info = mRunningActivities.valueAt(i);
                        // Should do this for all activities as it can happen for multiple
                        // displays. Package name is ignored as one package can affect
                        // others.
                        if (info.userId == userId) {
                            Slog.i(TAG_AM, "Package changed:" + packageName
                                    + ",user:" + userId + ",action:" + action);
                            info.resetCrashCounterLocked();
                            tryLaunch = true;
                            break;
                        }
                    }
                }
                if (tryLaunch) {
                    launchIfNecessary();
                }
            }
        }
    };

    // It says listener but is actually callback.
    private final TaskStackListener mTaskStackListener = new TaskStackListener() {
        @Override
        public void onTaskStackChanged() {
            launchIfNecessary();
        }

        @Override
        public void onTaskCreated(int taskId, ComponentName componentName) {
            launchIfNecessary();
        }

        @Override
        public void onTaskRemoved(int taskId) {
            launchIfNecessary();
        }

        @Override
        public void onTaskMovedToFront(int taskId) {
            launchIfNecessary();
        }

        @Override
        public void onTaskRemovalStarted(int taskId) {
            launchIfNecessary();
        }
    };


    private final IProcessObserver mProcessObserver = new IProcessObserver.Stub() {
        @Override
        public void onForegroundActivitiesChanged(int pid, int uid, boolean foregroundActivities) {
            launchIfNecessary();
        }

        @Override
        public void onForegroundServicesChanged(int pid, int uid, int fgServiceTypes) {
          // ignore
        }

        @Override
        public void onProcessDied(int pid, int uid) {
            launchIfNecessary();
        }
    };

    private final HandlerThread mHandlerThread;

    private final Runnable mActivityCheckRunnable = () -> {
        launchIfNecessary();
    };

    private final Object mLock = new Object();

    // key: displayId
    @GuardedBy("mLock")
    private final SparseArray<RunningActivityInfo> mRunningActivities =
            new SparseArray<>(/* capacity= */ 1); // default to one cluster only case

    @GuardedBy("mLock")
    private final SparseArray<Presentation> mBlockingPresentations = new SparseArray<>(1);

    @GuardedBy("mLock")
    private boolean mEventMonitoringActive;

    @GuardedBy("mLock")
    private CarPowerManager mCarPowerManager;

    private final CarPowerManager.CarPowerStateListener mCarPowerStateListener = (state) -> {
        if (state != CarPowerManager.CarPowerStateListener.ON) {
            return;
        }
        synchronized (mLock) {
            for (int i = 0; i < mRunningActivities.size(); i++) {
                RunningActivityInfo info = mRunningActivities.valueAt(i);
                info.resetCrashCounterLocked();
            }
        }
        launchIfNecessary();
    };

    public FixedActivityService(Context context) {
        this(context, ActivityManager.getService(), context.getSystemService(UserManager.class),
                context.getSystemService(DisplayManager.class));
    }

    FixedActivityService(Context context, IActivityManager activityManager,
            UserManager userManager, DisplayManager displayManager) {
        mContext = context;
        mAm = activityManager;
        mUm = userManager;
        mDm = displayManager;
        mHandlerThread = CarServiceUtils.getHandlerThread(
                FixedActivityService.class.getSimpleName());
    }

    @Override
    public void init() {
        // nothing to do
    }

    @Override
    public void release() {
        stopMonitoringEvents();
    }

    @Override
    public void dump(IndentingPrintWriter writer) {
        writer.println("*FixedActivityService*");
        synchronized (mLock) {
            writer.println("mRunningActivities:" + mRunningActivities
                    + " ,mEventMonitoringActive:" + mEventMonitoringActive);
            writer.println("mBlockingPresentations:");
            for (int i = 0; i < mBlockingPresentations.size(); i++) {
                Presentation p = mBlockingPresentations.valueAt(i);
                if (p == null) {
                    continue;
                }
                writer.println("display:" + mBlockingPresentations.keyAt(i)
                        + " showing:" + p.isShowing());
            }
        }
    }

    private void clearRunningActivitiesLocked() {
        for (int i = mRunningActivities.size() - 1; i >= 0; i--) {
            RunningActivityInfo info = mRunningActivities.valueAt(i);
            if (info == null || !isUserAllowedToLaunchActivity(info.userId)) {
                mRunningActivities.removeAt(i);
            }
        }
    }

    private void postRecheck(long delayMs) {
        mHandlerThread.getThreadHandler().postDelayed(mActivityCheckRunnable, delayMs);
    }

    private void startMonitoringEvents() {
        CarPowerManager carPowerManager;
        synchronized (mLock) {
            if (mEventMonitoringActive) {
                return;
            }
            mEventMonitoringActive = true;
            carPowerManager = CarLocalServices.createCarPowerManager(mContext);
            mCarPowerManager = carPowerManager;
        }
        CarUserService userService = CarLocalServices.getService(CarUserService.class);
        userService.addUserLifecycleListener(mUserLifecycleListener);
        IntentFilter filter = new IntentFilter();
        filter.addAction(Intent.ACTION_PACKAGE_CHANGED);
        filter.addAction(Intent.ACTION_PACKAGE_REPLACED);
        filter.addAction(Intent.ACTION_PACKAGE_ADDED);
        filter.addAction(Intent.ACTION_PACKAGE_REMOVED);
        filter.addDataScheme("package");
        mContext.registerReceiverAsUser(mBroadcastReceiver, UserHandle.ALL, filter,
                /* broadcastPermission= */ null, /* scheduler= */ null);
        try {
            mAm.registerTaskStackListener(mTaskStackListener);
            mAm.registerProcessObserver(mProcessObserver);
        } catch (RemoteException e) {
            Slog.e(TAG_AM, "remote exception from AM", e);
        }
        try {
            carPowerManager.setListener(mCarPowerStateListener);
        } catch (Exception e) {
            // should not happen
            Slog.e(TAG_AM, "Got exception from CarPowerManager", e);
        }
    }

    private void stopMonitoringEvents() {
        CarPowerManager carPowerManager;
        synchronized (mLock) {
            if (!mEventMonitoringActive) {
                return;
            }
            mEventMonitoringActive = false;
            carPowerManager = mCarPowerManager;
            mCarPowerManager = null;
        }
        if (carPowerManager != null) {
            carPowerManager.clearListener();
        }
        mHandlerThread.getThreadHandler().removeCallbacks(mActivityCheckRunnable);
        CarUserService userService = CarLocalServices.getService(CarUserService.class);
        userService.removeUserLifecycleListener(mUserLifecycleListener);
        try {
            mAm.unregisterTaskStackListener(mTaskStackListener);
            mAm.unregisterProcessObserver(mProcessObserver);
        } catch (RemoteException e) {
            Slog.e(TAG_AM, "remote exception from AM", e);
        }
        mContext.unregisterReceiver(mBroadcastReceiver);
    }

    @Nullable
    private List<RootTaskInfo> getRootTaskInfos() {
        try {
            return mAm.getAllRootTaskInfos();
        } catch (RemoteException e) {
            Slog.e(TAG_AM, "remote exception from AM", e);
        }
        return null;
    }

    /**
     * Launches all stored fixed mode activities if necessary.
     * @param displayId Display id to check if it is visible. If check is not necessary, should pass
     *        {@link Display#INVALID_DISPLAY}.
     * @return true if fixed Activity for given {@code displayId} is visible / successfully
     *         launched. It will return false for {@link Display#INVALID_DISPLAY} {@code displayId}.
     */
    private boolean launchIfNecessary(int displayId) {
        List<RootTaskInfo> infos = getRootTaskInfos();
        if (infos == null) {
            Slog.e(TAG_AM, "cannot get RootTaskInfo from AM");
            return false;
        }
        long now = SystemClock.elapsedRealtime();
        synchronized (mLock) {
            if (mRunningActivities.size() == 0) {
                // it must have been stopped.
                if (DBG) {
                    Slog.i(TAG_AM, "empty activity list", new RuntimeException());
                }
                return false;
            }
            for (int i = mRunningActivities.size() - 1; i >= 0; i--) {
                RunningActivityInfo activityInfo = mRunningActivities.valueAt(i);
                activityInfo.isVisible = false;
                if (isUserAllowedToLaunchActivity(activityInfo.userId)) {
                    continue;
                }
                final int displayIdForActivity = mRunningActivities.keyAt(i);
                if (activityInfo.taskId != INVALID_TASK_ID) {
                    Slog.i(TAG_AM, "Finishing fixed activity on user switching:"
                            + activityInfo);
                    try {
                        mAm.removeTask(activityInfo.taskId);
                    } catch (RemoteException e) {
                        Slog.e(TAG_AM, "remote exception from AM", e);
                    }
                    CarServiceUtils.runOnMain(() -> {
                        Display display = mDm.getDisplay(displayIdForActivity);
                        if (display == null) {
                            Slog.e(TAG_AM, "Display not available, cannot launnch window:"
                                    + displayIdForActivity);
                            return;
                        }
                        Presentation p = new Presentation(mContext, display,
                                android.R.style.Theme_Black_NoTitleBar_Fullscreen,
                                // TYPE_PRESENTATION can't be used in the internal display.
                                // Select TYPE_KEYGUARD_DIALOG, since it's used in
                                // {@Code KeyguardDisplayManager.KeyguardPresentation}.
                                TYPE_KEYGUARD_DIALOG);
                        p.setContentView(R.layout.activity_continuous_blank);
                        synchronized (mLock) {
                            RunningActivityInfo info = mRunningActivities.get(displayIdForActivity);
                            if (info != null && info.userId == ActivityManager.getCurrentUser()) {
                                Slog.i(TAG_AM, "Do not show Presentation, new req already made");
                                return;
                            }
                            mBlockingPresentations.append(displayIdForActivity, p);
                        }
                        p.show();
                    });
                }
                mRunningActivities.removeAt(i);
            }
            for (RootTaskInfo taskInfo : infos) {
                RunningActivityInfo activityInfo = mRunningActivities.get(taskInfo.displayId);
                if (activityInfo == null) {
                    continue;
                }
                int topUserId = taskInfo.childTaskUserIds[taskInfo.childTaskUserIds.length - 1];
                if (activityInfo.intent.getComponent().equals(taskInfo.topActivity)
                        && activityInfo.userId == topUserId && taskInfo.visible) {
                    // top one is matching.
                    activityInfo.isVisible = true;
                    activityInfo.taskId = taskInfo.childTaskIds[taskInfo.childTaskIds.length - 1];
                    continue;
                }
                activityInfo.previousTaskId =
                        taskInfo.childTaskIds[taskInfo.childTaskIds.length - 1];
                Slog.i(TAG_AM, "Unmatched top activity will be removed:"
                        + taskInfo.topActivity + " top task id:" + activityInfo.previousTaskId
                        + " user:" + topUserId + " display:" + taskInfo.displayId);
                activityInfo.inBackground = false;
                for (int i = 0; i < taskInfo.childTaskIds.length - 1; i++) {
                    if (activityInfo.taskId == taskInfo.childTaskIds[i]) {
                        activityInfo.inBackground = true;
                    }
                }
                if (!activityInfo.inBackground) {
                    activityInfo.taskId = INVALID_TASK_ID;
                }
            }

            for (int i = 0; i < mRunningActivities.size(); i++) {
                RunningActivityInfo activityInfo = mRunningActivities.valueAt(i);
                long timeSinceLastLaunchMs = now - activityInfo.lastLaunchTimeMs;
                if (activityInfo.isVisible) {
                    if (timeSinceLastLaunchMs >= CRASH_FORGET_INTERVAL_MS) {
                        activityInfo.consecutiveRetries = 0;
                    }
                    continue;
                }
                if (!isComponentAvailable(activityInfo.intent.getComponent(),
                        activityInfo.userId)) {
                    continue;
                }
                // For 1st call (consecutiveRetries == 0), do not wait as there can be no posting
                // for recheck.
                if (activityInfo.consecutiveRetries > 0 && (timeSinceLastLaunchMs
                        < RECHECK_INTERVAL_MS)) {
                    // wait until next check interval comes.
                    continue;
                }
                if (activityInfo.consecutiveRetries >= MAX_NUMBER_OF_CONSECUTIVE_CRASH_RETRY) {
                    // re-tried too many times, give up for now.
                    if (!activityInfo.failureLogged) {
                        activityInfo.failureLogged = true;
                        Slog.w(TAG_AM, "Too many relaunch failure of fixed activity:"
                                + activityInfo);
                    }
                    continue;
                }

                Slog.i(TAG_AM, "Launching Activity for fixed mode. Intent:" + activityInfo.intent
                        + ",userId:" + UserHandle.of(activityInfo.userId) + ",displayId:"
                        + mRunningActivities.keyAt(i));
                // Increase retry count if task is not in background. In case like other app is
                // launched and the target activity is still in background, do not consider it
                // as retry.
                if (!activityInfo.inBackground) {
                    activityInfo.consecutiveRetries++;
                }
                try {
                    postRecheck(RECHECK_INTERVAL_MS);
                    postRecheck(CRASH_FORGET_INTERVAL_MS);
                    mContext.startActivityAsUser(activityInfo.intent,
                            activityInfo.activityOptions.toBundle(),
                            UserHandle.of(activityInfo.userId));
                    activityInfo.isVisible = true;
                    activityInfo.lastLaunchTimeMs = SystemClock.elapsedRealtime();
                } catch (Exception e) { // Catch all for any app related issues.
                    Slog.w(TAG_AM, "Cannot start activity:" + activityInfo.intent, e);
                }
            }
            RunningActivityInfo activityInfo = mRunningActivities.get(displayId);
            if (activityInfo == null) {
                return false;
            }
            return activityInfo.isVisible;
        }
    }

    private void launchIfNecessary() {
        launchIfNecessary(Display.INVALID_DISPLAY);
    }

    private void logComponentNotFound(ComponentName component, @UserIdInt  int userId,
            Exception e) {
        Slog.e(TAG_AM, "Specified Component not found:" + component
                + " for userid:" + userId, e);
    }

    private boolean isComponentAvailable(ComponentName component, @UserIdInt int userId) {
        PackageInfo packageInfo;
        try {
            packageInfo = mContext.getPackageManager().getPackageInfoAsUser(
                    component.getPackageName(), PackageManager.GET_ACTIVITIES, userId);
        } catch (PackageManager.NameNotFoundException e) {
            logComponentNotFound(component, userId, e);
            return false;
        }
        if (packageInfo == null || packageInfo.activities == null) {
            // may not be necessary but additional safety check
            logComponentNotFound(component, userId, new RuntimeException());
            return false;
        }
        String fullName = component.getClassName();
        String shortName = component.getShortClassName();
        for (ActivityInfo info : packageInfo.activities) {
            if (info.name.equals(fullName) || info.name.equals(shortName)) {
                return true;
            }
        }
        logComponentNotFound(component, userId, new RuntimeException());
        return false;
    }

    private boolean isUserAllowedToLaunchActivity(@UserIdInt int userId) {
        int currentUser = ActivityManager.getCurrentUser();
        if (userId == currentUser || userId == UserHandle.USER_SYSTEM) {
            return true;
        }
        int[] profileIds = mUm.getEnabledProfileIds(currentUser);
        // null can happen in test env when UserManager is mocked. So this check is not necessary
        // in real env but add it to make test impl easier.
        if (profileIds == null) {
            return false;
        }
        for (int id : profileIds) {
            if (id == userId) {
                return true;
            }
        }
        return false;
    }

    private boolean isDisplayAllowedForFixedMode(int displayId) {
        if (displayId == Display.DEFAULT_DISPLAY || displayId == Display.INVALID_DISPLAY) {
            Slog.w(TAG_AM, "Target display cannot be used for fixed mode, displayId:" + displayId,
                    new RuntimeException());
            return false;
        }
        return true;
    }

    /**
     * Checks {@link InstrumentClusterRenderingService#startFixedActivityModeForDisplayAndUser(
     * Intent, ActivityOptions, int)}
     */
    public boolean startFixedActivityModeForDisplayAndUser(@NonNull Intent intent,
            @NonNull ActivityOptions options, int displayId, @UserIdInt int userId) {
        if (!isDisplayAllowedForFixedMode(displayId)) {
            return false;
        }
        if (options == null) {
            Slog.e(TAG_AM, "startFixedActivityModeForDisplayAndUser, null options");
            return false;
        }
        if (!isUserAllowedToLaunchActivity(userId)) {
            Slog.e(TAG_AM, "startFixedActivityModeForDisplayAndUser, requested user:" + userId
                    + " cannot launch activity, Intent:" + intent);
            return false;
        }
        ComponentName component = intent.getComponent();
        if (component == null) {
            Slog.e(TAG_AM,
                    "startFixedActivityModeForDisplayAndUser: No component specified for "
                            + "requested Intent"
                            + intent);
            return false;
        }
        if (!isComponentAvailable(component, userId)) {
            return false;
        }
        boolean startMonitoringEvents = false;
        synchronized (mLock) {
            Presentation p = mBlockingPresentations.removeReturnOld(displayId);
            if (p != null) {
                p.dismiss();
            }
            if (mRunningActivities.size() == 0) {
                startMonitoringEvents = true;
            }
            RunningActivityInfo activityInfo = mRunningActivities.get(displayId);
            boolean replaceEntry = true;
            if (activityInfo != null && activityInfo.intent.equals(intent)
                    && options.equals(activityInfo.activityOptions)
                    && userId == activityInfo.userId) {
                replaceEntry = false;
                if (activityInfo.isVisible) { // already shown.
                    return true;
                }
            }
            if (replaceEntry) {
                activityInfo = new RunningActivityInfo(intent, options, userId);
                mRunningActivities.put(displayId, activityInfo);
            }
        }
        boolean launched = launchIfNecessary(displayId);
        if (!launched) {
            synchronized (mLock) {
                mRunningActivities.remove(displayId);
            }
        }
        // If first trial fails, let client know and do not retry as it can be wrong setting.
        if (startMonitoringEvents && launched) {
            startMonitoringEvents();
        }
        return launched;
    }

    /** Check {@link InstrumentClusterRenderingService#stopFixedActivityMode(int)} */
    public void stopFixedActivityMode(int displayId) {
        if (!isDisplayAllowedForFixedMode(displayId)) {
            return;
        }
        boolean stopMonitoringEvents = false;
        synchronized (mLock) {
            mRunningActivities.remove(displayId);
            if (mRunningActivities.size() == 0) {
                stopMonitoringEvents = true;
            }
        }
        if (stopMonitoringEvents) {
            stopMonitoringEvents();
        }
    }
}
