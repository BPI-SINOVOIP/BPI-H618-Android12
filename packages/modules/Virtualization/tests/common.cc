/*
 * Copyright (C) 2021 The Android Open Source Project
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

#include "virt/VirtualizationTest.h"

namespace virt {

void VirtualizationTest::SetUp() {
    status_t err = getService<IVirtManager>(String16("android.system.virtmanager"), &mVirtManager);
    ASSERT_EQ(err, 0);
}

} // namespace virt
