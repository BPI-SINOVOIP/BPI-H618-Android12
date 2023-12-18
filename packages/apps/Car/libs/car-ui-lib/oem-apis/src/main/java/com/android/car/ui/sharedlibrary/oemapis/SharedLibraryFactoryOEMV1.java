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
package com.android.car.ui.sharedlibrary.oemapis;

import android.content.Context;
import android.view.View;

import com.android.car.ui.sharedlibrary.oemapis.appstyledview.AppStyledViewControllerOEMV1;
import com.android.car.ui.sharedlibrary.oemapis.recyclerview.AdapterOEMV1;
import com.android.car.ui.sharedlibrary.oemapis.recyclerview.ListItemOEMV1;
import com.android.car.ui.sharedlibrary.oemapis.recyclerview.RecyclerViewAttributesOEMV1;
import com.android.car.ui.sharedlibrary.oemapis.recyclerview.RecyclerViewOEMV1;
import com.android.car.ui.sharedlibrary.oemapis.recyclerview.ViewHolderOEMV1;
import com.android.car.ui.sharedlibrary.oemapis.toolbar.ToolbarControllerOEMV1;

import java.util.List;
import java.util.function.Consumer;
import java.util.function.Function;

/**
 * This interface contains methods to create customizable carui components.
 * <p>
 * It returns them as their OEM-versioned interfaces (i.e. ToolbarControllerOEMV1) and is versioned
 * itself so that no additional reflection or casting is necessary once the SharedLibraryFactory has
 * been created.
 * <p>
 * Multiple of these can be provided via {@link SharedLibraryVersionProviderOEMV1} to allow shared
 * libraries to provide an old implementation for old apps, and a newer implementation for newer
 * apps.
 */
@SuppressWarnings("AndroidJdkLibsChecker")
public interface SharedLibraryFactoryOEMV1 {

    /**
     * Gives the shared library access to two factories that will create FocusParkingViews and
     * FocusAreas. These views have their implementation in the static car-ui-lib.
     * <p>
     * When {@link #installBaseLayoutAround} creates a base layout, it should include a
     * FocusParkingView for rotary to work properly. If the base layout has a toolbar, it should
     * also be wrapped in a FocusArea.
     *
     * @param focusParkingViewFactory a function that will infinitely return new instances of
     *                                FocusParkingView
     * @param focusAreaFactory        a function that will infinitely return new instances of
     *                                FocusArea
     */
    void setRotaryFactories(
            Function<Context, FocusParkingViewOEMV1> focusParkingViewFactory,
            Function<Context, FocusAreaOEMV1> focusAreaFactory);

    /**
     * Creates the base layout, and optionally the toolbar.
     *
     * @param contentView           The view to install the base layout around.
     * @param insetsChangedListener A method to call when the insets change.
     * @param toolbarEnabled        Whether or not to add a toolbar to the base layout.
     * @param fullscreen            Whether or not this base layout / toolbar is taking up the whole
     *                              screen. This can be used to decide whether or not to add
     *                              decorations around the edge of it.
     * @return A {@link ToolbarControllerOEMV1} or null if {@code toolbarEnabled} was false.
     */
    ToolbarControllerOEMV1 installBaseLayoutAround(
            View contentView,
            Consumer<InsetsOEMV1> insetsChangedListener,
            boolean toolbarEnabled,
            boolean fullscreen);

    /**
     * If implementation of the library would like to opt out of controlling the base layout and
     * subsequently the toolbar they can do so by returning false from this method.
     *
     * @return false if {@link #installBaseLayoutAround} should not be called for this library
     */
    boolean customizesBaseLayout();

    /**
     * Creates a app styled view.
     *
     * @return the view used for app styled view.
     */
    AppStyledViewControllerOEMV1 createAppStyledView();

    /**
     * Creates an instance of CarUiRecyclerView
     *
     * @param context The visual context to create views with.
     * @param attrs   An object containing initial attributes for the button.
     */
    RecyclerViewOEMV1 createRecyclerView(Context context, RecyclerViewAttributesOEMV1 attrs);

    /**
     * Creates an instance of list item adapter
     */
    AdapterOEMV1<? extends ViewHolderOEMV1> createListItemAdapter(List<ListItemOEMV1> items);
}
