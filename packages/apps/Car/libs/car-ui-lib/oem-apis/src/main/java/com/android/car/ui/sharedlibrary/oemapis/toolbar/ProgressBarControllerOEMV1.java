/*
 * Copyright (C) 2020 The Android Open Source Project
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

package com.android.car.ui.sharedlibrary.oemapis.toolbar;

/**
 * Interface for a Progress Bar. It's methods are a subset of the methods of
 * {@link android.widget.ProgressBar}. This is so that an application doesn't
 * have access to customize the {@link android.widget.ProgressBar} or other
 * views in it's hierarchy in ways that were not intended.
 */
public interface ProgressBarControllerOEMV1 {
    /** Shows/hides the progress bar */
    void setVisible(boolean visible);
    /** Equivalent to {@link android.widget.ProgressBar#setIndeterminate(boolean)} */
    void setIndeterminate(boolean indeterminate);
    /** Equivalent to {@link android.widget.ProgressBar#setMax(int)} */
    void setMax(int max);
    /** Equivalent to {@link android.widget.ProgressBar#setMin(int)} */
    void setMin(int min);
    /** Equivalent to {@link android.widget.ProgressBar#setProgress(int)} */
    void setProgress(int progress);
}
