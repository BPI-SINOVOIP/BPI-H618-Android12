/*
 * Copyright (C) 2021 The Android Open Source Project
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
package com.android.networkstack.tethering;

import static android.system.OsConstants.ETH_P_IP;
import static android.system.OsConstants.ETH_P_IPV6;

import android.net.util.InterfaceParams;

import androidx.annotation.NonNull;

import java.io.IOException;

/**
 * The classes and the methods for BPF utilization.
 *
 * {@hide}
 */
public class BpfUtils {
    static {
        System.loadLibrary("tetherutilsjni");
    }

    // For better code clarity when used for 'bool ingress' parameter.
    static final boolean EGRESS = false;
    static final boolean INGRESS = true;

    // For better code clarify when used for 'bool downstream' parameter.
    //
    // This is talking about the direction of travel of the offloaded packets.
    //
    // Upstream means packets heading towards the internet/uplink (upload),
    // thus for tethering this is attached to ingress on the downstream interface,
    // while for clat this is attached to egress on the v4-* clat interface.
    //
    // Downstream means packets coming from the internet/uplink (download), thus
    // for both clat and tethering this is attached to ingress on the upstream interface.
    static final boolean DOWNSTREAM = true;
    static final boolean UPSTREAM = false;

    // The priority of clat/tether hooks - smaller is higher priority.
    // TC tether is higher priority then TC clat to match XDP winning over TC.
    // Sync from system/netd/server/OffloadUtils.h.
    static final short PRIO_TETHER6 = 1;
    static final short PRIO_TETHER4 = 2;
    // note that the above must be lower than PRIO_CLAT from netd's OffloadUtils.cpp

    private static String makeProgPath(boolean downstream, int ipVersion, boolean ether) {
        String path = "/sys/fs/bpf/tethering/prog_offload_schedcls_tether_"
                + (downstream ? "downstream" : "upstream")
                + ipVersion + "_"
                + (ether ? "ether" : "rawip");
        return path;
    }

    /**
     * Attach BPF program
     *
     * TODO: use interface index to replace interface name.
     */
    public static void attachProgram(@NonNull String iface, boolean downstream)
            throws IOException {
        final InterfaceParams params = InterfaceParams.getByName(iface);
        if (params == null) {
            throw new IOException("Fail to get interface params for interface " + iface);
        }

        boolean ether;
        try {
            ether = isEthernet(iface);
        } catch (IOException e) {
            throw new IOException("isEthernet(" + params.index + "[" + iface + "]) failure: " + e);
        }

        try {
            // tc filter add dev .. ingress prio 1 protocol ipv6 bpf object-pinned /sys/fs/bpf/...
            // direct-action
            tcFilterAddDevBpf(params.index, INGRESS, PRIO_TETHER6, (short) ETH_P_IPV6,
                    makeProgPath(downstream, 6, ether));
        } catch (IOException e) {
            throw new IOException("tc filter add dev (" + params.index + "[" + iface
                    + "]) ingress prio PRIO_TETHER6 protocol ipv6 failure: " + e);
        }

        try {
            // tc filter add dev .. ingress prio 2 protocol ip bpf object-pinned /sys/fs/bpf/...
            // direct-action
            tcFilterAddDevBpf(params.index, INGRESS, PRIO_TETHER4, (short) ETH_P_IP,
                    makeProgPath(downstream, 4, ether));
        } catch (IOException e) {
            throw new IOException("tc filter add dev (" + params.index + "[" + iface
                    + "]) ingress prio PRIO_TETHER4 protocol ip failure: " + e);
        }
    }

    /**
     * Detach BPF program
     *
     * TODO: use interface index to replace interface name.
     */
    public static void detachProgram(@NonNull String iface) throws IOException {
        final InterfaceParams params = InterfaceParams.getByName(iface);
        if (params == null) {
            throw new IOException("Fail to get interface params for interface " + iface);
        }

        try {
            // tc filter del dev .. ingress prio 1 protocol ipv6
            tcFilterDelDev(params.index, INGRESS, PRIO_TETHER6, (short) ETH_P_IPV6);
        } catch (IOException e) {
            throw new IOException("tc filter del dev (" + params.index + "[" + iface
                    + "]) ingress prio PRIO_TETHER6 protocol ipv6 failure: " + e);
        }

        try {
            // tc filter del dev .. ingress prio 2 protocol ip
            tcFilterDelDev(params.index, INGRESS, PRIO_TETHER4, (short) ETH_P_IP);
        } catch (IOException e) {
            throw new IOException("tc filter del dev (" + params.index + "[" + iface
                    + "]) ingress prio PRIO_TETHER4 protocol ip failure: " + e);
        }
    }

    private static native boolean isEthernet(String iface) throws IOException;

    private static native void tcFilterAddDevBpf(int ifIndex, boolean ingress, short prio,
            short proto, String bpfProgPath) throws IOException;

    private static native void tcFilterDelDev(int ifIndex, boolean ingress, short prio,
            short proto) throws IOException;
}
