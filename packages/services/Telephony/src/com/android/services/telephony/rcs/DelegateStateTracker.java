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

package com.android.services.telephony.rcs;

import android.os.RemoteException;
import android.telephony.ims.DelegateRegistrationState;
import android.telephony.ims.FeatureTagState;
import android.telephony.ims.SipDelegateConfiguration;
import android.telephony.ims.SipDelegateImsConfiguration;
import android.telephony.ims.aidl.ISipDelegate;
import android.telephony.ims.aidl.ISipDelegateConnectionStateCallback;
import android.telephony.ims.stub.DelegateConnectionStateCallback;
import android.util.LocalLog;
import android.util.Log;

import java.io.PrintWriter;
import java.util.ArrayList;
import java.util.List;
import java.util.Set;

/**
 * Manages the events sent back to the remote IMS application using the AIDL backing for the
 * {@link DelegateConnectionStateCallback} interface.
 */
public class DelegateStateTracker implements DelegateBinderStateManager.StateCallback {
    private static final String LOG_TAG = "DelegateST";

    private final int mSubId;
    private final ISipDelegateConnectionStateCallback mAppStateCallback;
    private final ISipDelegate mLocalDelegateImpl;

    private final LocalLog mLocalLog = new LocalLog(SipTransportController.LOG_SIZE);

    private List<FeatureTagState> mDelegateDeniedTags;
    private DelegateRegistrationState mLastRegState;
    private boolean mCreatedCalled = false;
    private int mRegistrationStateOverride = -1;

    public DelegateStateTracker(int subId, ISipDelegateConnectionStateCallback appStateCallback,
            ISipDelegate localDelegateImpl) {
        mSubId = subId;
        mAppStateCallback = appStateCallback;
        mLocalDelegateImpl = localDelegateImpl;
    }

    /**
     * Notify this state tracker that a new internal SipDelegate has been connected.
     *
     * Registration and state updates will be send via the
     * {@link SipDelegateBinderConnection.StateCallback} callback implemented by this class as they
     * arrive.
     * @param deniedTags The tags denied by the SipTransportController and ImsService creating the
     *         SipDelegate. These tags will need to be notified back to the IMS application.
     */
    public void sipDelegateConnected(Set<FeatureTagState> deniedTags) {
        logi("SipDelegate connected with denied tags:" + deniedTags);
        // From the IMS application perspective, we only call onCreated/onDestroyed once and
        // provide the local implementation of ISipDelegate, which doesn't change, even though
        // SipDelegates may be changing underneath.
        if (!mCreatedCalled) {
            mCreatedCalled = true;
            notifySipDelegateCreated();
        }
        mRegistrationStateOverride = -1;
        mDelegateDeniedTags = new ArrayList<>(deniedTags);
    }

    /**
     * The underlying SipDelegate is changing due to a state change in the SipDelegateController.
     *
     * This will trigger an override of the IMS application's registration state. All feature tags
     * in the REGISTERED state will be overridden to move to the deregistering state specified until
     * a new SipDelegate was successfully created and {@link #sipDelegateConnected(Set)} was called
     * or it was destroyed and {@link #sipDelegateDestroyed(int)} was called.
     * @param deregisteringReason The new deregistering reason that all feature tags in the
     *         registered state should now report.
     */
    public void sipDelegateChanging(int deregisteringReason) {
        logi("SipDelegate Changing");
        mRegistrationStateOverride = deregisteringReason;
        if (mLastRegState == null) {
            logw("sipDelegateChanging: invalid state, onRegistrationStateChanged never called.");
            mLastRegState = new DelegateRegistrationState.Builder().build();
        }
        onRegistrationStateChanged(mLastRegState);
    }

    /**
     * The underlying SipDelegate has been destroyed.
     *
     * This should only be called when the entire {@link SipDelegateController} is going down
     * because the application has requested that the SipDelegate be destroyed.
     *
     * This can also be called in error conditions where the IMS application or ImsService has
     * crashed.
     * @param reason The reason that will be sent to the IMS application for why the SipDelegate
     *         is being destroyed.
     */
    public void sipDelegateDestroyed(int reason) {
        logi("SipDelegate destroyed:" + reason);
        mRegistrationStateOverride = -1;
        try {
            mAppStateCallback.onDestroyed(reason);
        } catch (RemoteException e) {
            logw("sipDelegateDestroyed: IMS application is dead: " + e);
        }
    }

    /**
     * The underlying SipDelegate has reported that its registration state has changed.
     * @param registrationState The RegistrationState reported by the SipDelegate to be sent to the
     *         IMS application.
     */
    @Override
    public void onRegistrationStateChanged(DelegateRegistrationState registrationState) {
        if (mRegistrationStateOverride > DelegateRegistrationState.DEREGISTERED_REASON_UNKNOWN) {
            logi("onRegistrationStateChanged: overriding registered state to "
                    + mRegistrationStateOverride);
            registrationState = overrideRegistrationForDelegateChange(mRegistrationStateOverride,
                    registrationState);
        }
        if (registrationState.equals(mLastRegState)) {
            logi("onRegistrationStateChanged: skipping notification, state is the same.");
            return;
        }
        mLastRegState = registrationState;
        logi("onRegistrationStateChanged: sending reg state " + registrationState);
        try {
            mAppStateCallback.onFeatureTagStatusChanged(registrationState, mDelegateDeniedTags);
        } catch (RemoteException e) {
            logw("onRegistrationStateChanged: IMS application is dead: " + e);
        }
    }

    /**
     * THe underlying SipDelegate has reported that the IMS configuration has changed.
     * @param config The config to be sent to the IMS application.
     */
    @Override
    public void onImsConfigurationChanged(SipDelegateImsConfiguration config) {
        logi("onImsConfigurationChanged: Sending new IMS configuration.");
        try {
            mAppStateCallback.onImsConfigurationChanged(config);
        } catch (RemoteException e) {
            logw("onImsConfigurationChanged: IMS application is dead: " + e);
        }
    }

    /**
     * THe underlying SipDelegate has reported that the IMS configuration has changed.
     * @param config The config to be sent to the IMS application.
     */
    @Override
    public void onConfigurationChanged(SipDelegateConfiguration config) {
        logi("onImsConfigurationChanged: Sending new IMS configuration.");
        try {
            mAppStateCallback.onConfigurationChanged(config);
        } catch (RemoteException e) {
            logw("onImsConfigurationChanged: IMS application is dead: " + e);
        }
    }

    /** Write state about this tracker into the PrintWriter to be included in the dumpsys */
    public void dump(PrintWriter printWriter) {
        printWriter.println("Last reg state: " + mLastRegState);
        printWriter.println("Denied tags: " + mDelegateDeniedTags);
        printWriter.println();
        printWriter.println("Most recent logs: ");
        mLocalLog.dump(printWriter);
    }

    private DelegateRegistrationState overrideRegistrationForDelegateChange(
            int registerOverrideReason, DelegateRegistrationState state) {
        Set<String> registeredFeatures = state.getRegisteredFeatureTags();
        DelegateRegistrationState.Builder overriddenState = new DelegateRegistrationState.Builder();
        // keep other deregistering/deregistered tags the same.
        for (FeatureTagState dereging : state.getDeregisteringFeatureTags()) {
            overriddenState.addDeregisteringFeatureTag(dereging.getFeatureTag(),
                    dereging.getState());
        }
        for (FeatureTagState dereged : state.getDeregisteredFeatureTags()) {
            overriddenState.addDeregisteredFeatureTag(dereged.getFeatureTag(),
                    dereged.getState());
        }
        // Override REGISTERED only
        for (String ft : registeredFeatures) {
            overriddenState.addDeregisteringFeatureTag(ft, registerOverrideReason);
        }
        return overriddenState.build();
    }

    private void notifySipDelegateCreated() {
        try {
            mAppStateCallback.onCreated(mLocalDelegateImpl);
        } catch (RemoteException e) {
            logw("notifySipDelegateCreated: IMS application is dead: " + e);
        }
    }

    private void logi(String log) {
        Log.i(SipTransportController.LOG_TAG, LOG_TAG + "[" + mSubId + "] " + log);
        mLocalLog.log("[I] " + log);
    }
    private void logw(String log) {
        Log.w(SipTransportController.LOG_TAG, LOG_TAG + "[" + mSubId + "] " + log);
        mLocalLog.log("[W] " + log);
    }
}
