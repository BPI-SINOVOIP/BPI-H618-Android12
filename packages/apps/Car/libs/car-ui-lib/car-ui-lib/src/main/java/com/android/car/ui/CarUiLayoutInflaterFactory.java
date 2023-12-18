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
package com.android.car.ui;

import android.content.Context;
import android.util.AttributeSet;
import android.view.LayoutInflater;
import android.view.View;

import androidx.annotation.Nullable;
import androidx.appcompat.app.AppCompatViewInflater;

import com.android.car.ui.recyclerview.CarUiRecyclerView;
import com.android.car.ui.sharedlibrarysupport.SharedLibraryFactorySingleton;

/**
 * A custom {@link LayoutInflater.Factory2} that will create CarUi components such as {@link
 * CarUiRecyclerView}. It extends AppCompatViewInflater so that it can still let AppCompat
 * components be created correctly.
 */
public class CarUiLayoutInflaterFactory extends AppCompatViewInflater
        implements LayoutInflater.Factory2 {

    @Nullable
    protected View createView(Context context, String name, AttributeSet attrs) {
        View view = null;

        // Don't use CarUiTextView.class.getSimpleName(), as when proguard obfuscates the class name
        // it will no longer match what's in xml.
        if (CarUiRecyclerView.class.getName().equals(name)) {
            view = SharedLibraryFactorySingleton.get(context)
                    .createRecyclerView(context, attrs);
        } else if (name.contentEquals("CarUiTextView")) {
            view = SharedLibraryFactorySingleton.get(context).createTextView(context, attrs);
        }

        return view;
    }

    @Override
    public View onCreateView(String name, Context context, AttributeSet attrs) {
        // Deprecated, do nothing.
        return null;
    }

    @Override
    public View onCreateView(View parent, String name, Context context,
            AttributeSet attrs) {
        return createView(context, name, attrs);
    }
}
