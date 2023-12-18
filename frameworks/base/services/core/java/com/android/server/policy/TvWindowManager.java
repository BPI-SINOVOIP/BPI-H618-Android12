/*
 * Copyright (C) 2006 The Android Open Source Project
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

package com.android.server.policy;


import android.app.ActivityManager;
import android.app.ActivityManagerInternal;
import android.app.ActivityManagerNative;
import android.app.AppOpsManager;
import android.app.IUiModeManager;
import android.app.ProgressDialog;
import android.app.SearchManager;
import android.app.StatusBarManager;
import android.app.UiModeManager;
import android.content.ActivityNotFoundException;
import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.content.ContentResolver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.ServiceConnection;
import android.content.pm.ActivityInfo;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import android.content.res.CompatibilityInfo;
import android.content.res.Configuration;
import android.content.res.Resources;
import android.content.res.TypedArray;
import android.database.ContentObserver;
import android.graphics.PixelFormat;
import android.graphics.Rect;
import android.hardware.hdmi.HdmiControlManager;
import android.hardware.hdmi.HdmiPlaybackClient;
import android.hardware.hdmi.HdmiPlaybackClient.OneTouchPlayCallback;
import android.hardware.input.InputManagerInternal;
import android.media.AudioAttributes;
import android.media.AudioManager;
import android.media.AudioSystem;
import android.media.IAudioService;
import android.media.Ringtone;
import android.media.RingtoneManager;
import android.media.session.MediaSessionLegacyHelper;
import android.os.Binder;
import android.os.Build;
import android.os.Bundle;
import android.os.Debug;
import android.os.FactoryTest;
import android.os.Handler;
import android.os.IBinder;
import android.os.IDeviceIdleController;
import android.os.Looper;
import android.os.Message;
import android.os.Messenger;
import android.os.PowerManager;
import android.os.PowerManagerInternal;
import android.os.Process;
import android.os.RemoteException;
import android.os.ServiceManager;
import android.os.SystemClock;
import android.os.SystemProperties;
import android.os.UEventObserver;
import android.os.UserHandle;
import android.os.Vibrator;
import android.provider.MediaStore;
import android.provider.Settings;
import android.service.dreams.DreamManagerInternal;
import android.service.dreams.DreamService;
import android.service.dreams.IDreamManager;
import android.speech.RecognizerIntent;
import android.telecom.TelecomManager;
import android.util.DisplayMetrics;
import android.util.EventLog;
import android.util.Log;
import android.util.MutableBoolean;
import android.util.Slog;
import android.util.SparseArray;
import android.util.LongSparseArray;
import android.view.Display;
import android.view.Gravity;
import android.view.HapticFeedbackConstants;
import android.view.IApplicationToken;
import android.view.IWindowManager;
import android.view.InputChannel;
import android.view.InputDevice;
import android.view.InputEvent;
import android.view.InputEventReceiver;
import android.view.KeyCharacterMap;
import android.view.KeyCharacterMap.FallbackAction;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.Surface;
import android.view.View;
import android.view.ViewConfiguration;
import android.view.WindowManager;
import android.view.WindowManagerGlobal;
import android.view.accessibility.AccessibilityEvent;
import android.view.accessibility.AccessibilityManager;
import android.view.animation.Animation;
import android.view.animation.AnimationSet;
import android.view.animation.AnimationUtils;
import com.android.internal.R;
import com.android.internal.logging.MetricsLogger;
import com.android.internal.policy.PhoneWindow;
import com.android.internal.policy.IShortcutService;
import com.android.internal.statusbar.IStatusBarService;
import com.android.internal.util.ScreenShapeHelper;
import com.android.internal.widget.PointerLocationView;
import com.android.server.GestureLauncherService;
import com.android.server.LocalServices;
import com.android.server.policy.keyguard.KeyguardServiceDelegate;
import com.android.server.policy.keyguard.KeyguardServiceDelegate.DrawnListener;
import com.android.server.statusbar.StatusBarManagerInternal;

import java.io.File;
import java.io.FileReader;
import java.io.IOException;
import java.io.PrintWriter;
import java.util.HashSet;
import java.util.List;

/* support the mouse mode */
import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.NotificationChannel;
import android.widget.Toast;

import com.softwinner.Gpio;
import com.softwinner.Multiir;
//import android.hardware.display.DisplayManagerPolicy2;
import java.lang.Runnable;

/**
 * WindowManagerPolicy implementation for the Android phone UI.  This
 * introduces a new method suffix, Lp, for an internal lock of the
 * PhoneWindowManager.  This is used to protect some internal state, and
 * can be acquired with either the Lw and Li lock held, so has the restrictions
 * of both of those when held.
 */
public class TvWindowManager extends PhoneWindowManager {
    private final String TAG = "TvWindowManager";
    private static final String FACTORY_TEST_APP= "com.softwinner.agingdragonbox";

    private static final int NF_ID_ENTER_KEY_MOUSE_MODE = 1;
    boolean mKeyEnterMouseMode = false;
    NotificationManager mNotifationManager = null;
    Notification mNotificationEnterKeyMouseMode = null;
    Toast        mMouseToast = null;
    ActivityManager mActivityManager;
    InputManagerInternal mInputManagerInternal;
    WindowState mFocusedTvWindow;

    Runnable mPromptEnterMouseMode = new Runnable() {
        public void run() {
            if (mMouseToast == null) {
                mMouseToast = Toast.makeText(mContext, com.android.internal.R.string.enter_key_mouse_mode, Toast.LENGTH_SHORT);
                if (mMouseToast == null) {
                    Log.w(TAG, "Fail in creating toast.");
                } else {
                    mMouseToast.setGravity(Gravity.CENTER, 0, 0);
                }
            }
            if (mMouseToast != null) {
                mMouseToast.setText(com.android.internal.R.string.enter_key_mouse_mode);
                mMouseToast.show();
            }
        }
    };
    Runnable mPromptExitMouseMode = new Runnable() {
        public void run() {
            if (mMouseToast == null) {
                mMouseToast = Toast.makeText(mContext, com.android.internal.R.string.exit_key_mouse_mode, Toast.LENGTH_SHORT);
                if (mMouseToast == null) {
                    Log.w(TAG, "Fail in creating toast.");
                } else {
                    mMouseToast.setGravity(Gravity.CENTER, 0, 0);
                }
            }
            if (mMouseToast != null) {
                mMouseToast.setText(com.android.internal.R.string.exit_key_mouse_mode);
                mMouseToast.show();
            }
        }
    };

    /** {@inheritDoc} */
    @Override
    public void init(Context context, IWindowManager windowManager,
            WindowManagerFuncs windowManagerFuncs) {
        super.init(context, windowManager, windowManagerFuncs);

        IntentFilter filter = new IntentFilter();
        filter.addAction(Intent.ACTION_SCREEN_ON);
        filter.addAction(Intent.ACTION_SCREEN_OFF);
        context.registerReceiver(mFlickerIntentReceiver, filter);

        mInputManagerInternal = LocalServices.getService(InputManagerInternal.class);
        Resources resources = mContext.getResources();

        mLedNormalFlicker = mContext.getResources().getInteger(
                com.android.internal.R.integer.config_ledNormalFlicker);
        mLedStandbyAllOff = mContext.getResources().getBoolean(
                com.android.internal.R.bool.config_ledStandbyAllOff);
        mHandler = new  Handler();
        Gpio.configLevel(mContext);
/*
        mDisplayEnhanceModeStrArray = mContext.getResources().getStringArray(com.android.internal.R.array.display2_enhance_mode_names);
        mDmPolicy =  new DisplayManagerPolicy2(mContext);
*/
    }
    private Handler mHandler = null;

    @Override
    public void onSystemUiStarted() {
        super.onSystemUiStarted();
        Multiir.registerObserver(mContext);
    }

    @Override
    public long interceptKeyBeforeDispatching(IBinder focusedToken, KeyEvent event, int policyFlags) {
        final boolean keyguardOn = keyguardOn();
        final int keyCode = event.getKeyCode();
        final int repeatCount = event.getRepeatCount();
        final int metaState = event.getMetaState();
        final int flags = event.getFlags();
        final boolean down = event.getAction() == KeyEvent.ACTION_DOWN;
        final boolean canceled = event.isCanceled();

        if(down && keyCode != KeyEvent.KEYCODE_POWER && keyCode != KeyEvent.KEYCODE_UNKNOWN){
            startFlicker();
        }

        if (keyCode == KeyEvent.KEYCODE_MOUSE) {
            Log.v(TAG, "it's KEYCODE_MOUSE key and down = " + down);
            if (!down) {
                if (mNotifationManager == null) {
                    mNotifationManager = (NotificationManager)mContext.getSystemService(Context.NOTIFICATION_SERVICE);
                    if (mNotifationManager == null) {
                        Log.w(TAG, "Fail in get NotificationManager");
                        return -1;
                    }
                }
                if (mNotificationEnterKeyMouseMode == null) {
                    NotificationChannel channel = new NotificationChannel("TvWindow_Mouse", "MouseChannel", NotificationManager.IMPORTANCE_DEFAULT);
                    channel.setSound(null, null);
                    mNotifationManager.createNotificationChannel(channel);
                    Intent intent = new Intent();
                    PendingIntent contentIntent = PendingIntent.getActivity(mContext, 0, intent, PendingIntent.FLAG_UPDATE_CURRENT | PendingIntent.FLAG_IMMUTABLE);
                    mNotificationEnterKeyMouseMode  = new Notification.Builder(mContext).setWhen(System.currentTimeMillis())
                        .setTicker(mContext.getResources().getText(com.android.internal.R.string.enter_key_mouse_mode))
                        .setSmallIcon(com.android.internal.R.drawable.key_mouse)
                        .setContentIntent(contentIntent)
                        .setContentTitle(mContext.getResources().getText(com.android.internal.R.string.title_key_mouse_mode))
                        .setContentText(mContext.getResources().getText(com.android.internal.R.string.detail_key_mouse_mode))
                        .setChannel("TvWindow_Mouse").build();
                }
                if (mKeyEnterMouseMode) {
                    mNotifationManager.cancel(NF_ID_ENTER_KEY_MOUSE_MODE);

                    mHandler.removeCallbacks(mPromptExitMouseMode);
                    mHandler.post(mPromptExitMouseMode);

                    Multiir.exitMouseMode();
                    mInputManagerInternal.setHidePointerIcon(true);
                    mKeyEnterMouseMode = false;
                } else {
                    mNotifationManager.notify(NF_ID_ENTER_KEY_MOUSE_MODE, mNotificationEnterKeyMouseMode);

                    mHandler.removeCallbacks(mPromptEnterMouseMode);
                    mHandler.post(mPromptEnterMouseMode);

                    Multiir.enterMouseMode();
                    mKeyEnterMouseMode = true;
                    mInputManagerInternal.setHidePointerIcon(false);
                }
            }
            return -1;
        }

        if (mKeyEnterMouseMode) {
            switch (keyCode) {
                case KeyEvent.KEYCODE_DPAD_UP:
                case KeyEvent.KEYCODE_DPAD_DOWN:
                case KeyEvent.KEYCODE_DPAD_LEFT:
                case KeyEvent.KEYCODE_DPAD_RIGHT:
                case KeyEvent.KEYCODE_DPAD_CENTER:
                    int keyState = down ? 1 : 0;
                    Multiir.reportMouseKeyEvent(keyCode, keyState);
                    return -1;
            }
        }
        return super.interceptKeyBeforeDispatching(focusedToken, event, policyFlags);
    }

    private Handler mFlickerHandler = new Handler();
    private boolean mFlickerEnable  = true;
    private static final int FLICKER_INTERVAL = 50;
    private static final char portType = 'h';
    private static final int portNum = 20;
    private static final int on = 1;
    private static final int off = 0;
    private int mLedNormalFlicker;
    private boolean mLedStandbyAllOff;
    private BroadcastReceiver mFlickerIntentReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            Slog.d(TAG, "action = " + action);
            if (action.equals(Intent.ACTION_SCREEN_ON)) {
                Slog.d(TAG,"================================================== ACTION_SCREEN_ON");
                mFlickerEnable = true;
                mFlickerHandler.removeCallbacks(mStep1On);
                mFlickerHandler.removeCallbacks(mStep2Off);
                mFlickerHandler.removeCallbacks(mStep3On);
                Gpio.setNormalLedOn(true);
                Gpio.setStandbyLedOn(false);
            }else if (action.equals(Intent.ACTION_SCREEN_OFF)) {
                Slog.d(TAG,"================================================== ACTION_SCREEN_OFF");
                mFlickerEnable = false;
                mFlickerHandler.removeCallbacks(mStep1On);
                mFlickerHandler.removeCallbacks(mStep2Off);
                mFlickerHandler.removeCallbacks(mStep3On);
                if(mLedStandbyAllOff == false) {
                    Gpio.setNormalLedOn(false);
                    Gpio.setStandbyLedOn(true);
                } else {
                    Gpio.setNormalLedOn(false);
                    Gpio.setStandbyLedOn(false);
                }
            }
        }
    };

    private void startFlicker() {
        Slog.d("GpioService", "mFlickerEnable = " + mFlickerEnable);
        if (mFlickerEnable) {
            //Gpio.setNormalLedOn(false);
            //Gpio.setStandbyLedOn(false);
            mFlickerHandler.removeCallbacks(mStep1On);
            mFlickerHandler.removeCallbacks(mStep2Off);
            mFlickerHandler.removeCallbacks(mStep3On);
            mFlickerHandler.postDelayed(mStep1On, FLICKER_INTERVAL);
        }
    }


    private Runnable mStep1On = new Runnable() {
        public void run() {
            if (mFlickerEnable) {
                Gpio.setNormalLedOn(true);
                Gpio.setStandbyLedOn(false);
                mFlickerHandler.removeCallbacks(mStep2Off);
                mFlickerHandler.removeCallbacks(mStep3On);
                mFlickerHandler.postDelayed(mStep2Off, FLICKER_INTERVAL);
            }
        }
    };

    private Runnable mStep2Off = new Runnable() {
        public void run() {
            if (mFlickerEnable) {
                if (mLedNormalFlicker == 0) {
                    Gpio.setNormalLedOn(true);
                    Gpio.setStandbyLedOn(false);
                } else if (mLedNormalFlicker == 1) {
                    Gpio.setNormalLedOn(true);
                    Gpio.setStandbyLedOn(true);
                } else if (mLedNormalFlicker == 2) {
                    Gpio.setNormalLedOn(false);
                    Gpio.setStandbyLedOn(false);
                } else if (mLedNormalFlicker == 3) {
                    Gpio.setNormalLedOn(false);
                    Gpio.setStandbyLedOn(true);
                }
                mFlickerHandler.removeCallbacks(mStep3On);
                mFlickerHandler.postDelayed(mStep3On, FLICKER_INTERVAL);
            }
        }
    };

    private Runnable mStep3On = new Runnable() {
        public void run() {
            if (mFlickerEnable) {
                Gpio.setNormalLedOn(true);
                Gpio.setStandbyLedOn(false);
            }
        }
    };

    @Override
    public void onDefaultDisplayFocusChangedLw(WindowState newFocus) {
        super.onDefaultDisplayFocusChangedLw(newFocus);
        mFocusedTvWindow = newFocus;
    }

    @Override
    public int interceptKeyBeforeQueueing(KeyEvent event, int policyFlags) {
        int keyCode = event.getKeyCode();
        final boolean down = event.getAction() == KeyEvent.ACTION_DOWN;
        Log.d(TAG,"key event key = " + KeyEvent.keyCodeToString(keyCode));
        if (down) {
            switch (keyCode) {
                case KeyEvent.KEYCODE_SETTINGS:
                    Intent intent = new Intent();
                    intent.setComponent(new ComponentName("com.android.tv.settings", "com.android.tv.settings.MainSettings"));
                    intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
                    mContext.startActivity(intent);
                    break;
                case KeyEvent.KEYCODE_APPS:
                    if (mActivityManager == null)
                        mActivityManager = (ActivityManager)mContext.getSystemService(Context.ACTIVITY_SERVICE);
                    ComponentName cn = mActivityManager.getRunningTasks(1).get(0).topActivity;
                    String ActivityName = cn.getClassName();
                    Log.d(TAG, "current activity is " + ActivityName);
                    if (ActivityName.contains("launcher.device.apps.AppsActivity"))
                        break;
                    Intent app_intent = new Intent();
                    app_intent.setComponent(new ComponentName("com.android.tv.launcher", "com.android.tv.launcher.device.apps.AppsActivity"));
                    app_intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
                    mContext.startActivity(app_intent);
                    break;
                case KeyEvent.KEYCODE_PROG_RED:
                    Log.d(TAG,"key event red");
                    //mHandler.post(mDisplayModeRunnable);
                    break;
                case KeyEvent.KEYCODE_PROG_GREEN:
                    Log.d(TAG,"key event greed");
                    launchHomeFromHotKey(mDefaultDisplay.getDisplayId());
                    break;
                case KeyEvent.KEYCODE_PROG_YELLOW:
                    Log.d(TAG,"key event yellow");
                    Intent intent1 = new Intent();
                    intent1.setComponent(new ComponentName("com.android.settings", "com.android.settings.Settings"));
                    intent1.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
                    mContext.startActivity(intent1);
                    break;
                case KeyEvent.KEYCODE_PROG_BLUE:
                    Log.d(TAG,"key event blue");
                    if (mFocusedTvWindow != null && !"com.softwinner.TvdFileManager".equals(mFocusedTvWindow.getOwningPackage())) {
                        Intent intent2 = new Intent();
                        intent2.setComponent(new ComponentName("com.softwinner.TvdFileManager", "com.softwinner.TvdFileManager.MainUI"));
                        intent2.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
                        mContext.startActivity(intent2);
                    }
                    break;
                case KeyEvent.KEYCODE_MOVIE:
                    Log.v(TAG, "it's KEYCODE_MOVIE key and down = " + down);
                    if (mFocusedTvWindow != null && !"com.softwinner.TvdFileManager".equals(mFocusedTvWindow.getOwningPackage())) {
                        Intent intent3 = new Intent();
                        intent3.setComponent(new ComponentName("com.softwinner.TvdFileManager", "com.softwinner.TvdFileManager.MainUI"));
                        intent3.putExtra("media_type", "MEDIA_TYPE_VIDEO");
                        intent3.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
                        mContext.startActivity(intent3);
                    }
                    break;
            }
        } else {//key up
            switch (keyCode) {
                case KeyEvent.KEYCODE_HOME:
                    if (mFocusedTvWindow != null) {
                        String focusedWindowApp = mFocusedTvWindow.getOwningPackage();
                        Log.d(TAG,"the focus window is "+focusedWindowApp);
                        if (focusedWindowApp.equals(FACTORY_TEST_APP)) {
                            return 0;
                        }
                    }else{
                        Log.v(TAG, "the mFocusedWindow is null");
                    }
                    break;
            }
        }
        return super.interceptKeyBeforeQueueing(event,policyFlags);
    }
}
