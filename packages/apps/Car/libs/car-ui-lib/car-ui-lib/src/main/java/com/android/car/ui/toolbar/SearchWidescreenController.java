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
package com.android.car.ui.toolbar;

import static android.view.WindowInsets.Type.ime;

import static com.android.car.ui.imewidescreen.CarUiImeWideScreenController.CONTENT_AREA_SURFACE_DISPLAY_ID;
import static com.android.car.ui.imewidescreen.CarUiImeWideScreenController.CONTENT_AREA_SURFACE_HEIGHT;
import static com.android.car.ui.imewidescreen.CarUiImeWideScreenController.CONTENT_AREA_SURFACE_HOST_TOKEN;
import static com.android.car.ui.imewidescreen.CarUiImeWideScreenController.CONTENT_AREA_SURFACE_PACKAGE;
import static com.android.car.ui.imewidescreen.CarUiImeWideScreenController.CONTENT_AREA_SURFACE_WIDTH;
import static com.android.car.ui.imewidescreen.CarUiImeWideScreenController.SEARCH_RESULT_ITEM_ID_LIST;
import static com.android.car.ui.imewidescreen.CarUiImeWideScreenController.SEARCH_RESULT_SUPPLEMENTAL_ICON_ID_LIST;
import static com.android.car.ui.imewidescreen.CarUiImeWideScreenController.WIDE_SCREEN_ACTION;
import static com.android.car.ui.imewidescreen.CarUiImeWideScreenController.WIDE_SCREEN_CLEAR_DATA_ACTION;
import static com.android.car.ui.imewidescreen.CarUiImeWideScreenController.WIDE_SCREEN_POST_LOAD_SEARCH_RESULTS_ACTION;
import static com.android.car.ui.imewidescreen.CarUiImeWideScreenController.WIDE_SCREEN_SEARCH_RESULTS;
import static com.android.car.ui.utils.CarUiUtils.getBooleanSystemProperty;

import android.content.ContentValues;
import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.drawable.BitmapDrawable;
import android.hardware.display.DisplayManager;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.IBinder;
import android.os.Looper;
import android.os.Parcel;
import android.view.Display;
import android.view.SurfaceControlViewHost;
import android.view.View;
import android.view.ViewGroup;
import android.view.inputmethod.InputMethodManager;
import android.widget.FrameLayout;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import com.android.car.ui.CarUiText;
import com.android.car.ui.R;
import com.android.car.ui.core.SearchResultsProvider;
import com.android.car.ui.imewidescreen.CarUiImeSearchListItem;
import com.android.car.ui.recyclerview.CarUiContentListItem;
import com.android.car.ui.recyclerview.CarUiRecyclerView;

import java.util.List;
import java.util.function.BiConsumer;

/**
 * Internal class to car-ui-lib, do not use from outside.
 * <p>
 * Handles converting a {@link SearchConfig} into the appropriate commands to send to the widescreen
 * IME.
 * <p>
 * You must call {@link #setTextView} and {@link #getOnPrivateImeCommandListener} after creating
 * this object to finish initializing it, but they don't have to be called immediately. The
 * SearchWidescreenController will still accept calls to {@link #setSearchConfig} even before
 * the TextView has been set, it will just wait for the TextView before doing anything.
 */
@SuppressWarnings("AndroidJdkLibsChecker")
public class SearchWidescreenController {

    private final Handler mHandler = new Handler(Looper.getMainLooper());
    private final Context mContext;
    private final InputMethodManager mInputMethodManager;
    @Nullable
    private TextView mTextView;
    private SurfaceControlViewHost mSurfaceControlViewHost;
    private int mSurfaceHeight;
    private int mSurfaceWidth;
    private boolean mIsImeWidescreenViewSet = false;
    private ViewGroup mOriginalParent;
    private ViewGroup.LayoutParams mLayoutParams;

    @NonNull
    private SearchConfig mSearchConfig = SearchConfig.builder().build();

    public SearchWidescreenController(@NonNull Context context) {
        mContext = context;
        mInputMethodManager = context.getSystemService(InputMethodManager.class);
    }

    /**
     * Sets the text view that will be used to send the IME commands. This can be called at
     * any time after the creation of this SearchWidescreenController, the controller will
     * do nothing until the text view is set.
     */
    public void setTextView(@Nullable TextView textView) {
        if (mTextView != null) {
            throw new IllegalStateException("TextView already set");
        }
        mTextView = textView;
        initializeWindowInsetsListener();
    }

    /**
     * Sets the {@link SearchConfig} of data to display in the IME widescreen.
     */
    public void setSearchConfig(@Nullable SearchConfig searchConfig) {
        if (searchConfig == null) {
            searchConfig = SearchConfig.builder().build();
        }
        mSearchConfig = searchConfig;
    }

    @Nullable
    private View getSearchResultsView() {
        View view = mSearchConfig.getSearchResultsView();
        if (view instanceof CarUiRecyclerView) {
            return ((CarUiRecyclerView) view).getContainer();
        }
        return view;
    }

    private void initializeWindowInsetsListener() {
        View.OnApplyWindowInsetsListener onApplyWindowInsetsListener = (v, insets) -> {
            if (Build.VERSION.SDK_INT < Build.VERSION_CODES.R) {
                // We should never get here because we don't register the listener pre-R anyways.
                throw new IllegalStateException("Cannot check if the ime is visible pre R");
            }
            if (mTextView == null) {
                return v.onApplyWindowInsets(insets);
            }

            if (insets.isVisible(ime())) {
                FrameLayout wideScreenImeContentAreaViewContainer;
                View contentView = getSearchResultsView();
                if (!mIsImeWidescreenViewSet && contentView != null) {
                    // When the IME opens after the setSearchConfig()
                    // call, setup a container and attach the view to the IME surface.
                    // The container is necessary because the content view needs to be removed
                    // and added back into the app when the IME closes, and views cannot be
                    // detatched from a SurfaceControlViewHost. We save the content view's old
                    // LayoutParams and parent for when it's reattached to the app.
                    mLayoutParams = contentView.getLayoutParams();
                    mOriginalParent = (ViewGroup) contentView.getParent();
                    if (mOriginalParent != null) {
                        mOriginalParent.removeView(contentView);
                    }

                    FrameLayout.LayoutParams params = new FrameLayout.LayoutParams(
                            ViewGroup.LayoutParams.MATCH_PARENT,
                            ViewGroup.LayoutParams.MATCH_PARENT);
                    // This is different from getSearchResultsView() because getSearchResultsView()
                    // will turn CarUiRecyclerViews into their containing views. But here
                    // we want to actually know if it is a CarUiRecyclerView
                    View originalView = mSearchConfig.getSearchResultsView();
                    if (originalView instanceof CarUiRecyclerView) {
                        // We need to use a negative layout margin to have the list take the full
                        // content area.
                        params.topMargin = -originalView.getPaddingTop();
                        params.bottomMargin = -originalView.getPaddingBottom();
                        params.leftMargin = -originalView.getPaddingLeft();
                        params.rightMargin = -originalView.getPaddingRight();
                    }
                    wideScreenImeContentAreaViewContainer = new FrameLayout(mContext);
                    wideScreenImeContentAreaViewContainer.addView(contentView, params);
                } else {
                    wideScreenImeContentAreaViewContainer = null;
                }

                displaySearchWideScreen();
                mHandler.post(() -> {
                    if (mSurfaceControlViewHost != null
                            && wideScreenImeContentAreaViewContainer != null
                            && mSurfaceControlViewHost.getView() == null) {
                        // set the container with app's view into the Surface view.
                        mIsImeWidescreenViewSet = true;
                        mSurfaceControlViewHost.setView(
                                wideScreenImeContentAreaViewContainer, mSurfaceWidth,
                                mSurfaceHeight);
                    }
                });
            } else {
                removeView();
                mIsImeWidescreenViewSet = false;
            }
            return v.onApplyWindowInsets(insets);
        };

        // We need to add the window insets changed listener to the root view.
        // The text view may not be part of the view hierarchy yet, so in that case
        // add a listener for when it's attached to the window and then set the window insets
        // listener.
        // The listener has code that requires R, so only register it if the system is on R.
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R && mTextView != null) {
            if (mTextView.isAttachedToWindow()) {
                mTextView.getRootView().setOnApplyWindowInsetsListener(onApplyWindowInsetsListener);
            } else {
                mTextView.addOnAttachStateChangeListener(new View.OnAttachStateChangeListener() {
                    @Override
                    public void onViewAttachedToWindow(View v) {
                        mTextView.getRootView().setOnApplyWindowInsetsListener(
                                onApplyWindowInsetsListener);
                        mTextView.removeOnAttachStateChangeListener(this);
                    }

                    @Override
                    public void onViewDetachedFromWindow(View v) {
                    }
                });
            }
        }
    }

    private void displaySearchWideScreen() {
        Uri contentUri = SearchResultsProvider.getSearchResultsTableUri(mContext);
        // clear the table.
        mContext.getContentResolver().delete(contentUri, null, null);

        // If the app requested showing a view and we support it, we will show that instead
        // of the templatized list items.
        if (getSearchResultsView() != null && getSearchCapabilities().canShowSearchResultsView()) {
            return;
        }

        List<? extends CarUiImeSearchListItem> searchItems = mSearchConfig.getSearchResultItems();
        if (searchItems == null) {
            mInputMethodManager.sendAppPrivateCommand(mTextView, WIDE_SCREEN_ACTION, null);
            return;
        }

        for (int id = 0; id < searchItems.size(); id++) {
            CarUiImeSearchListItem item = searchItems.get(id);
            ContentValues values = new ContentValues();
            values.put(SearchResultsProvider.ITEM_ID, id);
            values.put(SearchResultsProvider.SECONDARY_IMAGE_ID, id);
            BitmapDrawable icon = (BitmapDrawable) item.getIcon();
            values.put(SearchResultsProvider.PRIMARY_IMAGE_BLOB,
                    icon != null ? bitmapToByteArray(icon.getBitmap()) : null);
            BitmapDrawable supplementalIcon = (BitmapDrawable) item.getSupplementalIcon();
            values.put(SearchResultsProvider.SECONDARY_IMAGE_BLOB,
                    supplementalIcon != null ? bitmapToByteArray(supplementalIcon.getBitmap())
                            : null);
            values.put(SearchResultsProvider.TITLE,
                    item.getTitle() != null ? item.getTitle().getPreferredText().toString() : null);
            values.put(SearchResultsProvider.SUBTITLE,
                    item.getBody() != null ? CarUiText.combineMultiLine(item.getBody()).toString()
                            : null);
            mContext.getContentResolver().insert(contentUri, values);
        }
        mInputMethodManager.sendAppPrivateCommand(mTextView, WIDE_SCREEN_SEARCH_RESULTS,
                new Bundle());
    }

    /**
     * Remove the app's view from the container and attach it back to its original parent.
     */
    private void removeView() {
        View contentView = getSearchResultsView();
        if (mOriginalParent != null && contentView != null) {
            mHandler.post(() -> {
                ViewGroup parent = (ViewGroup) contentView.getParent();
                if (parent != null) {
                    parent.removeView(contentView);
                }
                mOriginalParent.addView(contentView, mLayoutParams);
                mOriginalParent.requestLayout();
                mOriginalParent = null;
                mLayoutParams = null;
            });
        }
    }

    private void onItemClicked(String itemId) {
        List<? extends CarUiImeSearchListItem> items = mSearchConfig.getSearchResultItems();
        CarUiImeSearchListItem item = items == null
                ? null
                : items.get(Integer.parseInt(itemId));
        if (item != null) {
            CarUiContentListItem.OnClickListener listener = item.getOnClickListener();
            if (listener != null) {
                listener.onClick(item);
            }
        }
    }

    private void onSecondaryImageClicked(String secondaryImageId) {
        List<? extends CarUiImeSearchListItem> items = mSearchConfig.getSearchResultItems();
        CarUiImeSearchListItem item = items == null
                ? null
                : items.get(Integer.parseInt(secondaryImageId));
        if (item != null) {
            CarUiContentListItem.OnClickListener listener =
                    item.getSupplementalIconOnClickListener();
            if (listener != null) {
                listener.onClick(item);
            }
        }
    }

    private void onSurfaceInfo(int displayId, IBinder binder, int width, int height) {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.R) {
            // Other code like getSearchCapabilities().canShowSearchResultsView() returning false
            // should prevent us from getting here.
            throw new IllegalStateException("Views in the widescreen ime aren't supported pre R");
        }

        DisplayManager dm = mContext.getSystemService(DisplayManager.class);

        Display display = dm.getDisplay(displayId);

        mSurfaceControlViewHost = new SurfaceControlViewHost(mContext,
                display, binder);

        mSurfaceHeight = height;
        mSurfaceWidth = width;

        Bundle bundle = new Bundle();
        bundle.putParcelable(CONTENT_AREA_SURFACE_PACKAGE,
                mSurfaceControlViewHost.getSurfacePackage());
        mInputMethodManager.sendAppPrivateCommand(mTextView, WIDE_SCREEN_ACTION, bundle);
    }

    private void reLayout(int width, int height) {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.R) {
            // Other code like getSearchCapabilities().canShowSearchResultsView() returning false
            // should prevent us from getting here.
            throw new IllegalStateException("Views in the widescreen ime aren't supported pre R");
        }
        mSurfaceHeight = height;
        mSurfaceWidth = width;

        if (mSurfaceControlViewHost != null) {
            mSurfaceControlViewHost.relayout(width, height);
        }
    }

    private void onPostLoadSearchResults() {
        Uri contentUri = SearchResultsProvider.getSearchResultsTableUri(mContext);
        mContext.getContentResolver().delete(contentUri, null, null);
    }

    /**
     * Gets a listener that should be called when the text view passed to {@link #setTextView}
     * receives a call to it's {@link TextView#onPrivateIMECommand}
     */
    public BiConsumer<String, Bundle> getOnPrivateImeCommandListener() {
        return (action, data) -> {
            if (WIDE_SCREEN_CLEAR_DATA_ACTION.equals(action) && mTextView != null) {
                // clear the text.
                mTextView.setText("");
            }

            if (WIDE_SCREEN_POST_LOAD_SEARCH_RESULTS_ACTION.equals(action)) {
                onPostLoadSearchResults();
            }

            if (data == null) {
                return;
            }

            if (data.getString(SEARCH_RESULT_ITEM_ID_LIST) != null) {
                onItemClicked(data.getString(SEARCH_RESULT_ITEM_ID_LIST));
            }

            if (data.getString(SEARCH_RESULT_SUPPLEMENTAL_ICON_ID_LIST) != null) {
                onSecondaryImageClicked(data.getString(SEARCH_RESULT_SUPPLEMENTAL_ICON_ID_LIST));
            }

            int displayId = data.getInt(CONTENT_AREA_SURFACE_DISPLAY_ID);
            int height = data.getInt(CONTENT_AREA_SURFACE_HEIGHT);
            int width = data.getInt(CONTENT_AREA_SURFACE_WIDTH);
            IBinder binder = data.getBinder(CONTENT_AREA_SURFACE_HOST_TOKEN);

            if (binder != null) {
                onSurfaceInfo(displayId, binder, width, height);
                return;
            }

            if (height != 0 || width != 0) {
                reLayout(width, height);
            }
        };
    }

    /**
     * Gets the {@link SearchCapabilities} that the system currently supports.
     *
     * This non-static version does not require a context to be passed in explicitly.
     */
    public SearchCapabilities getSearchCapabilities() {
        return getSearchCapabilities(mContext);
    }

    /**
     * Gets the {@link SearchCapabilities} that the system currently supports.
     */
    public static SearchCapabilities getSearchCapabilities(Context context) {
        boolean isWideScreenMode = getBooleanSystemProperty(context.getResources(),
                R.string.car_ui_ime_wide_screen_system_property_name, false)
                && Build.VERSION.SDK_INT >= Build.VERSION_CODES.R;

        boolean allowAllAppsToShowSearchResultsView = context.getResources().getBoolean(
                R.bool.car_ui_ime_wide_screen_allow_app_hide_content_area);

        String[] allowedPackageNames = context.getResources()
                .getStringArray(R.array.car_ui_ime_wide_screen_allowed_package_list);
        boolean packageNameAllowed = false;
        for (String name : allowedPackageNames) {
            if (name.equals(context.getPackageName())) {
                packageNameAllowed = true;
                break;
            }
        }

        boolean canShowSearchResultsView = isWideScreenMode
                && (packageNameAllowed || allowAllAppsToShowSearchResultsView);

        return SearchCapabilities.builder()
                .setCanShowSearchResultsView(canShowSearchResultsView)
                .setCanShowSearchResultItems(isWideScreenMode)
                .build();
    }

    private static byte[] bitmapToByteArray(Bitmap bitmap) {
        Parcel parcel = Parcel.obtain();
        bitmap.writeToParcel(parcel, 0);
        byte[] bytes = parcel.marshall();
        parcel.recycle();
        return bytes;
    }
}
