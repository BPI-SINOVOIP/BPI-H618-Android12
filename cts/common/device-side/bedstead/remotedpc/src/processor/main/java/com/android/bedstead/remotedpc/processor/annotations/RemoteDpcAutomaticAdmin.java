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

package com.android.bedstead.remotedpc.processor.annotations;

import java.lang.annotation.ElementType;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.lang.annotation.Target;

/**
 * Applied to methods which map to an identically named method on the same interface but needs the
 * admin {@link android.content.ComponentName} added as the first argument.
 *
 * <p>This should always be paired with another method which accepts the
 * {@link android.content.ComponentName}.
 *
 * <p>For example:
 * <pre><code>
 *   boolean isUsingUnifiedPassword(@NonNull ComponentName admin);
 *   @RemoteDpcAutomaticAdmin boolean isUsingUnifiedPassword();
 * <code></pre>
 */
@Target(ElementType.METHOD)
@Retention(RetentionPolicy.CLASS)
public @interface RemoteDpcAutomaticAdmin {
}
