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

package com.android.car.ui.utils;

import static android.view.accessibility.AccessibilityNodeInfo.ACTION_FOCUS;

import static com.android.car.ui.utils.RotaryConstants.ROTARY_CONTAINER;
import static com.android.car.ui.utils.RotaryConstants.ROTARY_FOCUS_DELEGATING_CONTAINER;
import static com.android.car.ui.utils.RotaryConstants.ROTARY_HORIZONTALLY_SCROLLABLE;
import static com.android.car.ui.utils.RotaryConstants.ROTARY_VERTICALLY_SCROLLABLE;

import android.text.TextUtils;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewParent;

import androidx.annotation.IntDef;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.VisibleForTesting;

import com.android.car.ui.FocusArea;
import com.android.car.ui.FocusParkingView;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;

/**
 * Utility class used by {@link com.android.car.ui.FocusArea} and {@link
 * com.android.car.ui.FocusParkingView}.
 *
 * @hide
 */
public final class ViewUtils {

    /**
     * How many milliseconds to wait before trying to restore the focus inside the LazyLayoutView
     * the second time.
     */
    private static final int RESTORE_FOCUS_RETRY_DELAY_MS = 3000;

    /**
     * No view is focused, the focused view is not shown, or the focused view is a FocusParkingView.
     */
    @VisibleForTesting
    static final int NO_FOCUS = 1;

    /** A scrollable container is focused. */
    @VisibleForTesting
    static final int SCROLLABLE_CONTAINER_FOCUS = 2;

    /**
     * A regular view is focused. A regular View is a View that is neither a FocusParkingView nor a
     * scrollable container.
     */
    @VisibleForTesting
    static final int REGULAR_FOCUS = 3;

    /**
     * An implicit default focus view (i.e., the selected item or the first focusable item in a
     * scrollable container) is focused.
     */
    @VisibleForTesting
    static final int IMPLICIT_DEFAULT_FOCUS = 4;

    /** The {@code app:defaultFocus} view is focused. */
    @VisibleForTesting
    static final int DEFAULT_FOCUS = 5;

    /** The {@code android:focusedByDefault} view is focused. */
    @VisibleForTesting
    static final int FOCUSED_BY_DEFAULT = 6;

    /**
     * Focus level of a view. When adjusting the focus, the view with the highest focus level will
     * be focused.
     */
    @IntDef(flag = true, value = {NO_FOCUS, SCROLLABLE_CONTAINER_FOCUS, REGULAR_FOCUS,
            IMPLICIT_DEFAULT_FOCUS, DEFAULT_FOCUS, FOCUSED_BY_DEFAULT})
    @Retention(RetentionPolicy.SOURCE)
    private @interface FocusLevel {
    }

    /** This is a utility class. */
    private ViewUtils() {
    }

    /**
     * This is a functional interface and can therefore be used as the assignment target for a
     * lambda expression or method reference.
     *
     * @param <T> the type of the input to the predicate
     */
    private interface Predicate<T> {
        /** Evaluates this predicate on the given argument. */
        boolean test(@NonNull T t);
    }

    /**
     * An interface used to restore focus inside a view when its layout is completed.
     * <p>
     * The view that needs to restore focus lazily should implement this interface.
     */
    public interface LazyLayoutView {

        /**
         * Returnes whether the view's layout is completed and ready to restore focus inside it.
         */
        boolean isLayoutCompleted();

        /**
         * Adds a listener to be called when the view's layout is completed.
         */
        void addOnLayoutCompleteListener(@Nullable Runnable runnable);

        /**
         * Removes a listener to be called when the view's layout is completed.
         */
        void removeOnLayoutCompleteListener(@Nullable Runnable runnable);
    }

    /**
     * Hides the focus by searching the view tree for the {@link FocusParkingView}
     * and focusing on it.
     *
     * @param root the root view to search from
     * @return true if the FocusParkingView was successfully found and focused
     *         or if it was already focused
     */
    public static boolean hideFocus(@NonNull View root) {
        FocusParkingView fpv = (FocusParkingView) depthFirstSearch(root,
                /* targetPredicate= */ v -> v instanceof FocusParkingView,
                /* skipPredicate= */ null);
        if (fpv == null) {
            return false;
        }
        if (fpv.isFocused()) {
            return true;
        }
        return fpv.performAccessibilityAction(ACTION_FOCUS, /* arguments= */ null);
    }

    /** Gets the ancestor FocusArea of the {@code view}, if any. Returns null if not found. */
    @Nullable
    public static FocusArea getAncestorFocusArea(@NonNull View view) {
        ViewParent parent = view.getParent();
        while (parent != null) {
            if (parent instanceof FocusArea) {
                return (FocusArea) parent;
            }
            parent = parent.getParent();
        }
        return null;
    }

    /**
     * Gets the ancestor scrollable container of the {@code view}, if any. Returns null if not
     * found.
     */
    @Nullable
    public static ViewGroup getAncestorScrollableContainer(@Nullable View view) {
        if (view == null) {
            return null;
        }
        ViewParent parent = view.getParent();
        // A scrollable container can't contain a FocusArea, so let's return earlier if we found
        // a FocusArea.
        while (parent != null && parent instanceof ViewGroup && !(parent instanceof FocusArea)) {
            ViewGroup viewGroup = (ViewGroup) parent;
            if (isScrollableContainer(viewGroup)) {
                return viewGroup;
            }
            parent = parent.getParent();
        }
        return null;
    }

    /**
     * Focuses on the {@code view} if it can be focused.
     *
     * @return whether it was successfully focused or already focused
     */
    public static boolean requestFocus(@Nullable View view) {
        if (view == null || !canTakeFocus(view)) {
            return false;
        }
        if (view.isFocused()) {
            return true;
        }
        // Exit touch mode and focus the view. The view may not be focusable in touch mode, so we
        // need to exit touch mode before focusing it.
        return view.performAccessibilityAction(ACTION_FOCUS, /* arguments= */ null);
    }

    /**
     * Searches the {@code root}'s descendants for a view with the highest {@link FocusLevel}. If
     * the view's FocusLevel is higher than the {@code currentFocus}'s FocusLevel, focuses on the
     * view. If it tried to focus on a LazyLayoutView but failed, requests to adjust the focus
     * inside the LazyLayoutView later.
     *
     * @return whether the view is focused
     */
    public static boolean adjustFocus(@NonNull View root, @Nullable View currentFocus) {
        @FocusLevel int currentLevel = getFocusLevel(currentFocus);
        return adjustFocus(root, currentLevel, /* cachedFocusedView= */ null,
                /* defaultFocusOverridesHistory= */ false);
    }

    /**
     * Similar to {@link #adjustFocus(View, View)} but without requesting to adjust the focus
     * inside the LazyLayoutView later.
     */
    public static boolean adjustFocusImmediately(@NonNull View root, @Nullable View currentFocus) {
        @FocusLevel int currentLevel = getFocusLevel(currentFocus);
        return adjustFocus(root, currentLevel, /* cachedFocusedView= */ null,
                /* defaultFocusOverridesHistory= */ false, /* delayed= */ false);
    }

    /**
     * If the {@code currentFocus}'s FocusLevel is lower than REGULAR_FOCUS, adjusts focus within
     * {@code root}. See {@link #adjustFocus(View, int)}. Otherwise no-op.
     *
     * @return whether the focus has changed
     */
    public static boolean initFocus(@NonNull View root, @Nullable View currentFocus) {
        @FocusLevel int currentLevel = getFocusLevel(currentFocus);
        if (currentLevel >= REGULAR_FOCUS) {
            return false;
        }
        return adjustFocus(root, currentLevel, /* cachedFocusedView= */ null,
                /* defaultFocusOverridesHistory= */ false);
    }

    /**
     * Searches the {@code root}'s descendants for a view with the highest {@link FocusLevel}. If
     * the view's FocusLevel is higher than {@code currentLevel}, focuses on the view.
     *
     * @return whether the view is focused
     */
    @VisibleForTesting
    static boolean adjustFocus(@NonNull View root, @FocusLevel int currentLevel) {
        return adjustFocus(root, currentLevel, /* cachedFocusedView= */ null,
                /* defaultFocusOverridesHistory= */ false);
    }

    /**
     * Searches the {@code root}'s descendants for a view with the highest {@link FocusLevel} and
     * focuses on it or the {@code cachedFocusedView}.
     *
     * @return whether the view is focused
     */
    public static boolean adjustFocus(@NonNull View root,
            @Nullable View cachedFocusedView,
            boolean defaultFocusOverridesHistory) {
        return adjustFocus(root, NO_FOCUS, cachedFocusedView, defaultFocusOverridesHistory);
    }

    private static boolean adjustFocus(@NonNull View root,
            @FocusLevel int currentLevel,
            @Nullable View cachedFocusedView,
            boolean defaultFocusOverridesHistory) {
        return adjustFocus(root, currentLevel, cachedFocusedView, defaultFocusOverridesHistory,
                /* delayed= */true);
    }

    /**
     * Searches the {@code root}'s descendants for a view with the highest {@link FocusLevel}. If
     * the view's FocusLevel is higher than {@code currentLevel}, focuses on the view or {@code
     * cachedFocusedView}.
     *
     * @return whether the view is focused
     */
    private static boolean adjustFocus(@NonNull View root,
            @FocusLevel int currentLevel,
            @Nullable View cachedFocusedView,
            boolean defaultFocusOverridesHistory,
            boolean delayed) {
        // If the previously focused view has higher priority than the default focus, try to focus
        // on the previously focused view.
        if (!defaultFocusOverridesHistory && requestFocus(cachedFocusedView)) {
            return true;
        }

        // Try to focus on the default focus view.
        if (currentLevel < FOCUSED_BY_DEFAULT && focusOnFocusedByDefaultView(root)) {
            return true;
        }
        if (currentLevel < DEFAULT_FOCUS && focusOnDefaultFocusView(root)) {
            return true;
        }
        if (currentLevel < IMPLICIT_DEFAULT_FOCUS && focusOnImplicitDefaultFocusView(root)) {
            return true;
        }

        // When delayed is true, if there is a LazyLayoutView but it failed to adjust focus
        // inside it because it is not loaded yet or it's loaded but has no descendnats, request to
        // restore focus inside it later, and return false for now.
        if (delayed && currentLevel < IMPLICIT_DEFAULT_FOCUS) {
            LazyLayoutView lazyLayoutView = findLazyLayoutView(root);
            if (lazyLayoutView != null && !lazyLayoutView.isLayoutCompleted()) {
                initFocusDelayed(lazyLayoutView);
                return false;
            }
        }

        // If the previously focused view has lower priority than the default focus, try to focus
        // on the previously focused view.
        if (defaultFocusOverridesHistory && requestFocus(cachedFocusedView)) {
            return true;
        }

        // Try to focus on other views with low focus levels.
        if (currentLevel < REGULAR_FOCUS && focusOnFirstRegularView(root)) {
            return true;
        }
        if (currentLevel < SCROLLABLE_CONTAINER_FOCUS) {
            return focusOnScrollableContainer(root);
        }
        return false;
    }

    /**
     * If the {code lazyLayoutView} has a focusable descendant and no visible view is focused,
     * focuses on the descendant. Otherwise tries again when the {code lazyLayoutView} completes
     * layout or after a timeout, whichever comes first.
     */
    public static void initFocus(@NonNull LazyLayoutView lazyLayoutView) {
        if (initFocusImmediately(lazyLayoutView)) {
            return;
        }
        initFocusDelayed(lazyLayoutView);
    }

    private static void initFocusDelayed(@NonNull LazyLayoutView lazyLayoutView) {
        if (!(lazyLayoutView instanceof View)) {
            return;
        }
        View lazyView = (View) lazyLayoutView;
        Runnable[] onLayoutCompleteListener = new Runnable[1];
        Runnable delayedTask = () -> {
            lazyLayoutView.removeOnLayoutCompleteListener(onLayoutCompleteListener[0]);
            initFocusImmediately(lazyLayoutView);
        };
        onLayoutCompleteListener[0] = () -> {
            if (initFocusImmediately(lazyLayoutView)) {
                // Remove the delayedTask only when onLayoutCompleteListener has initialized the
                // focus succefully, because the delayedTask needs to kick in when it fails, such
                // as the lazyLayoutView is still loading after a timeout, or it's loaded but has
                // no descendants to take focus.
                lazyView.removeCallbacks(delayedTask);
                lazyLayoutView.removeOnLayoutCompleteListener(onLayoutCompleteListener[0]);
            }
        };
        lazyLayoutView.addOnLayoutCompleteListener(onLayoutCompleteListener[0]);
        lazyView.postDelayed(delayedTask, RESTORE_FOCUS_RETRY_DELAY_MS);
    }

    private static boolean initFocusImmediately(@NonNull LazyLayoutView lazyLayoutView) {
        if (!(lazyLayoutView instanceof View)) {
            return false;
        }
        View lazyView = (View) lazyLayoutView;
        View focusedView = lazyView.getRootView().findFocus();
        // If the currently focused view won't draw, it's not a valid focus.
        View visibleFocusedView =
                focusedView != null && !focusedView.willNotDraw() ? focusedView : null;
        // If there is a visible view focused, just return true.
        if (visibleFocusedView != null && !(visibleFocusedView instanceof FocusParkingView)) {
            return true;
        }

        return ViewUtils.adjustFocusImmediately(lazyView, visibleFocusedView);
    }

    @VisibleForTesting
    @FocusLevel
    static int getFocusLevel(@Nullable View view) {
        if (view == null || view instanceof FocusParkingView || !view.isShown()) {
            return NO_FOCUS;
        }
        if (view.isFocusedByDefault()) {
            return FOCUSED_BY_DEFAULT;
        }
        if (isDefaultFocus(view)) {
            return DEFAULT_FOCUS;
        }
        if (isImplicitDefaultFocusView(view)) {
            return IMPLICIT_DEFAULT_FOCUS;
        }
        if (isScrollableContainer(view)) {
            return SCROLLABLE_CONTAINER_FOCUS;
        }
        return REGULAR_FOCUS;
    }

    /** Returns whether the {@code view} is a {@code app:defaultFocus} view. */
    private static boolean isDefaultFocus(@NonNull View view) {
        FocusArea parent = getAncestorFocusArea(view);
        return parent != null && view == parent.getDefaultFocusView();
    }

    /**
     * Returns whether the {@code view} is an implicit default focus view, i.e., the selected
     * item or the first focusable item in a rotary container.
     */
    @VisibleForTesting
    static boolean isImplicitDefaultFocusView(@NonNull View view) {
        ViewGroup rotaryContainer = null;
        ViewParent parent = view.getParent();
        while (parent != null && parent instanceof ViewGroup) {
            ViewGroup viewGroup = (ViewGroup) parent;
            if (isRotaryContainer(viewGroup)) {
                rotaryContainer = viewGroup;
                break;
            }
            parent = parent.getParent();
        }
        if (rotaryContainer == null) {
            return false;
        }
        return findFirstSelectedFocusableDescendant(rotaryContainer) == view
                || findFirstFocusableDescendant(rotaryContainer) == view;
    }

    private static boolean isRotaryContainer(@NonNull View view) {
        CharSequence contentDescription = view.getContentDescription();
        return TextUtils.equals(contentDescription, ROTARY_CONTAINER)
                || TextUtils.equals(contentDescription, ROTARY_VERTICALLY_SCROLLABLE)
                || TextUtils.equals(contentDescription, ROTARY_HORIZONTALLY_SCROLLABLE);
    }

    private static boolean isScrollableContainer(@NonNull View view) {
        CharSequence contentDescription = view.getContentDescription();
        return TextUtils.equals(contentDescription, ROTARY_VERTICALLY_SCROLLABLE)
                || TextUtils.equals(contentDescription, ROTARY_HORIZONTALLY_SCROLLABLE);
    }

    private static boolean isFocusDelegatingContainer(@NonNull View view) {
        CharSequence contentDescription = view.getContentDescription();
        return TextUtils.equals(contentDescription, ROTARY_FOCUS_DELEGATING_CONTAINER);
    }

    /**
     * Focuses on the first {@code app:defaultFocus} view in the view tree, if any.
     *
     * @param root the root of the view tree
     * @return whether succeeded
     */
    private static boolean focusOnDefaultFocusView(@NonNull View root) {
        View defaultFocus = findDefaultFocusView(root);
        return requestFocus(defaultFocus);
    }

    /**
     * Focuses on the first {@code android:focusedByDefault} view in the view tree, if any.
     *
     * @param root the root of the view tree
     * @return whether succeeded
     */
    private static boolean focusOnFocusedByDefaultView(@NonNull View root) {
        View focusedByDefault = findFocusedByDefaultView(root);
        return requestFocus(focusedByDefault);
    }

    /**
     * Focuses on the first implicit default focus view in the view tree, if any.
     *
     * @param root the root of the view tree
     * @return whether succeeded
     */
    private static boolean focusOnImplicitDefaultFocusView(@NonNull View root) {
        View implicitDefaultFocus = findImplicitDefaultFocusView(root);
        return requestFocus(implicitDefaultFocus);
    }

    /**
     * Tries to focus on the first focusable view in the view tree in depth first order, excluding
     * the FocusParkingView and scrollable containers. If focusing on the first such view fails,
     * keeps trying other views in depth first order until succeeds or there are no more such views.
     *
     * @param root the root of the view tree
     * @return whether succeeded
     */
    private static boolean focusOnFirstRegularView(@NonNull View root) {
        View focusedView = ViewUtils.depthFirstSearch(root,
                /* targetPredicate= */
                v -> !isScrollableContainer(v) && canTakeFocus(v) && requestFocus(v),
                /* skipPredicate= */ v -> !v.isShown());
        return focusedView != null;
    }

    /**
     * Focuses on the first scrollable container in the view tree, if any.
     *
     * @param root the root of the view tree
     * @return whether succeeded
     */
    private static boolean focusOnScrollableContainer(@NonNull View root) {
        View focusedView = ViewUtils.depthFirstSearch(root,
                /* targetPredicate= */ v -> isScrollableContainer(v) && canTakeFocus(v),
                /* skipPredicate= */ v -> !v.isShown());
        return requestFocus(focusedView);
    }

    /**
     * Searches the {@code root}'s descendants in depth first order, and returns the first
     * {@code app:defaultFocus} view that can take focus. Returns null if not found.
     */
    @Nullable
    private static View findDefaultFocusView(@NonNull View view) {
        if (!view.isShown()) {
            return null;
        }
        if (view instanceof FocusArea) {
            FocusArea focusArea = (FocusArea) view;
            View defaultFocus = focusArea.getDefaultFocusView();
            if (defaultFocus != null && canTakeFocus(defaultFocus)) {
                return defaultFocus;
            }
        } else if (view instanceof ViewGroup) {
            ViewGroup parent = (ViewGroup) view;
            for (int i = 0; i < parent.getChildCount(); i++) {
                View child = parent.getChildAt(i);
                View defaultFocus = findDefaultFocusView(child);
                if (defaultFocus != null) {
                    return defaultFocus;
                }
            }
        }
        return null;
    }

    /**
     * Searches the {@code view} and its descendants in depth first order, and returns the first
     * {@code android:focusedByDefault} view that can take focus. Returns null if not found.
     */
    @VisibleForTesting
    @Nullable
    static View findFocusedByDefaultView(@NonNull View view) {
        return depthFirstSearch(view,
                /* targetPredicate= */ v -> v.isFocusedByDefault() && canTakeFocus(v),
                /* skipPredicate= */ v -> !v.isShown());
    }

    /**
     * Searches the {@code view} and its descendants in depth first order, and returns the first
     * implicit default focus view, i.e., the selected item or the first focusable item in the
     * first rotary container. Returns null if not found.
     */
    @VisibleForTesting
    @Nullable
    static View findImplicitDefaultFocusView(@NonNull View view) {
        View rotaryContainer = findRotaryContainer(view);
        if (rotaryContainer == null) {
            return null;
        }

        View selectedItem = findFirstSelectedFocusableDescendant(rotaryContainer);

        return selectedItem != null
                ? selectedItem
                : findFirstFocusableDescendant(rotaryContainer);
    }

    /**
     * Searches the {@code view}'s descendants in depth first order, and returns the first view
     * that can take focus, or null if not found.
     */
    @VisibleForTesting
    @Nullable
    static View findFirstFocusableDescendant(@NonNull View view) {
        return depthFirstSearch(view,
                /* targetPredicate= */ v -> v != view && canTakeFocus(v),
                /* skipPredicate= */ v -> !v.isShown());
    }

    /**
     * Searches the {@code view}'s descendants in depth first order, and returns the first view
     * that is selected and can take focus, or null if not found.
     */
    @VisibleForTesting
    @Nullable
    static View findFirstSelectedFocusableDescendant(@NonNull View view) {
        return depthFirstSearch(view,
                /* targetPredicate= */ v -> v != view && v.isSelected() && canTakeFocus(v),
                /* skipPredicate= */ v -> !v.isShown());
    }

    /**
     * Searches the {@code view} and its descendants in depth first order, and returns the first
     * rotary container shown on the screen. Returns null if not found.
     */
    @Nullable
    private static View findRotaryContainer(@NonNull View view) {
        return depthFirstSearch(view,
                /* targetPredicate= */ v -> isRotaryContainer(v),
                /* skipPredicate= */ v -> !v.isShown());
    }

    /**
     * Searches the {@code view} and its descendants in depth first order, and returns the first
     * LazyLayoutView shown on the screen. Returns null if not found.
     */
    @Nullable
    private static LazyLayoutView findLazyLayoutView(@NonNull View view) {
        return (LazyLayoutView) depthFirstSearch(view,
                /* targetPredicate= */ v -> v instanceof LazyLayoutView,
                /* skipPredicate= */ v -> !v.isShown());
    }

    /**
     * Searches the {@code view} and its descendants in depth first order, skips the views that
     * match {@code skipPredicate} and their descendants, and returns the first view that matches
     * {@code targetPredicate}. Returns null if not found.
     */
    @Nullable
    private static View depthFirstSearch(@NonNull View view,
            @NonNull Predicate<View> targetPredicate,
            @Nullable Predicate<View> skipPredicate) {
        if (skipPredicate != null && skipPredicate.test(view)) {
            return null;
        }
        if (targetPredicate.test(view)) {
            return view;
        }
        if (view instanceof ViewGroup) {
            ViewGroup parent = (ViewGroup) view;
            for (int i = 0; i < parent.getChildCount(); i++) {
                View child = parent.getChildAt(i);
                View target = depthFirstSearch(child, targetPredicate, skipPredicate);
                if (target != null) {
                    return target;
                }
            }
        }
        return null;
    }

    /** Returns whether {@code view} can be focused. */
    private static boolean canTakeFocus(@NonNull View view) {
        boolean focusable = view.isFocusable() || isFocusDelegatingContainer(view);
        return focusable && view.isEnabled() && view.isShown()
                && view.getWidth() > 0 && view.getHeight() > 0 && view.isAttachedToWindow()
                && !(view instanceof FocusParkingView)
                // If it's a scrollable container, it can be focused only when it has no focusable
                // descendants. We focus on it so that the rotary controller can scroll it.
                && (!isScrollableContainer(view) || findFirstFocusableDescendant(view) == null);
    }

    /**
     * Enables rotary scrolling for {@code view}, either vertically (if {@code isVertical} is true)
     * or horizontally (if {@code isVertical} is false). With rotary scrolling enabled, rotating the
     * rotary controller will scroll rather than moving the focus when moving the focus would cause
     * a lot of scrolling. Rotary scrolling should be enabled for scrolling views which contain
     * content which the user may want to see but can't interact with, either alone or along with
     * interactive (focusable) content.
     */
    public static void setRotaryScrollEnabled(@NonNull View view, boolean isVertical) {
        view.setContentDescription(
                isVertical ? ROTARY_VERTICALLY_SCROLLABLE : ROTARY_HORIZONTALLY_SCROLLABLE);
    }
}
