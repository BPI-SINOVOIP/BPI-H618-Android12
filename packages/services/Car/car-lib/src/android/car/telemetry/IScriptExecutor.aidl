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

package android.car.telemetry;

import android.car.telemetry.IScriptExecutorListener;
import android.os.Bundle;

/**
 * An internal API provided by isolated Script Executor process
 * for executing Lua scripts in a sandboxed environment
 *
 * @hide
 */
interface IScriptExecutor {
  /**
   * Executes a specified function in provided Lua script with given input arguments.
   *
   * @param scriptBody complete body of Lua script that also contains the function to be invoked
   * @param functionName the name of the function to execute
   * @param publishedData input data provided by the source which the function handles
   * @param savedState key-value pairs preserved from the previous invocation of the function
   * @param listener callback for the sandboxed environent to report back script execution results, errors, and logs
   */
  void invokeScript(String scriptBody,
                    String functionName,
                    in Bundle publishedData,
                    in @nullable Bundle savedState,
                    in IScriptExecutorListener listener);
}
