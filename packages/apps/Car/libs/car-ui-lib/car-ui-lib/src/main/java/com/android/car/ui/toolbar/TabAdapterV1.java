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

package com.android.car.ui.toolbar;

import com.android.car.ui.sharedlibrary.oemapis.toolbar.TabOEMV1;

import java.util.function.Consumer;

@SuppressWarnings("AndroidJdkLibsChecker")
class TabAdapterV1 {

    private final Tab mClientTab;
    private final TabOEMV1 mSharedLibraryTab;

    TabAdapterV1(Tab clientTab) {
        mClientTab = clientTab;
        Consumer<Tab> selectedListener = mClientTab.getSelectedListener();
        mSharedLibraryTab = TabOEMV1.builder()
                .setIcon(mClientTab.getIcon())
                .setTitle(mClientTab.getText())
                .setOnSelectedListener(selectedListener == null
                        ? null
                        : () -> selectedListener.accept(mClientTab))
                .build();
    }

    public Tab getClientTab() {
        return mClientTab;
    }

    public TabOEMV1 getSharedLibraryTab() {
        return mSharedLibraryTab;
    }
}
