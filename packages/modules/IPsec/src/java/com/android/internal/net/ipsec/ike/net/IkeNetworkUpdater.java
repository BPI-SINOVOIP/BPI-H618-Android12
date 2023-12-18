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

import android.net.Network;

/** IkeNetworkUpdater is an interface for updating the underlying Network for an IKE Session. */
public interface IkeNetworkUpdater {
    /**
     * Notify the IkeNetworkUpdater to use the specified Network as its underlying Network.
     *
     * <p>This may also be invoked if the LinkProperties for the specified Network drop the previous
     * local address for the IKE Session.
     *
     * @param network The Network to be used for the underlying Network
     */
    void onUnderlyingNetworkUpdated(Network network);

    /** Notify the IkeNetworkUpdater that the underlying Network died. */
    void onUnderlyingNetworkDied();
}
