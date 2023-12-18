/*
 * Copyright 2021 The Android Open Source Project
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
package android.system.virtmanager;

import android.system.virtmanager.IVirtualMachine;

/**
 * An object which a client may register with the Virt Manager to get callbacks about the state of
 * a particular VM.
 */
oneway interface IVirtualMachineCallback {
    /**
     * Called when the VM dies.
     *
     * Note that this will not be called if the Virt Manager itself dies, so you should also use
     * `link_to_death` to handle that.
     */
    void onDied(int cid);
}
