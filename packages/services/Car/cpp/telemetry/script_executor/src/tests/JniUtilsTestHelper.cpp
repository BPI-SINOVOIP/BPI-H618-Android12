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
#include "jni.h"

#include <cstdint>
#include <cstring>

namespace android {
namespace automotive {
namespace telemetry {
namespace script_executor {
namespace {

extern "C" {

#include "lua.h"

JNIEXPORT jlong JNICALL
Java_com_android_car_telemetry_JniUtilsTest_nativeCreateLuaEngine(JNIEnv* env, jobject object) {
    // Cast first to intptr_t to ensure int can hold the pointer without loss.
    return static_cast<jlong>(reinterpret_cast<intptr_t>(new LuaEngine()));
}

JNIEXPORT void JNICALL Java_com_android_car_telemetry_JniUtilsTest_nativeDestroyLuaEngine(
        JNIEnv* env, jobject object, jlong luaEnginePtr) {
    delete reinterpret_cast<LuaEngine*>(static_cast<intptr_t>(luaEnginePtr));
}

JNIEXPORT void JNICALL Java_com_android_car_telemetry_JniUtilsTest_nativePushBundleToLuaTableCaller(
        JNIEnv* env, jobject object, jlong luaEnginePtr, jobject bundle) {
    PushBundleToLuaTable(env, reinterpret_cast<LuaEngine*>(static_cast<intptr_t>(luaEnginePtr)),
                         bundle);
}

JNIEXPORT jint JNICALL Java_com_android_car_telemetry_JniUtilsTest_nativeGetObjectSize(
        JNIEnv* env, jobject object, jlong luaEnginePtr, jint index) {
    LuaEngine* engine = reinterpret_cast<LuaEngine*>(static_cast<intptr_t>(luaEnginePtr));
    return lua_rawlen(engine->GetLuaState(), static_cast<int>(index));
}

JNIEXPORT bool JNICALL Java_com_android_car_telemetry_JniUtilsTest_nativeHasBooleanValue(
        JNIEnv* env, jobject object, jlong luaEnginePtr, jstring key, jboolean value) {
    const char* rawKey = env->GetStringUTFChars(key, nullptr);
    LuaEngine* engine = reinterpret_cast<LuaEngine*>(static_cast<intptr_t>(luaEnginePtr));
    auto* luaState = engine->GetLuaState();
    lua_pushstring(luaState, rawKey);
    env->ReleaseStringUTFChars(key, rawKey);
    lua_gettable(luaState, -2);
    bool result = false;
    if (!lua_isboolean(luaState, -1))
        result = false;
    else
        result = static_cast<bool>(lua_toboolean(luaState, -1)) == static_cast<bool>(value);
    lua_pop(luaState, 1);
    return result;
}

JNIEXPORT bool JNICALL Java_com_android_car_telemetry_JniUtilsTest_nativeHasIntValue(
        JNIEnv* env, jobject object, jlong luaEnginePtr, jstring key, jint value) {
    const char* rawKey = env->GetStringUTFChars(key, nullptr);
    LuaEngine* engine = reinterpret_cast<LuaEngine*>(static_cast<intptr_t>(luaEnginePtr));
    // Assumes the table is on top of the stack.
    auto* luaState = engine->GetLuaState();
    lua_pushstring(luaState, rawKey);
    env->ReleaseStringUTFChars(key, rawKey);
    lua_gettable(luaState, -2);
    bool result = false;
    if (!lua_isinteger(luaState, -1))
        result = false;
    else
        result = lua_tointeger(luaState, -1) == static_cast<int>(value);
    lua_pop(luaState, 1);
    return result;
}

JNIEXPORT bool JNICALL Java_com_android_car_telemetry_JniUtilsTest_nativeHasDoubleValue(
        JNIEnv* env, jobject object, jlong luaEnginePtr, jstring key, jdouble value) {
    const char* rawKey = env->GetStringUTFChars(key, nullptr);
    LuaEngine* engine = reinterpret_cast<LuaEngine*>(static_cast<intptr_t>(luaEnginePtr));
    // Assumes the table is on top of the stack.
    auto* luaState = engine->GetLuaState();
    lua_pushstring(luaState, rawKey);
    env->ReleaseStringUTFChars(key, rawKey);
    lua_gettable(luaState, -2);
    bool result = false;
    if (!lua_isnumber(luaState, -1))
        result = false;
    else
        result = static_cast<double>(lua_tonumber(luaState, -1)) == static_cast<double>(value);
    lua_pop(luaState, 1);
    return result;
}

JNIEXPORT bool JNICALL Java_com_android_car_telemetry_JniUtilsTest_nativeHasStringValue(
        JNIEnv* env, jobject object, jlong luaEnginePtr, jstring key, jstring value) {
    const char* rawKey = env->GetStringUTFChars(key, nullptr);
    LuaEngine* engine = reinterpret_cast<LuaEngine*>(static_cast<intptr_t>(luaEnginePtr));
    // Assumes the table is on top of the stack.
    auto* luaState = engine->GetLuaState();
    lua_pushstring(luaState, rawKey);
    env->ReleaseStringUTFChars(key, rawKey);
    lua_gettable(luaState, -2);
    bool result = false;
    if (!lua_isstring(luaState, -1)) {
        result = false;
    } else {
        std::string s = lua_tostring(luaState, -1);
        const char* rawValue = env->GetStringUTFChars(value, nullptr);
        result = strcmp(lua_tostring(luaState, -1), rawValue) == 0;
        env->ReleaseStringUTFChars(value, rawValue);
    }
    lua_pop(luaState, 1);
    return result;
}

}  //  extern "C"

}  //  namespace
}  //  namespace script_executor
}  //  namespace telemetry
}  //  namespace automotive
}  //  namespace android
