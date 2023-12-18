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

#include <linux/if_ether.h>
#include <linux/in.h>
#include <linux/ip.h>

#include "bpf_helpers.h"
#include "bpf_net_helpers.h"
#include "bpf_tethering.h"

// Used only by TetheringPrivilegedTests, not by production code.
DEFINE_BPF_MAP_GRW(tether_downstream6_map, HASH, TetherDownstream6Key, Tether6Value, 16,
                   AID_NETWORK_STACK)

DEFINE_BPF_PROG_KVER("xdp/drop_ipv4_udp_ether", AID_ROOT, AID_NETWORK_STACK,
                      xdp_test, KVER(5, 9, 0))
(struct xdp_md *ctx) {
    void *data = (void *)(long)ctx->data;
    void *data_end = (void *)(long)ctx->data_end;

    struct ethhdr *eth = data;
    int hsize = sizeof(*eth);

    struct iphdr *ip = data + hsize;
    hsize += sizeof(struct iphdr);

    if (data + hsize > data_end) return XDP_PASS;
    if (eth->h_proto != htons(ETH_P_IP)) return XDP_PASS;
    if (ip->protocol == IPPROTO_UDP) return XDP_DROP;
    return XDP_PASS;
}

LICENSE("Apache 2.0");
