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
package com.android.car.ui.sharedlibrarysupport;

import android.content.Context;
import android.content.res.TypedArray;
import android.text.SpannableString;
import android.util.AttributeSet;
import android.view.View;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.recyclerview.widget.RecyclerView;

import com.android.car.ui.CarUiText;
import com.android.car.ui.FocusArea;
import com.android.car.ui.FocusAreaAdapterV1;
import com.android.car.ui.FocusParkingView;
import com.android.car.ui.FocusParkingViewAdapterV1;
import com.android.car.ui.R;
import com.android.car.ui.appstyledview.AppStyledViewController;
import com.android.car.ui.appstyledview.AppStyledViewControllerAdapterV1;
import com.android.car.ui.appstyledview.AppStyledViewControllerImpl;
import com.android.car.ui.baselayout.Insets;
import com.android.car.ui.baselayout.InsetsChangedListener;
import com.android.car.ui.recyclerview.CarUiContentListItem;
import com.android.car.ui.recyclerview.CarUiHeaderListItem;
import com.android.car.ui.recyclerview.CarUiLayoutStyle;
import com.android.car.ui.recyclerview.CarUiListItem;
import com.android.car.ui.recyclerview.CarUiListItemAdapterAdapterV1;
import com.android.car.ui.recyclerview.CarUiRecyclerView;
import com.android.car.ui.recyclerview.CarUiRecyclerView.CarUiRecyclerViewLayout;
import com.android.car.ui.recyclerview.RecyclerViewAdapterV1;
import com.android.car.ui.sharedlibrary.oemapis.InsetsOEMV1;
import com.android.car.ui.sharedlibrary.oemapis.SharedLibraryFactoryOEMV1;
import com.android.car.ui.sharedlibrary.oemapis.appstyledview.AppStyledViewControllerOEMV1;
import com.android.car.ui.sharedlibrary.oemapis.recyclerview.AdapterOEMV1;
import com.android.car.ui.sharedlibrary.oemapis.recyclerview.ContentListItemOEMV1;
import com.android.car.ui.sharedlibrary.oemapis.recyclerview.HeaderListItemOEMV1;
import com.android.car.ui.sharedlibrary.oemapis.recyclerview.LayoutStyleOEMV1;
import com.android.car.ui.sharedlibrary.oemapis.recyclerview.ListItemOEMV1;
import com.android.car.ui.sharedlibrary.oemapis.recyclerview.RecyclerViewAttributesOEMV1;
import com.android.car.ui.sharedlibrary.oemapis.recyclerview.RecyclerViewOEMV1;
import com.android.car.ui.sharedlibrary.oemapis.recyclerview.SpanSizeLookupOEMV1;
import com.android.car.ui.sharedlibrary.oemapis.recyclerview.ViewHolderOEMV1;
import com.android.car.ui.sharedlibrary.oemapis.toolbar.ToolbarControllerOEMV1;
import com.android.car.ui.toolbar.ToolbarController;
import com.android.car.ui.toolbar.ToolbarControllerAdapterV1;
import com.android.car.ui.utils.CarUiUtils;
import com.android.car.ui.widget.CarUiTextView;

import java.util.List;
import java.util.function.Consumer;

/**
 * This class is an wrapper around {@link SharedLibraryFactoryOEMV1} that implements {@link
 * SharedLibraryFactory}, to provide a version-agnostic way of interfacing with the OEM's
 * SharedLibraryFactory.
 */
public final class SharedLibraryFactoryAdapterV1 implements SharedLibraryFactory {
    SharedLibraryFactoryOEMV1 mOem;
    SharedLibraryFactoryStub mFactoryStub;

    public SharedLibraryFactoryAdapterV1(SharedLibraryFactoryOEMV1 oem, Context sharedLibContext) {
        mOem = oem;
        mFactoryStub = new SharedLibraryFactoryStub(sharedLibContext);

        mOem.setRotaryFactories(
                c -> new FocusParkingViewAdapterV1(new FocusParkingView(c)),
                c -> new FocusAreaAdapterV1(new FocusArea(c)));
    }

    @Override
    @Nullable
    public ToolbarController installBaseLayoutAround(
            View contentView,
            InsetsChangedListener insetsChangedListener,
            boolean toolbarEnabled,
            boolean fullscreen) {

        if (!mOem.customizesBaseLayout()) {
            return mFactoryStub.installBaseLayoutAround(contentView,
                    insetsChangedListener, toolbarEnabled, fullscreen);
        }

        ToolbarControllerOEMV1 toolbar = mOem.installBaseLayoutAround(contentView,
                insets -> insetsChangedListener.onCarUiInsetsChanged(adaptInsets(insets)),
                toolbarEnabled, fullscreen);

        return toolbar != null
                ? new ToolbarControllerAdapterV1(contentView.getContext(), toolbar)
                : null;
    }

    @NonNull
    @Override
    public CarUiTextView createTextView(Context context, AttributeSet attrs) {
        return mFactoryStub.createTextView(context, attrs);
    }


    @Override
    public AppStyledViewController createAppStyledView(Context activityContext) {
        AppStyledViewControllerOEMV1 appStyledViewControllerOEMV1 = mOem.createAppStyledView();
        return appStyledViewControllerOEMV1 == null ? new AppStyledViewControllerImpl(
                activityContext) : new AppStyledViewControllerAdapterV1(
                appStyledViewControllerOEMV1);
    }

    private Insets adaptInsets(InsetsOEMV1 insetsOEM) {
        return new Insets(insetsOEM.getLeft(), insetsOEM.getTop(),
                insetsOEM.getRight(), insetsOEM.getBottom());
    }

    @Override
    public CarUiRecyclerView createRecyclerView(@NonNull Context context,
            @Nullable AttributeSet attrs) {
        RecyclerViewAttributesOEMV1 oemAttrs = from(context, attrs);
        RecyclerViewOEMV1 oemRecyclerView = mOem.createRecyclerView(context, oemAttrs);
        if (oemRecyclerView != null) {
            RecyclerViewAdapterV1 rv = new RecyclerViewAdapterV1(context, attrs);
            rv.setRecyclerViewOEMV1(oemRecyclerView);
            return rv;
        } else {
            return mFactoryStub.createRecyclerView(context, attrs);
        }
    }

    @Override
    public RecyclerView.Adapter<? extends RecyclerView.ViewHolder> createListItemAdapter(
            List<? extends CarUiListItem> items) {
        List<ListItemOEMV1> oemItems = CarUiUtils.convertList(items,
                SharedLibraryFactoryAdapterV1::toOemListItem);

        AdapterOEMV1<? extends ViewHolderOEMV1> oemAdapter = mOem.createListItemAdapter(oemItems);
        return oemAdapter != null ? new CarUiListItemAdapterAdapterV1(oemAdapter)
                : mFactoryStub.createListItemAdapter(items);
    }

    private static RecyclerViewAttributesOEMV1 from(Context context, AttributeSet attrs) {
        RecyclerViewAttributesOEMV1 oemAttrs = null;
        if (attrs != null) {
            TypedArray a = context.obtainStyledAttributes(
                    attrs,
                    R.styleable.CarUiRecyclerView,
                    0,
                    R.style.Widget_CarUi_CarUiRecyclerView);
            final int carUiRecyclerViewLayout = a.getInt(
                    R.styleable.CarUiRecyclerView_layoutStyle,
                    CarUiRecyclerViewLayout.LINEAR);
            final int spanCount = a.getInt(
                    R.styleable.CarUiRecyclerView_numOfColumns, /* defValue= */ 1);
            final boolean rotaryScrollEnabled = a.getBoolean(
                    R.styleable.CarUiRecyclerView_rotaryScrollEnabled,
                    /* defValue=*/ false);
            final int orientation = a.getInt(
                    R.styleable.CarUiRecyclerView_android_orientation,
                    CarUiLayoutStyle.VERTICAL);
            final boolean reversed = a.getBoolean(
                    R.styleable.CarUiRecyclerView_reverseLayout, false);
            final int size = a.getInt(R.styleable.CarUiRecyclerView_carUiSize,
                    CarUiRecyclerView.SIZE_LARGE);
            a.recycle();

            final LayoutStyleOEMV1 layoutStyle = new LayoutStyleOEMV1() {
                @Override
                public int getSpanCount() {
                    return spanCount;
                }

                @Override
                public int getLayoutType() {
                    switch (carUiRecyclerViewLayout) {
                        case CarUiRecyclerViewLayout.GRID:
                            return LayoutStyleOEMV1.LAYOUT_TYPE_GRID;
                        case CarUiRecyclerViewLayout.LINEAR:
                        default:
                            return LayoutStyleOEMV1.LAYOUT_TYPE_LINEAR;
                    }
                }

                @Override
                public int getOrientation() {
                    switch (orientation) {
                        case CarUiLayoutStyle.HORIZONTAL:
                            return LayoutStyleOEMV1.ORIENTATION_HORIZONTAL;
                        case CarUiLayoutStyle.VERTICAL:
                        default:
                            return LayoutStyleOEMV1.ORIENTATION_VERTICAL;
                    }
                }

                @Override
                public boolean getReverseLayout() {
                    return reversed;
                }

                @Override
                public SpanSizeLookupOEMV1 getSpanSizeLookup() {
                    // This can be set via setLayoutStyle API later.
                    return null;
                }
            };

            oemAttrs = new RecyclerViewAttributesOEMV1() {
                @Override
                public boolean isRotaryScrollEnabled() {
                    return rotaryScrollEnabled;
                }

                @Override
                public int getSize() {
                    switch (size) {
                        case CarUiRecyclerView.SIZE_SMALL:
                            return RecyclerViewAttributesOEMV1.SIZE_SMALL;
                        case CarUiRecyclerView.SIZE_MEDIUM:
                            return RecyclerViewAttributesOEMV1.SIZE_MEDIUM;
                        case CarUiRecyclerView.SIZE_LARGE:
                        default:
                            return RecyclerViewAttributesOEMV1.SIZE_LARGE;
                    }
                }

                @Override
                public LayoutStyleOEMV1 getLayoutStyle() {
                    return layoutStyle;
                }
            };
        }
        return oemAttrs;
    }

    private static ListItemOEMV1 toOemListItem(CarUiListItem item) {
        if (item instanceof CarUiHeaderListItem) {
            CarUiHeaderListItem header = (CarUiHeaderListItem) item;
            return new HeaderListItemOEMV1.Builder(new SpannableString(header.getTitle()))
                    .setBody(new SpannableString(header.getBody()))
                    .build();
        } else if (item instanceof CarUiContentListItem) {
            CarUiContentListItem contentItem = (CarUiContentListItem) item;

            ContentListItemOEMV1.Builder builder = new ContentListItemOEMV1.Builder(
                    toOemListItemAction(contentItem.getAction()));

            if (contentItem.getTitle() != null) {
                builder.setTitle(
                        new SpannableString(contentItem.getTitle().getPreferredText()));
            }

            if (contentItem.getBody() != null) {
                builder.setBody(new SpannableString(
                        CarUiText.combineMultiLine(contentItem.getBody())));
            }

            builder.setIcon(contentItem.getIcon(),
                    toOemListItemIconType(contentItem.getPrimaryIconType()));

            if (contentItem.getAction() == CarUiContentListItem.Action.ICON) {
                Consumer<ContentListItemOEMV1> listener =
                        contentItem.getSupplementalIconOnClickListener() != null
                                ? oemItem ->
                                contentItem.getSupplementalIconOnClickListener().onClick(
                                        contentItem) : null;
                builder.setSupplementalIcon(contentItem.getSupplementalIcon(), listener);
            }

            if (contentItem.getOnClickListener() != null) {
                Consumer<ContentListItemOEMV1> listener =
                        contentItem.getOnClickListener() != null
                                ? oemItem ->
                                contentItem.getOnClickListener().onClick(contentItem) : null;
                builder.setOnItemClickedListener(listener);
            }

            builder.setOnCheckedChangeListener(
                    oemItem -> contentItem.setChecked(oemItem.isChecked()))
                    .setActionDividerVisible(contentItem.isActionDividerVisible())
                    .setEnabled(contentItem.isEnabled())
                    .setChecked(contentItem.isChecked())
                    .setActivated(contentItem.isActivated());
            return builder.build();
        } else {
            throw new IllegalStateException("Unexpected list item type");
        }
    }

    private static ContentListItemOEMV1.Action toOemListItemAction(
            CarUiContentListItem.Action action) {
        switch (action) {
            case NONE:
                return ContentListItemOEMV1.Action.NONE;
            case SWITCH:
                return ContentListItemOEMV1.Action.SWITCH;
            case CHECK_BOX:
                return ContentListItemOEMV1.Action.CHECK_BOX;
            case RADIO_BUTTON:
                return ContentListItemOEMV1.Action.RADIO_BUTTON;
            case ICON:
                return ContentListItemOEMV1.Action.ICON;
            case CHEVRON:
                return ContentListItemOEMV1.Action.CHEVRON;
            default:
                throw new IllegalStateException("Unexpected list item action type");
        }
    }

    private static ContentListItemOEMV1.IconType toOemListItemIconType(
            CarUiContentListItem.IconType iconType) {
        switch (iconType) {
            case CONTENT:
                return ContentListItemOEMV1.IconType.CONTENT;
            case STANDARD:
                return ContentListItemOEMV1.IconType.STANDARD;
            case AVATAR:
                return ContentListItemOEMV1.IconType.AVATAR;
            default:
                throw new IllegalStateException("Unexpected list item icon type");
        }
    }
}
