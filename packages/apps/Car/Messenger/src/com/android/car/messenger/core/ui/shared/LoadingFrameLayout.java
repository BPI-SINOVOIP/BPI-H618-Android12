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

package com.android.car.messenger.core.ui.shared;

import android.content.Context;
import android.content.res.TypedArray;
import android.util.AttributeSet;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.FrameLayout;
import android.widget.ImageView;
import android.widget.TextView;

import androidx.annotation.DrawableRes;
import androidx.annotation.IntDef;
import androidx.annotation.LayoutRes;
import androidx.annotation.MainThread;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.StringRes;

import com.android.car.messenger.R;

/** A widget that supports different {@link State}s: NEW, LOADING, CONTENT, EMPTY OR ERROR. */
public class LoadingFrameLayout extends FrameLayout {
    private static final int INVALID_RES_ID = 0;

    /** Possible states of a service request display. */
    @IntDef({State.NEW, State.LOADING, State.CONTENT, State.ERROR, State.EMPTY})
    public @interface State {
        int NEW = 0;
        int LOADING = 1;
        int CONTENT = 2;
        int ERROR = 3;
        int EMPTY = 4;
    }

    @NonNull private final Context mContext;
    @NonNull private ViewContainer mEmptyView;
    @NonNull private ViewContainer mLoadingView;
    @NonNull private ViewContainer mErrorView;

    @State private int mState = State.NEW;

    public LoadingFrameLayout(@NonNull Context context, @Nullable AttributeSet attrs) {
        this(context, attrs, 0);
    }

    public LoadingFrameLayout(
            @NonNull Context context, @Nullable AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
        mContext = context;
        TypedArray values =
                context.obtainStyledAttributes(attrs, R.styleable.LoadingFrameLayout, defStyle, 0);
        setLoadingView(
                values.getResourceId(
                        R.styleable.LoadingFrameLayout_progressViewLayout,
                        R.layout.loading_progress_view));
        setEmptyView();
        setErrorView();
        values.recycle();
    }

    @Override
    public void onFinishInflate() {
        super.onFinishInflate();
        // Start with a loading view when inflated from XML.
        showLoading();
    }

    private void setLoadingView(int loadingLayoutId) {
        mLoadingView = new ViewContainer(State.LOADING, loadingLayoutId);
    }

    private void setEmptyView() {
        mEmptyView = new ViewContainer(State.EMPTY);
    }

    private void setErrorView() {
        mErrorView = new ViewContainer(State.ERROR);
    }

    /** Shows the loading view, hides other views. */
    @MainThread
    public void showLoading() {
        switchTo(State.LOADING);
    }

    /**
     * Shows the error view where the action button is not available and hides other views.
     *
     * @param iconResId drawable resource id used for the top icon. When it is invalid, hide the
     *     icon view.
     * @param messageResId string resource id used for the error message. When it is invalid, hide
     *     the message view.
     * @param secondaryMessageResId string resource id for the secondary error message. When it is
     *     invalid, hide the secondary message view.
     */
    public void showError(
            @DrawableRes int iconResId,
            @StringRes int messageResId,
            @StringRes int secondaryMessageResId) {
        showError(iconResId, messageResId, secondaryMessageResId, INVALID_RES_ID, null, false);
    }

    /**
     * Shows the error view, hides other views.
     *
     * @param iconResId drawable resource id used for the top icon.When it is invalid, hide the icon
     *     view.
     * @param messageResId string resource id used for the error message. When it is invalid, hide
     *     the message view.
     * @param secondaryMessageResId string resource id for the secondary error message. When it is
     *     invalid, hide the secondary message view.
     * @param actionButtonTextResId string resource id for the action button.
     * @param actionButtonOnClickListener click listener set on the action button.
     * @param showActionButton boolean flag if the action button will show.
     */
    public void showError(
            @DrawableRes int iconResId,
            @StringRes int messageResId,
            @StringRes int secondaryMessageResId,
            @StringRes int actionButtonTextResId,
            @Nullable OnClickListener actionButtonOnClickListener,
            boolean showActionButton) {
        switchTo(State.ERROR);
        mErrorView.setIcon(iconResId);
        mErrorView.setMessage(messageResId);
        mErrorView.setSecondaryMessage(secondaryMessageResId);
        mErrorView.setActionButtonText(actionButtonTextResId);
        mErrorView.setActionButtonClickListener(actionButtonOnClickListener);
        mErrorView.setActionButtonVisible(showActionButton);
    }

    /**
     * Shows the empty view where the action button is not available and hides other views.
     *
     * @param iconResId drawable resource id used for the top icon. When it is invalid, hide the
     *     icon view.
     * @param messageResId string resource id used for the empty message. When it is invalid, hide
     *     the message view.
     * @param secondaryMessageResId string resource id for the secondary empty message. When it is
     *     invalid, hide the secondary message view.
     */
    public void showEmpty(
            @DrawableRes int iconResId,
            @StringRes int messageResId,
            @StringRes int secondaryMessageResId) {
        showEmpty(iconResId, messageResId, secondaryMessageResId, INVALID_RES_ID, null, false);
    }

    /**
     * Shows the empty view and hides other views.
     *
     * @param iconResId drawable resource id used for the top icon.When it is invalid, hide the icon
     *     view.
     * @param messageResId string resource id used for the empty message. When it is invalid, hide
     *     the message view.
     * @param secondaryMessageResId string resource id for the secondary empty message. When it is
     *     invalid, hide the secondary message view.
     * @param actionButtonTextResId string resource id for the action button.
     * @param actionButtonOnClickListener click listener set on the action button.
     * @param showActionButton boolean flag if the action button will show.
     */
    public void showEmpty(
            @DrawableRes int iconResId,
            @StringRes int messageResId,
            @StringRes int secondaryMessageResId,
            @StringRes int actionButtonTextResId,
            @Nullable OnClickListener actionButtonOnClickListener,
            boolean showActionButton) {
        mEmptyView.setIcon(iconResId);
        mEmptyView.setMessage(messageResId);
        mEmptyView.setSecondaryMessage(secondaryMessageResId);
        mEmptyView.setActionButtonText(actionButtonTextResId);
        mEmptyView.setActionButtonClickListener(actionButtonOnClickListener);
        mEmptyView.setActionButtonVisible(showActionButton);
        switchTo(State.EMPTY);
    }

    /** Shows the content view, hides other views. */
    public void showContent() {
        switchTo(State.CONTENT);
    }

    /** Hide all views. */
    public void reset() {
        switchTo(State.NEW);
    }

    private void switchTo(@State int state) {
        if (mState != state) {
            ViewUtils.setVisible((View) findViewById(R.id.list_view), state == State.CONTENT);
            mLoadingView.setVisibilityFromState(state);
            mErrorView.setVisibilityFromState(state);
            mEmptyView.setVisibilityFromState(state);
            mState = state;
        }
    }

    /**
     * Container for views held by this LoadingFrameLayout. Used for deferring view inflation until
     * the view is about to be shown.
     */
    private class ViewContainer {
        @State private final int mViewState;
        private View mView;
        private ImageView mIconView;
        private TextView mActionButton;
        private TextView mMessageView;
        private TextView mSecondaryMessageView;

        private ViewContainer(@State int state) {
            mViewState = state;
            mView = inflateView();
            LoadingFrameLayout.this.addView(mView);
        }

        private ViewContainer(@State int state, @LayoutRes int layout) {
            mViewState = state;
            mView = LayoutInflater.from(mContext).inflate(layout, LoadingFrameLayout.this, false);
            LoadingFrameLayout.this.addView(mView);
        }

        private View inflateView() {
            View view =
                    LayoutInflater.from(mContext)
                            .inflate(R.layout.loading_info_view, LoadingFrameLayout.this, false);
            mMessageView = view.findViewById(R.id.loading_info_message);
            mSecondaryMessageView = view.findViewById(R.id.loading_info_secondary_message);
            mIconView = view.findViewById(R.id.loading_info_icon);
            mActionButton = view.findViewById(R.id.loading_info_action_button);
            return view;
        }

        public void setVisibilityFromState(@State int newState) {
            if (mViewState == newState) {
                show();
            } else {
                hide();
            }
        }

        private void show() {
            mView.setVisibility(View.VISIBLE);
        }

        private void hide() {
            if (mView != null) {
                mView.setVisibility(View.GONE);
                mView.clearFocus();
            }
        }

        private void setMessage(@StringRes int messageResId) {
            if (mMessageView == null) {
                return;
            }
            if (messageResId != INVALID_RES_ID) {
                mMessageView.setText(messageResId);
            } else {
                ViewUtils.setVisible(mMessageView, false);
            }
        }

        private void setSecondaryMessage(@StringRes int secondaryMessageResId) {
            if (mSecondaryMessageView == null) {
                return;
            }
            if (secondaryMessageResId != INVALID_RES_ID) {
                mSecondaryMessageView.setText(secondaryMessageResId);
            } else {
                ViewUtils.setVisible(mSecondaryMessageView, false);
            }
        }

        private void setActionButtonClickListener(OnClickListener actionButtonOnClickListener) {
            if (mActionButton == null) {
                return;
            }
            mActionButton.setOnClickListener(actionButtonOnClickListener);
        }

        private void setActionButtonText(@StringRes int actionButtonTextResId) {
            if (mActionButton == null) {
                return;
            }
            if (actionButtonTextResId != INVALID_RES_ID) {
                mActionButton.setText(actionButtonTextResId);
            }
        }

        private void setActionButtonVisible(boolean visible) {
            ViewUtils.setVisible(mActionButton, visible);
        }

        private void setIcon(@DrawableRes int iconResId) {
            if (iconResId != INVALID_RES_ID) {
                if (mIconView != null) {
                    mIconView.setImageResource(iconResId);
                }
            } else {
                ViewUtils.setVisible(mIconView, false);
            }
        }
    }
}
