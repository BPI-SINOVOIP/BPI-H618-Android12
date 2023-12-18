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

// This file maintain a list of advanced features that can be switched on/off
// with feature control.
//
// The features in this file depend on system image builds. It needs to be
// enabled in BOTH system images and emulator to be actually enabled.
// To add system image independent features, please add them to
// FeatureControlDefHost.h
//
// To add a new item, please add a new line in the following format:
// FEATURE_CONTROL_ITEM(YOUR_FEATURE_NAME)
// You will also need to edit its default value in the following two places:
// android/data/advancedFeatures.ini
// $(system_image)/development/sys-img/advancedFeatures.ini

// This file is supposed to be included multiple times. It should not have
// #pragma once here.

FEATURE_CONTROL_ITEM(GrallocSync)
FEATURE_CONTROL_ITEM(EncryptUserData)
FEATURE_CONTROL_ITEM(IntelPerformanceMonitoringUnit)
FEATURE_CONTROL_ITEM(GLAsyncSwap)
FEATURE_CONTROL_ITEM(GLDMA)
FEATURE_CONTROL_ITEM(GLDMA2)
FEATURE_CONTROL_ITEM(GLDirectMem)
FEATURE_CONTROL_ITEM(GLESDynamicVersion)
FEATURE_CONTROL_ITEM(Wifi)
FEATURE_CONTROL_ITEM(PlayStoreImage)
FEATURE_CONTROL_ITEM(LogcatPipe)
FEATURE_CONTROL_ITEM(SystemAsRoot)
FEATURE_CONTROL_ITEM(KernelDeviceTreeBlobSupport)
FEATURE_CONTROL_ITEM(DynamicPartition)
FEATURE_CONTROL_ITEM(RefCountPipe)
FEATURE_CONTROL_ITEM(HostComposition)
FEATURE_CONTROL_ITEM(WifiConfigurable)
FEATURE_CONTROL_ITEM(VirtioInput)
FEATURE_CONTROL_ITEM(MultiDisplay)
FEATURE_CONTROL_ITEM(VulkanNullOptionalStrings)
FEATURE_CONTROL_ITEM(YUV420888toNV21)
FEATURE_CONTROL_ITEM(YUVCache)
FEATURE_CONTROL_ITEM(KeycodeForwarding)
FEATURE_CONTROL_ITEM(VulkanIgnoredHandles)
FEATURE_CONTROL_ITEM(VirtioGpuNext)
FEATURE_CONTROL_ITEM(Mac80211hwsimUserspaceManaged)
FEATURE_CONTROL_ITEM(HasSharedSlotsHostMemoryAllocator)
FEATURE_CONTROL_ITEM(HardwareDecoder)
FEATURE_CONTROL_ITEM(VirtioWifi)
FEATURE_CONTROL_ITEM(ModemSimulator)
FEATURE_CONTROL_ITEM(VirtioMouse)
FEATURE_CONTROL_ITEM(VirtconsoleLogcat)
FEATURE_CONTROL_ITEM(VulkanQueueSubmitWithCommands)
FEATURE_CONTROL_ITEM(VulkanBatchedDescriptorSetUpdate)
