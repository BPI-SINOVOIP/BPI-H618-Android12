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
package com.android.car.ui.recyclerview;

import static com.android.car.ui.utils.CarUiUtils.requireViewByRefId;
import static com.android.car.ui.utils.RotaryConstants.ROTARY_CONTAINER;
import static com.android.car.ui.utils.RotaryConstants.ROTARY_HORIZONTALLY_SCROLLABLE;
import static com.android.car.ui.utils.RotaryConstants.ROTARY_VERTICALLY_SCROLLABLE;
import static com.android.car.ui.utils.ViewUtils.LazyLayoutView;
import static com.android.car.ui.utils.ViewUtils.setRotaryScrollEnabled;

import android.car.drivingstate.CarUxRestrictions;
import android.content.Context;
import android.content.res.TypedArray;
import android.graphics.Rect;
import android.os.Parcelable;
import android.text.TextUtils;
import android.util.AttributeSet;
import android.view.InputDevice;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewPropertyAnimator;
import android.widget.FrameLayout;
import android.widget.LinearLayout;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.recyclerview.widget.GridLayoutManager;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import com.android.car.ui.R;
import com.android.car.ui.recyclerview.decorations.grid.GridDividerItemDecoration;
import com.android.car.ui.recyclerview.decorations.grid.GridOffsetItemDecoration;
import com.android.car.ui.recyclerview.decorations.linear.LinearDividerItemDecoration;
import com.android.car.ui.recyclerview.decorations.linear.LinearOffsetItemDecoration;
import com.android.car.ui.recyclerview.decorations.linear.LinearOffsetItemDecoration.OffsetPosition;
import com.android.car.ui.utils.CarUxRestrictionsUtil;

import java.lang.reflect.Constructor;
import java.util.HashSet;
import java.util.Objects;
import java.util.Set;

/**
 * View that extends a {@link RecyclerView} and wraps itself into a {@link LinearLayout} which could
 * potentially include a scrollbar that has page up and down arrows. Interaction with this view is
 * similar to a {@code RecyclerView} as it takes the same adapter and the layout manager.
 */
public final class CarUiRecyclerViewImpl extends CarUiRecyclerView implements LazyLayoutView {

    private static final String TAG = "CarUiRecyclerView";

    private final CarUxRestrictionsUtil.OnUxRestrictionsChangedListener mListener =
            new UxRestrictionChangedListener();

    @NonNull
    private final CarUxRestrictionsUtil mCarUxRestrictionsUtil;
    private boolean mScrollBarEnabled;
    @Nullable
    private String mScrollBarClass;
    private int mScrollBarPaddingTop;
    private int mScrollBarPaddingBottom;
    @Nullable
    private ScrollBar mScrollBar;

    @Nullable
    private GridOffsetItemDecoration mTopOffsetItemDecorationGrid;
    @Nullable
    private GridOffsetItemDecoration mBottomOffsetItemDecorationGrid;
    @Nullable
    private RecyclerView.ItemDecoration mTopOffsetItemDecorationLinear;
    @Nullable
    private RecyclerView.ItemDecoration mBottomOffsetItemDecorationLinear;
    @Nullable
    private GridDividerItemDecoration mDividerItemDecorationGrid;
    @Nullable
    private RecyclerView.ItemDecoration mDividerItemDecorationLinear;
    private int mNumOfColumns;
    private boolean mInstallingExtScrollBar = false;
    private int mContainerVisibility = View.VISIBLE;
    @Nullable
    private Rect mContainerPadding;
    @Nullable
    private Rect mContainerPaddingRelative;
    @Nullable
    private ViewGroup mContainer;
    @Size
    private int mSize;

    // Set to true when when styled attributes are read and initialized.
    private boolean mIsInitialized;
    private boolean mEnableDividers;

    private boolean mHasScrolled = false;

    @NonNull
    private final Set<Runnable> mOnLayoutCompletedListeners = new HashSet<>();

    private OnScrollListener mOnScrollListener = new OnScrollListener() {
        @Override
        public void onScrolled(@NonNull RecyclerView recyclerView, int dx, int dy) {
            if (dx > 0 || dy > 0) {
                mHasScrolled = true;
                removeOnScrollListener(this);
            }
        }
    };

    public CarUiRecyclerViewImpl(@NonNull Context context) {
        this(context, null);
    }

    public CarUiRecyclerViewImpl(@NonNull Context context, @Nullable AttributeSet attrs) {
        this(context, attrs, R.attr.carUiRecyclerViewStyle);
    }

    public CarUiRecyclerViewImpl(@NonNull Context context, @Nullable AttributeSet attrs,
            int defStyle) {
        super(context, attrs, defStyle);
        mCarUxRestrictionsUtil = CarUxRestrictionsUtil.getInstance(context);
        init(context, attrs, defStyle);
    }

    private void init(Context context, AttributeSet attrs, int defStyleAttr) {
        setClipToPadding(false);
        TypedArray a = context.obtainStyledAttributes(
                attrs,
                R.styleable.CarUiRecyclerView,
                defStyleAttr,
                R.style.Widget_CarUi_CarUiRecyclerView);
        initRotaryScroll(a);

        mScrollBarEnabled = context.getResources().getBoolean(R.bool.car_ui_scrollbar_enable);

        mScrollBarPaddingTop = context.getResources()
                .getDimensionPixelSize(R.dimen.car_ui_scrollbar_padding_top);
        mScrollBarPaddingBottom = context.getResources()
                .getDimensionPixelSize(R.dimen.car_ui_scrollbar_padding_bottom);

        @CarUiRecyclerViewLayout int carUiRecyclerViewLayout =
                a.getInt(R.styleable.CarUiRecyclerView_layoutStyle, CarUiRecyclerViewLayout.LINEAR);
        mNumOfColumns = a.getInt(R.styleable.CarUiRecyclerView_numOfColumns, /* defValue= */ 2);
        mEnableDividers =
                a.getBoolean(R.styleable.CarUiRecyclerView_enableDivider, /* defValue= */ false);

        mDividerItemDecorationLinear = new LinearDividerItemDecoration(
                context.getDrawable(R.drawable.car_ui_recyclerview_divider));

        mDividerItemDecorationGrid =
                new GridDividerItemDecoration(
                        context.getDrawable(R.drawable.car_ui_divider),
                        context.getDrawable(R.drawable.car_ui_divider),
                        mNumOfColumns);

        mTopOffsetItemDecorationLinear =
                new LinearOffsetItemDecoration(0, OffsetPosition.START);
        mBottomOffsetItemDecorationLinear =
                new LinearOffsetItemDecoration(0, OffsetPosition.END);
        mTopOffsetItemDecorationGrid =
                new GridOffsetItemDecoration(0, mNumOfColumns,
                        OffsetPosition.START);
        mBottomOffsetItemDecorationGrid =
                new GridOffsetItemDecoration(0, mNumOfColumns,
                        OffsetPosition.END);

        mIsInitialized = true;

        // Check if a layout manager has already been set via XML
        boolean isLayoutMangerSet = getLayoutManager() != null;
        if (!isLayoutMangerSet && carUiRecyclerViewLayout
                == CarUiRecyclerView.CarUiRecyclerViewLayout.LINEAR) {
            setLayoutManager(new LinearLayoutManager(getContext()) {
                @Override
                public void onLayoutCompleted(RecyclerView.State state) {
                    super.onLayoutCompleted(state);
                    // Iterate through a copied set instead of the original set because the original
                    // set might be modified during iteration.
                    Set<Runnable> onLayoutCompletedListeners =
                        new HashSet<>(mOnLayoutCompletedListeners);
                    for (Runnable runnable : onLayoutCompletedListeners) {
                        runnable.run();
                    }
                }
            });
        } else if (!isLayoutMangerSet && carUiRecyclerViewLayout
                == CarUiRecyclerView.CarUiRecyclerViewLayout.GRID) {
            setLayoutManager(new GridLayoutManager(getContext(), mNumOfColumns) {
                @Override
                public void onLayoutCompleted(RecyclerView.State state) {
                    super.onLayoutCompleted(state);
                    // Iterate through a copied set instead of the original set because the original
                    // set might be modified during iteration.
                    Set<Runnable> onLayoutCompletedListeners =
                        new HashSet<>(mOnLayoutCompletedListeners);
                    for (Runnable runnable : onLayoutCompletedListeners) {
                        runnable.run();
                    }
                }
            });
        }
        addOnScrollListener(mOnScrollListener);

        mSize = a.getInt(R.styleable.CarUiRecyclerView_carUiSize, SIZE_LARGE);

        a.recycle();

        if (!mScrollBarEnabled) {
            return;
        }

        mContainer = new FrameLayout(getContext());

        setVerticalScrollBarEnabled(false);
        setHorizontalScrollBarEnabled(false);

        mScrollBarClass = context.getResources().getString(R.string.car_ui_scrollbar_component);
    }

    @Override
    public void setLayoutManager(@Nullable LayoutManager layoutManager) {
        // Cannot setup item decorations before stylized attributes have been read.
        if (mIsInitialized) {
            addItemDecorations(layoutManager);
        }
        super.setLayoutManager(layoutManager);
    }

    @Override
    public void setLayoutStyle(CarUiLayoutStyle layoutStyle) {
        LayoutManager layoutManager;
        if (layoutStyle.getLayoutType() == CarUiRecyclerViewLayout.LINEAR) {
            layoutManager = new LinearLayoutManager(getContext(),
                    layoutStyle.getOrientation(),
                    layoutStyle.getReverseLayout()) {
                @Override
                public void onLayoutCompleted(RecyclerView.State state) {
                    super.onLayoutCompleted(state);
                    // Iterate through a copied set instead of the original set because the original
                    // set might be modified during iteration.
                    Set<Runnable> onLayoutCompletedListeners =
                        new HashSet<>(mOnLayoutCompletedListeners);
                    for (Runnable runnable : onLayoutCompletedListeners) {
                        runnable.run();
                    }
                }
            };
        } else {
            layoutManager = new GridLayoutManager(getContext(),
                    layoutStyle.getSpanCount(),
                    layoutStyle.getOrientation(),
                    layoutStyle.getReverseLayout()) {
                @Override
                public void onLayoutCompleted(RecyclerView.State state) {
                    super.onLayoutCompleted(state);
                    // Iterate through a copied set instead of the original set because the original
                    // set might be modified during iteration.
                    Set<Runnable> onLayoutCompletedListeners =
                        new HashSet<>(mOnLayoutCompletedListeners);
                    for (Runnable runnable : onLayoutCompletedListeners) {
                        runnable.run();
                    }
                }
            };
            // TODO(b/190444037): revisit usage of LayoutStyles and their casting
            if (layoutStyle instanceof CarUiGridLayoutStyle) {
                ((GridLayoutManager) layoutManager).setSpanSizeLookup(
                        ((CarUiGridLayoutStyle) layoutStyle).getSpanSizeLookup());
            }
        }
        setLayoutManager(layoutManager);
    }

    /**
     * {@inheritDoc}
     * <p>
     * Note that this method will never return true if this view has no items in it's adapter. This
     * is fine since an RecyclerView with empty items is not able to restore focus inside it.
     */
    @Override
    public boolean isLayoutCompleted() {
        RecyclerView.Adapter adapter = getAdapter();
        return adapter != null && adapter.getItemCount() > 0 && !isComputingLayout();
    }

    @Override
    public void addOnLayoutCompleteListener(@Nullable Runnable runnable) {
        if (runnable != null) {
            mOnLayoutCompletedListeners.add(runnable);
        }
    }

    @Override
    public void removeOnLayoutCompleteListener(@Nullable Runnable runnable) {
        if (runnable != null) {
            mOnLayoutCompletedListeners.remove(runnable);
        }
    }

    @Override
    public View getContainer() {
        return mContainer;
    }

    // This method should not be invoked before item decorations are initialized by the #init()
    // method.
    private void addItemDecorations(LayoutManager layoutManager) {
        // remove existing Item decorations.
        removeItemDecoration(Objects.requireNonNull(mDividerItemDecorationGrid));
        removeItemDecoration(Objects.requireNonNull(mTopOffsetItemDecorationGrid));
        removeItemDecoration(Objects.requireNonNull(mBottomOffsetItemDecorationGrid));
        removeItemDecoration(Objects.requireNonNull(mDividerItemDecorationLinear));
        removeItemDecoration(Objects.requireNonNull(mTopOffsetItemDecorationLinear));
        removeItemDecoration(Objects.requireNonNull(mBottomOffsetItemDecorationLinear));

        if (layoutManager instanceof GridLayoutManager) {
            if (mEnableDividers) {
                addItemDecoration(Objects.requireNonNull(mDividerItemDecorationGrid));
            }
            addItemDecoration(Objects.requireNonNull(mTopOffsetItemDecorationGrid));
            addItemDecoration(Objects.requireNonNull(mBottomOffsetItemDecorationGrid));
            setNumOfColumns(((GridLayoutManager) layoutManager).getSpanCount());
        } else {
            if (mEnableDividers) {
                addItemDecoration(Objects.requireNonNull(mDividerItemDecorationLinear));
            }
            addItemDecoration(Objects.requireNonNull(mTopOffsetItemDecorationLinear));
            addItemDecoration(Objects.requireNonNull(mBottomOffsetItemDecorationLinear));
        }
    }

    /**
     * If this view's {@code rotaryScrollEnabled} attribute is set to true, sets the content
     * description so that the {@code RotaryService} will treat it as a scrollable container and
     * initializes this view accordingly.
     */
    private void initRotaryScroll(@Nullable TypedArray styledAttributes) {
        boolean rotaryScrollEnabled = styledAttributes != null && styledAttributes.getBoolean(
                R.styleable.CarUiRecyclerView_rotaryScrollEnabled, /* defValue=*/ false);
        if (rotaryScrollEnabled) {
            int orientation = styledAttributes
                    .getInt(R.styleable.CarUiRecyclerView_android_orientation,
                            LinearLayout.VERTICAL);
            setRotaryScrollEnabled(
                    this, /* isVertical= */ orientation == LinearLayout.VERTICAL);
        } else {
            CharSequence contentDescription = getContentDescription();
            rotaryScrollEnabled = contentDescription != null
                    && (ROTARY_HORIZONTALLY_SCROLLABLE.contentEquals(contentDescription)
                    || ROTARY_VERTICALLY_SCROLLABLE.contentEquals(contentDescription));
        }

        // If rotary scrolling is enabled, set a generic motion event listener to convert
        // SOURCE_ROTARY_ENCODER scroll events into SOURCE_MOUSE scroll events that RecyclerView
        // knows how to handle.
        setOnGenericMotionListener(rotaryScrollEnabled ? (v, event) -> {
            if (event.getAction() == MotionEvent.ACTION_SCROLL) {
                if (event.getSource() == InputDevice.SOURCE_ROTARY_ENCODER) {
                    MotionEvent mouseEvent = MotionEvent.obtain(event);
                    mouseEvent.setSource(InputDevice.SOURCE_MOUSE);
                    CarUiRecyclerViewImpl.super.onGenericMotionEvent(mouseEvent);
                    return true;
                }
            }
            return false;
        } : null);

        // If rotary scrolling is enabled, mark this view as focusable. This view will be focused
        // when no focusable elements are visible.
        setFocusable(rotaryScrollEnabled);

        // Focus this view before descendants so that the RotaryService can focus this view when it
        // wants to.
        setDescendantFocusability(ViewGroup.FOCUS_BEFORE_DESCENDANTS);

        // Disable the default focus highlight. No highlight should appear when this view is
        // focused.
        setDefaultFocusHighlightEnabled(false);

        // If rotary scrolling is enabled, set a focus change listener to highlight the scrollbar
        // thumb when this recycler view is focused, i.e. when no focusable descendant is visible.
        setOnFocusChangeListener(rotaryScrollEnabled ? (v, hasFocus) -> {
            if (mScrollBar != null) mScrollBar.setHighlightThumb(hasFocus);
        } : null);

        // This view is a rotary container if it's not a scrollable container.
        if (!rotaryScrollEnabled) {
            super.setContentDescription(ROTARY_CONTAINER);
        }
    }

    @Override
    protected void onRestoreInstanceState(Parcelable state) {
        super.onRestoreInstanceState(state);

        // If we're restoring an existing RecyclerView, consider
        // it as having already scrolled some.
        mHasScrolled = true;
    }

    @Override
    public void requestLayout() {
        super.requestLayout();
        if (mScrollBar != null) {
            mScrollBar.requestLayout();
        }
    }

    /**
     * Sets the number of columns in which grid needs to be divided.
     */
    private void setNumOfColumns(int numberOfColumns) {
        mNumOfColumns = numberOfColumns;
        if (mTopOffsetItemDecorationGrid != null) {
            mTopOffsetItemDecorationGrid.setNumOfColumns(mNumOfColumns);
        }
        if (mDividerItemDecorationGrid != null) {
            mDividerItemDecorationGrid.setNumOfColumns(mNumOfColumns);
        }
    }

    /**
     * Changes the visibility of the entire container. If the container is not present i.e scrollbar
     * is not visible then the visibility or Recyclerview is changed.
     */
    @Override
    public void setVisibility(int visibility) {
        super.setVisibility(visibility);
        mContainerVisibility = visibility;
        if (mContainer != null) {
            mContainer.setVisibility(visibility);
        }
    }

    @Override
    protected void onAttachedToWindow() {
        super.onAttachedToWindow();
        mCarUxRestrictionsUtil.register(mListener);
        if (mInstallingExtScrollBar || !mScrollBarEnabled) {
            return;
        }
        // When CarUiRV is detached from the current parent and attached to the container with
        // the scrollBar, onAttachedToWindow() will get called immediately when attaching the
        // CarUiRV to the container. This flag will help us keep track of this state and avoid
        // recursion. We also want to reset the state of this flag as soon as the container is
        // successfully attached to the CarUiRV's original parent.
        mInstallingExtScrollBar = true;
        installExternalScrollBar();
        mInstallingExtScrollBar = false;
    }

    /**
     * This method will detach the current recycler view from its parent and attach it to the
     * container which is a LinearLayout. Later the entire container is attached to the parent where
     * the recycler view was set with the same layout params.
     */
    private void installExternalScrollBar() {
        if (mContainer.getParent() != null) {
            // We've already installed the parent container.
            // onAttachToWindow() can be called multiple times, but on the second time
            // we will crash if we try to add mContainer as a child of a view again while
            // it already has a parent.
            return;
        }

        mContainer.removeAllViews();
        LayoutInflater inflater = LayoutInflater.from(getContext());

        switch (mSize) {
            case SIZE_SMALL:
                // Small layout is rendered without scrollbar
                return;
            case SIZE_MEDIUM:
                inflater.inflate(R.layout.car_ui_recycler_view_medium, mContainer, true);
                break;
            case SIZE_LARGE:
            default:
                inflater.inflate(R.layout.car_ui_recycler_view, mContainer, true);
        }

        mContainer.setVisibility(mContainerVisibility);

        if (mContainerPadding != null) {
            mContainer.setPadding(mContainerPadding.left, mContainerPadding.top,
                    mContainerPadding.right, mContainerPadding.bottom);
        } else if (mContainerPaddingRelative != null) {
            mContainer.setPaddingRelative(mContainerPaddingRelative.left,
                    mContainerPaddingRelative.top, mContainerPaddingRelative.right,
                    mContainerPaddingRelative.bottom);
        } else {
            mContainer.setPadding(getPaddingLeft(), /* top= */ 0,
                    getPaddingRight(), /* bottom= */ 0);
            setPadding(/* left= */ 0, getPaddingTop(),
                    /* right= */ 0, getPaddingBottom());
        }

        mContainer.setLayoutParams(getLayoutParams());
        ViewGroup parent = (ViewGroup) getParent();
        int index = parent.indexOfChild(this);
        parent.removeViewInLayout(this);

        FrameLayout.LayoutParams params = new FrameLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT);
        ((CarUiRecyclerViewContainer) requireViewByRefId(mContainer, R.id.car_ui_recycler_view))
                .addRecyclerView(this, params);
        parent.addView(mContainer, index);

        createScrollBarFromConfig(requireViewByRefId(mContainer, R.id.car_ui_scroll_bar));
    }

    private void createScrollBarFromConfig(@NonNull View scrollView) {
        Class<?> cls;
        try {
            cls = !TextUtils.isEmpty(mScrollBarClass)
                    ? getContext().getClassLoader().loadClass(mScrollBarClass)
                    : DefaultScrollBar.class;
        } catch (ReflectiveOperationException e) {
            throw new IllegalArgumentException("Error loading scroll bar component: "
                    + mScrollBarClass, e);
        }
        try {
            Constructor<?> cnst = cls.getDeclaredConstructor();
            cnst.setAccessible(true);
            mScrollBar = (ScrollBar) cnst.newInstance();
        } catch (ReflectiveOperationException e) {
            throw new IllegalArgumentException("Error creating scroll bar component: "
                    + mScrollBarClass, e);
        }

        mScrollBar.initialize(this, scrollView);

        setScrollBarPadding(mScrollBarPaddingTop, mScrollBarPaddingBottom);
    }

    @Override
    public void setAlpha(float value) {
        if (mScrollBarEnabled) {
            mContainer.setAlpha(value);
        } else {
            super.setAlpha(value);
        }
    }

    @Override
    public ViewPropertyAnimator animate() {
        return mScrollBarEnabled ? mContainer.animate() : super.animate();
    }

    @Override
    protected void onDetachedFromWindow() {
        super.onDetachedFromWindow();
        mCarUxRestrictionsUtil.unregister(mListener);
    }

    @Override
    public int getPaddingLeft() {
        if (mContainerPadding != null) {
            return mContainerPadding.left;
        }

        return super.getPaddingLeft();
    }

    @Override
    public int getPaddingRight() {
        if (mContainerPadding != null) {
            return mContainerPadding.right;
        }

        return super.getPaddingRight();
    }

    @Override
    public void setPadding(int left, int top, int right, int bottom) {
        mContainerPaddingRelative = null;
        if (mScrollBarEnabled) {
            boolean isAtStart = (mScrollBar != null && mScrollBar.isAtStart());
            super.setPadding(0, top, 0, bottom);
            if (!mHasScrolled || isAtStart) {
                // If we haven't scrolled, and thus are still at the top of the screen,
                // we should stay scrolled to the top after applying padding. Without this
                // scroll, the padding will start scrolled offscreen. We need the padding
                // to be onscreen to shift the content into a good visible range.
                scrollToPosition(0);
            }
            mContainerPadding = new Rect(left, 0, right, 0);
            if (mContainer != null) {
                mContainer.setPadding(left, 0, right, 0);
            }
            setScrollBarPadding(mScrollBarPaddingTop, mScrollBarPaddingBottom);
        } else {
            super.setPadding(left, top, right, bottom);
        }
    }

    @Override
    public void setPaddingRelative(int start, int top, int end, int bottom) {
        mContainerPadding = null;
        if (mScrollBarEnabled) {
            super.setPaddingRelative(0, top, 0, bottom);
            if (!mHasScrolled) {
                // If we haven't scrolled, and thus are still at the top of the screen,
                // we should stay scrolled to the top after applying padding. Without this
                // scroll, the padding will start scrolled offscreen. We need the padding
                // to be onscreen to shift the content into a good visible range.
                scrollToPosition(0);
            }
            mContainerPaddingRelative = new Rect(start, 0, end, 0);
            if (mContainer != null) {
                mContainer.setPaddingRelative(start, 0, end, 0);
            }
            setScrollBarPadding(mScrollBarPaddingTop, mScrollBarPaddingBottom);
        } else {
            super.setPaddingRelative(start, top, end, bottom);
        }
    }

    /**
     * Sets the scrollbar's padding top and bottom. This padding is applied in addition to the
     * padding of the RecyclerView.
     */
    private void setScrollBarPadding(int paddingTop, int paddingBottom) {
        if (mScrollBarEnabled) {
            mScrollBarPaddingTop = paddingTop;
            mScrollBarPaddingBottom = paddingBottom;

            if (mScrollBar != null) {
                mScrollBar.setPadding(paddingTop + getPaddingTop(),
                        paddingBottom + getPaddingBottom());
            }
        }
    }

    @Override
    public void setContentDescription(CharSequence contentDescription) {
        super.setContentDescription(contentDescription);
        initRotaryScroll(/* styledAttributes= */ null);
    }

    @Override
    public void setAdapter(@Nullable Adapter adapter) {
        if (mScrollBar != null) {
            // Make sure this is called before super so that scrollbar can get a reference to
            // the adapter using RecyclerView#getAdapter()
            mScrollBar.adapterChanged(adapter);
        }
        super.setAdapter(adapter);
    }

    private class UxRestrictionChangedListener implements
            CarUxRestrictionsUtil.OnUxRestrictionsChangedListener {

        @Override
        public void onRestrictionsChanged(@NonNull CarUxRestrictions carUxRestrictions) {
            Adapter<?> adapter = getAdapter();
            // If the adapter does not implement ItemCap, then the max items on it cannot be
            // updated.
            if (!(adapter instanceof CarUiRecyclerView.ItemCap)) {
                return;
            }

            int maxItems = CarUiRecyclerView.ItemCap.UNLIMITED;
            if ((carUxRestrictions.getActiveRestrictions()
                    & CarUxRestrictions.UX_RESTRICTIONS_LIMIT_CONTENT)
                    != 0) {
                maxItems = carUxRestrictions.getMaxCumulativeContentItems();
            }

            int originalCount = adapter.getItemCount();
            ((CarUiRecyclerView.ItemCap) adapter).setMaxItems(maxItems);
            int newCount = adapter.getItemCount();

            if (newCount == originalCount) {
                return;
            }

            if (newCount < originalCount) {
                adapter.notifyItemRangeRemoved(newCount, originalCount - newCount);
            } else {
                adapter.notifyItemRangeInserted(originalCount, newCount - originalCount);
            }
        }
    }
}
