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

#include "minikin/Buffer.h"

#include <gtest/gtest.h>

namespace minikin {

class TestObject {
public:
    void writeTo(BufferWriter* writer) const {
        // Total size = 20
        // uint8_t (1)
        writer->write<uint8_t>(0xAB);
        // padding (1), uint16_t (2)
        writer->write<uint16_t>(0xCDEF);
        // uint8_t(1)
        writer->write<uint8_t>(0x01);
        // padding (3), array size (4), uint32_t (4) * 2
        uint32_t uint32Array[] = {0x98765432, 0x98765433};
        writer->writeArray<uint32_t>(uint32Array, 2);
    }
};

TEST(BufferTest, testMeasureWriteRead) {
    TestObject testObject;
    BufferWriter fakeWriter(nullptr);
    testObject.writeTo(&fakeWriter);
    ASSERT_EQ(fakeWriter.size(), 20u);
    std::vector<uint8_t> buffer(fakeWriter.size());

    BufferWriter writer(buffer.data());
    testObject.writeTo(&writer);
    ASSERT_EQ(writer.size(), buffer.size());

    BufferReader reader(buffer.data());
    ASSERT_EQ(reader.data(), buffer.data());
    ASSERT_EQ(reader.pos(), 0u);
    ASSERT_EQ(reader.read<uint8_t>(), 0xABu);
    ASSERT_EQ(reader.pos(), 1u);
    ASSERT_EQ(reader.read<uint16_t>(), 0xCDEFu);
    ASSERT_EQ(reader.pos(), 4u);
    ASSERT_EQ(reader.read<uint8_t>(), 0x01u);
    ASSERT_EQ(reader.pos(), 5u);
    auto [uint32Array, size] = reader.readArray<uint32_t>();
    ASSERT_EQ(size, 2u);
    ASSERT_EQ(uint32Array[0], 0x98765432u);
    ASSERT_EQ(uint32Array[1], 0x98765433u);
    ASSERT_EQ(reader.pos(), 20u);
}

TEST(BufferTest, testSkip) {
    TestObject testObject;
    BufferWriter fakeWriter(nullptr);
    testObject.writeTo(&fakeWriter);
    ASSERT_EQ(fakeWriter.size(), 20u);
    std::vector<uint8_t> buffer(fakeWriter.size());

    BufferWriter writer(buffer.data());
    testObject.writeTo(&writer);
    ASSERT_EQ(writer.size(), buffer.size());

    BufferReader reader(buffer.data());
    ASSERT_EQ(reader.data(), buffer.data());
    ASSERT_EQ(reader.pos(), 0u);
    reader.skip<uint8_t>();
    ASSERT_EQ(reader.pos(), 1u);
    reader.read<uint16_t>();
    ASSERT_EQ(reader.pos(), 4u);
    reader.skip<uint8_t>();
    ASSERT_EQ(reader.pos(), 5u);
    reader.skipArray<uint32_t>();
    ASSERT_EQ(reader.pos(), 20u);
}

}  // namespace minikin
