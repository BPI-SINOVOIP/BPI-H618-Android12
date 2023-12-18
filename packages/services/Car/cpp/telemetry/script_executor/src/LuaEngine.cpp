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

#include "LuaEngine.h"

#include <utility>

extern "C" {
#include "lauxlib.h"
#include "lualib.h"
}

namespace android {
namespace automotive {
namespace telemetry {
namespace script_executor {

LuaEngine::LuaEngine() {
    // Instantiate Lua environment
    mLuaState = luaL_newstate();
    luaL_openlibs(mLuaState);
}

LuaEngine::~LuaEngine() {
    lua_close(mLuaState);
}

lua_State* LuaEngine::GetLuaState() {
    return mLuaState;
}

void LuaEngine::ResetListener(ScriptExecutorListener* listener) {
    mListener.reset(listener);
}

int LuaEngine::LoadScript(const char* scriptBody) {
    // As the first step in Lua script execution we want to load
    // the body of the script into Lua stack and have it processed by Lua
    // to catch any errors.
    // More on luaL_dostring: https://www.lua.org/manual/5.3/manual.html#lual_dostring
    // If error, pushes the error object into the stack.
    const auto status = luaL_dostring(mLuaState, scriptBody);
    if (status) {
        // Removes error object from the stack.
        // Lua stack must be properly maintained due to its limited size,
        // ~20 elements and its critical function because all interaction with
        // Lua happens via the stack.
        // Starting read about Lua stack: https://www.lua.org/pil/24.2.html
        // TODO(b/192284232): add test case to trigger this.
        lua_pop(mLuaState, 1);
    }
    return status;
}

bool LuaEngine::PushFunction(const char* functionName) {
    // Interaction between native code and Lua happens via Lua stack.
    // In such model, a caller first pushes the name of the function
    // that needs to be called, followed by the function's input
    // arguments, one input value pushed at a time.
    // More info: https://www.lua.org/pil/24.2.html
    lua_getglobal(mLuaState, functionName);
    const auto status = lua_isfunction(mLuaState, /*idx= */ -1);
    // TODO(b/192284785): add test case for wrong function name in Lua.
    if (status == 0) lua_pop(mLuaState, 1);
    return status;
}

int LuaEngine::Run() {
    // Performs blocking call of the provided Lua function. Assumes all
    // input arguments are in the Lua stack as well in proper order.
    // On how to call Lua functions: https://www.lua.org/pil/25.2.html
    // Doc on lua_pcall: https://www.lua.org/manual/5.3/manual.html#lua_pcall
    // TODO(b/189241508): Once we implement publishedData parsing, nargs should
    // change from 1 to 2.
    // TODO(b/192284612): add test case for failed call.
    return lua_pcall(mLuaState, /* nargs= */ 1, /* nresults= */ 0, /*errfunc= */ 0);
}

}  // namespace script_executor
}  // namespace telemetry
}  // namespace automotive
}  // namespace android
