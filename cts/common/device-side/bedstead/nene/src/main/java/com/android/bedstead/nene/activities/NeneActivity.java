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

package com.android.bedstead.nene.activities;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;

/**
 * Interface for use by classes which are able to be used in Nene activity test apis.
 *
 * <p>Methods on this interface should match exactly methods on {@link Activity}.
 */
public interface NeneActivity {
    /** See {@link Activity#startLockTask}. */
    void startLockTask();

    /** See {@link Activity#stopLockTask}. */
    void stopLockTask();

    /** See {@link Activity#finish()}. */
    void finish();

    /** See {@link Activity#isFinishing}. */
    boolean isFinishing();

    /** See {@link Activity#startActivity}. */
    void startActivity(Intent intent);

    /** See {@link Activity#startActivity}. */
    void startActivity(Intent intent, Bundle options);
}
