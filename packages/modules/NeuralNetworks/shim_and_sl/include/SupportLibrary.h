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

#ifndef ANDROID_PACKAGES_MODULES_NEURALNETWORKS_SL_SUPPORT_LIBRARY_H
#define ANDROID_PACKAGES_MODULES_NEURALNETWORKS_SL_SUPPORT_LIBRARY_H

#include <memory>
#include <string>

#include "NeuralNetworksSupportLibraryImpl.h"
#include "NeuralNetworksTypes.h"

#ifndef __NNAPI_FL5_MIN_ANDROID_API__
#define __NNAPI_FL5_MIN_ANDROID_API__ __ANDROID_API_S__
#endif

/**
 * Helper struct, derived from the latest NnApiSLDriverImpl.
 *
 * Owns the .so handle, and will close it in destructor.
 * Sets proper implStructFeatureLevel in constructor.
 *
 * It's derived from the latest NnApiSLDriverImplFL* struct,
 * so it contains all possible functionality.
 *
 * When a new NnApiSLDriverImpl is introduced, this class
 * has to switch base class to it and provide constructors for
 * all existing NnApiSLDriverImplFL* structs.
 *
 * There's expectation that for M>N, NnApiSLDriverImplFL(M) is
 * a strict superset of NnApiSLDriverImplFL(N), and *NnApiSLDriverImplFL(M) can
 * be reinterpret_cast to *NnApiSLDriverImplFL(N) safely.
 *
 * The base->implFeatureLevel is set to the actual Feature Level
 * implemented by the SLDriverImpl,
 */
struct NnApiSupportLibrary : NnApiSLDriverImplFL5 {
    NnApiSupportLibrary(const NnApiSLDriverImplFL5& impl, void* libHandle);
    ~NnApiSupportLibrary();

    void* libHandle = nullptr;
};

/**
 * Loads the NNAPI support library.
 * The NnApiSupportLibrary structure is filled with all the pointers. If one
 * function doesn't exist, a null pointer is stored.
 */
std::unique_ptr<const NnApiSupportLibrary> loadNnApiSupportLibrary(const std::string& libName);
std::unique_ptr<const NnApiSupportLibrary> loadNnApiSupportLibrary(void* libHandle);

#endif  // ANDROID_PACKAGES_MODULES_NEURALNETWORKS_SL_SUPPORT_LIBRARY_H
