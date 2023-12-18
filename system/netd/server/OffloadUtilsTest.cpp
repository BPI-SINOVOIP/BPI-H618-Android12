/*
 * Copyright 2019 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * OffloadUtilsTest.cpp - unit tests for OffloadUtils.cpp
 */

#include <gtest/gtest.h>

#include "OffloadUtils.h"

#include <linux/if_arp.h>
#include <stdlib.h>
#include <sys/wait.h>

#include "bpf/BpfUtils.h"
#include "netdbpf/bpf_shared.h"

namespace android {
namespace net {

class OffloadUtilsTest : public ::testing::Test {
  public:
    void SetUp() {}
};

TEST_F(OffloadUtilsTest, HardwareAddressTypeOfNonExistingIf) {
    ASSERT_EQ(-ENODEV, hardwareAddressType("not_existing_if"));
}

TEST_F(OffloadUtilsTest, HardwareAddressTypeOfLoopback) {
    ASSERT_EQ(ARPHRD_LOOPBACK, hardwareAddressType("lo"));
}

// If wireless 'wlan0' interface exists it should be Ethernet.
TEST_F(OffloadUtilsTest, HardwareAddressTypeOfWireless) {
    int type = hardwareAddressType("wlan0");
    if (type == -ENODEV) return;

    ASSERT_EQ(ARPHRD_ETHER, type);
}

// If cellular 'rmnet_data0' interface exists it should
// *probably* not be Ethernet and instead be RawIp.
TEST_F(OffloadUtilsTest, HardwareAddressTypeOfCellular) {
    int type = hardwareAddressType("rmnet_data0");
    if (type == -ENODEV) return;

    ASSERT_NE(ARPHRD_ETHER, type);

    // ARPHRD_RAWIP is 530 on some pre-4.14 Qualcomm devices.
    if (type == 530) return;

    ASSERT_EQ(ARPHRD_RAWIP, type);
}

TEST_F(OffloadUtilsTest, IsEthernetOfNonExistingIf) {
    auto res = isEthernet("not_existing_if");
    ASSERT_FALSE(res.ok());
    ASSERT_EQ(ENODEV, res.error().code());
}

TEST_F(OffloadUtilsTest, IsEthernetOfLoopback) {
    auto res = isEthernet("lo");
    ASSERT_FALSE(res.ok());
    ASSERT_EQ(EAFNOSUPPORT, res.error().code());
}

// If wireless 'wlan0' interface exists it should be Ethernet.
// See also HardwareAddressTypeOfWireless.
TEST_F(OffloadUtilsTest, IsEthernetOfWireless) {
    auto res = isEthernet("wlan0");
    if (!res.ok() && res.error().code() == ENODEV) return;

    ASSERT_RESULT_OK(res);
    ASSERT_TRUE(res.value());
}

// If cellular 'rmnet_data0' interface exists it should
// *probably* not be Ethernet and instead be RawIp.
// See also HardwareAddressTypeOfCellular.
TEST_F(OffloadUtilsTest, IsEthernetOfCellular) {
    auto res = isEthernet("rmnet_data0");
    if (!res.ok() && res.error().code() == ENODEV) return;

    ASSERT_RESULT_OK(res);
    ASSERT_FALSE(res.value());
}

TEST_F(OffloadUtilsTest, DeviceMTUOfNonExistingIf) {
    ASSERT_EQ(-ENODEV, deviceMTU("not_existing_if"));
}

TEST_F(OffloadUtilsTest, DeviceMTUofLoopback) {
    ASSERT_EQ(65536, deviceMTU("lo"));
}

TEST_F(OffloadUtilsTest, GetClatEgress4MapFd) {
    int fd = getClatEgress4MapFd();
    ASSERT_GE(fd, 3);  // 0,1,2 - stdin/out/err, thus fd >= 3
    EXPECT_EQ(FD_CLOEXEC, fcntl(fd, F_GETFD));
    close(fd);
}

TEST_F(OffloadUtilsTest, GetClatEgress4RawIpProgFd) {
    int fd = getClatEgress4ProgFd(RAWIP);
    ASSERT_GE(fd, 3);
    EXPECT_EQ(FD_CLOEXEC, fcntl(fd, F_GETFD));
    close(fd);
}

TEST_F(OffloadUtilsTest, GetClatEgress4EtherProgFd) {
    int fd = getClatEgress4ProgFd(ETHER);
    ASSERT_GE(fd, 3);
    EXPECT_EQ(FD_CLOEXEC, fcntl(fd, F_GETFD));
    close(fd);
}

TEST_F(OffloadUtilsTest, GetClatIngress6MapFd) {
    int fd = getClatIngress6MapFd();
    ASSERT_GE(fd, 3);  // 0,1,2 - stdin/out/err, thus fd >= 3
    EXPECT_EQ(FD_CLOEXEC, fcntl(fd, F_GETFD));
    close(fd);
}

TEST_F(OffloadUtilsTest, GetClatIngress6RawIpProgFd) {
    int fd = getClatIngress6ProgFd(RAWIP);
    ASSERT_GE(fd, 3);
    EXPECT_EQ(FD_CLOEXEC, fcntl(fd, F_GETFD));
    close(fd);
}

TEST_F(OffloadUtilsTest, GetClatIngress6EtherProgFd) {
    int fd = getClatIngress6ProgFd(ETHER);
    ASSERT_GE(fd, 3);
    EXPECT_EQ(FD_CLOEXEC, fcntl(fd, F_GETFD));
    close(fd);
}

// See Linux kernel source in include/net/flow.h
#define LOOPBACK_IFINDEX 1

TEST_F(OffloadUtilsTest, AttachReplaceDetachClsactLo) {
    // This attaches and detaches a configuration-less and thus no-op clsact
    // qdisc to loopback interface (and it takes fractions of a second)
    EXPECT_EQ(0, tcQdiscAddDevClsact(LOOPBACK_IFINDEX));
    EXPECT_EQ(0, tcQdiscReplaceDevClsact(LOOPBACK_IFINDEX));
    EXPECT_EQ(0, tcQdiscDelDevClsact(LOOPBACK_IFINDEX));
    EXPECT_EQ(-EINVAL, tcQdiscDelDevClsact(LOOPBACK_IFINDEX));
}

static void checkAttachDetachBpfFilterClsactLo(const bool ingress, const bool ethernet) {
    // Older kernels return EINVAL instead of ENOENT due to lacking proper error propagation...
    const int errNOENT = android::bpf::isAtLeastKernelVersion(4, 19, 0) ? ENOENT : EINVAL;

    int clatBpfFd = ingress ? getClatIngress6ProgFd(ethernet) : getClatEgress4ProgFd(ethernet);
    ASSERT_GE(clatBpfFd, 3);

    // This attaches and detaches a clsact plus ebpf program to loopback
    // interface, but it should not affect traffic by virtue of us not
    // actually populating the ebpf control map.
    // Furthermore: it only takes fractions of a second.
    EXPECT_EQ(-EINVAL, tcFilterDelDevIngressClatIpv6(LOOPBACK_IFINDEX));
    EXPECT_EQ(-EINVAL, tcFilterDelDevEgressClatIpv4(LOOPBACK_IFINDEX));
    EXPECT_EQ(0, tcQdiscAddDevClsact(LOOPBACK_IFINDEX));
    EXPECT_EQ(-errNOENT, tcFilterDelDevIngressClatIpv6(LOOPBACK_IFINDEX));
    EXPECT_EQ(-errNOENT, tcFilterDelDevEgressClatIpv4(LOOPBACK_IFINDEX));
    if (ingress) {
        EXPECT_EQ(0, tcFilterAddDevIngressClatIpv6(LOOPBACK_IFINDEX, clatBpfFd, ethernet));
        EXPECT_EQ(0, tcFilterDelDevIngressClatIpv6(LOOPBACK_IFINDEX));
    } else {
        EXPECT_EQ(0, tcFilterAddDevEgressClatIpv4(LOOPBACK_IFINDEX, clatBpfFd, ethernet));
        EXPECT_EQ(0, tcFilterDelDevEgressClatIpv4(LOOPBACK_IFINDEX));
    }
    EXPECT_EQ(-errNOENT, tcFilterDelDevIngressClatIpv6(LOOPBACK_IFINDEX));
    EXPECT_EQ(-errNOENT, tcFilterDelDevEgressClatIpv4(LOOPBACK_IFINDEX));
    EXPECT_EQ(0, tcQdiscDelDevClsact(LOOPBACK_IFINDEX));
    EXPECT_EQ(-EINVAL, tcFilterDelDevIngressClatIpv6(LOOPBACK_IFINDEX));
    EXPECT_EQ(-EINVAL, tcFilterDelDevEgressClatIpv4(LOOPBACK_IFINDEX));

    close(clatBpfFd);
}

TEST_F(OffloadUtilsTest, CheckAttachBpfFilterRawIpClsactEgressLo) {
    checkAttachDetachBpfFilterClsactLo(EGRESS, RAWIP);
}

TEST_F(OffloadUtilsTest, CheckAttachBpfFilterEthernetClsactEgressLo) {
    checkAttachDetachBpfFilterClsactLo(EGRESS, ETHER);
}

TEST_F(OffloadUtilsTest, CheckAttachBpfFilterRawIpClsactIngressLo) {
    checkAttachDetachBpfFilterClsactLo(INGRESS, RAWIP);
}

TEST_F(OffloadUtilsTest, CheckAttachBpfFilterEthernetClsactIngressLo) {
    checkAttachDetachBpfFilterClsactLo(INGRESS, ETHER);
}

}  // namespace net
}  // namespace android
