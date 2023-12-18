/*
 * Copyright (C) 2016 The Android Open Source Project
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
package com.android.car.cluster;

import static android.car.cluster.renderer.InstrumentClusterRenderingService.EXTRA_BUNDLE_KEY_FOR_INSTRUMENT_CLUSTER_HELPER;
import static android.car.settings.CarSettings.Global.DISABLE_INSTRUMENTATION_SERVICE;

import static com.android.car.internal.ExcludeFromCodeCoverageGeneratedReport.DUMP_INFO;

import android.annotation.SystemApi;
import android.app.ActivityOptions;
import android.car.cluster.IInstrumentClusterManagerCallback;
import android.car.cluster.IInstrumentClusterManagerService;
import android.car.cluster.renderer.IInstrumentCluster;
import android.car.cluster.renderer.IInstrumentClusterHelper;
import android.car.cluster.renderer.IInstrumentClusterNavigation;
import android.car.navigation.CarNavigationInstrumentCluster;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.os.Binder;
import android.os.Bundle;
import android.os.Handler;
import android.os.IBinder;
import android.os.Message;
import android.os.RemoteException;
import android.os.UserHandle;
import android.provider.Settings;
import android.text.TextUtils;
import android.util.IndentingPrintWriter;
import android.util.Log;
import android.util.Slog;
import android.view.KeyEvent;

import com.android.car.CarInputService;
import com.android.car.CarInputService.KeyEventListener;
import com.android.car.CarLocalServices;
import com.android.car.CarLog;
import com.android.car.CarServiceBase;
import com.android.car.R;
import com.android.car.am.FixedActivityService;
import com.android.car.cluster.ClusterNavigationService.ContextOwner;
import com.android.car.internal.ExcludeFromCodeCoverageGeneratedReport;
import com.android.car.user.CarUserService;
import com.android.internal.annotations.GuardedBy;
import com.android.internal.annotations.VisibleForTesting;

import java.lang.ref.WeakReference;

/**
 * Service responsible for interaction with car's instrument cluster.
 *
 * @hide
 */
@SystemApi
public class InstrumentClusterService implements CarServiceBase, KeyEventListener,
        ClusterNavigationService.ClusterNavigationServiceCallback {
    private static final String TAG = CarLog.TAG_CLUSTER;
    private static final ContextOwner NO_OWNER = new ContextOwner(0, 0);

    private static final long RENDERER_SERVICE_WAIT_TIMEOUT_MS = 5000;
    private static final long RENDERER_WAIT_MAX_RETRY = 2;

    private final Context mContext;
    private final CarInputService mCarInputService;
    private final ClusterNavigationService mClusterNavigationService;
    /**
     * TODO: (b/121277787) Remove this on main.
     * @deprecated CarInstrumentClusterManager is being deprecated.
     */
    @Deprecated
    private final ClusterManagerService mClusterManagerService = new ClusterManagerService();
    private final Object mLock = new Object();
    @GuardedBy("mLock")
    private ContextOwner mNavContextOwner = NO_OWNER;
    @GuardedBy("mLock")
    private IInstrumentCluster mRendererService;
    // If renderer service crashed / stopped and this class fails to rebind with it immediately,
    // we should wait some time before next attempt. This may happen during APK update for example.
    private final DeferredRebinder mDeferredRebinder;
    // Whether {@link android.car.cluster.renderer.InstrumentClusterRendererService} is bound
    // (although not necessarily connected)
    @GuardedBy("mLock")
    private boolean mRendererBound = false;

    private final String mRenderingServiceConfig;

    @GuardedBy("mLock")
    private IInstrumentClusterNavigation mIInstrumentClusterNavigationFromRenderer;

    @Override
    public void onNavigationStateChanged(Bundle bundle) {
        // No retry here as new events will be sent later.
        IInstrumentClusterNavigation navigationBinder = getNavigationBinder(
                /* retryOnFail= */ false);
        if (navigationBinder == null) {
            Slog.e(TAG, "onNavigationStateChanged failed, renderer not ready, Bundle:"
                    + bundle);
            return;
        }
        try {
            navigationBinder.onNavigationStateChanged(bundle);
        } catch (RemoteException e) {
            Slog.e(TAG, "onNavigationStateChanged failed, bundle:" + bundle, e);
        }
    }

    @Override
    public CarNavigationInstrumentCluster getInstrumentClusterInfo() {
        // Failure in this call leads into an issue in the client, so throw exception
        // when it cannot be recovered / retried.
        for (int i = 0; i < RENDERER_WAIT_MAX_RETRY; i++) {
            IInstrumentClusterNavigation navigationBinder = getNavigationBinder(
                    /* retryOnFail= */ true);
            if (navigationBinder == null) {
                continue;
            }
            try {
                return navigationBinder.getInstrumentClusterInfo();
            } catch (RemoteException e) {
                Slog.e(TAG, "getInstrumentClusterInfo failed", e);
            }
        }
        throw new IllegalStateException("cannot access renderer service");
    }

    @Override
    public void notifyNavContextOwnerChanged(ContextOwner owner) {
        IInstrumentCluster service = getInstrumentClusterRendererService();
        if (service != null) {
            notifyNavContextOwnerChanged(service, owner);
        }
    }

    /**
     * Connection to {@link android.car.cluster.renderer.InstrumentClusterRendererService}
     */
    @VisibleForTesting
    final ServiceConnection mRendererServiceConnection = new ServiceConnection() {
        @Override
        public void onServiceConnected(ComponentName name, IBinder binder) {
            if (Log.isLoggable(TAG, Log.DEBUG)) {
                Slog.d(TAG, "onServiceConnected, name: " + name + ", binder: " + binder);
            }
            IInstrumentCluster service = IInstrumentCluster.Stub.asInterface(binder);
            ContextOwner navContextOwner;
            synchronized (mLock) {
                mRendererService = service;
                navContextOwner = mNavContextOwner;
                mLock.notifyAll();
            }
            if (navContextOwner != null && service != null) {
                notifyNavContextOwnerChanged(service, navContextOwner);
            }
        }

        @Override
        public void onServiceDisconnected(ComponentName name) {
            if (Log.isLoggable(TAG, Log.DEBUG)) {
                Slog.d(TAG, "onServiceDisconnected, name: " + name);
            }
            mContext.unbindService(this);
            synchronized (mLock) {
                mRendererBound = false;
                mRendererService = null;
                mIInstrumentClusterNavigationFromRenderer = null;

            }
            mDeferredRebinder.rebind();
        }
    };

    private final IInstrumentClusterHelper mInstrumentClusterHelper =
            new IInstrumentClusterHelper.Stub() {
                @Override
                public boolean startFixedActivityModeForDisplayAndUser(Intent intent,
                        Bundle activityOptionsBundle, int userId) {
                    Binder.clearCallingIdentity();
                    ActivityOptions options = new ActivityOptions(activityOptionsBundle);
                    FixedActivityService service = CarLocalServices.getService(
                            FixedActivityService.class);
                    return service.startFixedActivityModeForDisplayAndUser(intent, options,
                            options.getLaunchDisplayId(), userId);
                }

                @Override
                public void stopFixedActivityMode(int displayId) {
                    Binder.clearCallingIdentity();
                    FixedActivityService service = CarLocalServices.getService(
                            FixedActivityService.class);
                    service.stopFixedActivityMode(displayId);
                }
            };

    public InstrumentClusterService(Context context, ClusterNavigationService navigationService,
            CarInputService carInputService) {
        mContext = context;
        mClusterNavigationService = navigationService;
        mCarInputService = carInputService;
        mRenderingServiceConfig = mContext.getString(R.string.instrumentClusterRendererService);
        mDeferredRebinder = new DeferredRebinder(this);
    }

    @GuardedBy("mLock")
    private IInstrumentCluster waitForRendererLocked() {
        if (mRendererService == null) {
            try {
                mLock.wait(RENDERER_SERVICE_WAIT_TIMEOUT_MS);
            } catch (InterruptedException e) {
                Slog.d(TAG, "waitForRenderer, interrupted", e);
                Thread.currentThread().interrupt();
            }
        }
        return mRendererService;
    }

    private IInstrumentClusterNavigation getNavigationBinder(boolean retryOnFail) {
        IInstrumentCluster renderer;
        synchronized (mLock) {
            if (mIInstrumentClusterNavigationFromRenderer != null) {
                return mIInstrumentClusterNavigationFromRenderer;
            }
            renderer = waitForRendererLocked();
        }
        IInstrumentClusterNavigation navigationBinder = null;
        for (int i = 0; i < RENDERER_WAIT_MAX_RETRY; i++) {
            if (renderer == null) {
                synchronized (mLock) {
                    renderer = waitForRendererLocked();
                }
            }
            try {
                navigationBinder = renderer.getNavigationService();
                break;
            } catch (RemoteException e) {
                Slog.e(TAG, "RemoteException from renderer", e);
                renderer = null;
            }
        }
        if (navigationBinder == null) {
            return navigationBinder;
        }
        synchronized (mLock) {
            mIInstrumentClusterNavigationFromRenderer = navigationBinder;
        }
        return navigationBinder;
    }

    @Override
    public void init() {
        if (Log.isLoggable(TAG, Log.DEBUG)) {
            Slog.d(TAG, "init");
        }

        // TODO(b/124246323) Start earlier once data storage for cluster is clarified
        //  for early boot.
        if (!isRendererServiceEnabled()) {
            synchronized (mLock) {
                mRendererBound = false;
            }
            return;
        }
        mClusterNavigationService.setClusterServiceCallback(this);
        mCarInputService.setInstrumentClusterKeyListener(this /* KeyEventListener */);
        CarLocalServices.getService(CarUserService.class).runOnUser0Unlock(() -> {
            boolean bound = bindInstrumentClusterRendererService();
            synchronized (mLock) {
                mRendererBound = bound;
            }
        });
    }

    @Override
    public void release() {
        if (Log.isLoggable(TAG, Log.DEBUG)) {
            Slog.d(TAG, "release");
        }

        synchronized (mLock) {
            if (mRendererBound) {
                mContext.unbindService(mRendererServiceConnection);
                mRendererBound = false;
            }
        }
    }

    @Override
    @ExcludeFromCodeCoverageGeneratedReport(reason = DUMP_INFO)
    public void dump(IndentingPrintWriter writer) {
        writer.println("**" + getClass().getSimpleName() + "**");
        synchronized (mLock) {
            writer.println("bound with renderer: " + mRendererBound);
            writer.println("renderer service: " + mRendererService);
            writer.println("context owner: " + mNavContextOwner);
            writer.println("mRenderingServiceConfig:" + mRenderingServiceConfig);
            writer.println("mIInstrumentClusterNavigationFromRenderer:"
                    + mIInstrumentClusterNavigationFromRenderer);
        }
    }

    private static void notifyNavContextOwnerChanged(IInstrumentCluster service,
            ContextOwner owner) {
        try {
            service.setNavigationContextOwner(owner.uid, owner.pid);
        } catch (RemoteException e) {
            Slog.e(TAG, "Failed to call setNavigationContextOwner", e);
        }
    }

    private boolean isRendererServiceEnabled() {
        if (TextUtils.isEmpty(mRenderingServiceConfig)) {
            Slog.d(TAG, "Instrument cluster renderer was not configured");
            return false;
        }
        boolean explicitlyDisabled = "true".equals(Settings.Global
                .getString(mContext.getContentResolver(), DISABLE_INSTRUMENTATION_SERVICE));
        if (explicitlyDisabled) {
            Slog.i(TAG, "Instrument cluster renderer explicitly disabled by settings");
            return false;
        }
        return true;
    }

    private boolean bindInstrumentClusterRendererService() {
        if (!isRendererServiceEnabled()) {
            return false;
        }

        Slog.d(TAG, "bindInstrumentClusterRendererService, component: " + mRenderingServiceConfig);

        Intent intent = new Intent();
        intent.setComponent(ComponentName.unflattenFromString(mRenderingServiceConfig));
        // Litle bit inefficiency here as Intent.getIBinderExtra() is a hidden API.
        Bundle bundle = new Bundle();
        bundle.putBinder(EXTRA_BUNDLE_KEY_FOR_INSTRUMENT_CLUSTER_HELPER,
                mInstrumentClusterHelper.asBinder());
        intent.putExtra(EXTRA_BUNDLE_KEY_FOR_INSTRUMENT_CLUSTER_HELPER, bundle);
        return mContext.bindServiceAsUser(intent, mRendererServiceConnection,
                Context.BIND_AUTO_CREATE | Context.BIND_IMPORTANT, UserHandle.SYSTEM);
    }

    /**
     * @deprecated {@link android.car.cluster.CarInstrumentClusterManager} is now deprecated.
     */
    @Deprecated
    public IInstrumentClusterManagerService.Stub getManagerService() {
        return mClusterManagerService;
    }

    @Override
    public void onKeyEvent(KeyEvent event) {
        if (Log.isLoggable(TAG, Log.DEBUG)) {
            Slog.d(TAG, "InstrumentClusterService#onKeyEvent: " + event);
        }

        IInstrumentCluster service = getInstrumentClusterRendererService();
        if (service != null) {
            try {
                service.onKeyEvent(event);
            } catch (RemoteException e) {
                Slog.e(TAG, "onKeyEvent", e);
            }
        }
    }

    private IInstrumentCluster getInstrumentClusterRendererService() {
        synchronized (mLock) {
            return mRendererService;
        }
    }

    /**
     * TODO: (b/121277787) Remove on main
     * @deprecated CarClusterManager is being deprecated.
     */
    @Deprecated
    private class ClusterManagerService extends IInstrumentClusterManagerService.Stub {
        @Override
        public void startClusterActivity(Intent intent) throws RemoteException {
            // No op.
        }

        @Override
        public void registerCallback(IInstrumentClusterManagerCallback callback)
                throws RemoteException {
            // No op.
        }

        @Override
        public void unregisterCallback(IInstrumentClusterManagerCallback callback)
                throws RemoteException {
            // No op.
        }
    }

    private static final class DeferredRebinder extends Handler {
        private static final String TAG = DeferredRebinder.class.getSimpleName();

        private static final long NEXT_REBIND_ATTEMPT_DELAY_MS = 1000L;
        private static final int NUMBER_OF_ATTEMPTS = 10;

        private final WeakReference<InstrumentClusterService> mService;

        private DeferredRebinder(InstrumentClusterService service) {
            mService = new WeakReference<InstrumentClusterService>(service);
        }

        public void rebind() {
            InstrumentClusterService service = mService.get();
            if (service == null) {
                Slog.i(TAG, "rebind null service");
                return;
            }
            service.mRendererBound = service.bindInstrumentClusterRendererService();

            if (!service.mRendererBound) {
                removeMessages(0);
                sendMessageDelayed(obtainMessage(0, NUMBER_OF_ATTEMPTS, 0),
                        NEXT_REBIND_ATTEMPT_DELAY_MS);
            }
        }

        @Override
        public void handleMessage(Message msg) {
            InstrumentClusterService service = mService.get();
            if (service == null) {
                Slog.i(TAG, "handleMessage null service");
                return;
            }
            service.mRendererBound = service.bindInstrumentClusterRendererService();

            if (service.mRendererBound) {
                Slog.w(TAG, "Failed to bound to render service, next attempt in "
                        + NEXT_REBIND_ATTEMPT_DELAY_MS + "ms.");

                int attempts = msg.arg1;
                if (--attempts >= 0) {
                    sendMessageDelayed(obtainMessage(0, attempts, 0), NEXT_REBIND_ATTEMPT_DELAY_MS);
                } else {
                    Slog.wtf(TAG, "Failed to rebind with cluster rendering service");
                }
            }
        }
    }
}
