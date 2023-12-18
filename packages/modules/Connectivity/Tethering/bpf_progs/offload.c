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

#include <linux/if.h>
#include <linux/ip.h>
#include <linux/ipv6.h>
#include <linux/pkt_cls.h>
#include <linux/tcp.h>

// bionic kernel uapi linux/udp.h header is munged...
#define __kernel_udphdr udphdr
#include <linux/udp.h>

#include "bpf_helpers.h"
#include "bpf_net_helpers.h"
#include "bpf_tethering.h"

// From kernel:include/net/ip.h
#define IP_DF 0x4000  // Flag: "Don't Fragment"

// ----- Helper functions for offsets to fields -----

// They all assume simple IP packets:
//   - no VLAN ethernet tags
//   - no IPv4 options (see IPV4_HLEN/TCP4_OFFSET/UDP4_OFFSET)
//   - no IPv6 extension headers
//   - no TCP options (see TCP_HLEN)

//#define ETH_HLEN sizeof(struct ethhdr)
#define IP4_HLEN sizeof(struct iphdr)
#define IP6_HLEN sizeof(struct ipv6hdr)
#define TCP_HLEN sizeof(struct tcphdr)
#define UDP_HLEN sizeof(struct udphdr)

// Offsets from beginning of L4 (TCP/UDP) header
#define TCP_OFFSET(field) offsetof(struct tcphdr, field)
#define UDP_OFFSET(field) offsetof(struct udphdr, field)

// Offsets from beginning of L3 (IPv4) header
#define IP4_OFFSET(field) offsetof(struct iphdr, field)
#define IP4_TCP_OFFSET(field) (IP4_HLEN + TCP_OFFSET(field))
#define IP4_UDP_OFFSET(field) (IP4_HLEN + UDP_OFFSET(field))

// Offsets from beginning of L3 (IPv6) header
#define IP6_OFFSET(field) offsetof(struct ipv6hdr, field)
#define IP6_TCP_OFFSET(field) (IP6_HLEN + TCP_OFFSET(field))
#define IP6_UDP_OFFSET(field) (IP6_HLEN + UDP_OFFSET(field))

// Offsets from beginning of L2 (ie. Ethernet) header (which must be present)
#define ETH_IP4_OFFSET(field) (ETH_HLEN + IP4_OFFSET(field))
#define ETH_IP4_TCP_OFFSET(field) (ETH_HLEN + IP4_TCP_OFFSET(field))
#define ETH_IP4_UDP_OFFSET(field) (ETH_HLEN + IP4_UDP_OFFSET(field))
#define ETH_IP6_OFFSET(field) (ETH_HLEN + IP6_OFFSET(field))
#define ETH_IP6_TCP_OFFSET(field) (ETH_HLEN + IP6_TCP_OFFSET(field))
#define ETH_IP6_UDP_OFFSET(field) (ETH_HLEN + IP6_UDP_OFFSET(field))

// ----- Tethering Error Counters -----

DEFINE_BPF_MAP_GRW(tether_error_map, ARRAY, uint32_t, uint32_t, BPF_TETHER_ERR__MAX,
                   AID_NETWORK_STACK)

#define COUNT_AND_RETURN(counter, ret) do {                     \
    uint32_t code = BPF_TETHER_ERR_ ## counter;                 \
    uint32_t *count = bpf_tether_error_map_lookup_elem(&code);  \
    if (count) __sync_fetch_and_add(count, 1);                  \
    return ret;                                                 \
} while(0)

#define TC_DROP(counter) COUNT_AND_RETURN(counter, TC_ACT_SHOT)
#define TC_PUNT(counter) COUNT_AND_RETURN(counter, TC_ACT_OK)

#define XDP_DROP(counter) COUNT_AND_RETURN(counter, XDP_DROP)
#define XDP_PUNT(counter) COUNT_AND_RETURN(counter, XDP_PASS)

// ----- Tethering Data Stats and Limits -----

// Tethering stats, indexed by upstream interface.
DEFINE_BPF_MAP_GRW(tether_stats_map, HASH, TetherStatsKey, TetherStatsValue, 16, AID_NETWORK_STACK)

// Tethering data limit, indexed by upstream interface.
// (tethering allowed when stats[iif].rxBytes + stats[iif].txBytes < limit[iif])
DEFINE_BPF_MAP_GRW(tether_limit_map, HASH, TetherLimitKey, TetherLimitValue, 16, AID_NETWORK_STACK)

// ----- IPv6 Support -----

DEFINE_BPF_MAP_GRW(tether_downstream6_map, HASH, TetherDownstream6Key, Tether6Value, 64,
                   AID_NETWORK_STACK)

DEFINE_BPF_MAP_GRW(tether_downstream64_map, HASH, TetherDownstream64Key, TetherDownstream64Value,
                   1024, AID_NETWORK_STACK)

DEFINE_BPF_MAP_GRW(tether_upstream6_map, HASH, TetherUpstream6Key, Tether6Value, 64,
                   AID_NETWORK_STACK)

static inline __always_inline int do_forward6(struct __sk_buff* skb, const bool is_ethernet,
        const bool downstream) {
    // Must be meta-ethernet IPv6 frame
    if (skb->protocol != htons(ETH_P_IPV6)) return TC_ACT_OK;

    // Require ethernet dst mac address to be our unicast address.
    if (is_ethernet && (skb->pkt_type != PACKET_HOST)) return TC_ACT_OK;

    const int l2_header_size = is_ethernet ? sizeof(struct ethhdr) : 0;

    // Since the program never writes via DPA (direct packet access) auto-pull/unclone logic does
    // not trigger and thus we need to manually make sure we can read packet headers via DPA.
    // Note: this is a blind best effort pull, which may fail or pull less - this doesn't matter.
    // It has to be done early cause it will invalidate any skb->data/data_end derived pointers.
    try_make_readable(skb, l2_header_size + IP6_HLEN + TCP_HLEN);

    void* data = (void*)(long)skb->data;
    const void* data_end = (void*)(long)skb->data_end;
    struct ethhdr* eth = is_ethernet ? data : NULL;  // used iff is_ethernet
    struct ipv6hdr* ip6 = is_ethernet ? (void*)(eth + 1) : data;

    // Must have (ethernet and) ipv6 header
    if (data + l2_header_size + sizeof(*ip6) > data_end) return TC_ACT_OK;

    // Ethertype - if present - must be IPv6
    if (is_ethernet && (eth->h_proto != htons(ETH_P_IPV6))) return TC_ACT_OK;

    // IP version must be 6
    if (ip6->version != 6) TC_PUNT(INVALID_IP_VERSION);

    // Cannot decrement during forward if already zero or would be zero,
    // Let the kernel's stack handle these cases and generate appropriate ICMP errors.
    if (ip6->hop_limit <= 1) TC_PUNT(LOW_TTL);

    // If hardware offload is running and programming flows based on conntrack entries,
    // try not to interfere with it.
    if (ip6->nexthdr == IPPROTO_TCP) {
        struct tcphdr* tcph = (void*)(ip6 + 1);

        // Make sure we can get at the tcp header
        if (data + l2_header_size + sizeof(*ip6) + sizeof(*tcph) > data_end)
            TC_PUNT(INVALID_TCP_HEADER);

        // Do not offload TCP packets with any one of the SYN/FIN/RST flags
        if (tcph->syn || tcph->fin || tcph->rst) TC_PUNT(TCP_CONTROL_PACKET);
    }

    // Protect against forwarding packets sourced from ::1 or fe80::/64 or other weirdness.
    __be32 src32 = ip6->saddr.s6_addr32[0];
    if (src32 != htonl(0x0064ff9b) &&                        // 64:ff9b:/32 incl. XLAT464 WKP
        (src32 & htonl(0xe0000000)) != htonl(0x20000000))    // 2000::/3 Global Unicast
        TC_PUNT(NON_GLOBAL_SRC);

    // Protect against forwarding packets destined to ::1 or fe80::/64 or other weirdness.
    __be32 dst32 = ip6->daddr.s6_addr32[0];
    if (dst32 != htonl(0x0064ff9b) &&                        // 64:ff9b:/32 incl. XLAT464 WKP
        (dst32 & htonl(0xe0000000)) != htonl(0x20000000))    // 2000::/3 Global Unicast
        TC_PUNT(NON_GLOBAL_DST);

    // In the upstream direction do not forward traffic within the same /64 subnet.
    if (!downstream && (src32 == dst32) && (ip6->saddr.s6_addr32[1] == ip6->daddr.s6_addr32[1]))
        TC_PUNT(LOCAL_SRC_DST);

    TetherDownstream6Key kd = {
            .iif = skb->ifindex,
            .neigh6 = ip6->daddr,
    };

    TetherUpstream6Key ku = {
            .iif = skb->ifindex,
    };
    if (is_ethernet) __builtin_memcpy(downstream ? kd.dstMac : ku.dstMac, eth->h_dest, ETH_ALEN);

    Tether6Value* v = downstream ? bpf_tether_downstream6_map_lookup_elem(&kd)
                                 : bpf_tether_upstream6_map_lookup_elem(&ku);

    // If we don't find any offload information then simply let the core stack handle it...
    if (!v) return TC_ACT_OK;

    uint32_t stat_and_limit_k = downstream ? skb->ifindex : v->oif;

    TetherStatsValue* stat_v = bpf_tether_stats_map_lookup_elem(&stat_and_limit_k);

    // If we don't have anywhere to put stats, then abort...
    if (!stat_v) TC_PUNT(NO_STATS_ENTRY);

    uint64_t* limit_v = bpf_tether_limit_map_lookup_elem(&stat_and_limit_k);

    // If we don't have a limit, then abort...
    if (!limit_v) TC_PUNT(NO_LIMIT_ENTRY);

    // Required IPv6 minimum mtu is 1280, below that not clear what we should do, abort...
    if (v->pmtu < IPV6_MIN_MTU) TC_PUNT(BELOW_IPV6_MTU);

    // Approximate handling of TCP/IPv6 overhead for incoming LRO/GRO packets: default
    // outbound path mtu of 1500 is not necessarily correct, but worst case we simply
    // undercount, which is still better then not accounting for this overhead at all.
    // Note: this really shouldn't be device/path mtu at all, but rather should be
    // derived from this particular connection's mss (ie. from gro segment size).
    // This would require a much newer kernel with newer ebpf accessors.
    // (This is also blindly assuming 12 bytes of tcp timestamp option in tcp header)
    uint64_t packets = 1;
    uint64_t bytes = skb->len;
    if (bytes > v->pmtu) {
        const int tcp_overhead = sizeof(struct ipv6hdr) + sizeof(struct tcphdr) + 12;
        const int mss = v->pmtu - tcp_overhead;
        const uint64_t payload = bytes - tcp_overhead;
        packets = (payload + mss - 1) / mss;
        bytes = tcp_overhead * packets + payload;
    }

    // Are we past the limit?  If so, then abort...
    // Note: will not overflow since u64 is 936 years even at 5Gbps.
    // Do not drop here.  Offload is just that, whenever we fail to handle
    // a packet we let the core stack deal with things.
    // (The core stack needs to handle limits correctly anyway,
    // since we don't offload all traffic in both directions)
    if (stat_v->rxBytes + stat_v->txBytes + bytes > *limit_v) TC_PUNT(LIMIT_REACHED);

    if (!is_ethernet) {
        // Try to inject an ethernet header, and simply return if we fail.
        // We do this even if TX interface is RAWIP and thus does not need an ethernet header,
        // because this is easier and the kernel will strip extraneous ethernet header.
        if (bpf_skb_change_head(skb, sizeof(struct ethhdr), /*flags*/ 0)) {
            __sync_fetch_and_add(downstream ? &stat_v->rxErrors : &stat_v->txErrors, 1);
            TC_PUNT(CHANGE_HEAD_FAILED);
        }

        // bpf_skb_change_head() invalidates all pointers - reload them
        data = (void*)(long)skb->data;
        data_end = (void*)(long)skb->data_end;
        eth = data;
        ip6 = (void*)(eth + 1);

        // I do not believe this can ever happen, but keep the verifier happy...
        if (data + sizeof(struct ethhdr) + sizeof(*ip6) > data_end) {
            __sync_fetch_and_add(downstream ? &stat_v->rxErrors : &stat_v->txErrors, 1);
            TC_DROP(TOO_SHORT);
        }
    };

    // At this point we always have an ethernet header - which will get stripped by the
    // kernel during transmit through a rawip interface.  ie. 'eth' pointer is valid.
    // Additionally note that 'is_ethernet' and 'l2_header_size' are no longer correct.

    // CHECKSUM_COMPLETE is a 16-bit one's complement sum,
    // thus corrections for it need to be done in 16-byte chunks at even offsets.
    // IPv6 nexthdr is at offset 6, while hop limit is at offset 7
    uint8_t old_hl = ip6->hop_limit;
    --ip6->hop_limit;
    uint8_t new_hl = ip6->hop_limit;

    // bpf_csum_update() always succeeds if the skb is CHECKSUM_COMPLETE and returns an error
    // (-ENOTSUPP) if it isn't.
    bpf_csum_update(skb, 0xFFFF - ntohs(old_hl) + ntohs(new_hl));

    __sync_fetch_and_add(downstream ? &stat_v->rxPackets : &stat_v->txPackets, packets);
    __sync_fetch_and_add(downstream ? &stat_v->rxBytes : &stat_v->txBytes, bytes);

    // Overwrite any mac header with the new one
    // For a rawip tx interface it will simply be a bunch of zeroes and later stripped.
    *eth = v->macHeader;

    // Redirect to forwarded interface.
    //
    // Note that bpf_redirect() cannot fail unless you pass invalid flags.
    // The redirect actually happens after the ebpf program has already terminated,
    // and can fail for example for mtu reasons at that point in time, but there's nothing
    // we can do about it here.
    return bpf_redirect(v->oif, 0 /* this is effectively BPF_F_EGRESS */);
}

DEFINE_BPF_PROG("schedcls/tether_downstream6_ether", AID_ROOT, AID_NETWORK_STACK,
                sched_cls_tether_downstream6_ether)
(struct __sk_buff* skb) {
    return do_forward6(skb, /* is_ethernet */ true, /* downstream */ true);
}

DEFINE_BPF_PROG("schedcls/tether_upstream6_ether", AID_ROOT, AID_NETWORK_STACK,
                sched_cls_tether_upstream6_ether)
(struct __sk_buff* skb) {
    return do_forward6(skb, /* is_ethernet */ true, /* downstream */ false);
}

// Note: section names must be unique to prevent programs from appending to each other,
// so instead the bpf loader will strip everything past the final $ symbol when actually
// pinning the program into the filesystem.
//
// bpf_skb_change_head() is only present on 4.14+ and 2 trivial kernel patches are needed:
//   ANDROID: net: bpf: Allow TC programs to call BPF_FUNC_skb_change_head
//   ANDROID: net: bpf: permit redirect from ingress L3 to egress L2 devices at near max mtu
// (the first of those has already been upstreamed)
//
// 5.4 kernel support was only added to Android Common Kernel in R,
// and thus a 5.4 kernel always supports this.
//
// Hence, these mandatory (must load successfully) implementations for 5.4+ kernels:
DEFINE_BPF_PROG_KVER("schedcls/tether_downstream6_rawip$5_4", AID_ROOT, AID_NETWORK_STACK,
                     sched_cls_tether_downstream6_rawip_5_4, KVER(5, 4, 0))
(struct __sk_buff* skb) {
    return do_forward6(skb, /* is_ethernet */ false, /* downstream */ true);
}

DEFINE_BPF_PROG_KVER("schedcls/tether_upstream6_rawip$5_4", AID_ROOT, AID_NETWORK_STACK,
                     sched_cls_tether_upstream6_rawip_5_4, KVER(5, 4, 0))
(struct __sk_buff* skb) {
    return do_forward6(skb, /* is_ethernet */ false, /* downstream */ false);
}

// and these identical optional (may fail to load) implementations for [4.14..5.4) patched kernels:
DEFINE_OPTIONAL_BPF_PROG_KVER_RANGE("schedcls/tether_downstream6_rawip$4_14",
                                    AID_ROOT, AID_NETWORK_STACK,
                                    sched_cls_tether_downstream6_rawip_4_14,
                                    KVER(4, 14, 0), KVER(5, 4, 0))
(struct __sk_buff* skb) {
    return do_forward6(skb, /* is_ethernet */ false, /* downstream */ true);
}

DEFINE_OPTIONAL_BPF_PROG_KVER_RANGE("schedcls/tether_upstream6_rawip$4_14",
                                    AID_ROOT, AID_NETWORK_STACK,
                                    sched_cls_tether_upstream6_rawip_4_14,
                                    KVER(4, 14, 0), KVER(5, 4, 0))
(struct __sk_buff* skb) {
    return do_forward6(skb, /* is_ethernet */ false, /* downstream */ false);
}

// and define no-op stubs for [4.9,4.14) and unpatched [4.14,5.4) kernels.
// (if the above real 4.14+ program loaded successfully, then bpfloader will have already pinned
// it at the same location this one would be pinned at and will thus skip loading this stub)
DEFINE_BPF_PROG_KVER_RANGE("schedcls/tether_downstream6_rawip$stub", AID_ROOT, AID_NETWORK_STACK,
                           sched_cls_tether_downstream6_rawip_stub, KVER_NONE, KVER(5, 4, 0))
(struct __sk_buff* skb) {
    return TC_ACT_OK;
}

DEFINE_BPF_PROG_KVER_RANGE("schedcls/tether_upstream6_rawip$stub", AID_ROOT, AID_NETWORK_STACK,
                           sched_cls_tether_upstream6_rawip_stub, KVER_NONE, KVER(5, 4, 0))
(struct __sk_buff* skb) {
    return TC_ACT_OK;
}

// ----- IPv4 Support -----

DEFINE_BPF_MAP_GRW(tether_downstream4_map, HASH, Tether4Key, Tether4Value, 1024, AID_NETWORK_STACK)

DEFINE_BPF_MAP_GRW(tether_upstream4_map, HASH, Tether4Key, Tether4Value, 1024, AID_NETWORK_STACK)

static inline __always_inline int do_forward4(struct __sk_buff* skb, const bool is_ethernet,
        const bool downstream, const bool updatetime) {
    // Require ethernet dst mac address to be our unicast address.
    if (is_ethernet && (skb->pkt_type != PACKET_HOST)) return TC_ACT_OK;

    // Must be meta-ethernet IPv4 frame
    if (skb->protocol != htons(ETH_P_IP)) return TC_ACT_OK;

    const int l2_header_size = is_ethernet ? sizeof(struct ethhdr) : 0;

    // Since the program never writes via DPA (direct packet access) auto-pull/unclone logic does
    // not trigger and thus we need to manually make sure we can read packet headers via DPA.
    // Note: this is a blind best effort pull, which may fail or pull less - this doesn't matter.
    // It has to be done early cause it will invalidate any skb->data/data_end derived pointers.
    try_make_readable(skb, l2_header_size + IP4_HLEN + TCP_HLEN);

    void* data = (void*)(long)skb->data;
    const void* data_end = (void*)(long)skb->data_end;
    struct ethhdr* eth = is_ethernet ? data : NULL;  // used iff is_ethernet
    struct iphdr* ip = is_ethernet ? (void*)(eth + 1) : data;

    // Must have (ethernet and) ipv4 header
    if (data + l2_header_size + sizeof(*ip) > data_end) return TC_ACT_OK;

    // Ethertype - if present - must be IPv4
    if (is_ethernet && (eth->h_proto != htons(ETH_P_IP))) return TC_ACT_OK;

    // IP version must be 4
    if (ip->version != 4) TC_PUNT(INVALID_IP_VERSION);

    // We cannot handle IP options, just standard 20 byte == 5 dword minimal IPv4 header
    if (ip->ihl != 5) TC_PUNT(HAS_IP_OPTIONS);

    // Calculate the IPv4 one's complement checksum of the IPv4 header.
    __wsum sum4 = 0;
    for (int i = 0; i < sizeof(*ip) / sizeof(__u16); ++i) {
        sum4 += ((__u16*)ip)[i];
    }
    // Note that sum4 is guaranteed to be non-zero by virtue of ip4->version == 4
    sum4 = (sum4 & 0xFFFF) + (sum4 >> 16);  // collapse u32 into range 1 .. 0x1FFFE
    sum4 = (sum4 & 0xFFFF) + (sum4 >> 16);  // collapse any potential carry into u16
    // for a correct checksum we should get *a* zero, but sum4 must be positive, ie 0xFFFF
    if (sum4 != 0xFFFF) TC_PUNT(CHECKSUM);

    // Minimum IPv4 total length is the size of the header
    if (ntohs(ip->tot_len) < sizeof(*ip)) TC_PUNT(TRUNCATED_IPV4);

    // We are incapable of dealing with IPv4 fragments
    if (ip->frag_off & ~htons(IP_DF)) TC_PUNT(IS_IP_FRAG);

    // Cannot decrement during forward if already zero or would be zero,
    // Let the kernel's stack handle these cases and generate appropriate ICMP errors.
    if (ip->ttl <= 1) TC_PUNT(LOW_TTL);

    // If we cannot update the 'last_used' field due to lack of bpf_ktime_get_boot_ns() helper,
    // then it is not safe to offload UDP due to the small conntrack timeouts, as such,
    // in such a situation we can only support TCP.  This also has the added nice benefit of
    // using a separate error counter, and thus making it obvious which version of the program
    // is loaded.
    if (!updatetime && ip->protocol != IPPROTO_TCP) TC_PUNT(NON_TCP);

    // We do not support offloading anything besides IPv4 TCP and UDP, due to need for NAT,
    // but no need to check this if !updatetime due to check immediately above.
    if (updatetime && (ip->protocol != IPPROTO_TCP) && (ip->protocol != IPPROTO_UDP))
        TC_PUNT(NON_TCP_UDP);

    // We want to make sure that the compiler will, in the !updatetime case, entirely optimize
    // out all the non-tcp logic.  Also note that at this point is_udp === !is_tcp.
    const bool is_tcp = !updatetime || (ip->protocol == IPPROTO_TCP);

    // This is a bit of a hack to make things easier on the bpf verifier.
    // (In particular I believe the Linux 4.14 kernel's verifier can get confused later on about
    // what offsets into the packet are valid and can spuriously reject the program, this is
    // because it fails to realize that is_tcp && !is_tcp is impossible)
    //
    // For both TCP & UDP we'll need to read and modify the src/dst ports, which so happen to
    // always be in the first 4 bytes of the L4 header.  Additionally for UDP we'll need access
    // to the checksum field which is in bytes 7 and 8.  While for TCP we'll need to read the
    // TCP flags (at offset 13) and access to the checksum field (2 bytes at offset 16).
    // As such we *always* need access to at least 8 bytes.
    if (data + l2_header_size + sizeof(*ip) + 8 > data_end) TC_PUNT(SHORT_L4_HEADER);

    struct tcphdr* tcph = is_tcp ? (void*)(ip + 1) : NULL;
    struct udphdr* udph = is_tcp ? NULL : (void*)(ip + 1);

    if (is_tcp) {
        // Make sure we can get at the tcp header
        if (data + l2_header_size + sizeof(*ip) + sizeof(*tcph) > data_end)
            TC_PUNT(SHORT_TCP_HEADER);

        // If hardware offload is running and programming flows based on conntrack entries, try not
        // to interfere with it, so do not offload TCP packets with any one of the SYN/FIN/RST flags
        if (tcph->syn || tcph->fin || tcph->rst) TC_PUNT(TCP_CONTROL_PACKET);
    } else { // UDP
        // Make sure we can get at the udp header
        if (data + l2_header_size + sizeof(*ip) + sizeof(*udph) > data_end)
            TC_PUNT(SHORT_UDP_HEADER);

        // Skip handling of CHECKSUM_COMPLETE packets with udp checksum zero due to need for
        // additional updating of skb->csum (this could be fixed up manually with more effort).
        //
        // Note that the in-kernel implementation of 'int64_t bpf_csum_update(skb, u32 csum)' is:
        //   if (skb->ip_summed == CHECKSUM_COMPLETE)
        //     return (skb->csum = csum_add(skb->csum, csum));
        //   else
        //     return -ENOTSUPP;
        //
        // So this will punt any CHECKSUM_COMPLETE packet with a zero UDP checksum,
        // and leave all other packets unaffected (since it just at most adds zero to skb->csum).
        //
        // In practice this should almost never trigger because most nics do not generate
        // CHECKSUM_COMPLETE packets on receive - especially so for nics/drivers on a phone.
        //
        // Additionally since we're forwarding, in most cases the value of the skb->csum field
        // shouldn't matter (it's not used by physical nic egress).
        //
        // It only matters if we're ingressing through a CHECKSUM_COMPLETE capable nic
        // and egressing through a virtual interface looping back to the kernel itself
        // (ie. something like veth) where the CHECKSUM_COMPLETE/skb->csum can get reused
        // on ingress.
        //
        // If we were in the kernel we'd simply probably call
        //   void skb_checksum_complete_unset(struct sk_buff *skb) {
        //     if (skb->ip_summed == CHECKSUM_COMPLETE) skb->ip_summed = CHECKSUM_NONE;
        //   }
        // here instead.  Perhaps there should be a bpf helper for that?
        if (!udph->check && (bpf_csum_update(skb, 0) >= 0)) TC_PUNT(UDP_CSUM_ZERO);
    }

    Tether4Key k = {
            .iif = skb->ifindex,
            .l4Proto = ip->protocol,
            .src4.s_addr = ip->saddr,
            .dst4.s_addr = ip->daddr,
            .srcPort = is_tcp ? tcph->source : udph->source,
            .dstPort = is_tcp ? tcph->dest : udph->dest,
    };
    if (is_ethernet) __builtin_memcpy(k.dstMac, eth->h_dest, ETH_ALEN);

    Tether4Value* v = downstream ? bpf_tether_downstream4_map_lookup_elem(&k)
                                 : bpf_tether_upstream4_map_lookup_elem(&k);

    // If we don't find any offload information then simply let the core stack handle it...
    if (!v) return TC_ACT_OK;

    uint32_t stat_and_limit_k = downstream ? skb->ifindex : v->oif;

    TetherStatsValue* stat_v = bpf_tether_stats_map_lookup_elem(&stat_and_limit_k);

    // If we don't have anywhere to put stats, then abort...
    if (!stat_v) TC_PUNT(NO_STATS_ENTRY);

    uint64_t* limit_v = bpf_tether_limit_map_lookup_elem(&stat_and_limit_k);

    // If we don't have a limit, then abort...
    if (!limit_v) TC_PUNT(NO_LIMIT_ENTRY);

    // Required IPv4 minimum mtu is 68, below that not clear what we should do, abort...
    if (v->pmtu < 68) TC_PUNT(BELOW_IPV4_MTU);

    // Approximate handling of TCP/IPv4 overhead for incoming LRO/GRO packets: default
    // outbound path mtu of 1500 is not necessarily correct, but worst case we simply
    // undercount, which is still better then not accounting for this overhead at all.
    // Note: this really shouldn't be device/path mtu at all, but rather should be
    // derived from this particular connection's mss (ie. from gro segment size).
    // This would require a much newer kernel with newer ebpf accessors.
    // (This is also blindly assuming 12 bytes of tcp timestamp option in tcp header)
    uint64_t packets = 1;
    uint64_t bytes = skb->len;
    if (bytes > v->pmtu) {
        const int tcp_overhead = sizeof(struct iphdr) + sizeof(struct tcphdr) + 12;
        const int mss = v->pmtu - tcp_overhead;
        const uint64_t payload = bytes - tcp_overhead;
        packets = (payload + mss - 1) / mss;
        bytes = tcp_overhead * packets + payload;
    }

    // Are we past the limit?  If so, then abort...
    // Note: will not overflow since u64 is 936 years even at 5Gbps.
    // Do not drop here.  Offload is just that, whenever we fail to handle
    // a packet we let the core stack deal with things.
    // (The core stack needs to handle limits correctly anyway,
    // since we don't offload all traffic in both directions)
    if (stat_v->rxBytes + stat_v->txBytes + bytes > *limit_v) TC_PUNT(LIMIT_REACHED);

    if (!is_ethernet) {
        // Try to inject an ethernet header, and simply return if we fail.
        // We do this even if TX interface is RAWIP and thus does not need an ethernet header,
        // because this is easier and the kernel will strip extraneous ethernet header.
        if (bpf_skb_change_head(skb, sizeof(struct ethhdr), /*flags*/ 0)) {
            __sync_fetch_and_add(downstream ? &stat_v->rxErrors : &stat_v->txErrors, 1);
            TC_PUNT(CHANGE_HEAD_FAILED);
        }

        // bpf_skb_change_head() invalidates all pointers - reload them
        data = (void*)(long)skb->data;
        data_end = (void*)(long)skb->data_end;
        eth = data;
        ip = (void*)(eth + 1);
        tcph = is_tcp ? (void*)(ip + 1) : NULL;
        udph = is_tcp ? NULL : (void*)(ip + 1);

        // I do not believe this can ever happen, but keep the verifier happy...
        if (data + sizeof(struct ethhdr) + sizeof(*ip) + (is_tcp ? sizeof(*tcph) : sizeof(*udph)) > data_end) {
            __sync_fetch_and_add(downstream ? &stat_v->rxErrors : &stat_v->txErrors, 1);
            TC_DROP(TOO_SHORT);
        }
    };

    // At this point we always have an ethernet header - which will get stripped by the
    // kernel during transmit through a rawip interface.  ie. 'eth' pointer is valid.
    // Additionally note that 'is_ethernet' and 'l2_header_size' are no longer correct.

    // Overwrite any mac header with the new one
    // For a rawip tx interface it will simply be a bunch of zeroes and later stripped.
    *eth = v->macHeader;

    // Decrement the IPv4 TTL, we already know it's greater than 1.
    // u8 TTL field is followed by u8 protocol to make a u16 for ipv4 header checksum update.
    // Since we're keeping the ipv4 checksum valid (which means the checksum of the entire
    // ipv4 header remains 0), the overall checksum of the entire packet does not change.
    const int sz2 = sizeof(__be16);
    const __be16 old_ttl_proto = *(__be16 *)&ip->ttl;
    const __be16 new_ttl_proto = old_ttl_proto - htons(0x0100);
    bpf_l3_csum_replace(skb, ETH_IP4_OFFSET(check), old_ttl_proto, new_ttl_proto, sz2);
    bpf_skb_store_bytes(skb, ETH_IP4_OFFSET(ttl), &new_ttl_proto, sz2, 0);

    const int l4_offs_csum = is_tcp ? ETH_IP4_TCP_OFFSET(check) : ETH_IP4_UDP_OFFSET(check);
    const int sz4 = sizeof(__be32);
    // UDP 0 is special and stored as FFFF (this flag also causes a csum of 0 to be unmodified)
    const int l4_flags = is_tcp ? 0 : BPF_F_MARK_MANGLED_0;
    const __be32 old_daddr = k.dst4.s_addr;
    const __be32 old_saddr = k.src4.s_addr;
    const __be32 new_daddr = v->dst46.s6_addr32[3];
    const __be32 new_saddr = v->src46.s6_addr32[3];

    bpf_l4_csum_replace(skb, l4_offs_csum, old_daddr, new_daddr, sz4 | BPF_F_PSEUDO_HDR | l4_flags);
    bpf_l3_csum_replace(skb, ETH_IP4_OFFSET(check), old_daddr, new_daddr, sz4);
    bpf_skb_store_bytes(skb, ETH_IP4_OFFSET(daddr), &new_daddr, sz4, 0);

    bpf_l4_csum_replace(skb, l4_offs_csum, old_saddr, new_saddr, sz4 | BPF_F_PSEUDO_HDR | l4_flags);
    bpf_l3_csum_replace(skb, ETH_IP4_OFFSET(check), old_saddr, new_saddr, sz4);
    bpf_skb_store_bytes(skb, ETH_IP4_OFFSET(saddr), &new_saddr, sz4, 0);

    // The offsets for TCP and UDP ports: source (u16 @ L4 offset 0) & dest (u16 @ L4 offset 2) are
    // actually the same, so the compiler should just optimize them both down to a constant.
    bpf_l4_csum_replace(skb, l4_offs_csum, k.srcPort, v->srcPort, sz2 | l4_flags);
    bpf_skb_store_bytes(skb, is_tcp ? ETH_IP4_TCP_OFFSET(source) : ETH_IP4_UDP_OFFSET(source),
                        &v->srcPort, sz2, 0);

    bpf_l4_csum_replace(skb, l4_offs_csum, k.dstPort, v->dstPort, sz2 | l4_flags);
    bpf_skb_store_bytes(skb, is_tcp ? ETH_IP4_TCP_OFFSET(dest) : ETH_IP4_UDP_OFFSET(dest),
                        &v->dstPort, sz2, 0);

    // This requires the bpf_ktime_get_boot_ns() helper which was added in 5.8,
    // and backported to all Android Common Kernel 4.14+ trees.
    if (updatetime) v->last_used = bpf_ktime_get_boot_ns();

    __sync_fetch_and_add(downstream ? &stat_v->rxPackets : &stat_v->txPackets, packets);
    __sync_fetch_and_add(downstream ? &stat_v->rxBytes : &stat_v->txBytes, bytes);

    // Redirect to forwarded interface.
    //
    // Note that bpf_redirect() cannot fail unless you pass invalid flags.
    // The redirect actually happens after the ebpf program has already terminated,
    // and can fail for example for mtu reasons at that point in time, but there's nothing
    // we can do about it here.
    return bpf_redirect(v->oif, 0 /* this is effectively BPF_F_EGRESS */);
}

// Full featured (required) implementations for 5.8+ kernels (these are S+ by definition)

DEFINE_BPF_PROG_KVER("schedcls/tether_downstream4_rawip$5_8", AID_ROOT, AID_NETWORK_STACK,
                     sched_cls_tether_downstream4_rawip_5_8, KVER(5, 8, 0))
(struct __sk_buff* skb) {
    return do_forward4(skb, /* is_ethernet */ false, /* downstream */ true, /* updatetime */ true);
}

DEFINE_BPF_PROG_KVER("schedcls/tether_upstream4_rawip$5_8", AID_ROOT, AID_NETWORK_STACK,
                     sched_cls_tether_upstream4_rawip_5_8, KVER(5, 8, 0))
(struct __sk_buff* skb) {
    return do_forward4(skb, /* is_ethernet */ false, /* downstream */ false, /* updatetime */ true);
}

DEFINE_BPF_PROG_KVER("schedcls/tether_downstream4_ether$5_8", AID_ROOT, AID_NETWORK_STACK,
                     sched_cls_tether_downstream4_ether_5_8, KVER(5, 8, 0))
(struct __sk_buff* skb) {
    return do_forward4(skb, /* is_ethernet */ true, /* downstream */ true, /* updatetime */ true);
}

DEFINE_BPF_PROG_KVER("schedcls/tether_upstream4_ether$5_8", AID_ROOT, AID_NETWORK_STACK,
                     sched_cls_tether_upstream4_ether_5_8, KVER(5, 8, 0))
(struct __sk_buff* skb) {
    return do_forward4(skb, /* is_ethernet */ true, /* downstream */ false, /* updatetime */ true);
}

// Full featured (optional) implementations for 4.14-S, 4.19-S & 5.4-S kernels
// (optional, because we need to be able to fallback for 4.14/4.19/5.4 pre-S kernels)

DEFINE_OPTIONAL_BPF_PROG_KVER_RANGE("schedcls/tether_downstream4_rawip$opt",
                                    AID_ROOT, AID_NETWORK_STACK,
                                    sched_cls_tether_downstream4_rawip_opt,
                                    KVER(4, 14, 0), KVER(5, 8, 0))
(struct __sk_buff* skb) {
    return do_forward4(skb, /* is_ethernet */ false, /* downstream */ true, /* updatetime */ true);
}

DEFINE_OPTIONAL_BPF_PROG_KVER_RANGE("schedcls/tether_upstream4_rawip$opt",
                                    AID_ROOT, AID_NETWORK_STACK,
                                    sched_cls_tether_upstream4_rawip_opt,
                                    KVER(4, 14, 0), KVER(5, 8, 0))
(struct __sk_buff* skb) {
    return do_forward4(skb, /* is_ethernet */ false, /* downstream */ false, /* updatetime */ true);
}

DEFINE_OPTIONAL_BPF_PROG_KVER_RANGE("schedcls/tether_downstream4_ether$opt",
                                    AID_ROOT, AID_NETWORK_STACK,
                                    sched_cls_tether_downstream4_ether_opt,
                                    KVER(4, 14, 0), KVER(5, 8, 0))
(struct __sk_buff* skb) {
    return do_forward4(skb, /* is_ethernet */ true, /* downstream */ true, /* updatetime */ true);
}

DEFINE_OPTIONAL_BPF_PROG_KVER_RANGE("schedcls/tether_upstream4_ether$opt",
                                    AID_ROOT, AID_NETWORK_STACK,
                                    sched_cls_tether_upstream4_ether_opt,
                                    KVER(4, 14, 0), KVER(5, 8, 0))
(struct __sk_buff* skb) {
    return do_forward4(skb, /* is_ethernet */ true, /* downstream */ false, /* updatetime */ true);
}

// Partial (TCP-only: will not update 'last_used' field) implementations for 4.14+ kernels.
// These will be loaded only if the above optional ones failed (loading of *these* must succeed
// for 5.4+, since that is always an R patched kernel).
//
// [Note: as a result TCP connections will not have their conntrack timeout refreshed, however,
// since /proc/sys/net/netfilter/nf_conntrack_tcp_timeout_established defaults to 432000 (seconds),
// this in practice means they'll break only after 5 days.  This seems an acceptable trade-off.
//
// Additionally kernel/tests change "net-test: add bpf_ktime_get_ns / bpf_ktime_get_boot_ns tests"
// which enforces and documents the required kernel cherrypicks will make it pretty unlikely that
// many devices upgrading to S will end up relying on these fallback programs.

// RAWIP: Required for 5.4-R kernels -- which always support bpf_skb_change_head().

DEFINE_BPF_PROG_KVER_RANGE("schedcls/tether_downstream4_rawip$5_4", AID_ROOT, AID_NETWORK_STACK,
                           sched_cls_tether_downstream4_rawip_5_4, KVER(5, 4, 0), KVER(5, 8, 0))
(struct __sk_buff* skb) {
    return do_forward4(skb, /* is_ethernet */ false, /* downstream */ true, /* updatetime */ false);
}

DEFINE_BPF_PROG_KVER_RANGE("schedcls/tether_upstream4_rawip$5_4", AID_ROOT, AID_NETWORK_STACK,
                           sched_cls_tether_upstream4_rawip_5_4, KVER(5, 4, 0), KVER(5, 8, 0))
(struct __sk_buff* skb) {
    return do_forward4(skb, /* is_ethernet */ false, /* downstream */ false, /* updatetime */ false);
}

// RAWIP: Optional for 4.14/4.19 (R) kernels -- which support bpf_skb_change_head().
// [Note: fallback for 4.14/4.19 (P/Q) kernels is below in stub section]

DEFINE_OPTIONAL_BPF_PROG_KVER_RANGE("schedcls/tether_downstream4_rawip$4_14",
                                    AID_ROOT, AID_NETWORK_STACK,
                                    sched_cls_tether_downstream4_rawip_4_14,
                                    KVER(4, 14, 0), KVER(5, 4, 0))
(struct __sk_buff* skb) {
    return do_forward4(skb, /* is_ethernet */ false, /* downstream */ true, /* updatetime */ false);
}

DEFINE_OPTIONAL_BPF_PROG_KVER_RANGE("schedcls/tether_upstream4_rawip$4_14",
                                    AID_ROOT, AID_NETWORK_STACK,
                                    sched_cls_tether_upstream4_rawip_4_14,
                                    KVER(4, 14, 0), KVER(5, 4, 0))
(struct __sk_buff* skb) {
    return do_forward4(skb, /* is_ethernet */ false, /* downstream */ false, /* updatetime */ false);
}

// ETHER: Required for 4.14-Q/R, 4.19-Q/R & 5.4-R kernels.

DEFINE_BPF_PROG_KVER_RANGE("schedcls/tether_downstream4_ether$4_14", AID_ROOT, AID_NETWORK_STACK,
                           sched_cls_tether_downstream4_ether_4_14, KVER(4, 14, 0), KVER(5, 8, 0))
(struct __sk_buff* skb) {
    return do_forward4(skb, /* is_ethernet */ true, /* downstream */ true, /* updatetime */ false);
}

DEFINE_BPF_PROG_KVER_RANGE("schedcls/tether_upstream4_ether$4_14", AID_ROOT, AID_NETWORK_STACK,
                           sched_cls_tether_upstream4_ether_4_14, KVER(4, 14, 0), KVER(5, 8, 0))
(struct __sk_buff* skb) {
    return do_forward4(skb, /* is_ethernet */ true, /* downstream */ false, /* updatetime */ false);
}

// Placeholder (no-op) implementations for older Q kernels

// RAWIP: 4.9-P/Q, 4.14-P/Q & 4.19-Q kernels -- without bpf_skb_change_head() for tc programs

DEFINE_BPF_PROG_KVER_RANGE("schedcls/tether_downstream4_rawip$stub", AID_ROOT, AID_NETWORK_STACK,
                           sched_cls_tether_downstream4_rawip_stub, KVER_NONE, KVER(5, 4, 0))
(struct __sk_buff* skb) {
    return TC_ACT_OK;
}

DEFINE_BPF_PROG_KVER_RANGE("schedcls/tether_upstream4_rawip$stub", AID_ROOT, AID_NETWORK_STACK,
                           sched_cls_tether_upstream4_rawip_stub, KVER_NONE, KVER(5, 4, 0))
(struct __sk_buff* skb) {
    return TC_ACT_OK;
}

// ETHER: 4.9-P/Q kernel

DEFINE_BPF_PROG_KVER_RANGE("schedcls/tether_downstream4_ether$stub", AID_ROOT, AID_NETWORK_STACK,
                           sched_cls_tether_downstream4_ether_stub, KVER_NONE, KVER(4, 14, 0))
(struct __sk_buff* skb) {
    return TC_ACT_OK;
}

DEFINE_BPF_PROG_KVER_RANGE("schedcls/tether_upstream4_ether$stub", AID_ROOT, AID_NETWORK_STACK,
                           sched_cls_tether_upstream4_ether_stub, KVER_NONE, KVER(4, 14, 0))
(struct __sk_buff* skb) {
    return TC_ACT_OK;
}

// ----- XDP Support -----

DEFINE_BPF_MAP_GRW(tether_dev_map, DEVMAP_HASH, uint32_t, uint32_t, 64, AID_NETWORK_STACK)

static inline __always_inline int do_xdp_forward6(struct xdp_md *ctx, const bool is_ethernet,
        const bool downstream) {
    return XDP_PASS;
}

static inline __always_inline int do_xdp_forward4(struct xdp_md *ctx, const bool is_ethernet,
        const bool downstream) {
    return XDP_PASS;
}

static inline __always_inline int do_xdp_forward_ether(struct xdp_md *ctx, const bool downstream) {
    const void* data = (void*)(long)ctx->data;
    const void* data_end = (void*)(long)ctx->data_end;
    const struct ethhdr* eth = data;

    // Make sure we actually have an ethernet header
    if ((void*)(eth + 1) > data_end) return XDP_PASS;

    if (eth->h_proto == htons(ETH_P_IPV6))
        return do_xdp_forward6(ctx, /* is_ethernet */ true, downstream);
    if (eth->h_proto == htons(ETH_P_IP))
        return do_xdp_forward4(ctx, /* is_ethernet */ true, downstream);

    // Anything else we don't know how to handle...
    return XDP_PASS;
}

static inline __always_inline int do_xdp_forward_rawip(struct xdp_md *ctx, const bool downstream) {
    const void* data = (void*)(long)ctx->data;
    const void* data_end = (void*)(long)ctx->data_end;

    // The top nibble of both IPv4 and IPv6 headers is the IP version.
    if (data_end - data < 1) return XDP_PASS;
    const uint8_t v = (*(uint8_t*)data) >> 4;

    if (v == 6) return do_xdp_forward6(ctx, /* is_ethernet */ false, downstream);
    if (v == 4) return do_xdp_forward4(ctx, /* is_ethernet */ false, downstream);

    // Anything else we don't know how to handle...
    return XDP_PASS;
}

#define DEFINE_XDP_PROG(str, func) \
    DEFINE_BPF_PROG_KVER(str, AID_ROOT, AID_NETWORK_STACK, func, KVER(5, 9, 0))(struct xdp_md *ctx)

DEFINE_XDP_PROG("xdp/tether_downstream_ether",
                 xdp_tether_downstream_ether) {
    return do_xdp_forward_ether(ctx, /* downstream */ true);
}

DEFINE_XDP_PROG("xdp/tether_downstream_rawip",
                 xdp_tether_downstream_rawip) {
    return do_xdp_forward_rawip(ctx, /* downstream */ true);
}

DEFINE_XDP_PROG("xdp/tether_upstream_ether",
                 xdp_tether_upstream_ether) {
    return do_xdp_forward_ether(ctx, /* downstream */ false);
}

DEFINE_XDP_PROG("xdp/tether_upstream_rawip",
                 xdp_tether_upstream_rawip) {
    return do_xdp_forward_rawip(ctx, /* downstream */ false);
}

LICENSE("Apache 2.0");
CRITICAL("tethering");
