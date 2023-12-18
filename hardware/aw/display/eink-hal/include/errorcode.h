/*
 * Copyright (C) 2016 The Android Open Source Project
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

#ifndef __HWC2_ERROR_H__
#define __HWC2_ERROR_H__

// the reason why we cannot take with hardware composition
enum FallbackErrorCode {
    eHardwareLayer = 0,
    eForceGpuComposition,
    eNoFreeChannel,
    eScaleError,
    eMemoryLimit,
    eCoverByClientTarget,
    eNullBufferHandle,
    eSkipBySurfaceFlinger,
    eClientTarget,
    eNotSupportFormat,
    eAfbcBuffer,
    eNonPhysicalContinuousMemory,
    eTransformError,
};

static inline const char* getErrorCodeString(int error) {
    switch (error) {
        case eHardwareLayer: return "hardware layer";
        case eForceGpuComposition: return "force gpu composition";
        case eNoFreeChannel: return "no free channel";
        case eScaleError: return "scale error";
        case eMemoryLimit: return "bandwidth limit";
        case eCoverByClientTarget: return "cover by client target";
        case eNullBufferHandle: return "bufferhandle is null";
        case eSkipBySurfaceFlinger: return "skip by surfaceflinger";
        case eClientTarget: return "client target";
        case eNotSupportFormat: return "format not supported";
        case eAfbcBuffer: return "AFBC not supported";
        case eNonPhysicalContinuousMemory: return "non physical continuous memory";
        case eTransformError: return "transform not supported";
        default: return "unknown";
    }
}

#endif

