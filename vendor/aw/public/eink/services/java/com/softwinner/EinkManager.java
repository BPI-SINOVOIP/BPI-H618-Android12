/*
 * Copyright (C) 2007 The Android Open Source Project
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

package com.softwinner;

import android.graphics.Bitmap;
import android.os.IBinder;
import android.os.RemoteException;
import android.os.ServiceManager;
import android.util.Log;
import android.view.Surface;

import com.softwinner.IEinkMode;

import java.lang.String;
import java.util.HashMap;
import java.util.Map;

/**
 * EinkManager
 */
public class EinkManager {
    public static final String TAG = "EinkManager";
    private static final Map<String, Integer> NAME_TO_MODE = new HashMap<>();
    private static boolean sReady = false;
    private static IEinkService mBinder;
    private static IEinkService getService() {
        synchronized (EinkManager.class) {
            if (mBinder == null) {
                mBinder = IEinkService.Stub.asInterface(ServiceManager.getService("eink"));
                if (mBinder != null) {
                    sReady = true;

                    IBinder.DeathRecipient deathRecipient = new IBinder.DeathRecipient() {
                        @Override
                        public void binderDied() {
                            Log.d(TAG, "binderDied clear IBinder");
                            synchronized (EinkManager.class) {
                                mBinder = null;
                                sReady = false;
                            }
                        }
                    };

                    try {
                        mBinder.asBinder().linkToDeath(deathRecipient, 0);
                    } catch(RemoteException e) {
                        e.printStackTrace();
                    }
                }
            }
            return mBinder;
        }
    }

    static {
        NAME_TO_MODE.put("INIT", IEinkMode.INIT);
        NAME_TO_MODE.put("DU", IEinkMode.DU);
        NAME_TO_MODE.put("GC16", IEinkMode.GC16);
        NAME_TO_MODE.put("GC4", IEinkMode.GC4);
        NAME_TO_MODE.put("A2", IEinkMode.A2);
        NAME_TO_MODE.put("GL16", IEinkMode.GL16);
        NAME_TO_MODE.put("GLR16", IEinkMode.GLR16);
        NAME_TO_MODE.put("GLD16", IEinkMode.GLD16);
        NAME_TO_MODE.put("GU16", IEinkMode.GU16);
        NAME_TO_MODE.put("CLEAR", IEinkMode.CLEAR);
        NAME_TO_MODE.put("GC4L", IEinkMode.GC4L);
        NAME_TO_MODE.put("GCC16", IEinkMode.GCC16);
        NAME_TO_MODE.put("RECT", IEinkMode.RECT);
    }

    // can't instantiate this class
    private EinkManager() {
    }

    public static boolean ready() {
        return sReady;
    }

    public static void init(Surface surface, int[] location) {
        if (getService() != null) {
            try {
                mBinder.init(surface, location);
            } catch (RemoteException e) {
            }
        }
    }

    public static void start() {
        if (getService() != null) {
            try {
                mBinder.start();
            } catch (RemoteException e) {
            }
        }
    }

    public static void stop() {
        if (getService() != null) {
            try {
                mBinder.stop();
            } catch (RemoteException e) {
            }
        }
    }

    public static void setWindowFocus(boolean winFocus) {
        if (getService() != null) {
            try {
                mBinder.setWindowFocus(winFocus);
            } catch (RemoteException e) {
                Log.w(TAG, "Cannot call service", e);
            }
        }
    }
    /*public static void pause() {
        if (getService() != null) {
            try {
                mBinder.pause();
            } catch (RemoteException e) {
            }
        }
    }*/

    /*public static void resume() {
        if (getService() != null) {
            try {
                mBinder.resume();
            } catch (RemoteException e) {
            }
        }
    }*/

    public static void clear() {
        if (getService() != null) {
            try {
                mBinder.clear();
            } catch (RemoteException e) {
            }
        }
    }

    public static void refresh() {
        if (getService() != null) {
            try {
                mBinder.refresh();
            } catch (RemoteException e) {
            }
        }
    }

    public static void setBackground(Bitmap bitmap) {
        if (getService() != null) {
            try {
                mBinder.setBackground(bitmap);
            } catch (RemoteException e) {
            }
        }
    }

    public static boolean save(String path, boolean withBackground) {
        if (getService() != null) {
            try {
                return mBinder.save(path, withBackground);
            } catch (RemoteException e) {
            }
        }
        return false;
    }

    public static int getRefreshMode() {
        if (getService() != null) {
            try {
                return mBinder.getRefreshMode();
            } catch (RemoteException e) {
            }
        }
        return -1;
    }

    public static String getRefreshModeName() {
        int mode = getRefreshMode();
        for (Map.Entry<String, Integer> e : NAME_TO_MODE.entrySet()) {
            if (e.getValue() == mode)
                return e.getKey();
        }
        return null;
    }

    public static int checkRefreshMode(String appMode, String sysMode) {
        int mode = 0;
        int sys = IEinkMode.GU16;
        if (appMode != null) {
            mode = NAME_TO_MODE.get(appMode);
        }
        if (sysMode != null) {
            sys = NAME_TO_MODE.get(sysMode);
        }
        switch (mode) {
            case IEinkMode.DU:
            case IEinkMode.GC16:
            case IEinkMode.GC4:
            case IEinkMode.A2:
            case IEinkMode.GL16:
            case IEinkMode.GLR16:
            case IEinkMode.GLD16:
            case IEinkMode.GU16:
            case IEinkMode.CLEAR:
            case IEinkMode.GC4L:
            case IEinkMode.GCC16:
            case IEinkMode.RECT:
                return mode;
            default:
                return sys;
        }
    }

    public static void setRefreshMode(int mode) {
        if (getService() != null) {
            try {
                mBinder.setRefreshMode(mode);
            } catch (RemoteException e) {
            }
        }
    }

    public static void setRefreshMode(String mode) {
        setRefreshMode(NAME_TO_MODE.get(mode));
    }

    public static int forceGlobalRefresh(boolean rightNow) {
        if (getService() != null) {
            try {
                return mBinder.forceGlobalRefresh(rightNow);
            } catch (RemoteException e) {
            }
        }
        return -1;
    }

    /**
     * description
     *
     * @param start Start auto test or stop.
     * @param interval How often to clear all screen. In seconds.
     * @return
     */
    public static int autoTest(boolean start, int interval, int pressureMin, int pressureMax) {
        int ret = -1;
        if (getService() != null) {
            try {
                ret = mBinder.autoTest(start, interval, pressureMin, pressureMax);
            } catch (RemoteException e) {
                Log.w(TAG, "Cannot call service", e);
            }
        }
        return ret;
    }
}

