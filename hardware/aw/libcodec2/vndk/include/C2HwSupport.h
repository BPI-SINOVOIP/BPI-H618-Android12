/*
 * Copyright (C) 2021 by Allwinnertech Co., Ltd.
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

#ifndef VNDK_INCLUDE_C2HWSUPPORT_H__
#define VNDK_INCLUDE_C2HWSUPPORT_H__

#include <C2Component.h>
#include <C2ComponentFactory.h>

#include <memory>

namespace android {

/**
 * Returns the allwinner codec2 store.
 */
std::shared_ptr<C2ComponentStore> GetCodec2HwComponentStore();
}  // namespace android

#endif  // VNDK_INCLUDE_C2HWSUPPORT_H__
