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

#ifndef MINIKIN_BUFFER_H
#define MINIKIN_BUFFER_H

#include <cstring>
#include <string_view>
#include <type_traits>
#include <utility>

namespace minikin {

// This is a helper class to read data from a memory buffer.
// This class does not copy memory, and may return pointers to parts of the memory buffer.
// Thus the memory buffer should outlive objects created using this class.
class BufferReader {
public:
    BufferReader(const void* buffer) : BufferReader(buffer, 0) {}
    BufferReader(const void* buffer, uint32_t pos)
            : mData(reinterpret_cast<const uint8_t*>(buffer)), mPos(pos) {}

    template <typename T>
    static uint32_t align(uint32_t pos) {
        // This should be true for all types, unless custom alignment attributes are set.
        static_assert(sizeof(T) % alignof(T) == 0, "sizeof(T) must be a multiple of alignof(T)");
        // We align to sizeof(T) instead of alignof(T), because the buffer may be shared between
        // 32-bit processes and 64-bit processes. alignof(T) may change between the two.
        // We assume that T is a type whose size is fixed (e.g. uint32_t).
        return (pos + sizeof(T) - 1) / sizeof(T) * sizeof(T);
    }

    template <typename T>
    const T& read() {
        static_assert(std::is_pod<T>::value, "T must be a POD");
        mPos = BufferReader::align<T>(mPos);
        const T* data = reinterpret_cast<const T*>(mData + mPos);
        mPos += sizeof(T);
        return *data;
    }

    template <typename T>
    void skip() {
        static_assert(std::is_pod<T>::value, "T must be a POD");
        mPos = BufferReader::align<T>(mPos);
        mPos += sizeof(T);
    }

    // Return a pointer to an array and its number of elements.
    template <typename T>
    std::pair<const T*, uint32_t> readArray() {
        static_assert(std::is_pod<T>::value, "T must be a POD");
        uint32_t size = read<uint32_t>();
        mPos = BufferReader::align<T>(mPos);
        const T* data = reinterpret_cast<const T*>(mData + mPos);
        mPos += size * sizeof(T);
        return std::make_pair(data, size);
    }

    template <typename T>
    void skipArray() {
        static_assert(std::is_pod<T>::value, "T must be a POD");
        uint32_t size = read<uint32_t>();
        mPos = BufferReader::align<T>(mPos);
        mPos += size * sizeof(T);
    }

    std::string_view readString() {
        auto [data, size] = readArray<char>();
        return std::string_view(data, size);
    }

    void skipString() { skipArray<char>(); }

    const void* data() const { return mData; }
    size_t pos() const { return mPos; }

private:
    const uint8_t* mData;
    size_t mPos;
};

// This is a helper class to write data to a memory buffer.
class BufferWriter {
public:
    // Create a buffer writer. Passing nullptr creates a fake writer,
    // which can be used to measure the buffer size needed.
    BufferWriter(void* buffer) : mData(reinterpret_cast<uint8_t*>(buffer)), mPos(0) {}

    BufferWriter(BufferWriter&&) = default;
    BufferWriter& operator=(BufferWriter&&) = default;

    // Write a single data of type T.
    // Please always specify T explicitly using <>. std::common_type_t<T> resolves to T, but
    // disables template argument deduction.
    // TODO: use std::type_identity_t when C++20 is available.
    template <typename T>
    void write(const std::common_type_t<T>& data) {
        static_assert(std::is_pod<T>::value, "T must be a POD");
        mPos = BufferReader::align<T>(mPos);
        if (mData != nullptr) {
            memcpy(mData + mPos, &data, sizeof(T));
        }
        mPos += sizeof(T);
    }

    // Write an array of type T.
    // Please always specify T explicitly using <>. std::common_type_t<T> resolves to T, but
    // disables template argument deduction.
    // TODO: use std::type_identity_t when C++20 is available.
    template <typename T>
    void writeArray(const std::common_type_t<T>* data, uint32_t size) {
        static_assert(std::is_pod<T>::value, "T must be a POD");
        write<uint32_t>(size);
        mPos = BufferReader::align<T>(mPos);
        if (mData != nullptr) {
            memcpy(mData + mPos, data, size * sizeof(T));
        }
        mPos += size * sizeof(T);
    }

    void writeString(std::string_view string) { writeArray<char>(string.data(), string.size()); }

    // Return the number of bytes written.
    size_t size() const { return mPos; }

private:
    uint8_t* mData;
    size_t mPos;

    // Forbid copy and assign.
    BufferWriter(const BufferWriter&) = delete;
    void operator=(const BufferWriter&) = delete;
};

}  // namespace minikin

#endif  // MINIKIN_BUFFER_H
