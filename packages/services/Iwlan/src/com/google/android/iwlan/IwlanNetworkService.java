/*
 * Copyright 2020 The Android Open Source Project
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

package com.google.android.iwlan;

import static android.net.NetworkCapabilities.TRANSPORT_CELLULAR;
import static android.net.NetworkCapabilities.TRANSPORT_WIFI;

import android.content.Context;
import android.content.Intent;
import android.net.ConnectivityManager;
import android.net.LinkProperties;
import android.net.Network;
import android.net.NetworkCapabilities;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.IBinder;
import android.os.Looper;
import android.os.Message;
import android.telephony.AccessNetworkConstants;
import android.telephony.NetworkRegistrationInfo;
import android.telephony.NetworkService;
import android.telephony.NetworkServiceCallback;
import android.telephony.SubscriptionManager;
import android.telephony.TelephonyManager;
import android.util.Log;

import com.android.internal.annotations.VisibleForTesting;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

public class IwlanNetworkService extends NetworkService {
    private static final String TAG = IwlanNetworkService.class.getSimpleName();
    private Context mContext;
    private IwlanNetworkMonitorCallback mNetworkMonitorCallback;
    private IwlanOnSubscriptionsChangedListener mSubsChangeListener;
    private HandlerThread mNetworkCallbackHandlerThread;
    private static boolean sNetworkConnected;
    private static List<IwlanNetworkServiceProvider> sIwlanNetworkServiceProviderList =
            new ArrayList<IwlanNetworkServiceProvider>();

    @VisibleForTesting
    enum Transport {
        UNSPECIFIED_NETWORK,
        MOBILE,
        WIFI;
    }

    private static Transport sDefaultDataTransport = Transport.UNSPECIFIED_NETWORK;

    final class IwlanNetworkMonitorCallback extends ConnectivityManager.NetworkCallback {
        /** Called when the framework connects and has declared a new network ready for use. */
        @Override
        public void onAvailable(Network network) {
            Log.d(TAG, "onAvailable: " + network);
        }

        /**
         * Called when the network is about to be lost, typically because there are no outstanding
         * requests left for it. This may be paired with a {@link NetworkCallback#onAvailable} call
         * with the new replacement network for graceful handover. This method is not guaranteed to
         * be called before {@link NetworkCallback#onLost} is called, for example in case a network
         * is suddenly disconnected.
         */
        @Override
        public void onLosing(Network network, int maxMsToLive) {
            Log.d(TAG, "onLosing: maxMsToLive: " + maxMsToLive + " network: " + network);
        }

        /**
         * Called when a network disconnects or otherwise no longer satisfies this request or
         * callback.
         */
        @Override
        public void onLost(Network network) {
            Log.d(TAG, "onLost: " + network);
            IwlanNetworkService.setNetworkConnected(false, Transport.UNSPECIFIED_NETWORK);
        }

        /** Called when the network corresponding to this request changes {@link LinkProperties}. */
        @Override
        public void onLinkPropertiesChanged(Network network, LinkProperties linkProperties) {
            Log.d(TAG, "onLinkPropertiesChanged: " + linkProperties);
        }

        /** Called when access to the specified network is blocked or unblocked. */
        @Override
        public void onBlockedStatusChanged(Network network, boolean blocked) {
            // TODO: check if we need to handle this
            Log.d(TAG, "onBlockedStatusChanged: " + " BLOCKED:" + blocked);
        }

        @Override
        public void onCapabilitiesChanged(
                Network network, NetworkCapabilities networkCapabilities) {
            // onCapabilitiesChanged is guaranteed to be called immediately after onAvailable per
            // API
            Log.d(TAG, "onCapabilitiesChanged: " + network);
            if (networkCapabilities != null) {
                if (networkCapabilities.hasTransport(TRANSPORT_CELLULAR)) {
                    IwlanNetworkService.setNetworkConnected(
                            true, IwlanNetworkService.Transport.MOBILE);
                } else if (networkCapabilities.hasTransport(TRANSPORT_WIFI)) {
                    IwlanNetworkService.setNetworkConnected(
                            true, IwlanNetworkService.Transport.WIFI);
                } else {
                    Log.w(TAG, "Network does not have cellular or wifi capability");
                }
            }
        }
    }

    final class IwlanOnSubscriptionsChangedListener
            extends SubscriptionManager.OnSubscriptionsChangedListener {
        /**
         * Callback invoked when there is any change to any SubscriptionInfo. Typically this method
         * invokes {@link SubscriptionManager#getActiveSubscriptionInfoList}
         */
        @Override
        public void onSubscriptionsChanged() {
            for (IwlanNetworkServiceProvider np : sIwlanNetworkServiceProviderList) {
                np.subscriptionChanged();
            }
        }
    }

    @VisibleForTesting
    class IwlanNetworkServiceProvider extends NetworkServiceProvider {
        private final IwlanNetworkService mIwlanNetworkService;
        private final String SUB_TAG;
        private boolean mIsSubActive = false;
        private HandlerThread mHandlerThread;
        private Handler mHandler;

        private final class NSPHandler extends Handler {
            private final String TAG =
                    IwlanNetworkService.class.getSimpleName()
                            + NSPHandler.class.getSimpleName()
                            + "["
                            + getSlotIndex()
                            + "]";

            @Override
            public void handleMessage(Message msg) {
                Log.d(TAG, "msg.what = " + msg.what);
                switch (msg.what) {
                    case IwlanEventListener.CROSS_SIM_CALLING_ENABLE_EVENT:
                        Log.d(TAG, "CROSS_SIM_CALLING_ENABLE_EVENT");
                        notifyNetworkRegistrationInfoChanged();
                        break;
                    case IwlanEventListener.CROSS_SIM_CALLING_DISABLE_EVENT:
                        Log.d(TAG, "CROSS_SIM_CALLING_DISABLE_EVENT");
                        notifyNetworkRegistrationInfoChanged();
                        break;
                    default:
                        Log.d(TAG, "Unknown message received!");
                        break;
                }
            }

            NSPHandler(Looper looper) {
                super(looper);
            }
        }

        Looper getLooper() {
            mHandlerThread = new HandlerThread("NSPHandlerThread");
            mHandlerThread.start();
            return mHandlerThread.getLooper();
        }

        /**
         * Constructor
         *
         * @param slotIndex SIM slot id the data service provider associated with.
         */
        public IwlanNetworkServiceProvider(int slotIndex, IwlanNetworkService iwlanNetworkService) {
            super(slotIndex);
            SUB_TAG = TAG + "[" + slotIndex + "]";
            mIwlanNetworkService = iwlanNetworkService;

            // Register IwlanEventListener
            initHandler();
            List<Integer> events = new ArrayList<Integer>();
            events.add(IwlanEventListener.CROSS_SIM_CALLING_ENABLE_EVENT);
            events.add(IwlanEventListener.CROSS_SIM_CALLING_DISABLE_EVENT);
            IwlanEventListener.getInstance(mContext, slotIndex).addEventListener(events, mHandler);
        }

        void initHandler() {
            mHandler = new NSPHandler(getLooper());
        }

        @Override
        public void requestNetworkRegistrationInfo(int domain, NetworkServiceCallback callback) {
            if (callback == null) {
                Log.d(SUB_TAG, "Error: callback is null. returning");
                return;
            }
            if (domain != NetworkRegistrationInfo.DOMAIN_PS) {
                callback.onRequestNetworkRegistrationInfoComplete(
                        NetworkServiceCallback.RESULT_ERROR_UNSUPPORTED, null);
                return;
            }

            NetworkRegistrationInfo.Builder nriBuilder = new NetworkRegistrationInfo.Builder();
            nriBuilder
                    .setAccessNetworkTechnology(TelephonyManager.NETWORK_TYPE_IWLAN)
                    .setAvailableServices(Arrays.asList(NetworkRegistrationInfo.SERVICE_TYPE_DATA))
                    .setTransportType(AccessNetworkConstants.TRANSPORT_TYPE_WLAN)
                    .setEmergencyOnly(!mIsSubActive)
                    .setDomain(NetworkRegistrationInfo.DOMAIN_PS);

            if (!IwlanNetworkService.isNetworkConnected(
                    IwlanHelper.isDefaultDataSlot(mContext, getSlotIndex()),
                    IwlanHelper.isCrossSimCallingEnabled(mContext, getSlotIndex()))) {
                nriBuilder.setRegistrationState(
                        NetworkRegistrationInfo.REGISTRATION_STATE_NOT_REGISTERED_SEARCHING);
                Log.d(SUB_TAG, "reg state REGISTRATION_STATE_NOT_REGISTERED_SEARCHING");
            } else {
                nriBuilder.setRegistrationState(NetworkRegistrationInfo.REGISTRATION_STATE_HOME);
                Log.d(SUB_TAG, "reg state REGISTRATION_STATE_HOME");
            }

            callback.onRequestNetworkRegistrationInfoComplete(
                    NetworkServiceCallback.RESULT_SUCCESS, nriBuilder.build());
        }

        /**
         * Called when the instance of network service is destroyed (e.g. got unbind or binder died)
         * or when the network service provider is removed. The extended class should implement this
         * method to perform cleanup works.
         */
        @Override
        public void close() {
            mIwlanNetworkService.removeNetworkServiceProvider(this);
            IwlanEventListener.getInstance(mContext, getSlotIndex()).removeEventListener(mHandler);
            mHandlerThread.quit();
        }

        @VisibleForTesting
        void subscriptionChanged() {
            boolean subActive = false;
            SubscriptionManager sm = SubscriptionManager.from(mContext);
            if (sm.getActiveSubscriptionInfoForSimSlotIndex(getSlotIndex()) != null) {
                subActive = true;
            }
            if (subActive == mIsSubActive) {
                return;
            }
            mIsSubActive = subActive;
            if (subActive) {
                Log.d(SUB_TAG, "sub changed from not_ready --> ready");
            } else {
                Log.d(SUB_TAG, "sub changed from ready --> not_ready");
            }

            notifyNetworkRegistrationInfoChanged();
        }
    }

    /**
     * Create the instance of {@link NetworkServiceProvider}. Network service provider must override
     * this method to facilitate the creation of {@link NetworkServiceProvider} instances. The
     * system will call this method after binding the network service for each active SIM slot id.
     *
     * @param slotIndex SIM slot id the network service associated with.
     * @return Network service object. Null if failed to create the provider (e.g. invalid slot
     *     index)
     */
    @Override
    public NetworkServiceProvider onCreateNetworkServiceProvider(int slotIndex) {
        Log.d(TAG, "onCreateNetworkServiceProvider: slotidx:" + slotIndex);

        // TODO: validity check slot index

        if (sIwlanNetworkServiceProviderList.isEmpty()) {
            // first invocation
            mNetworkCallbackHandlerThread =
                    new HandlerThread(IwlanNetworkService.class.getSimpleName());
            mNetworkCallbackHandlerThread.start();
            Looper looper = mNetworkCallbackHandlerThread.getLooper();
            Handler handler = new Handler(looper);

            // register for default network callback
            ConnectivityManager connectivityManager =
                    mContext.getSystemService(ConnectivityManager.class);
            mNetworkMonitorCallback = new IwlanNetworkMonitorCallback();
            connectivityManager.registerDefaultNetworkCallback(mNetworkMonitorCallback, handler);
            Log.d(TAG, "Registered with Connectivity Service");

            /* register with subscription manager */
            SubscriptionManager subscriptionManager =
                    mContext.getSystemService(SubscriptionManager.class);
            mSubsChangeListener = new IwlanOnSubscriptionsChangedListener();
            subscriptionManager.addOnSubscriptionsChangedListener(mSubsChangeListener);
            Log.d(TAG, "Registered with Subscription Service");
        }

        IwlanNetworkServiceProvider np = new IwlanNetworkServiceProvider(slotIndex, this);
        sIwlanNetworkServiceProviderList.add(np);
        return np;
    }

    public static boolean isNetworkConnected(boolean isDds, boolean isCstEnabled) {
        if (!isDds && isCstEnabled) {
            // Only Non-DDS sub with CST enabled, can use any transport.
            return sNetworkConnected;
        } else {
            // For all other cases, only wifi transport can be used.
            return ((sDefaultDataTransport == Transport.WIFI) && sNetworkConnected);
        }
    }

    public static void setNetworkConnected(boolean connected, Transport transport) {
        if (connected == sNetworkConnected && transport == sDefaultDataTransport) {
            return;
        }
        if (connected && (transport == IwlanNetworkService.Transport.UNSPECIFIED_NETWORK)) {
            return;
        }
        sNetworkConnected = connected;
        sDefaultDataTransport = transport;

        for (IwlanNetworkServiceProvider np : sIwlanNetworkServiceProviderList) {
            np.notifyNetworkRegistrationInfoChanged();
        }
    }

    public void removeNetworkServiceProvider(IwlanNetworkServiceProvider np) {
        sIwlanNetworkServiceProviderList.remove(np);
        if (sIwlanNetworkServiceProviderList.isEmpty()) {
            // deinit network related stuff
            ConnectivityManager connectivityManager =
                    mContext.getSystemService(ConnectivityManager.class);
            connectivityManager.unregisterNetworkCallback(mNetworkMonitorCallback);
            mNetworkCallbackHandlerThread.quit(); // no need to quitSafely
            mNetworkCallbackHandlerThread = null;
            mNetworkMonitorCallback = null;

            // deinit subscription manager related stuff
            SubscriptionManager subscriptionManager =
                    mContext.getSystemService(SubscriptionManager.class);
            subscriptionManager.removeOnSubscriptionsChangedListener(mSubsChangeListener);
            mSubsChangeListener = null;
        }
    }

    Context getContext() {
        return getApplicationContext();
    }

    @VisibleForTesting
    void setAppContext(Context appContext) {
        mContext = appContext;
    }

    @VisibleForTesting
    IwlanNetworkServiceProvider getNetworkServiceProvider(int slotIndex) {
        for (IwlanNetworkServiceProvider np : sIwlanNetworkServiceProviderList) {
            if (np.getSlotIndex() == slotIndex) {
                return np;
            }
        }
        return null;
    }

    @Override
    public void onCreate() {
        mContext = getApplicationContext();
    }

    @Override
    public IBinder onBind(Intent intent) {
        Log.d(TAG, "IwlanNetworkService onBind");
        return super.onBind(intent);
    }
}
