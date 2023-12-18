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
package com.android.wallpaper.model

import androidx.lifecycle.MutableLiveData
import androidx.lifecycle.ViewModel

/** ViewModel class to keep track of workspace updates. */
class WorkspaceViewModel : ViewModel() {

    /**
     * Triggers workspace updates through flipping the value from {@code false} to {@code true},
     * or from {@code true} to {@code false}.
     */
    val updateWorkspace: MutableLiveData<Boolean> by lazy {
        MutableLiveData<Boolean>()
    }
}
