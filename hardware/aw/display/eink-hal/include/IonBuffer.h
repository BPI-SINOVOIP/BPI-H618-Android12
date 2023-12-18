/*
 * Copyright (C) 2019 The Android Open Source Project
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

#ifndef SUNXI_ION_BUFFER_H
#define SUNXI_ION_BUFFER_H

#include <ion/ion.h>

namespace sunxi {

typedef struct PrivateBuffer {
    int handle;
    uint32_t size;
    uint64_t usage;
	void *virt_addr;
} PrivateBuffer_t;

class IonBuffer : private PrivateBuffer_t {
public:
    // Create a IonBuffer by allocating and managing a buffer internally.
    IonBuffer(uint32_t inSize, uint64_t inUsage);
   ~IonBuffer();
    const PrivateBuffer_t* getPrivateBuffer() const;
    bool isValid() const { return handle != -1; }

private:
    IonBuffer();
    IonBuffer(const IonBuffer& rhs);
    IonBuffer& operator = (const IonBuffer& rhs);
    const IonBuffer& operator = (const IonBuffer& rhs) const;

    int initWithSize(uint32_t inSize, uint64_t inUsage);
};

} // namespace sunxi

#endif
