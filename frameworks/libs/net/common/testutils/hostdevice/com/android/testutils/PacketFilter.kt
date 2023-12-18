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

package com.android.testutils

import java.net.Inet4Address
import java.util.function.Predicate

const val ETHER_TYPE_OFFSET = 12
const val ETHER_HEADER_LENGTH = 14
const val IPV4_PROTOCOL_OFFSET = ETHER_HEADER_LENGTH + 9
const val IPV4_CHECKSUM_OFFSET = ETHER_HEADER_LENGTH + 10
const val IPV4_DST_OFFSET = ETHER_HEADER_LENGTH + 16
const val IPV4_HEADER_LENGTH = 20
const val IPV4_UDP_OFFSET = ETHER_HEADER_LENGTH + IPV4_HEADER_LENGTH
const val IPV4_UDP_SRCPORT_OFFSET = IPV4_UDP_OFFSET
const val IPV4_UDP_DSTPORT_OFFSET = IPV4_UDP_OFFSET + 2
const val UDP_HEADER_LENGTH = 8
const val BOOTP_OFFSET = IPV4_UDP_OFFSET + UDP_HEADER_LENGTH
const val BOOTP_TID_OFFSET = BOOTP_OFFSET + 4
const val BOOTP_CLIENT_MAC_OFFSET = BOOTP_OFFSET + 28
const val DHCP_OPTIONS_OFFSET = BOOTP_OFFSET + 240

const val ARP_OPCODE_OFFSET = ETHER_HEADER_LENGTH + 6
const val ARP_SENDER_MAC_OFFSET = ETHER_HEADER_LENGTH + 8
const val ARP_TARGET_IPADDR_OFFSET = ETHER_HEADER_LENGTH + 24

/**
 * A [Predicate] that matches a [ByteArray] if it contains the specified [bytes] at the specified
 * [offset].
 */
class OffsetFilter(val offset: Int, vararg val bytes: Byte) : Predicate<ByteArray> {
    override fun test(packet: ByteArray) =
            bytes.withIndex().all { it.value == packet[offset + it.index] }
}

/**
 * A [Predicate] that matches ethernet-encapped packets that contain an UDP over IPv4 datagram.
 */
class IPv4UdpFilter : Predicate<ByteArray> {
    private val impl = OffsetFilter(ETHER_TYPE_OFFSET, 0x08, 0x00 /* IPv4 */).and(
            OffsetFilter(IPV4_PROTOCOL_OFFSET, 17 /* UDP */))
    override fun test(t: ByteArray) = impl.test(t)
}

/**
 * A [Predicate] that matches ethernet-encapped packets sent to the specified IPv4 destination.
 */
class IPv4DstFilter(dst: Inet4Address) : Predicate<ByteArray> {
    private val impl = OffsetFilter(IPV4_DST_OFFSET, *dst.address)
    override fun test(t: ByteArray) = impl.test(t)
}

/**
 * A [Predicate] that matches ethernet-encapped ARP requests.
 */
class ArpRequestFilter : Predicate<ByteArray> {
    private val impl = OffsetFilter(ETHER_TYPE_OFFSET, 0x08, 0x06 /* ARP */)
            .and(OffsetFilter(ARP_OPCODE_OFFSET, 0x00, 0x01 /* request */))
    override fun test(t: ByteArray) = impl.test(t)
}

/**
 * A [Predicate] that matches ethernet-encapped DHCP packets sent from a DHCP client.
 */
class DhcpClientPacketFilter : Predicate<ByteArray> {
    private val impl = IPv4UdpFilter()
            .and(OffsetFilter(IPV4_UDP_SRCPORT_OFFSET, 0x00, 0x44 /* 68 */))
            .and(OffsetFilter(IPV4_UDP_DSTPORT_OFFSET, 0x00, 0x43 /* 67 */))
    override fun test(t: ByteArray) = impl.test(t)
}

/**
 * A [Predicate] that matches a [ByteArray] if it contains a ethernet-encapped DHCP packet that
 * contains the specified option with the specified [bytes] as value.
 */
class DhcpOptionFilter(val option: Byte, vararg val bytes: Byte) : Predicate<ByteArray> {
    override fun test(packet: ByteArray): Boolean {
        val option = findDhcpOption(packet, option) ?: return false
        return option.contentEquals(bytes)
    }
}

/**
 * Find a DHCP option in a packet and return its value, if found.
 */
fun findDhcpOption(packet: ByteArray, option: Byte): ByteArray? =
        findOptionOffset(packet, option, DHCP_OPTIONS_OFFSET)?.let {
            val optionLen = packet[it + 1]
            return packet.copyOfRange(it + 2 /* type, length bytes */, it + 2 + optionLen)
        }

private tailrec fun findOptionOffset(packet: ByteArray, option: Byte, searchOffset: Int): Int? {
    if (packet.size <= searchOffset + 2 /* type, length bytes */) return null

    return if (packet[searchOffset] == option) searchOffset else {
        val optionLen = packet[searchOffset + 1]
        findOptionOffset(packet, option, searchOffset + 2 + optionLen)
    }
}
