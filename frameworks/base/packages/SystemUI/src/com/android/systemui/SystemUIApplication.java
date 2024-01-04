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
 * limitations under the License
 */

package com.android.systemui;

import android.app.ActivityThread;
import android.app.Application;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.ApplicationInfo;
import android.content.res.Configuration;
import android.net.wifi.WifiManager;
import android.os.Process;
import android.os.SystemProperties;
import android.os.Trace;
import android.os.Build;
import android.os.UserHandle;
import android.util.Log;
import android.util.TimingsTraceLog;
import android.view.SurfaceControl;

import com.android.internal.protolog.common.ProtoLog;
import com.android.systemui.dagger.ContextComponentHelper;
import com.android.systemui.dagger.GlobalRootComponent;
import com.android.systemui.dagger.SysUIComponent;
import com.android.systemui.dump.DumpManager;
import com.android.systemui.shared.system.ThreadedRendererCompat;
import com.android.systemui.util.NotificationChannels;

import java.lang.reflect.Constructor;
import java.lang.reflect.InvocationTargetException;

// AW:Added for BOOTEVENT
import java.io.FileOutputStream;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

/**
 * Application class for SystemUI.
 */
public class SystemUIApplication extends Application implements
        SystemUIAppComponentFactory.ContextInitializer {

    public static final String TAG = "SystemUIService";
    private static final boolean DEBUG = true;

    private ContextComponentHelper mComponentHelper;
    private BootCompleteCacheImpl mBootCompleteCache;

    /**
     * Hold a reference on the stuff we start.
     */
    private SystemUI[] mServices;
    private String[] mNames;
    private boolean mServicesStarted;
    private SystemUIAppComponentFactory.ContextAvailableCallback mContextAvailableCallback;
    private GlobalRootComponent mRootComponent;
    private SysUIComponent mSysUIComponent;

    private final List<String> mBaseStartServices = new ArrayList<>();
    private Context mContext;

    public SystemUIApplication() {
        super();
        Log.v(TAG, "SystemUIApplication constructed.");
        // SysUI may be building without protolog preprocessing in some cases
        ProtoLog.REQUIRE_PROTOLOGTOOL = false;
    }

    @Override
    public void onCreate() {
        super.onCreate();
        mContext = this;
        Log.v(TAG, "SystemUIApplication created.");
        // This line is used to setup Dagger's dependency injection and should be kept at the
        // top of this method.
        TimingsTraceLog log = new TimingsTraceLog("SystemUIBootTiming",
                Trace.TRACE_TAG_APP);
        log.traceBegin("DependencyInjection");
        mContextAvailableCallback.onContextAvailable(this);
        mRootComponent = SystemUIFactory.getInstance().getRootComponent();
        mSysUIComponent = SystemUIFactory.getInstance().getSysUIComponent();
        mComponentHelper = mSysUIComponent.getContextComponentHelper();
        mBootCompleteCache = mSysUIComponent.provideBootCacheImpl();
        log.traceEnd();

        // Set the application theme that is inherited by all services. Note that setting the
        // application theme in the manifest does only work for activities. Keep this in sync with
        // the theme set there.
        setTheme(R.style.Theme_SystemUI);


        if(SystemProperties.get("ro.build.characteristics").equals("homlet")){
            Log.d(TAG,"is homlet");
            mBaseStartServices.add("com.android.systemui.util.NotificationChannels");
            mBaseStartServices.add("com.android.systemui.ScreenDecorations");
	} else {
            mBaseStartServices.add("com.android.systemui.util.NotificationChannels");
            mBaseStartServices.add("com.android.systemui.ScreenDecorations");
            mBaseStartServices.add("com.android.systemui.stackdivider.Divider");
            mBaseStartServices.add("com.android.systemui.keyguard.KeyguardViewMediator");
            mBaseStartServices.add("com.android.systemui.statusbar.phone.StatusBar");
            mBaseStartServices.add("com.android.systemui.recents.Recents");
            mBaseStartServices.add("com.android.systemui.globalactions.GlobalActionsComponent");
        }

        if (Process.myUserHandle().equals(UserHandle.SYSTEM)) {
            IntentFilter bootCompletedFilter = new IntentFilter(Intent.ACTION_BOOT_COMPLETED);
            bootCompletedFilter.setPriority(IntentFilter.SYSTEM_HIGH_PRIORITY);

            // If SF GPU context priority is set to realtime, then SysUI should run at high.
            // The priority is defaulted at medium.
            int sfPriority = SurfaceControl.getGPUContextPriority();
            Log.i(TAG, "Found SurfaceFlinger's GPU Priority: " + sfPriority);
            if (sfPriority == ThreadedRendererCompat.EGL_CONTEXT_PRIORITY_REALTIME_NV) {
                Log.i(TAG, "Setting SysUI's GPU Context priority to: "
                        + ThreadedRendererCompat.EGL_CONTEXT_PRIORITY_HIGH_IMG);
                ThreadedRendererCompat.setContextPriority(
                        ThreadedRendererCompat.EGL_CONTEXT_PRIORITY_HIGH_IMG);
            }

            registerReceiver(new BroadcastReceiver() {
                @Override
                public void onReceive(Context context, Intent intent) {
                    if (mBootCompleteCache.isBootComplete()) return;

                    if (DEBUG) Log.v(TAG, "BOOT_COMPLETED received");
                    unregisterReceiver(this);
                    mBootCompleteCache.setBootComplete();
                    if (mServicesStarted) {
                        final int N = mServices.length;
                        for (int i = 0; i < N; i++) {
                            long ti = System.currentTimeMillis();
                            if (mServices[i] == null) {
                                String clsName = mNames[i];
                                if (DEBUG) Log.d(TAG, "loading: " + clsName);
                                try {
                                    SystemUI obj = mComponentHelper.resolveSystemUI(clsName);
                                    if (obj == null) {
                                        Constructor constructor = Class.forName(clsName).getConstructor(Context.class);
                                        obj = (SystemUI) constructor.newInstance(mContext);
                                    }
                                    mServices[i] = obj;
                                } catch (ClassNotFoundException
                                        | NoSuchMethodException
                                        | IllegalAccessException
                                        | InstantiationException
                                        | InvocationTargetException ex) {
                                    throw new RuntimeException(ex);
                                 }

                                if (DEBUG) Log.d(TAG, "running: " + mServices[i]);
                                mServices[i].start();
                            }
                            mServices[i].onBootCompleted();
                            ti = System.currentTimeMillis() - ti;
                            Log.d("SystemUIBootTiming", "SystemUIService: bootcomplete " + mServices[i].getClass().getName() + " took " + ti + " ms");
                        }
                    }
                }
            }, bootCompletedFilter);

            IntentFilter localeChangedFilter = new IntentFilter(Intent.ACTION_LOCALE_CHANGED);
            registerReceiver(new BroadcastReceiver() {
                @Override
                public void onReceive(Context context, Intent intent) {
                    if (Intent.ACTION_LOCALE_CHANGED.equals(intent.getAction())) {
                        if (!mBootCompleteCache.isBootComplete()) return;
                        // Update names of SystemUi notification channels
                        NotificationChannels.createAll(context);
                    }
                }
            }, localeChangedFilter);
        } else {
            // We don't need to startServices for sub-process that is doing some tasks.
            // (screenshots, sweetsweetdesserts or tuner ..)
            String processName = ActivityThread.currentProcessName();
            ApplicationInfo info = getApplicationInfo();
            if (processName != null && processName.startsWith(info.processName + ":")) {
                return;
            }
            // For a secondary user, boot-completed will never be called because it has already
            // been broadcasted on startup for the primary SystemUI process.  Instead, for
            // components which require the SystemUI component to be initialized per-user, we
            // start those components now for the current non-system user.
            startSecondaryUserServicesIfNeeded();
        }

        if (Build.DROIDBOOST_ENABLED) {
            // AW:turn off wifi and start after locked boot completed
            IntentFilter bootToostFilter = new IntentFilter();
            bootToostFilter.setPriority(IntentFilter.SYSTEM_HIGH_PRIORITY);
            bootToostFilter.addAction(Intent.ACTION_LOCKED_BOOT_COMPLETED);
            bootToostFilter.addAction(Intent.ACTION_SHUTDOWN);
            registerReceiver(new BroadcastReceiver() {
                @Override
                public void onReceive(Context context, Intent intent) {
                    WifiManager wm = (WifiManager) context.getSystemService(Context.WIFI_SERVICE);
                    if (Intent.ACTION_LOCKED_BOOT_COMPLETED.equals(intent.getAction())) {
                        Log.d(TAG,"Starting Wifi now");
                        boolean enable = Prefs.getBoolean(context, "wifiEnabled", false);
                        if (enable) {
                            wm.setWifiEnabled(true);
                            Prefs.remove(context, "wifiEnabled");
                        }
                    } else if (Intent.ACTION_SHUTDOWN.equals(intent.getAction())) {
                        if (wm.isWifiEnabled()) {
                            Prefs.putBoolean(context, "wifiEnabled", true);
                            wm.setWifiEnabled(false);
                        }
                    }
                }
            }, bootToostFilter);
        }
    }
    // AW:Added for BOOTEVENT
    private static boolean sBootEventenable = SystemProperties.getBoolean("persist.sys.bootevent", true);
    static void logBootEvent(String bootevent) {
        if (!sBootEventenable) {
            return ;
        }
        FileOutputStream fos =null;
        try {
            fos = new FileOutputStream("/proc/bootevent");
            fos.write(bootevent.getBytes());
            fos.flush();
        } catch (FileNotFoundException e) {
            Log.e("BOOTEVENT","Failure open /proc/bootevent,not found!",e);
        } catch (java.io.IOException e) {
            Log.e("BOOTEVENT","Failure open /proc/bootevent entry",e);
        } finally {
            if (fos != null) {
                try {
                    fos.close();
                } catch (IOException e) {
                    Log.e ("BOOTEVENT","Failure close /proc/bootevent entry",e);
                }
            }
        }
    }

    /**
     * Makes sure that all the SystemUI services are running. If they are already running, this is a
     * no-op. This is needed to conditinally start all the services, as we only need to have it in
     * the main process.
     * <p>This method must only be called from the main thread.</p>
     */

    public void startServicesIfNeeded() {
        String[] names = SystemUIFactory.getInstance().getSystemUIServiceComponents(getResources());
        startServicesIfNeeded(/* metricsPrefix= */ "StartServices", names);
    }

    /**
     * Ensures that all the Secondary user SystemUI services are running. If they are already
     * running, this is a no-op. This is needed to conditionally start all the services, as we only
     * need to have it in the main process.
     * <p>This method must only be called from the main thread.</p>
     */
    void startSecondaryUserServicesIfNeeded() {
        String[] names = SystemUIFactory.getInstance().getSystemUIServiceComponentsPerUser(
                getResources());
        startServicesIfNeeded(/* metricsPrefix= */ "StartSecondaryServices", names);
    }

    private void startServicesIfNeeded(String metricsPrefix, String[] services) {
        if (mServicesStarted) {
            return;
        }
        mNames = services;
        mServices = new SystemUI[services.length];

        if (!mBootCompleteCache.isBootComplete()) {
            // check to see if maybe it was already completed long before we began
            // see ActivityManagerService.finishBooting()
            if ("1".equals(SystemProperties.get("sys.boot_completed"))) {
                mBootCompleteCache.setBootComplete();
                if (DEBUG) {
                    Log.v(TAG, "BOOT_COMPLETED was already sent");
                }
            }
        }

        final DumpManager dumpManager = mSysUIComponent.createDumpManager();

        // AW:Added for BOOTEVENT
        logBootEvent("SystemUIService:Starting SystemUI services");
        Log.v(TAG, "Starting SystemUI services for user " +
                Process.myUserHandle().getIdentifier() + ".");
        TimingsTraceLog log = new TimingsTraceLog("SystemUIBootTiming",
                Trace.TRACE_TAG_APP);
        log.traceBegin(metricsPrefix);
        final int N = services.length;
        for (int i = 0; i < N; i++) {
            String clsName = services[i];
            if (!mBootCompleteCache.isBootComplete() && !mBaseStartServices.contains(clsName)) {
                continue;
            }
            if (DEBUG) Log.d(TAG, "loading: " + clsName);
            log.traceBegin(metricsPrefix + clsName);
            long ti = System.currentTimeMillis();
            try {
                SystemUI obj = mComponentHelper.resolveSystemUI(clsName);
                if (obj == null) {
                    Constructor constructor = Class.forName(clsName).getConstructor(Context.class);
                    obj = (SystemUI) constructor.newInstance(this);
                }
                mServices[i] = obj;
            } catch (ClassNotFoundException
                    | NoSuchMethodException
                    | IllegalAccessException
                    | InstantiationException
                    | InvocationTargetException ex) {
                throw new RuntimeException(ex);
            }

            if (DEBUG) Log.d(TAG, "running: " + mServices[i]);
            mServices[i].start();
            log.traceEnd();

            // Warn if initialization of component takes too long
            ti = System.currentTimeMillis() - ti;
            if (ti > 1000) {
                Log.w(TAG, "Initialization of " + clsName + " took " + ti + " ms");
            }

            if (ti > 30) {
                // AW:Added for BOOTEVENT
                logBootEvent("SystemUIService: running " + clsName + " took " + ti + " ms");
            }

            if (mBootCompleteCache.isBootComplete()) {
                mServices[i].onBootCompleted();
            }

            dumpManager.registerDumpable(mServices[i].getClass().getName(), mServices[i]);
        }
        mSysUIComponent.getInitController().executePostInitTasks();
        log.traceEnd();

        mServicesStarted = true;
    }

    @Override
    public void onConfigurationChanged(Configuration newConfig) {
        if (mServicesStarted) {
            mSysUIComponent.getConfigurationController().onConfigurationChanged(newConfig);
            int len = mServices.length;
            for (int i = 0; i < len; i++) {
                if (mServices[i] != null) {
                    mServices[i].onConfigurationChanged(newConfig);
                }
            }
        }
    }

    public SystemUI[] getServices() {
        return mServices;
    }

    @Override
    public void setContextAvailableCallback(
            SystemUIAppComponentFactory.ContextAvailableCallback callback) {
        mContextAvailableCallback = callback;
    }

}
