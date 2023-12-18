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
package com.chassis.car.ui.sharedlibrary;

import android.content.Context;
import android.view.View;

import androidx.annotation.Nullable;

import com.android.car.ui.sharedlibrary.oemapis.FocusAreaOEMV1;
import com.android.car.ui.sharedlibrary.oemapis.FocusParkingViewOEMV1;
import com.android.car.ui.sharedlibrary.oemapis.InsetsOEMV1;
import com.android.car.ui.sharedlibrary.oemapis.SharedLibraryFactoryOEMV1;
import com.android.car.ui.sharedlibrary.oemapis.appstyledview.AppStyledViewControllerOEMV1;
import com.android.car.ui.sharedlibrary.oemapis.recyclerview.AdapterOEMV1;
import com.android.car.ui.sharedlibrary.oemapis.recyclerview.ListItemOEMV1;
import com.android.car.ui.sharedlibrary.oemapis.recyclerview.RecyclerViewAttributesOEMV1;
import com.android.car.ui.sharedlibrary.oemapis.recyclerview.RecyclerViewOEMV1;
import com.android.car.ui.sharedlibrary.oemapis.recyclerview.ViewHolderOEMV1;
import com.android.car.ui.sharedlibrary.oemapis.toolbar.ToolbarControllerOEMV1;

import com.chassis.car.ui.sharedlibrary.toolbar.BaseLayoutInstaller;

import java.util.List;
import java.util.function.Consumer;
import java.util.function.Function;

/**
 * An implementation of {@link SharedLibraryFactoryImpl} for creating the reference design
 * car-ui-lib components.
 */
@SuppressWarnings("AndroidJdkLibsChecker")
public class SharedLibraryFactoryImpl implements SharedLibraryFactoryOEMV1 {

    private final Context mSharedLibraryContext;
    @Nullable
    private Function<Context, FocusParkingViewOEMV1> mFocusParkingViewFactory;
    @Nullable
    private Function<Context, FocusAreaOEMV1> mFocusAreaFactory;

    public SharedLibraryFactoryImpl(Context sharedLibraryContext) {
        mSharedLibraryContext = sharedLibraryContext;
    }

    @Override
    public void setRotaryFactories(
            Function<Context, FocusParkingViewOEMV1> focusParkingViewFactory,
            Function<Context, FocusAreaOEMV1> focusAreaFactory) {
        mFocusParkingViewFactory = focusParkingViewFactory;
        mFocusAreaFactory = focusAreaFactory;
    }

    @Override
    public ToolbarControllerOEMV1 installBaseLayoutAround(View contentView,
            Consumer<InsetsOEMV1> insetsChangedListener, boolean toolbarEnabled,
            boolean fullscreen) {

        return BaseLayoutInstaller.installBaseLayoutAround(
                mSharedLibraryContext,
                contentView,
                insetsChangedListener,
                toolbarEnabled,
                fullscreen,
                mFocusParkingViewFactory,
                mFocusAreaFactory);
    }

    @Override
    public boolean customizesBaseLayout() {
        return true;
    }

    @Override
    public AppStyledViewControllerOEMV1 createAppStyledView() {
        //return new AppStyleViewControllerImpl(mSharedLibraryContext);
        return null;
    }

    @Override
    public RecyclerViewOEMV1 createRecyclerView(Context context,
            RecyclerViewAttributesOEMV1 attrs) {
        //return new RecyclerViewImpl(context, attrs);
        return null;
    }

    @Override
    public AdapterOEMV1<? extends ViewHolderOEMV1> createListItemAdapter(
            List<ListItemOEMV1> items) {
        //return new ListItemAdapter(items);
        return null;
    }
}
