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

#include "doh.h"

#include <gmock/gmock-matchers.h>
#include <gtest/gtest.h>

TEST(DoHFFITest, SmokeTest) {
    EXPECT_STREQ(doh_init(), "1.0");
    DohServer* doh = doh_new("https://dns.google/dns-query", "8.8.8.8", 0, "");
    EXPECT_TRUE(doh != nullptr);

    // www.example.com
    uint8_t query[] = "q80BAAABAAAAAAAAA3d3dwdleGFtcGxlA2NvbQAAAQAB";
    uint8_t answer[8192];
    ssize_t len = doh_query(doh, query, sizeof query, answer, sizeof answer);
    EXPECT_GT(len, 0);
    doh_delete(doh);
}
