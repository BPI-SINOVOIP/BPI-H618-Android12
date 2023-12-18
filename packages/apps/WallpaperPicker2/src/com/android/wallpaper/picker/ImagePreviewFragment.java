/*
 * Copyright (C) 2019 The Android Open Source Project
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

import static android.view.View.IMPORTANT_FOR_ACCESSIBILITY_NO_HIDE_DESCENDANTS;
import static android.view.View.IMPORTANT_FOR_ACCESSIBILITY_YES;
import static android.view.View.MeasureSpec.EXACTLY;
import static android.view.View.MeasureSpec.makeMeasureSpec;

import static com.android.wallpaper.widget.BottomActionBar.BottomAction.APPLY;
import static com.android.wallpaper.widget.BottomActionBar.BottomAction.EDIT;
import static com.android.wallpaper.widget.BottomActionBar.BottomAction.INFORMATION;

import android.animation.Animator;
import android.animation.AnimatorListenerAdapter;
import android.app.Activity;
import android.app.WallpaperColors;
import android.content.Context;
import android.content.res.Resources;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Color;
import android.graphics.ColorSpace;
import android.graphics.Point;
import android.graphics.PointF;
import android.graphics.Rect;
import android.os.Bundle;
import android.os.Handler;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.Surface;
import android.view.SurfaceControlViewHost;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewGroup.LayoutParams;
import android.widget.ImageView;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.cardview.widget.CardView;
import androidx.constraintlayout.widget.ConstraintLayout;
import androidx.constraintlayout.widget.ConstraintSet;
import androidx.fragment.app.FragmentActivity;

import com.android.wallpaper.R;
import com.android.wallpaper.asset.Asset;
import com.android.wallpaper.asset.CurrentWallpaperAssetVN;
import com.android.wallpaper.model.WallpaperInfo;
import com.android.wallpaper.module.BitmapCropper;
import com.android.wallpaper.module.InjectorProvider;
import com.android.wallpaper.module.WallpaperPersister.Destination;
import com.android.wallpaper.module.WallpaperPersister.SetWallpaperCallback;
import com.android.wallpaper.util.FullScreenAnimation;
import com.android.wallpaper.util.ResourceUtils;
import com.android.wallpaper.util.ScreenSizeCalculator;
import com.android.wallpaper.util.SizeCalculator;
import com.android.wallpaper.util.WallpaperCropUtils;
import com.android.wallpaper.widget.BottomActionBar;
import com.android.wallpaper.widget.BottomActionBar.AccessibilityCallback;
import com.android.wallpaper.widget.LockScreenPreviewer;

import com.bumptech.glide.Glide;
import com.bumptech.glide.MemoryCategory;
import com.davemorrissey.labs.subscaleview.ImageSource;
import com.davemorrissey.labs.subscaleview.SubsamplingScaleImageView;

import java.io.ByteArrayOutputStream;
import java.util.Locale;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.Executor;
import java.util.concurrent.Executors;
import java.util.concurrent.Future;
import java.util.concurrent.atomic.AtomicInteger;

/**
 * Fragment which displays the UI for previewing an individual static wallpaper and its attribution
 * information.
 */
public class ImagePreviewFragment extends PreviewFragment {

    private static final String TAG = "ImagePreviewFragment";
    private static final float DEFAULT_WALLPAPER_MAX_ZOOM = 8f;
    private static final Executor sExecutor = Executors.newCachedThreadPool();

    private final WallpaperSurfaceCallback mWallpaperSurfaceCallback =
            new WallpaperSurfaceCallback();

    private final AtomicInteger mImageScaleChangeCounter = new AtomicInteger(0);
    private final AtomicInteger mRecalculateColorCounter = new AtomicInteger(0);

    private SubsamplingScaleImageView mFullResImageView;
    private Asset mWallpaperAsset;
    private Point mScreenSize;
    private Point mRawWallpaperSize; // Native size of wallpaper image.
    private ImageView mLowResImageView;
    private TouchForwardingLayout mTouchForwardingLayout;
    private ConstraintLayout mContainer;
    private SurfaceView mWallpaperSurface;

    protected SurfaceView mWorkspaceSurface;
    protected WorkspaceSurfaceHolderCallback mWorkspaceSurfaceCallback;
    protected ViewGroup mLockPreviewContainer;
    protected LockScreenPreviewer mLockScreenPreviewer;
    private Future<Integer> mPlaceholderColorFuture;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        mWallpaperAsset = mWallpaper.getAsset(requireContext().getApplicationContext());
        mPlaceholderColorFuture = mWallpaper.computePlaceholderColor(requireContext());
    }

    @Override
    protected int getLayoutResId() {
        return R.layout.fragment_image_preview;
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
            Bundle savedInstanceState) {
        View view = super.onCreateView(inflater, container, savedInstanceState);

        Activity activity = requireActivity();
        mScreenSize = ScreenSizeCalculator.getInstance().getScreenSize(
                activity.getWindowManager().getDefaultDisplay());

        mContainer = view.findViewById(R.id.container);
        mTouchForwardingLayout = mContainer.findViewById(R.id.touch_forwarding_layout);
        mTouchForwardingLayout.setForwardingEnabled(true);

        // Set aspect ratio on the preview card dynamically.
        ConstraintSet set = new ConstraintSet();
        set.clone(mContainer);
        String ratio = String.format(Locale.US, "%d:%d", mScreenSize.x, mScreenSize.y);
        set.setDimensionRatio(mTouchForwardingLayout.getId(), ratio);
        set.applyTo(mContainer);

        mWorkspaceSurface = mContainer.findViewById(R.id.workspace_surface);
        mWorkspaceSurfaceCallback = createWorkspaceSurfaceCallback(mWorkspaceSurface);
        mWallpaperSurface = mContainer.findViewById(R.id.wallpaper_surface);
        mLockPreviewContainer = mContainer.findViewById(R.id.lock_screen_preview_container);
        int placeHolderColor = ResourceUtils.getColorAttr(getContext(),
                android.R.attr.colorBackground);
        mWorkspaceSurface.setResizeBackgroundColor(placeHolderColor);
        mLockScreenPreviewer = new LockScreenPreviewer(getLifecycle(), getContext(),
                mLockPreviewContainer);
        mLockScreenPreviewer.setDateViewVisibility(!mFullScreenAnimation.isFullScreen());
        mFullScreenAnimation.setFullScreenStatusListener(
                isFullScreen -> mLockScreenPreviewer.setDateViewVisibility(!isFullScreen));
        setUpTabs(view.findViewById(R.id.separated_tabs));

        view.measure(makeMeasureSpec(mScreenSize.x, EXACTLY),
                makeMeasureSpec(mScreenSize.y, EXACTLY));
        ((CardView) mWorkspaceSurface.getParent())
                .setRadius(SizeCalculator.getPreviewCornerRadius(
                        activity, mContainer.getMeasuredWidth()));

        renderImageWallpaper();
        renderWorkspaceSurface();

        // Trim some memory from Glide to make room for the full-size image in this fragment.
        Glide.get(activity).setMemoryCategory(MemoryCategory.LOW);

        return view;
    }

    @Override
    public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);
    }

    protected void onWallpaperColorsChanged(@Nullable WallpaperColors colors) {
        // Make it enabled since the buttons are disabled while wallpaper is moving.
        mBottomActionBar.enableActionButtonsWithBottomSheet(true);

        mLockScreenPreviewer.setColor(colors);

        mFullScreenAnimation.setFullScreenTextColor(
                colors == null || (colors.getColorHints()
                        & WallpaperColors.HINT_SUPPORTS_DARK_TEXT) == 0
                            ? FullScreenAnimation.FullScreenTextColor.LIGHT
                            : FullScreenAnimation.FullScreenTextColor.DARK);
    }

    @Override
    protected boolean isLoaded() {
        return mFullResImageView != null && mFullResImageView.hasImage();
    }

    @Override
    public void onClickOk() {
        FragmentActivity activity = getActivity();
        if (activity != null) {
            activity.finish();
        }
    }

    @Override
    public void onDestroy() {
        super.onDestroy();

        if (mFullResImageView != null) {
            mFullResImageView.recycle();
        }

        if (mLockScreenPreviewer != null) {
            mLockScreenPreviewer.release();
        }

        mWallpaperSurfaceCallback.cleanUp();
        mWorkspaceSurfaceCallback.cleanUp();
    }

    @Override
    protected void onBottomActionBarReady(BottomActionBar bottomActionBar) {
        super.onBottomActionBarReady(bottomActionBar);
        mBottomActionBar.bindBottomSheetContentWithAction(
                new WallpaperInfoContent(getContext()), INFORMATION);
        mBottomActionBar.showActionsOnly(INFORMATION, EDIT, APPLY);

        mBottomActionBar.setActionClickListener(APPLY, this::onSetWallpaperClicked);

        View separatedTabsContainer = getView().findViewById(R.id.separated_tabs_container);
        // Update target view's accessibility param since it will be blocked by the bottom sheet
        // when expanded.
        mBottomActionBar.setAccessibilityCallback(new AccessibilityCallback() {
            @Override
            public void onBottomSheetCollapsed() {
                mContainer.setImportantForAccessibility(IMPORTANT_FOR_ACCESSIBILITY_YES);
                separatedTabsContainer.setImportantForAccessibility(
                        IMPORTANT_FOR_ACCESSIBILITY_YES);
            }

            @Override
            public void onBottomSheetExpanded() {
                mContainer.setImportantForAccessibility(
                        IMPORTANT_FOR_ACCESSIBILITY_NO_HIDE_DESCENDANTS);
                separatedTabsContainer.setImportantForAccessibility(
                        IMPORTANT_FOR_ACCESSIBILITY_NO_HIDE_DESCENDANTS);

            }
        });

        mBottomActionBar.show();
        // To avoid applying the wallpaper when the wallpaper's not parsed.
        mBottomActionBar.disableActions();
        // If the wallpaper is parsed, enable the bottom action bar.
        if (mRawWallpaperSize != null) {
            mBottomActionBar.enableActions();
        }
    }

    /**
     * Initializes MosaicView by initializing tiling, setting a fallback page bitmap, and
     * initializing a zoom-scroll observer and click listener.
     */
    private void initFullResView() {
        if (mRawWallpaperSize == null || mFullResImageView == null) {
            return;
        }

        // Minimum scale will only be respected under this scale type.
        mFullResImageView.setMinimumScaleType(SubsamplingScaleImageView.SCALE_TYPE_CUSTOM);
        // When we set a minimum scale bigger than the scale with which the full image is shown,
        // disallow user to pan outside the view we show the wallpaper in.
        mFullResImageView.setPanLimit(SubsamplingScaleImageView.PAN_LIMIT_INSIDE);

        // Then set a fallback "page bitmap" to cover the whole MosaicView, which is an actual
        // (lower res) version of the image to be displayed.
        Point targetPageBitmapSize = new Point(mRawWallpaperSize);
        mWallpaperAsset.decodeBitmap(targetPageBitmapSize.x, targetPageBitmapSize.y,
                pageBitmap -> {
                    // Check that the activity is still around since the decoding task started.
                    if (getActivity() == null) {
                        return;
                    }

                    // The page bitmap may be null if there was a decoding error, so show an
                    // error dialog.
                    if (pageBitmap == null) {
                        showLoadWallpaperErrorDialog();
                        return;
                    }
                    // Some of these may be null depending on if the Fragment is paused, stopped,
                    // or destroyed.
                    mWallpaperSurface.setBackgroundColor(Color.TRANSPARENT);
                    if (mFullResImageView != null) {
                        // Set page bitmap.
                        mFullResImageView.setImage(ImageSource.bitmap(pageBitmap));

                        setDefaultWallpaperZoomAndScroll(
                                mWallpaperAsset instanceof CurrentWallpaperAssetVN);
                        crossFadeInMosaicView();
                        mFullResImageView.setOnStateChangedListener(
                                new SubsamplingScaleImageView.DefaultOnStateChangedListener() {
                                    @Override
                                    public void onCenterChanged(PointF newCenter, int origin) {
                                        super.onCenterChanged(newCenter, origin);
                                        // Disallow bottom sheet to popup when wallpaper is moving
                                        // by user dragging.
                                        mBottomActionBar.enableActionButtonsWithBottomSheet(false);
                                        mImageScaleChangeCounter.incrementAndGet();
                                        mFullResImageView.postDelayed(() -> {
                                            if (mImageScaleChangeCounter.decrementAndGet() == 0) {
                                                recalculateColors();
                                            }
                                        }, /* delayMillis= */ 100);
                                    }
                                });
                        mFullResImageView.post(this::recalculateColors);
                    }
                });

        mFullResImageView.setOnTouchListener((v, ev) -> {
            // Consume the touch event for collapsing bottom sheet while it is expanded or
            // dragging (not collapsed).
            if (mBottomActionBar != null && !mBottomActionBar.isBottomSheetCollapsed()) {
                mBottomActionBar.collapseBottomSheetIfExpanded();
                return true;
            }
            return false;
        });
    }

    private void recalculateColors() {
        Context context = getContext();
        if (context == null) {
            Log.e(TAG, "Got null context, skip recalculating colors");
            return;
        }

        BitmapCropper bitmapCropper = InjectorProvider.getInjector().getBitmapCropper();
        bitmapCropper.cropAndScaleBitmap(mWallpaperAsset, mFullResImageView.getScale(),
                calculateCropRect(context), /* adjustForRtl= */ false,
                new BitmapCropper.Callback() {
                    @Override
                    public void onBitmapCropped(Bitmap croppedBitmap) {
                        mRecalculateColorCounter.incrementAndGet();
                        sExecutor.execute(() -> {
                            boolean shouldRecycle = false;
                            ByteArrayOutputStream tmpOut = new ByteArrayOutputStream();
                            Bitmap cropped = croppedBitmap;
                            if (cropped.compress(Bitmap.CompressFormat.PNG, 100, tmpOut)) {
                                byte[] outByteArray = tmpOut.toByteArray();
                                BitmapFactory.Options options = new BitmapFactory.Options();
                                options.inPreferredColorSpace =
                                        ColorSpace.get(ColorSpace.Named.SRGB);
                                cropped = BitmapFactory.decodeByteArray(outByteArray, 0,
                                        outByteArray.length);
                            }
                            if (cropped.getConfig() == Bitmap.Config.HARDWARE) {
                                cropped = cropped.copy(Bitmap.Config.ARGB_8888, false);
                                shouldRecycle = true;
                            }
                            WallpaperColors colors = WallpaperColors.fromBitmap(cropped);
                            if (shouldRecycle) {
                                cropped.recycle();
                            }
                            if (mRecalculateColorCounter.decrementAndGet() == 0) {
                                Handler.getMain().post(() -> onWallpaperColorsChanged(colors));
                            }
                        });
                    }

                    @Override
                    public void onError(@Nullable Throwable e) {
                        Log.w(TAG, "Recalculate colors, crop and scale bitmap failed.", e);
                    }
                });
    }

    /**
     * Makes the MosaicView visible with an alpha fade-in animation while fading out the loading
     * indicator.
     */
    private void crossFadeInMosaicView() {
        long shortAnimationDuration = getResources().getInteger(
                android.R.integer.config_shortAnimTime);

        mFullResImageView.setAlpha(0f);
        mFullResImageView.animate()
                .alpha(1f)
                .setInterpolator(ALPHA_OUT)
                .setDuration(shortAnimationDuration)
                .setListener(new AnimatorListenerAdapter() {
                    @Override
                    public void onAnimationEnd(Animator animation) {
                        // Clear the thumbnail bitmap reference to save memory since it's no longer
                        // visible.
                        if (mLowResImageView != null) {
                            mLowResImageView.setImageBitmap(null);
                        }
                    }
                });
    }

    /**
     * Sets the default wallpaper zoom and scroll position based on a "crop surface" (with extra
     * width to account for parallax) superimposed on the screen. Shows as much of the wallpaper as
     * possible on the crop surface and align screen to crop surface such that the default preview
     * matches what would be seen by the user in the left-most home screen.
     *
     * <p>This method is called once in the Fragment lifecycle after the wallpaper asset has loaded
     * and rendered to the layout.
     *
     * @param offsetToStart {@code true} if we want to offset the visible rectangle to the start
     *                                  side of the raw wallpaper; {@code false} otherwise.
     */
    private void setDefaultWallpaperZoomAndScroll(boolean offsetToStart) {
        // Determine minimum zoom to fit maximum visible area of wallpaper on crop surface.
        int cropWidth = mWallpaperSurface.getMeasuredWidth();
        int cropHeight = mWallpaperSurface.getMeasuredHeight();
        Point crop = new Point(cropWidth, cropHeight);
        Rect visibleRawWallpaperRect =
                WallpaperCropUtils.calculateVisibleRect(mRawWallpaperSize, crop);
        if (offsetToStart) {
            if (WallpaperCropUtils.isRtl(requireContext())) {
                visibleRawWallpaperRect.offsetTo(mRawWallpaperSize.x
                                - visibleRawWallpaperRect.width(), visibleRawWallpaperRect.top);
            } else {
                visibleRawWallpaperRect.offsetTo(/* newLeft= */ 0, visibleRawWallpaperRect.top);
            }
        }

        final PointF centerPosition = new PointF(visibleRawWallpaperRect.centerX(),
                visibleRawWallpaperRect.centerY());

        Point visibleRawWallpaperSize = new Point(visibleRawWallpaperRect.width(),
                visibleRawWallpaperRect.height());

        final float defaultWallpaperZoom = WallpaperCropUtils.calculateMinZoom(
                visibleRawWallpaperSize, crop);
        final float minWallpaperZoom = defaultWallpaperZoom;


        // Set min wallpaper zoom and max zoom on MosaicView widget.
        mFullResImageView.setMaxScale(Math.max(DEFAULT_WALLPAPER_MAX_ZOOM, defaultWallpaperZoom));
        mFullResImageView.setMinScale(minWallpaperZoom);

        // Set center to composite positioning between scaled wallpaper and screen.
        mFullResImageView.setScaleAndCenter(minWallpaperZoom, centerPosition);
    }

    private Rect calculateCropRect(Context context) {
        float wallpaperZoom = mFullResImageView.getScale();
        Context appContext = context.getApplicationContext();

        Rect visibleFileRect = new Rect();
        mFullResImageView.visibleFileRect(visibleFileRect);

        int cropWidth = mWallpaperSurface.getMeasuredWidth();
        int cropHeight = mWallpaperSurface.getMeasuredHeight();
        int maxCrop = Math.max(cropWidth, cropHeight);
        int minCrop = Math.min(cropWidth, cropHeight);
        Point hostViewSize = new Point(cropWidth, cropHeight);

        Resources res = appContext.getResources();
        Point cropSurfaceSize = WallpaperCropUtils.calculateCropSurfaceSize(res, maxCrop, minCrop);

        return WallpaperCropUtils.calculateCropRect(appContext, hostViewSize,
                cropSurfaceSize, mRawWallpaperSize, visibleFileRect, wallpaperZoom);
    }

    @Override
    protected void setCurrentWallpaper(@Destination int destination) {
        mWallpaperSetter.setCurrentWallpaper(getActivity(), mWallpaper, mWallpaperAsset,
                destination, mFullResImageView.getScale(), calculateCropRect(getContext()),
                new SetWallpaperCallback() {
                    @Override
                    public void onSuccess(WallpaperInfo wallpaperInfo) {
                        finishActivity(/* success= */ true);
                    }

                    @Override
                    public void onError(@Nullable Throwable throwable) {
                        showSetWallpaperErrorDialog(destination);
                    }
                });
    }

    private void renderWorkspaceSurface() {
        mWorkspaceSurface.setZOrderMediaOverlay(true);
        mWorkspaceSurface.getHolder().addCallback(mWorkspaceSurfaceCallback);
    }

    private void renderImageWallpaper() {
        mWallpaperSurface.getHolder().addCallback(mWallpaperSurfaceCallback);
    }

    private class WallpaperSurfaceCallback implements SurfaceHolder.Callback {
        private Surface mLastSurface;
        private SurfaceControlViewHost mHost;

        @Override
        public void surfaceCreated(SurfaceHolder holder) {
            if (mLastSurface != holder.getSurface()) {
                mLastSurface = holder.getSurface();
                if (mFullResImageView != null) {
                    mFullResImageView.recycle();
                }
                Context context = getContext();
                View wallpaperPreviewContainer = LayoutInflater.from(context).inflate(
                        R.layout.fullscreen_wallpaper_preview, null);
                mFullResImageView = wallpaperPreviewContainer.findViewById(R.id.full_res_image);
                mLowResImageView = wallpaperPreviewContainer.findViewById(R.id.low_res_image);
                mWallpaperAsset.decodeRawDimensions(getActivity(), dimensions -> {
                    // Don't continue loading the wallpaper if the Fragment is detached.
                    if (getActivity() == null) {
                        return;
                    }

                    // Return early and show a dialog if dimensions are null (signaling a decoding
                    // error).
                    if (dimensions == null) {
                        showLoadWallpaperErrorDialog();
                        return;
                    }

                    // To avoid applying the wallpaper when it's not parsed. Now it's parsed, enable
                    // the bottom action bar to allow applying the wallpaper.
                    if (mBottomActionBar != null) {
                        mBottomActionBar.enableActions();
                    }

                    mRawWallpaperSize = dimensions;
                    initFullResView();
                });
                // Scale the mWallpaperSurface based on system zoom's scale so that the wallpaper is
                // rendered in a larger surface than what preview shows, simulating the behavior of
                // the actual wallpaper surface.
                float scale = WallpaperCropUtils.getSystemWallpaperMaximumScale(context);
                int origWidth = mWallpaperSurface.getWidth();
                int width = (int) (origWidth * scale);
                int origHeight = mWallpaperSurface.getHeight();
                int height = (int) (origHeight * scale);
                int left = (origWidth - width) / 2;
                int top = (origHeight - height) / 2;

                if (WallpaperCropUtils.isRtl(context)) {
                    left *= -1;
                }

                LayoutParams params = mWallpaperSurface.getLayoutParams();
                params.width = width;
                params.height = height;
                mWallpaperSurface.setX(left);
                mWallpaperSurface.setY(top);
                mWallpaperSurface.setLayoutParams(params);
                mWallpaperSurface.requestLayout();

                // Load a low-res placeholder image if there's a thumbnail available from the asset
                // that can be shown to the user more quickly than the full-sized image.
                Activity activity = requireActivity();
                int placeHolderColor = ResourceUtils.getColorAttr(activity,
                        android.R.attr.colorBackground);
                if (mPlaceholderColorFuture.isDone()) {
                    try {
                        placeHolderColor = mWallpaper.computePlaceholderColor(context).get();
                    } catch (InterruptedException | ExecutionException e) {
                        // Do nothing
                    }
                }
                mWallpaperSurface.setResizeBackgroundColor(placeHolderColor);
                mWallpaperSurface.setBackgroundColor(placeHolderColor);

                mWallpaperAsset.loadLowResDrawable(activity, mLowResImageView, placeHolderColor,
                        mPreviewBitmapTransformation);

                wallpaperPreviewContainer.measure(
                        makeMeasureSpec(width, EXACTLY),
                        makeMeasureSpec(height, EXACTLY));
                wallpaperPreviewContainer.layout(0, 0, width, height);
                mTouchForwardingLayout.setTargetView(mFullResImageView);

                cleanUp();
                mHost = new SurfaceControlViewHost(context,
                        context.getDisplay(), mWallpaperSurface.getHostToken());
                mHost.setView(wallpaperPreviewContainer, wallpaperPreviewContainer.getWidth(),
                        wallpaperPreviewContainer.getHeight());
                mWallpaperSurface.setChildSurfacePackage(mHost.getSurfacePackage());
            }
        }

        @Override
        public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) { }

        @Override
        public void surfaceDestroyed(SurfaceHolder holder) { }

        public void cleanUp() {
            if (mHost != null) {
                mHost.release();
                mHost = null;
            }
        }
    }

    @Override
    protected void updateScreenPreview(boolean isHomeSelected) {
        mWorkspaceSurface.setVisibility(isHomeSelected ? View.VISIBLE : View.INVISIBLE);

        mLockPreviewContainer.setVisibility(isHomeSelected ? View.INVISIBLE : View.VISIBLE);

        mFullScreenAnimation.setIsHomeSelected(isHomeSelected);
    }
}
