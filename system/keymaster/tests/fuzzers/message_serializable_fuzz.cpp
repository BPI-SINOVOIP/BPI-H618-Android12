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

#include <functional>
#include <memory>

#include <keymaster/serializable.h>

#include "fuzzer/FuzzedDataProvider.h"
#include "serializable_types.h"

static constexpr uint16_t kMinBufferSize = 1;
static constexpr uint16_t kMaxBufferSize = 2048;

void RunDeserialize(keymaster::Serializable* ser, FuzzedDataProvider* fdp) {
    uint16_t buf_size = fdp->ConsumeIntegralInRange<uint16_t>(kMinBufferSize, kMaxBufferSize);
    std::unique_ptr<uint8_t[]> in_buf = std::unique_ptr<uint8_t[]>(new uint8_t[buf_size]);
    const uint8_t* data_ptr = in_buf.get();
    // memset((void*) data_ptr, 0x41, buf_size);
    int32_t end = fdp->ConsumeIntegralInRange<int32_t>(0, buf_size);
    ser->Deserialize(&data_ptr, data_ptr + end);
}

void RunSerialize(keymaster::Serializable* ser, FuzzedDataProvider* fdp) {
    uint16_t buf_size = ser->SerializedSize();
    std::unique_ptr<uint8_t[]> out_buf = std::unique_ptr<uint8_t[]>(new uint8_t[buf_size]);
    // memset((void*) out_buf.get(), 0x41, buf_size);
    int32_t end = fdp->ConsumeIntegralInRange<int32_t>(0, buf_size);
    ser->Serialize(out_buf.get(), out_buf.get() + end);
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    FuzzedDataProvider fdp(data, size);
    std::unique_ptr<keymaster::Serializable> serializable =
        keymaster::getSerializable(fdp.ConsumeEnum<keymaster::SerializableType>());
    /*if(fdp.remaining_bytes() > 1) {
            RunDeserialize(serializable.get(), &fdp);
    }*/
    for (size_t i = 0; fdp.remaining_bytes() > 0; i++) {
        if (fdp.ConsumeBool()) {
            RunSerialize(serializable.get(), &fdp);
        } else {
            RunDeserialize(serializable.get(), &fdp);
        }
    }
    return 0;
}
