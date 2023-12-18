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
package com.android.wallpaper.picker;

import static com.android.wallpaper.picker.WallpaperPickerDelegate.PREVIEW_LIVE_WALLPAPER_REQUEST_CODE;
import static com.android.wallpaper.picker.WallpaperPickerDelegate.PREVIEW_WALLPAPER_REQUEST_CODE;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.Intent;
import android.graphics.Point;
import android.graphics.PorterDuff;
import android.graphics.Rect;
import android.net.Uri;
import android.os.Bundle;
import android.provider.Settings;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.ProgressBar;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.cardview.widget.CardView;
import androidx.fragment.app.Fragment;
import androidx.recyclerview.widget.GridLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import com.android.wallpaper.R;
import com.android.wallpaper.asset.Asset;
import com.android.wallpaper.model.Category;
import com.android.wallpaper.model.CategoryProvider;
import com.android.wallpaper.model.LiveWallpaperInfo;
import com.android.wallpaper.model.WallpaperInfo;
import com.android.wallpaper.module.InjectorProvider;
import com.android.wallpaper.module.UserEventLogger;
import com.android.wallpaper.util.DeepLinkUtils;
import com.android.wallpaper.util.DisplayMetricsRetriever;
import com.android.wallpaper.util.ResourceUtils;
import com.android.wallpaper.util.SizeCalculator;
import com.android.wallpaper.widget.WallpaperPickerRecyclerViewAccessibilityDelegate;
import com.android.wallpaper.widget.WallpaperPickerRecyclerViewAccessibilityDelegate.BottomSheetHost;

import com.bumptech.glide.Glide;

import java.util.ArrayList;
import java.util.List;

/**
 * Displays the UI which contains the categories of the wallpaper.
 */
public class CategorySelectorFragment extends AppbarFragment {

    // The number of ViewHolders that don't pertain to category tiles.
    // Currently 2: one for the metadata section and one for the "Select wallpaper" header.
    private static final int NUM_NON_CATEGORY_VIEW_HOLDERS = 0;
    private static final int SETTINGS_APP_INFO_REQUEST_CODE = 1;
    private static final String TAG = "CategorySelectorFragment";

    /**
     * Interface to be implemented by an Fragment hosting a {@link CategorySelectorFragment}
     */
    public interface CategorySelectorFragmentHost {

        /**
         * Requests to show the Android custom photo picker for the sake of picking a photo
         * to set as the device's wallpaper.
         */
        void requestCustomPhotoPicker(MyPhotosStarter.PermissionChangedListener listener);

        /**
         * Shows the wallpaper page of the specific category.
         *
         * @param category the wallpaper's {@link Category}
         */
        void show(Category category);


        /**
         * Indicates if the host has toolbar to show the title. If it does, we should set the title
         * there.
         */
        boolean isHostToolbarShown();

        /**
         * Sets the title in the host's toolbar.
         */
        void setToolbarTitle(CharSequence title);

        /**
         * Fetches the wallpaper categories.
         */
        void fetchCategories();

        /**
         * Cleans up the listeners which will be notified when there's a package event.
         */
        void cleanUp();
    }

    private final CategoryProvider mCategoryProvider;

    private RecyclerView mImageGrid;
    private CategoryAdapter mAdapter;
    private ArrayList<Category> mCategories = new ArrayList<>();
    private Point mTileSizePx;
    private boolean mAwaitingCategories;
    private boolean mIsFeaturedCollectionAvailable;

    public CategorySelectorFragment() {
        mAdapter = new CategoryAdapter(mCategories);
        mCategoryProvider = InjectorProvider.getInjector().getCategoryProvider(getContext());
    }

    @Nullable
    @Override
    public View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container,
                             @Nullable Bundle savedInstanceState) {
        View view = inflater.inflate(R.layout.fragment_category_selector, container,
                /* attachToRoot= */ false);
        mImageGrid = view.findViewById(R.id.category_grid);
        mImageGrid.addItemDecoration(new GridPaddingDecoration(getResources().getDimensionPixelSize(
                R.dimen.grid_item_category_padding_horizontal)));

        mTileSizePx = SizeCalculator.getCategoryTileSize(getActivity());

        mImageGrid.setAdapter(mAdapter);

        GridLayoutManager gridLayoutManager = new GridLayoutManager(getActivity(),
                getNumColumns() * CategorySpanSizeLookup.DEFAULT_CATEGORY_SPAN_SIZE);
        gridLayoutManager.setSpanSizeLookup(new CategorySpanSizeLookup(mAdapter));
        mImageGrid.setLayoutManager(gridLayoutManager);
        mImageGrid.setAccessibilityDelegateCompat(
                new WallpaperPickerRecyclerViewAccessibilityDelegate(
                        mImageGrid, (BottomSheetHost) getParentFragment(), getNumColumns()));

        if (getCategorySelectorFragmentHost().isHostToolbarShown()) {
            view.findViewById(R.id.header_bar).setVisibility(View.GONE);
            getCategorySelectorFragmentHost().setToolbarTitle(getText(R.string.wallpaper_title));
        } else {
            setUpToolbar(view);
            setTitle(getText(R.string.wallpaper_title));
        }

        if (!DeepLinkUtils.isDeepLink(getActivity().getIntent())) {
            getCategorySelectorFragmentHost().fetchCategories();
        }

        // For nav bar edge-to-edge effect.
        view.setOnApplyWindowInsetsListener((v, windowInsets) -> {
            // For status bar height.
            v.setPadding(
                    v.getPaddingLeft(),
                    windowInsets.getSystemWindowInsetTop(),
                    v.getPaddingRight(),
                    v.getPaddingBottom());

            View gridView = v.findViewById(R.id.category_grid);
            gridView.setPadding(
                    gridView.getPaddingLeft(),
                    gridView.getPaddingTop(),
                    gridView.getPaddingRight(),
                    windowInsets.getSystemWindowInsetBottom());
            return windowInsets.consumeSystemWindowInsets();
        });
        return view;
    }

    @Override
    public void onDestroyView() {
        getCategorySelectorFragmentHost().cleanUp();
        super.onDestroyView();
    }

    /**
     * Inserts the given category into the categories list in priority order.
     */
    void addCategory(Category category, boolean loading) {
        // If not previously waiting for categories, enter the waiting state by showing the loading
        // indicator.
        if (loading && !mAwaitingCategories) {
            mAdapter.notifyItemChanged(getNumColumns());
            mAdapter.notifyItemInserted(getNumColumns());
            mAwaitingCategories = true;
        }
        // Not add existing category to category list
        if (mCategories.indexOf(category) >= 0) {
            updateCategory(category);
            return;
        }

        int priority = category.getPriority();

        int index = 0;
        while (index < mCategories.size() && priority >= mCategories.get(index).getPriority()) {
            index++;
        }

        mCategories.add(index, category);
        if (mAdapter != null) {
            // Offset the index because of the static metadata element at beginning of RecyclerView.
            mAdapter.notifyItemInserted(index + NUM_NON_CATEGORY_VIEW_HOLDERS);
        }
    }

    void removeCategory(Category category) {
        int index = mCategories.indexOf(category);
        if (index >= 0) {
            mCategories.remove(index);
            mAdapter.notifyItemRemoved(index + NUM_NON_CATEGORY_VIEW_HOLDERS);
        }
    }

    void updateCategory(Category category) {
        int index = mCategories.indexOf(category);
        if (index >= 0) {
            mCategories.remove(index);
            mCategories.add(index, category);
            mAdapter.notifyItemChanged(index + NUM_NON_CATEGORY_VIEW_HOLDERS);
        }
    }

    void clearCategories() {
        mCategories.clear();
        mAdapter.notifyDataSetChanged();
    }

    /**
     * Notifies the CategoryFragment that no further categories are expected so it may hide
     * the loading indicator.
     */
    void doneFetchingCategories() {
        if (mAwaitingCategories) {
            mAdapter.notifyItemRemoved(mAdapter.getItemCount() - 1);
            mAwaitingCategories = false;
        }

        mIsFeaturedCollectionAvailable = mCategoryProvider.isFeaturedCollectionAvailable();
    }

    void notifyDataSetChanged() {
        mAdapter.notifyDataSetChanged();
    }

    private int getNumColumns() {
        Activity activity = getActivity();
        return activity == null ? 1 : SizeCalculator.getNumCategoryColumns(activity);
    }


    private CategorySelectorFragmentHost getCategorySelectorFragmentHost() {
        Fragment parentFragment = getParentFragment();
        if (parentFragment != null) {
            return (CategorySelectorFragmentHost) parentFragment;
        } else {
            return (CategorySelectorFragmentHost) getActivity();
        }
    }

    /**
     * ViewHolder subclass for a category tile in the RecyclerView.
     */
    private class CategoryHolder extends RecyclerView.ViewHolder implements View.OnClickListener {
        private Category mCategory;
        private ImageView mImageView;
        private ImageView mOverlayIconView;
        private TextView mTitleView;

        CategoryHolder(View itemView) {
            super(itemView);
            itemView.setOnClickListener(this);

            mImageView = itemView.findViewById(R.id.image);
            mOverlayIconView = itemView.findViewById(R.id.overlay_icon);
            mTitleView = itemView.findViewById(R.id.category_title);

            CardView categoryView = itemView.findViewById(R.id.category);
            categoryView.getLayoutParams().height = mTileSizePx.y;
            categoryView.setRadius(getResources().getDimension(R.dimen.grid_item_all_radius_small));
        }

        @Override
        public void onClick(View view) {
            Activity activity = getActivity();
            final UserEventLogger eventLogger =
                    InjectorProvider.getInjector().getUserEventLogger(activity);
            eventLogger.logCategorySelected(mCategory.getCollectionId());

            if (mCategory.supportsCustomPhotos()) {
                getCategorySelectorFragmentHost().requestCustomPhotoPicker(
                        new MyPhotosStarter.PermissionChangedListener() {
                            @Override
                            public void onPermissionsGranted() {
                                drawThumbnailAndOverlayIcon();
                            }

                            @Override
                            public void onPermissionsDenied(boolean dontAskAgain) {
                                // No-op
                            }
                        });
                return;
            }

            if (mCategory.isSingleWallpaperCategory()) {
                WallpaperInfo wallpaper = mCategory.getSingleWallpaper();
                // Log click on individual wallpaper
                eventLogger.logIndividualWallpaperSelected(mCategory.getCollectionId());

                InjectorProvider.getInjector().getWallpaperPersister(activity)
                        .setWallpaperInfoInPreview(wallpaper);
                wallpaper.showPreview(activity,
                        new PreviewActivity.PreviewActivityIntentFactory(),
                        wallpaper instanceof LiveWallpaperInfo ? PREVIEW_LIVE_WALLPAPER_REQUEST_CODE
                                : PREVIEW_WALLPAPER_REQUEST_CODE);
                return;
            }

            getCategorySelectorFragmentHost().show(mCategory);
        }

        /**
         * Binds the given category to this CategoryHolder.
         */
        private void bindCategory(Category category) {
            mCategory = category;
            mTitleView.setText(category.getTitle());
            drawThumbnailAndOverlayIcon();
        }

        /**
         * Draws the CategoryHolder's thumbnail and overlay icon.
         */
        private void drawThumbnailAndOverlayIcon() {
            mOverlayIconView.setImageDrawable(mCategory.getOverlayIcon(
                    getActivity().getApplicationContext()));

            // Size the overlay icon according to the category.
            int overlayIconDimenDp = mCategory.getOverlayIconSizeDp();
            DisplayMetrics metrics = DisplayMetricsRetriever.getInstance().getDisplayMetrics(
                    getResources(), getActivity().getWindowManager().getDefaultDisplay());
            int overlayIconDimenPx = (int) (overlayIconDimenDp * metrics.density);
            mOverlayIconView.getLayoutParams().width = overlayIconDimenPx;
            mOverlayIconView.getLayoutParams().height = overlayIconDimenPx;

            Asset thumbnail = mCategory.getThumbnail(getActivity().getApplicationContext());
            if (thumbnail != null) {
                thumbnail.loadDrawable(getActivity(), mImageView,
                        ResourceUtils.getColorAttr(
                            getActivity(),
                            android.R.attr.colorSecondary
                        ));
            } else {
                // TODO(orenb): Replace this workaround for b/62584914 with a proper way of
                //  unloading the ImageView such that no incorrect image is improperly loaded upon
                //  rapid scroll.
                Object nullObj = null;
                Glide.with(getActivity())
                        .asDrawable()
                        .load(nullObj)
                        .into(mImageView);

            }
        }
    }

    private class FeaturedCategoryHolder extends CategoryHolder {

        FeaturedCategoryHolder(View itemView) {
            super(itemView);
            CardView categoryView = itemView.findViewById(R.id.category);
            categoryView.getLayoutParams().height =
                    SizeCalculator.getFeaturedCategoryTileSize(getActivity()).y;
            categoryView.setRadius(getResources().getDimension(R.dimen.grid_item_all_radius));
        }
    }

    private class MyPhotosCategoryHolder extends CategoryHolder {

        MyPhotosCategoryHolder(View itemView) {
            super(itemView);
            // Reuse the height of featured category since My Photos category & featured category
            // have the same height in current UI design.
            CardView categoryView = itemView.findViewById(R.id.category);
            int height = SizeCalculator.getFeaturedCategoryTileSize(getActivity()).y;
            categoryView.getLayoutParams().height = height;
            // Use the height as the card corner radius for the "My photos" category
            // for a stadium border.
            categoryView.setRadius(height);
        }
    }

    /**
     * ViewHolder subclass for the loading indicator ("spinner") shown when categories are being
     * fetched.
     */
    private class LoadingIndicatorHolder extends RecyclerView.ViewHolder {
        private LoadingIndicatorHolder(View view) {
            super(view);
            ProgressBar progressBar = view.findViewById(R.id.loading_indicator);
            progressBar.getIndeterminateDrawable().setColorFilter(
                    ResourceUtils.getColorAttr(
                            getActivity(),
                            android.R.attr.colorAccent
                    ), PorterDuff.Mode.SRC_IN);
        }
    }

    /**
     * RecyclerView Adapter subclass for the category tiles in the RecyclerView.
     */
    private class CategoryAdapter extends RecyclerView.Adapter<RecyclerView.ViewHolder>
            implements MyPhotosStarter.PermissionChangedListener {
        private static final int ITEM_VIEW_TYPE_MY_PHOTOS = 1;
        private static final int ITEM_VIEW_TYPE_FEATURED_CATEGORY = 2;
        private static final int ITEM_VIEW_TYPE_CATEGORY = 3;
        private static final int ITEM_VIEW_TYPE_LOADING_INDICATOR = 4;
        private List<Category> mCategories;

        private CategoryAdapter(List<Category> categories) {
            mCategories = categories;
        }

        @Override
        public int getItemViewType(int position) {
            if (mAwaitingCategories && position == getItemCount() - 1) {
                return ITEM_VIEW_TYPE_LOADING_INDICATOR;
            }

            if (position == 0) {
                return ITEM_VIEW_TYPE_MY_PHOTOS;
            }

            if (mIsFeaturedCollectionAvailable && (position == 1 || position == 2)) {
                return ITEM_VIEW_TYPE_FEATURED_CATEGORY;
            }

            return ITEM_VIEW_TYPE_CATEGORY;
        }

        @Override
        public RecyclerView.ViewHolder onCreateViewHolder(ViewGroup parent, int viewType) {
            LayoutInflater layoutInflater = LayoutInflater.from(getActivity());
            View view;

            switch (viewType) {
                case ITEM_VIEW_TYPE_LOADING_INDICATOR:
                    view = layoutInflater.inflate(R.layout.grid_item_loading_indicator,
                            parent, /* attachToRoot= */ false);
                    return new LoadingIndicatorHolder(view);
                case ITEM_VIEW_TYPE_MY_PHOTOS:
                    view = layoutInflater.inflate(R.layout.grid_item_category,
                            parent, /* attachToRoot= */ false);
                    return new MyPhotosCategoryHolder(view);
                case ITEM_VIEW_TYPE_FEATURED_CATEGORY:
                    view = layoutInflater.inflate(R.layout.grid_item_category,
                            parent, /* attachToRoot= */ false);
                    return new FeaturedCategoryHolder(view);
                case ITEM_VIEW_TYPE_CATEGORY:
                    view = layoutInflater.inflate(R.layout.grid_item_category,
                            parent, /* attachToRoot= */ false);
                    return new CategoryHolder(view);
                default:
                    Log.e(TAG, "Unsupported viewType " + viewType + " in CategoryAdapter");
                    return null;
            }
        }

        @Override
        public void onBindViewHolder(RecyclerView.ViewHolder holder, int position) {
            int viewType = getItemViewType(position);

            switch (viewType) {
                case ITEM_VIEW_TYPE_MY_PHOTOS:
                case ITEM_VIEW_TYPE_FEATURED_CATEGORY:
                case ITEM_VIEW_TYPE_CATEGORY:
                    // Offset position to get category index to account for the non-category view
                    // holders.
                    Category category = mCategories.get(position - NUM_NON_CATEGORY_VIEW_HOLDERS);
                    ((CategoryHolder) holder).bindCategory(category);
                    break;
                case ITEM_VIEW_TYPE_LOADING_INDICATOR:
                    // No op.
                    break;
                default:
                    Log.e(TAG, "Unsupported viewType " + viewType + " in CategoryAdapter");
            }
        }

        @Override
        public int getItemCount() {
            // Add to size of categories to account for the metadata related views.
            // Add 1 more for the loading indicator if not yet done loading.
            int size = mCategories.size() + NUM_NON_CATEGORY_VIEW_HOLDERS;
            if (mAwaitingCategories) {
                size += 1;
            }

            return size;
        }

        @Override
        public void onPermissionsGranted() {
            notifyDataSetChanged();
        }

        @Override
        public void onPermissionsDenied(boolean dontAskAgain) {
            if (!dontAskAgain) {
                return;
            }

            String permissionNeededMessage =
                    getString(R.string.permission_needed_explanation_go_to_settings);
            AlertDialog dialog = new AlertDialog.Builder(getActivity(), R.style.LightDialogTheme)
                    .setMessage(permissionNeededMessage)
                    .setPositiveButton(android.R.string.ok, null /* onClickListener */)
                    .setNegativeButton(
                            R.string.settings_button_label,
                            (dialogInterface, i) -> {
                                Intent appInfoIntent =
                                        new Intent(Settings.ACTION_APPLICATION_DETAILS_SETTINGS);
                                Uri uri = Uri.fromParts("package",
                                        getActivity().getPackageName(), /* fragment= */ null);
                                appInfoIntent.setData(uri);
                                startActivityForResult(
                                        appInfoIntent, SETTINGS_APP_INFO_REQUEST_CODE);
                            })
                    .create();
            dialog.show();
        }
    }

    private class GridPaddingDecoration extends RecyclerView.ItemDecoration {

        private final int mPadding;

        GridPaddingDecoration(int padding) {
            mPadding = padding;
        }

        @Override
        public void getItemOffsets(Rect outRect, View view, RecyclerView parent,
                                   RecyclerView.State state) {
            int position = parent.getChildAdapterPosition(view) - NUM_NON_CATEGORY_VIEW_HOLDERS;
            if (position >= 0) {
                outRect.left = mPadding;
                outRect.right = mPadding;
            }

            RecyclerView.ViewHolder viewHolder = parent.getChildViewHolder(view);
            if (viewHolder instanceof MyPhotosCategoryHolder
                    || viewHolder instanceof FeaturedCategoryHolder) {
                outRect.bottom = getResources().getDimensionPixelSize(
                        R.dimen.grid_item_featured_category_padding_bottom);
            } else {
                outRect.bottom = getResources().getDimensionPixelSize(
                        R.dimen.grid_item_category_padding_bottom);
            }
        }
    }

    /**
     * SpanSizeLookup subclass which provides that the item in the first position spans the number
     * of columns in the RecyclerView and all other items only take up a single span.
     */
    private class CategorySpanSizeLookup extends GridLayoutManager.SpanSizeLookup {
        private static final int DEFAULT_CATEGORY_SPAN_SIZE = 2;

        CategoryAdapter mAdapter;

        private CategorySpanSizeLookup(CategoryAdapter adapter) {
            mAdapter = adapter;
        }

        @Override
        public int getSpanSize(int position) {
            if (position < NUM_NON_CATEGORY_VIEW_HOLDERS || mAdapter.getItemViewType(position)
                    == CategoryAdapter.ITEM_VIEW_TYPE_LOADING_INDICATOR || mAdapter.getItemViewType(
                    position) == CategoryAdapter.ITEM_VIEW_TYPE_MY_PHOTOS) {
                return getNumColumns() * DEFAULT_CATEGORY_SPAN_SIZE;
            }

            if (mAdapter.getItemViewType(position)
                    == CategoryAdapter.ITEM_VIEW_TYPE_FEATURED_CATEGORY) {
                return getNumColumns() * DEFAULT_CATEGORY_SPAN_SIZE / 2;
            }

            return DEFAULT_CATEGORY_SPAN_SIZE;
        }
    }
}
