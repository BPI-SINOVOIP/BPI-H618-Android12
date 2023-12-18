// Copyright 2020 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "host-common/HostmemIdMapping.h"

#include <gtest/gtest.h>

using android::emulation::HostmemIdMapping;

// Tests creation and destruction.
TEST(HostmemIdMapping, Basic) {
    HostmemIdMapping m;
}

// Tests basic operations on an entry: add, remove, get entry info
TEST(HostmemIdMapping, BasicEntry) {
    HostmemIdMapping m;
    auto id = m.add(0, 1);
    EXPECT_EQ(HostmemIdMapping::kInvalidHostmemId, id);
    id = m.add(1, 0);
    EXPECT_EQ(HostmemIdMapping::kInvalidHostmemId, id);
    id = m.add(1, 2);
    EXPECT_NE(HostmemIdMapping::kInvalidHostmemId, id);

    auto entry = m.get(id);
    EXPECT_EQ(id, entry.id);
    EXPECT_EQ(1, entry.hva);
    EXPECT_EQ(2, entry.size);

    m.remove(id);

    entry = m.get(id);

    EXPECT_EQ(HostmemIdMapping::kInvalidHostmemId, entry.id);
    EXPECT_EQ(0, entry.hva);
    EXPECT_EQ(0, entry.size);
}

// Tests the clear() method.
TEST(HostmemIdMapping, Clear) {
    HostmemIdMapping m;
    auto id1 = m.add(1, 2);
    auto id2 = m.add(3, 4);

    m.clear();

    auto entry = m.get(id1);
    EXPECT_EQ(HostmemIdMapping::kInvalidHostmemId, entry.id);
    EXPECT_EQ(0, entry.hva);
    EXPECT_EQ(0, entry.size);

    entry = m.get(id2);
    EXPECT_EQ(HostmemIdMapping::kInvalidHostmemId, entry.id);
    EXPECT_EQ(0, entry.hva);
    EXPECT_EQ(0, entry.size);
}

