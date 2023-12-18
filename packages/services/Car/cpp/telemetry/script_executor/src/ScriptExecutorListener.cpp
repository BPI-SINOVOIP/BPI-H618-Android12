/*
 * Copyright (c) 2021, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "ScriptExecutorListener.h"

#include <android-base/logging.h>
#include <android_runtime/AndroidRuntime.h>

namespace android {
namespace automotive {
namespace telemetry {
namespace script_executor {

ScriptExecutorListener::~ScriptExecutorListener() {
    if (mScriptExecutorListener != NULL) {
        JNIEnv* env = AndroidRuntime::getJNIEnv();
        env->DeleteGlobalRef(mScriptExecutorListener);
    }
}

ScriptExecutorListener::ScriptExecutorListener(JNIEnv* env, jobject script_executor_listener) {
    mScriptExecutorListener = env->NewGlobalRef(script_executor_listener);
}

void ScriptExecutorListener::onError(const int errorType, const std::string& message,
                                     const std::string& stackTrace) {
    LOG(INFO) << "errorType: " << errorType << ", message: " << message
              << ", stackTrace: " << stackTrace;
}

}  // namespace script_executor
}  // namespace telemetry
}  // namespace automotive
}  // namespace android
