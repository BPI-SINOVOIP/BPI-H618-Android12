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
package com.android.wallpaper.util;

import static android.graphics.Matrix.MSCALE_X;
import static android.graphics.Matrix.MSCALE_Y;
import static android.graphics.Matrix.MSKEW_X;
import static android.graphics.Matrix.MSKEW_Y;

import android.app.WallpaperColors;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.graphics.Matrix;
import android.graphics.Rect;
import android.graphics.RectF;
import android.os.IBinder;
import android.os.ParcelFileDescriptor;
import android.os.RemoteException;
import android.service.wallpaper.IWallpaperConnection;
import android.service.wallpaper.IWallpaperEngine;
import android.service.wallpaper.IWallpaperService;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.SurfaceControl;
import android.view.SurfaceHolder;
import android.view.SurfaceHolder.Callback;
import android.view.SurfaceView;
import android.view.WindowManager.LayoutParams;

import androidx.annotation.Nullable;

/**
 * Implementation of {@link IWallpaperConnection} that handles communication with a
 * {@link android.service.wallpaper.WallpaperService}
 */
public class WallpaperConnection extends IWallpaperConnection.Stub implements ServiceConnection {

    /**
     * Returns whether live preview is available in framework.
     */
    public static boolean isPreviewAvailable() {
        try {
            return IWallpaperEngine.class.getMethod("mirrorSurfaceControl") != null;
        } catch (NoSuchMethodException | SecurityException e) {
            return false;
        }
    }

    private static final String TAG = "WallpaperConnection";
    private final Context mContext;
    private final Intent mIntent;
    private final WallpaperConnectionListener mListener;
    private final SurfaceView mContainerView;
    private final SurfaceView mSecondContainerView;
    private IWallpaperService mService;
    @Nullable private IWallpaperEngine mEngine;
    private boolean mConnected;
    private boolean mIsVisible;
    private boolean mIsEngineVisible;
    private boolean mEngineReady;

    /**
     * @param intent used to bind the wallpaper service
     * @param context Context used to start and bind the live wallpaper service
     * @param listener if provided, it'll be notified of connection/disconnection events
     * @param containerView SurfaceView that will display the wallpaper
     */
    public WallpaperConnection(Intent intent, Context context,
            @Nullable WallpaperConnectionListener listener, SurfaceView containerView) {
        this(intent, context, listener, containerView, null);
    }

    /**
     * @param intent used to bind the wallpaper service
     * @param context Context used to start and bind the live wallpaper service
     * @param listener if provided, it'll be notified of connection/disconnection events
     * @param containerView SurfaceView that will display the wallpaper
     * @param secondaryContainerView optional SurfaceView that will display a second, mirrored
     *                               version of the wallpaper
     */
    public WallpaperConnection(Intent intent, Context context,
            @Nullable WallpaperConnectionListener listener, SurfaceView containerView,
            @Nullable SurfaceView secondaryContainerView) {
        mContext = context.getApplicationContext();
        mIntent = intent;
        mListener = listener;
        mContainerView = containerView;
        mSecondContainerView = secondaryContainerView;
    }

    /**
     * Bind the Service for this connection.
     */
    public boolean connect() {
        synchronized (this) {
            if (mConnected) {
                return true;
            }
            if (!mContext.bindService(mIntent, this,
                    Context.BIND_AUTO_CREATE | Context.BIND_IMPORTANT)) {
                return false;
            }

            mConnected = true;
        }

        if (mListener != null) {
            mListener.onConnected();
        }

        return true;
    }

    /**
     * Disconnect and destroy the WallpaperEngine for this connection.
     */
    public void disconnect() {
        synchronized (this) {
            mConnected = false;
            if (mEngine != null) {
                try {
                    mEngine.destroy();
                } catch (RemoteException e) {
                    // Ignore
                }
                mEngine = null;
            }
            try {
                mContext.unbindService(this);
            } catch (IllegalArgumentException e) {
                Log.i(TAG, "Can't unbind wallpaper service. "
                        + "It might have crashed, just ignoring.");
            }
            mService = null;
        }
        if (mListener != null) {
            mListener.onDisconnected();
        }
    }

    /**
     * @see ServiceConnection#onServiceConnected(ComponentName, IBinder)
     */
    public void onServiceConnected(ComponentName name, IBinder service) {
        mService = IWallpaperService.Stub.asInterface(service);
        try {
            int displayId = mContainerView.getDisplay().getDisplayId();

            mService.attach(this, mContainerView.getWindowToken(),
                    LayoutParams.TYPE_APPLICATION_MEDIA,
                    true, mContainerView.getWidth(), mContainerView.getHeight(),
                    new Rect(0, 0, 0, 0), displayId);
        } catch (RemoteException e) {
            Log.w(TAG, "Failed attaching wallpaper; clearing", e);
        }
    }

    @Override
    public void onLocalWallpaperColorsChanged(RectF area,
            WallpaperColors colors, int displayId) {

    }

    /**
     * @see ServiceConnection#onServiceDisconnected(ComponentName)
     */
    public void onServiceDisconnected(ComponentName name) {
        mService = null;
        mEngine = null;
        Log.w(TAG, "Wallpaper service gone: " + name);
    }

    /**
     * @see IWallpaperConnection#attachEngine(IWallpaperEngine, int)
     */
    public void attachEngine(IWallpaperEngine engine, int displayId) {
        synchronized (this) {
            if (mConnected) {
                mEngine = engine;
                if (mIsVisible) {
                    setEngineVisibility(true);
                }

                // Some wallpapers don't trigger #onWallpaperColorsChanged from remote. Requesting
                // wallpaper color here to ensure the #onWallpaperColorsChanged would get called.
                try {
                    mEngine.requestWallpaperColors();
                } catch (RemoteException e) {
                    Log.w(TAG, "Failed requesting wallpaper colors", e);
                }
            } else {
                try {
                    engine.destroy();
                } catch (RemoteException e) {
                    // Ignore
                }
            }
        }
    }

    /**
     * Returns the engine handled by this WallpaperConnection
     */
    @Nullable
    public IWallpaperEngine getEngine() {
        return mEngine;
    }

    /**
     * @see IWallpaperConnection#setWallpaper(String)
     */
    public ParcelFileDescriptor setWallpaper(String name) {
        return null;
    }

    @Override
    public void onWallpaperColorsChanged(WallpaperColors colors, int displayId) {
        mContainerView.post(() -> {
            if (mListener != null) {
                mListener.onWallpaperColorsChanged(colors, displayId);
            }
        });
    }

    @Override
    public void engineShown(IWallpaperEngine engine)  {
        mEngineReady = true;
        if (mContainerView != null) {
            mContainerView.post(() -> reparentWallpaperSurface(mContainerView));
        }
        if (mSecondContainerView != null) {
            mSecondContainerView.post(() -> reparentWallpaperSurface(mSecondContainerView));
        }

        mContainerView.post(() -> {
            if (mListener != null) {
                mListener.onEngineShown();
            }
        });
    }

    /**
     * Returns true if the wallpaper engine has been initialized.
     */
    public boolean isEngineReady() {
        return mEngineReady;
    }

    /**
     * Sets the engine's visibility.
     */
    public void setVisibility(boolean visible) {
        mIsVisible = visible;
        setEngineVisibility(visible);
    }

    private void setEngineVisibility(boolean visible) {
        if (mEngine != null && visible != mIsEngineVisible) {
            try {
                mEngine.setVisibility(visible);
                mIsEngineVisible = visible;
            } catch (RemoteException e) {
                Log.w(TAG, "Failure setting wallpaper visibility ", e);
            }
        }
    }

    private void reparentWallpaperSurface(SurfaceView parentSurface) {
        if (mEngine == null) {
            Log.i(TAG, "Engine is null, was the service disconnected?");
            return;
        }
        if (parentSurface.getSurfaceControl() != null) {
            mirrorAndReparent(parentSurface);
        } else {
            Log.d(TAG, "SurfaceView not initialized yet, adding callback");
            parentSurface.getHolder().addCallback(new Callback() {
                @Override
                public void surfaceChanged(SurfaceHolder surfaceHolder, int i, int i1, int i2) {

                }

                @Override
                public void surfaceCreated(SurfaceHolder surfaceHolder) {
                    mirrorAndReparent(parentSurface);
                    parentSurface.getHolder().removeCallback(this);
                }

                @Override
                public void surfaceDestroyed(SurfaceHolder surfaceHolder) {

                }
            });
        }
    }

    private void mirrorAndReparent(SurfaceView parentSurface) {
        if (mEngine == null) {
            Log.i(TAG, "Engine is null, was the service disconnected?");
            return;
        }
        try {
            SurfaceControl parentSC = parentSurface.getSurfaceControl();
            SurfaceControl wallpaperMirrorSC = mEngine.mirrorSurfaceControl();
            if (wallpaperMirrorSC == null) {
                return;
            }
            float[] values = getScale(parentSurface);
            SurfaceControl.Transaction t = new SurfaceControl.Transaction();
            t.setMatrix(wallpaperMirrorSC, values[MSCALE_X], values[MSKEW_Y],
                    values[MSKEW_X], values[MSCALE_Y]);
            t.reparent(wallpaperMirrorSC, parentSC);
            t.show(wallpaperMirrorSC);
            t.apply();
        } catch (RemoteException e) {
            Log.e(TAG, "Couldn't reparent wallpaper surface", e);
        }
    }

    private float[] getScale(SurfaceView parentSurface) {
        Matrix m = new Matrix();
        float[] values = new float[9];
        Rect surfacePosition = parentSurface.getHolder().getSurfaceFrame();
        DisplayMetrics metrics = DisplayMetricsRetriever.getInstance().getDisplayMetrics(
                mContainerView.getResources(), mContainerView.getDisplay());
        m.postScale(((float) surfacePosition.width()) / metrics.widthPixels,
                ((float) surfacePosition.height()) / metrics.heightPixels);
        m.getValues(values);
        return values;
    }

    /**
     * Interface to be notified of connect/disconnect events from {@link WallpaperConnection}
     */
    public interface WallpaperConnectionListener {
        /**
         * Called after the Wallpaper service has been bound.
         */
        default void onConnected() {}

        /**
         * Called after the Wallpaper engine has been terminated and the service has been unbound.
         */
        default void onDisconnected() {}

        /**
         * Called after the wallpaper has been rendered for the first time.
         */
        default void onEngineShown() {}

        /**
         * Called after the wallpaper color is available or updated.
         */
        default void onWallpaperColorsChanged(WallpaperColors colors, int displayId) {}
    }
}
