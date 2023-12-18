/*
 * Copyright 2016 The Android Open Source Project
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
 * FirewallControllerTest.cpp - unit tests for FirewallController.cpp
 */

#include <string>
#include <vector>
#include <stdio.h>

#include <gtest/gtest.h>

#include <android-base/file.h>
#include <android-base/stringprintf.h>
#include <android-base/strings.h>

#include "FirewallController.h"
#include "IptablesBaseTest.h"

using android::base::Join;
using android::base::WriteStringToFile;

namespace android {
namespace net {

class FirewallControllerTest : public IptablesBaseTest {
protected:
    FirewallControllerTest() {
        FirewallController::execIptablesRestore = fakeExecIptablesRestore;
        // This unit test currently doesn't cover the eBPF owner match case so
        // we have to manually turn eBPF support off.
        // TODO: find a way to unit test the eBPF code path.
        mFw.mUseBpfOwnerMatch = false;
    }
    FirewallController mFw;

    std::string makeUidRules(IptablesTarget a, const char* b, bool c,
                             const std::vector<int32_t>& d) {
        return mFw.makeUidRules(a, b, c, d);
    }

    int createChain(const char* a, FirewallType b) {
        return mFw.createChain(a, b);
    }
};

TEST_F(FirewallControllerTest, TestCreateAllowlistChain) {
    std::vector<std::string> expectedRestore4 = {
            "*filter",
            ":fw_allowlist -",
            "-A fw_allowlist -m owner --uid-owner 0-9999 -j RETURN",
            "-A fw_allowlist -m owner ! --uid-owner 0-4294967294 -j RETURN",
            "-A fw_allowlist -p esp -j RETURN",
            "-A fw_allowlist -i lo -j RETURN",
            "-A fw_allowlist -o lo -j RETURN",
            "-A fw_allowlist -p tcp --tcp-flags RST RST -j RETURN",
            "-A fw_allowlist -j DROP",
            "COMMIT\n"};
    std::vector<std::string> expectedRestore6 = {
            "*filter",
            ":fw_allowlist -",
            "-A fw_allowlist -m owner --uid-owner 0-9999 -j RETURN",
            "-A fw_allowlist -m owner ! --uid-owner 0-4294967294 -j RETURN",
            "-A fw_allowlist -p esp -j RETURN",
            "-A fw_allowlist -i lo -j RETURN",
            "-A fw_allowlist -o lo -j RETURN",
            "-A fw_allowlist -p tcp --tcp-flags RST RST -j RETURN",
            "-A fw_allowlist -p icmpv6 --icmpv6-type packet-too-big -j RETURN",
            "-A fw_allowlist -p icmpv6 --icmpv6-type router-solicitation -j RETURN",
            "-A fw_allowlist -p icmpv6 --icmpv6-type router-advertisement -j RETURN",
            "-A fw_allowlist -p icmpv6 --icmpv6-type neighbour-solicitation -j RETURN",
            "-A fw_allowlist -p icmpv6 --icmpv6-type neighbour-advertisement -j RETURN",
            "-A fw_allowlist -p icmpv6 --icmpv6-type redirect -j RETURN",
            "-A fw_allowlist -j DROP",
            "COMMIT\n"};
    std::vector<std::pair<IptablesTarget, std::string>> expectedRestoreCommands = {
            {V4, Join(expectedRestore4, '\n')},
            {V6, Join(expectedRestore6, '\n')},
    };

    createChain("fw_allowlist", ALLOWLIST);
    expectIptablesRestoreCommands(expectedRestoreCommands);
}

TEST_F(FirewallControllerTest, TestCreateDenylistChain) {
    std::vector<std::string> expectedRestore = {
            "*filter",
            ":fw_denylist -",
            "-A fw_denylist -i lo -j RETURN",
            "-A fw_denylist -o lo -j RETURN",
            "-A fw_denylist -p tcp --tcp-flags RST RST -j RETURN",
            "COMMIT\n"};
    std::vector<std::pair<IptablesTarget, std::string>> expectedRestoreCommands = {
            {V4, Join(expectedRestore, '\n')},
            {V6, Join(expectedRestore, '\n')},
    };

    createChain("fw_denylist", DENYLIST);
    expectIptablesRestoreCommands(expectedRestoreCommands);
}

TEST_F(FirewallControllerTest, TestSetStandbyRule) {
    ExpectedIptablesCommands expected = {
        { V4V6, "*filter\n-D fw_standby -m owner --uid-owner 12345 -j DROP\nCOMMIT\n" }
    };
    mFw.setUidRule(STANDBY, 12345, ALLOW);
    expectIptablesRestoreCommands(expected);

    expected = {
        { V4V6, "*filter\n-A fw_standby -m owner --uid-owner 12345 -j DROP\nCOMMIT\n" }
    };
    mFw.setUidRule(STANDBY, 12345, DENY);
    expectIptablesRestoreCommands(expected);
}

TEST_F(FirewallControllerTest, TestSetDozeRule) {
    ExpectedIptablesCommands expected = {
        { V4V6, "*filter\n-I fw_dozable -m owner --uid-owner 54321 -j RETURN\nCOMMIT\n" }
    };
    mFw.setUidRule(DOZABLE, 54321, ALLOW);
    expectIptablesRestoreCommands(expected);

    expected = {
        { V4V6, "*filter\n-D fw_dozable -m owner --uid-owner 54321 -j RETURN\nCOMMIT\n" }
    };
    mFw.setUidRule(DOZABLE, 54321, DENY);
    expectIptablesRestoreCommands(expected);
}

TEST_F(FirewallControllerTest, TestSetFirewallRule) {
    ExpectedIptablesCommands expected = {
        { V4V6, "*filter\n"
                "-A fw_INPUT -m owner --uid-owner 54321 -j DROP\n"
                "-A fw_OUTPUT -m owner --uid-owner 54321 -j DROP\n"
                "COMMIT\n" }
    };
    mFw.setUidRule(NONE, 54321, DENY);
    expectIptablesRestoreCommands(expected);

    expected = {
        { V4V6, "*filter\n"
                "-D fw_INPUT -m owner --uid-owner 54321 -j DROP\n"
                "-D fw_OUTPUT -m owner --uid-owner 54321 -j DROP\n"
                "COMMIT\n" }
    };
    mFw.setUidRule(NONE, 54321, ALLOW);
    expectIptablesRestoreCommands(expected);
}

TEST_F(FirewallControllerTest, TestReplaceAllowlistUidRule) {
    std::string expected =
            "*filter\n"
            ":FW_allowchain -\n"
            "-A FW_allowchain -m owner --uid-owner 10023 -j RETURN\n"
            "-A FW_allowchain -m owner --uid-owner 10059 -j RETURN\n"
            "-A FW_allowchain -m owner --uid-owner 10124 -j RETURN\n"
            "-A FW_allowchain -m owner --uid-owner 10111 -j RETURN\n"
            "-A FW_allowchain -m owner --uid-owner 110122 -j RETURN\n"
            "-A FW_allowchain -m owner --uid-owner 210153 -j RETURN\n"
            "-A FW_allowchain -m owner --uid-owner 210024 -j RETURN\n"
            "-A FW_allowchain -m owner --uid-owner 0-9999 -j RETURN\n"
            "-A FW_allowchain -m owner ! --uid-owner 0-4294967294 -j RETURN\n"
            "-A FW_allowchain -p esp -j RETURN\n"
            "-A FW_allowchain -i lo -j RETURN\n"
            "-A FW_allowchain -o lo -j RETURN\n"
            "-A FW_allowchain -p tcp --tcp-flags RST RST -j RETURN\n"
            "-A FW_allowchain -p icmpv6 --icmpv6-type packet-too-big -j RETURN\n"
            "-A FW_allowchain -p icmpv6 --icmpv6-type router-solicitation -j RETURN\n"
            "-A FW_allowchain -p icmpv6 --icmpv6-type router-advertisement -j RETURN\n"
            "-A FW_allowchain -p icmpv6 --icmpv6-type neighbour-solicitation -j RETURN\n"
            "-A FW_allowchain -p icmpv6 --icmpv6-type neighbour-advertisement -j RETURN\n"
            "-A FW_allowchain -p icmpv6 --icmpv6-type redirect -j RETURN\n"
            "-A FW_allowchain -j DROP\n"
            "COMMIT\n";

    std::vector<int32_t> uids = { 10023, 10059, 10124, 10111, 110122, 210153, 210024 };
    EXPECT_EQ(expected, makeUidRules(V6, "FW_allowchain", true, uids));
}

TEST_F(FirewallControllerTest, TestReplaceDenylistUidRule) {
    std::string expected =
            "*filter\n"
            ":FW_denychain -\n"
            "-A FW_denychain -i lo -j RETURN\n"
            "-A FW_denychain -o lo -j RETURN\n"
            "-A FW_denychain -p tcp --tcp-flags RST RST -j RETURN\n"
            "-A FW_denychain -m owner --uid-owner 10023 -j DROP\n"
            "-A FW_denychain -m owner --uid-owner 10059 -j DROP\n"
            "-A FW_denychain -m owner --uid-owner 10124 -j DROP\n"
            "COMMIT\n";

    std::vector<int32_t> uids = { 10023, 10059, 10124 };
    EXPECT_EQ(expected, makeUidRules(V4, "FW_denychain", false, uids));
}

TEST_F(FirewallControllerTest, TestEnableChildChains) {
    std::vector<std::string> expected = {
        "*filter\n"
        "-A fw_INPUT -j fw_dozable\n"
        "-A fw_OUTPUT -j fw_dozable\n"
        "COMMIT\n"
    };
    EXPECT_EQ(0, mFw.enableChildChains(DOZABLE, true));
    expectIptablesRestoreCommands(expected);

    expected = {
        "*filter\n"
        "-D fw_INPUT -j fw_powersave\n"
        "-D fw_OUTPUT -j fw_powersave\n"
        "COMMIT\n"
    };
    EXPECT_EQ(0, mFw.enableChildChains(POWERSAVE, false));
    expectIptablesRestoreCommands(expected);
}

TEST_F(FirewallControllerTest, TestFirewall) {
    std::vector<std::string> enableCommands = {
        "*filter\n"
        "-A fw_INPUT -j DROP\n"
        "-A fw_OUTPUT -j REJECT\n"
        "-A fw_FORWARD -j REJECT\n"
        "COMMIT\n"
    };
    std::vector<std::string> disableCommands = {
        "*filter\n"
        ":fw_INPUT -\n"
        ":fw_OUTPUT -\n"
        ":fw_FORWARD -\n"
        "COMMIT\n"
    };
    std::vector<std::string> noCommands = {};

    EXPECT_EQ(0, mFw.resetFirewall());
    expectIptablesRestoreCommands(disableCommands);

    EXPECT_EQ(0, mFw.resetFirewall());
    expectIptablesRestoreCommands(disableCommands);

    EXPECT_EQ(0, mFw.setFirewallType(DENYLIST));
    expectIptablesRestoreCommands(disableCommands);

    EXPECT_EQ(0, mFw.setFirewallType(DENYLIST));
    expectIptablesRestoreCommands(noCommands);

    std::vector<std::string> disableEnableCommands;
    disableEnableCommands.insert(
            disableEnableCommands.end(), disableCommands.begin(), disableCommands.end());
    disableEnableCommands.insert(
            disableEnableCommands.end(), enableCommands.begin(), enableCommands.end());

    EXPECT_EQ(0, mFw.setFirewallType(ALLOWLIST));
    expectIptablesRestoreCommands(disableEnableCommands);

    std::vector<std::string> ifaceCommands = {
        "*filter\n"
        "-I fw_INPUT -i rmnet_data0 -j RETURN\n"
        "-I fw_OUTPUT -o rmnet_data0 -j RETURN\n"
        "COMMIT\n"
    };
    EXPECT_EQ(0, mFw.setInterfaceRule("rmnet_data0", ALLOW));
    expectIptablesRestoreCommands(ifaceCommands);

    EXPECT_EQ(0, mFw.setInterfaceRule("rmnet_data0", ALLOW));
    expectIptablesRestoreCommands(noCommands);

    ifaceCommands = {
        "*filter\n"
        "-D fw_INPUT -i rmnet_data0 -j RETURN\n"
        "-D fw_OUTPUT -o rmnet_data0 -j RETURN\n"
        "COMMIT\n"
    };
    EXPECT_EQ(0, mFw.setInterfaceRule("rmnet_data0", DENY));
    expectIptablesRestoreCommands(ifaceCommands);

    EXPECT_EQ(0, mFw.setInterfaceRule("rmnet_data0", DENY));
    expectIptablesRestoreCommands(noCommands);

    EXPECT_EQ(0, mFw.setFirewallType(ALLOWLIST));
    expectIptablesRestoreCommands(noCommands);

    EXPECT_EQ(0, mFw.resetFirewall());
    expectIptablesRestoreCommands(disableCommands);

    // TODO: calling resetFirewall and then setFirewallType(ALLOWLIST) does
    // nothing. This seems like a clear bug.
    EXPECT_EQ(0, mFw.setFirewallType(ALLOWLIST));
    expectIptablesRestoreCommands(noCommands);
}

TEST_F(FirewallControllerTest, TestDiscoverMaximumValidUid) {
    struct {
        const std::string description;
        const std::string content;
        const uint32_t expected;
    } testCases[] = {
            {
                    .description = "root namespace case",
                    .content = "         0          0 4294967295",
                    .expected = 4294967294,
            },
            {
                    .description = "container namespace case",
                    .content = "         0     655360       5000\n"
                               "      5000        600         50\n"
                               "      5050     660410    1994950\n",
                    .expected = 1999999,
            },
            {
                    .description = "garbage content case",
                    .content = "garbage",
                    .expected = 4294967294,
            },
            {
                    .description = "no content case",
                    .content = "",
                    .expected = 4294967294,
            },
    };

    const std::string tempFile = "/data/local/tmp/fake_uid_mapping";

    for (const auto& test : testCases) {
        EXPECT_TRUE(WriteStringToFile(test.content, tempFile, false));
        uint32_t got = FirewallController::discoverMaximumValidUid(tempFile);
        EXPECT_EQ(0, remove(tempFile.c_str()));
        if (got != test.expected) {
            FAIL() << test.description << ":\n"
                   << test.content << "\ngot " << got << ", but expected " << test.expected;
        }
    }

    // Also check when the file is not defined
    EXPECT_NE(0, access(tempFile.c_str(), F_OK));
    EXPECT_EQ(4294967294, FirewallController::discoverMaximumValidUid(tempFile));
}

}  // namespace net
}  // namespace android
