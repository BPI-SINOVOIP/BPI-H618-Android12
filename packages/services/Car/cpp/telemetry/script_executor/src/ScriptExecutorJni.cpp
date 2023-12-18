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

#include "JniUtils.h"
#include "LuaEngine.h"
#include "ScriptExecutorListener.h"
#include "jni.h"

#include <android-base/logging.h>

#include <cstdint>

namespace android {
namespace automotive {
namespace telemetry {
namespace script_executor {

extern "C" {

JNIEXPORT jlong JNICALL
Java_com_android_car_telemetry_ScriptExecutor_nativeInitLuaEngine(JNIEnv* env, jobject object) {
    // Cast first to intptr_t to ensure int can hold the pointer without loss.
    return static_cast<jlong>(reinterpret_cast<intptr_t>(new LuaEngine()));
}

JNIEXPORT void JNICALL Java_com_android_car_telemetry_ScriptExecutor_nativeDestroyLuaEngine(
        JNIEnv* env, jobject object, jlong luaEnginePtr) {
    delete reinterpret_cast<LuaEngine*>(static_cast<intptr_t>(luaEnginePtr));
}

// Parses the inputs and loads them to Lua one at a time.
// Loading of data into Lua also triggers checks on Lua side to verify the
// inputs are valid. For example, pushing "functionName" into Lua stack verifies
// that the function name actually exists in the previously loaded body of the
// script.
//
// The steps are:
// Step 1: Parse the inputs for obvious programming errors.
// Step 2: Parse and load the body of the script.
// Step 3: Parse and push function name we want to execute in the provided
// script body to Lua stack. If the function name doesn't exist, we exit.
// Step 4: Parse publishedData, convert it into Lua table and push it to the
// stack.
// Step 5: Parse savedState Bundle object, convert it into Lua table and push it
// to the stack.
// Any errors that occur at the stage above result in quick exit or crash.
//
// All interaction with Lua happens via Lua stack. Therefore, order of how the
// inputs are parsed and processed is critical because Lua API methods such as
// lua_pcall assume specific order between function name and the input arguments
// on the stack.
// More information about how to work with Lua stack: https://www.lua.org/pil/24.2.html
// and how Lua functions are called via Lua API: https://www.lua.org/pil/25.2.html
//
// Finally, once parsing and pushing to Lua stack is complete, we do
//
// Step 6: attempt to run the provided function.
JNIEXPORT void JNICALL Java_com_android_car_telemetry_ScriptExecutor_nativeInvokeScript(
        JNIEnv* env, jobject object, jlong luaEnginePtr, jstring scriptBody, jstring functionName,
        jobject publishedData, jobject savedState, jobject listener) {
    if (!luaEnginePtr) {
        env->FatalError("luaEnginePtr parameter cannot be nil");
    }
    if (scriptBody == nullptr) {
        env->FatalError("scriptBody parameter cannot be null");
    }
    if (functionName == nullptr) {
        env->FatalError("functionName parameter cannot be null");
    }
    if (listener == nullptr) {
        env->FatalError("listener parameter cannot be null");
    }

    LuaEngine* engine = reinterpret_cast<LuaEngine*>(static_cast<intptr_t>(luaEnginePtr));

    // Load and parse the script
    const char* scriptStr = env->GetStringUTFChars(scriptBody, nullptr);
    auto status = engine->LoadScript(scriptStr);
    env->ReleaseStringUTFChars(scriptBody, scriptStr);
    // status == 0 if the script loads successfully.
    if (status) {
        env->ThrowNew(env->FindClass("java/lang/IllegalArgumentException"),
                      "Failed to load the script.");
        return;
    }
    engine->ResetListener(new ScriptExecutorListener(env, listener));

    // Push the function name we want to invoke to Lua stack
    const char* functionNameStr = env->GetStringUTFChars(functionName, nullptr);
    status = engine->PushFunction(functionNameStr);
    env->ReleaseStringUTFChars(functionName, functionNameStr);
    // status == 1 if the name is indeed a function.
    if (!status) {
        env->ThrowNew(env->FindClass("java/lang/IllegalArgumentException"),
                      "symbol functionName does not correspond to a function.");
        return;
    }

    // TODO(b/189241508): Provide implementation to parse publishedData input,
    // convert it into Lua table and push into Lua stack.
    if (publishedData) {
        env->ThrowNew(env->FindClass("java/lang/RuntimeException"),
                      "Parsing of publishedData is not implemented yet.");
        return;
    }

    // Unpack bundle in savedState, convert to Lua table and push it to Lua
    // stack.
    PushBundleToLuaTable(env, engine, savedState);

    // Execute the function. This will block until complete or error.
    if (engine->Run()) {
        env->ThrowNew(env->FindClass("java/lang/RuntimeException"),
                      "Runtime error occurred while running the function.");
        return;
    }
}

}  // extern "C"

}  // namespace script_executor
}  // namespace telemetry
}  // namespace automotive
}  // namespace android
