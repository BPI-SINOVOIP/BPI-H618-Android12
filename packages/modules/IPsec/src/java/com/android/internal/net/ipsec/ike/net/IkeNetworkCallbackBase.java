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

package com.android.internal.net.ipsec.ike.net;

import static android.net.ipsec.ike.IkeManager.getIkeLog;

import android.net.ConnectivityManager.NetworkCallback;
import android.net.LinkAddress;
import android.net.LinkProperties;
import android.net.Network;

import com.android.internal.annotations.VisibleForTesting;

import java.net.InetAddress;

/** IkeNetworkCallbackBase is a template for IKE-specific NetworkCallback implementations. */
public abstract class IkeNetworkCallbackBase extends NetworkCallback {
    private static final String TAG = IkeNetworkCallbackBase.class.getSimpleName();

    protected final IkeNetworkUpdater mIkeNetworkUpdater;
    protected Network mCurrNetwork;
    private InetAddress mCurrAddress;

    protected IkeNetworkCallbackBase(
            IkeNetworkUpdater ikeNetworkUpdater, Network currNetwork, InetAddress currAddress) {
        mIkeNetworkUpdater = ikeNetworkUpdater;
        mCurrNetwork = currNetwork;
        mCurrAddress = currAddress;
    }

    @Override
    public void onLost(Network network) {
        // This Network loss is only meaningful if it's for the current Network
        if (!mCurrNetwork.equals(network)) {
            return;
        }

        logd("onLost invoked for current Network " + mCurrNetwork);
        mIkeNetworkUpdater.onUnderlyingNetworkDied();
    }

    @Override
    public void onLinkPropertiesChanged(Network network, LinkProperties linkProperties) {
        // This LinkProperties update is only meaningful if it's for the current Network
        if (!mCurrNetwork.equals(network)) {
            return;
        }

        // Use getAllLinkAddresses (instead of getLinkAddresses()) so that the return value also
        // includes addresses of stacked LinkProperties. This is useful for handling the address of
        // a CLAT interface.
        for (LinkAddress linkAddress : linkProperties.getAllLinkAddresses()) {
            if (mCurrAddress.equals(linkAddress.getAddress())) {
                return;
            }
        }

        // The underlying Network didn't change, but the current address disappeared. A MOBIKE
        // event is necessary to update the local address and notify the peer of this change.
        logd(
                "onLinkPropertiesChanged indicates current address "
                        + mCurrAddress
                        + " lost on current Network "
                        + mCurrNetwork.getNetId());
        mIkeNetworkUpdater.onUnderlyingNetworkUpdated(mCurrNetwork);
    }

    /**
     * Sets the current Network that this NetworkCallback is monitoring for.
     *
     * <p>MUST be called on the Handler specified when registering this NetworkCallback with {@link
     * ConnectivityManager}.
     */
    public void setNetwork(Network network) {
        mCurrNetwork = network;
    }

    /** Returns the current Network that this NetworkCallback is monitoring for. */
    @VisibleForTesting
    public Network getNetwork() {
        return mCurrNetwork;
    }

    /**
     * Sets the current address that this NetworkCallback is monitoring for.
     *
     * <p>MUST be called on the Handler specified when registering this NetworkCallback with {@link
     * ConnectivityManager}.
     */
    public void setAddress(InetAddress address) {
        mCurrAddress = address;
    }

    /** Returns the current Address that this NetworkCallback is monitoring for. */
    @VisibleForTesting
    public InetAddress getAddress() {
        return mCurrAddress;
    }

    protected void logd(String msg) {
        getIkeLog().d(TAG, msg);
    }
}
