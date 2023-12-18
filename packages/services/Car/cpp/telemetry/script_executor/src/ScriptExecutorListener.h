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

#ifndef CPP_TELEMETRY_SCRIPT_EXECUTOR_SRC_SCRIPTEXECUTORLISTENER_H_
#define CPP_TELEMETRY_SCRIPT_EXECUTOR_SRC_SCRIPTEXECUTORLISTENER_H_

#include "jni.h"

#include <string>

namespace android {
namespace automotive {
namespace telemetry {
namespace script_executor {

//  Wrapper class for IScriptExecutorListener.aidl.
class ScriptExecutorListener {
public:
    ScriptExecutorListener(JNIEnv* jni, jobject script_executor_listener);

    virtual ~ScriptExecutorListener();

    void onScriptFinished() {}

    void onSuccess() {}

    void onError(const int errorType, const std::string& message, const std::string& stackTrace);

private:
    // Stores a jni global reference to Java Script Executor listener object.
    jobject mScriptExecutorListener;
};

}  // namespace script_executor
}  // namespace telemetry
}  // namespace automotive
}  // namespace android

#endif  // CPP_TELEMETRY_SCRIPT_EXECUTOR_SRC_SCRIPTEXECUTORLISTENER_H_
