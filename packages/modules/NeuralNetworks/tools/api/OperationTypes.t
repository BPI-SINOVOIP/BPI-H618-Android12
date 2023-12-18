%% template file for generating OperationTypes.h.
%% see README.md.
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

#ifndef ANDROID_FRAMEWORKS_ML_NN_COMMON_NNAPI_OPERATION_TYPES_H
#define ANDROID_FRAMEWORKS_ML_NN_COMMON_NNAPI_OPERATION_TYPES_H

namespace android::nn {

%insert Operation_1.0_Comment
enum class OperationType {
%insert Operation_1.0

%insert Operation_1.1

%insert Operation_1.2

%insert Operation_1.3

    /**
     * DEPRECATED. Since HAL version 1.2, extensions are the preferred
     * alternative to OEM operation and data types.
     *
     * This operation is OEM specific. It should only be used for OEM
     * applications.
     */
    OEM_OPERATION = 10000,
};

}  // namespace android::nn

#endif  // ANDROID_FRAMEWORKS_ML_NN_COMMON_NNAPI_OPERATION_TYPES_H
