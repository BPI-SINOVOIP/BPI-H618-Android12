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

package com.google.android.iwlan.epdg;

import android.net.LinkAddress;
import android.telephony.data.NetworkSliceInfo;

import com.google.auto.value.AutoValue;

import java.net.InetAddress;
import java.util.List;
import java.util.Optional;

@AutoValue
public abstract class TunnelLinkProperties {
    public abstract List<LinkAddress> internalAddresses();

    public abstract List<InetAddress> dnsAddresses();

    public abstract List<InetAddress> pcscfAddresses();

    public abstract String ifaceName();

    public abstract Optional<NetworkSliceInfo> sliceInfo();

    static Builder builder() {
        return new AutoValue_TunnelLinkProperties.Builder().setSliceInfo(Optional.empty());
    }

    @AutoValue.Builder
    abstract static class Builder {
        abstract Builder setInternalAddresses(List<LinkAddress> internalAddresses);

        abstract Builder setDnsAddresses(List<InetAddress> dnsAddresses);

        abstract Builder setPcscfAddresses(List<InetAddress> pcscfAddresses);

        abstract Builder setIfaceName(String ifaceName);

        public Builder setSliceInfo(NetworkSliceInfo si) {
            return setSliceInfo(Optional.ofNullable(si));
        }

        abstract Builder setSliceInfo(Optional<NetworkSliceInfo> si);

        abstract TunnelLinkProperties build();
    }
}
