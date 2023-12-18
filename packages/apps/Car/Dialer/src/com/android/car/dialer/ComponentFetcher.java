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

package com.android.car.dialer;

import android.app.Application;
import android.content.Context;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import dagger.hilt.internal.GeneratedComponentManager;

/** Util class to retrieve dagger components from context. */
public final class ComponentFetcher {

    /** Fetches the component {@code T} from context. */
    public static <T> T from(@NonNull Context context, Class<T> componentClass) {
        return fromObject(context, componentClass);
    }

    /** Fetches the component {@code T} from application. */
    public static <T> T from(@NonNull Application application, Class<T> componentClass) {
        return fromObject(application, componentClass);
    }

    private static <T> T fromObject(@NonNull Object object, Class<T> componentClass) {
        T component = componentClass.cast(from(object));
        if (component == null) {
            throw new IllegalArgumentException(
                    "Given object doesn't have the component installed: "
                            + componentClass.getName());
        }
        return component;
    }

    @Nullable
    private static <T> T from(@NonNull Object object) {
        if (object instanceof GeneratedComponentManager<?>) {
            return (T) ((GeneratedComponentManager<?>) object).generatedComponent();
        }
        return null;
    }

    private ComponentFetcher() {
    }
}
