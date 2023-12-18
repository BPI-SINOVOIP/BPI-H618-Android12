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

package com.android.compos;

import com.android.compos.Metadata;

/** {@hide} */
interface ICompService {
    /**
     * Execute a command composed of the args, in a context that may be specified in the Metadata,
     * e.g. with file descriptors pre-opened. The service is responsible to decide what executables
     * it may run.
     *
     * @param args The command line arguments to run. The 0-th args is normally the program name,
     *             which may not be used by the service. The service may be configured to always use
     *             a fixed executable, or possibly use the 0-th args are the executable lookup hint.
     * @param metadata Additional information of the execution
     * @return exit code of the program
     */
    byte execute(in String[] args, in Metadata metadata);
}
