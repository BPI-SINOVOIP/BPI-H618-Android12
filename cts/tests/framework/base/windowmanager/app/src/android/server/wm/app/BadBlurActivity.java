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
 * limitations under the License
 */

package android.server.wm.app;

/**
 * This activity is used to test 2 things:
 * 1. Blur behind does not work if WindowManager.LayoutParams.FLAG_BLUR_BEHIND is not set,
 *    respectively if windowBlurBehindEnabled is not set.
 * 2. Background blur does not work for opaque activities (where windowIsTranslucent is false)
 *
 * In the style of this activity windowBlurBehindEnabled is false and windowIsTranslucent is false.
 * As a result, we expect that neither blur behind, nor background blur is rendered, even though
 * they are requested with setBlurBehindRadius and setBackgroundBlurRadius.
 */
public class BadBlurActivity extends BlurActivity {
}
