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
package com.chassis.car.ui.sharedlibrary.toolbar;

import android.content.Context;
import android.content.res.Resources;
import android.graphics.drawable.Drawable;
import android.os.Bundle;
import android.text.Editable;
import android.text.TextUtils;
import android.text.TextWatcher;
import android.view.KeyEvent;
import android.view.View;
import android.view.ViewStub;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputMethodManager;
import android.widget.ImageView;
import android.widget.TextView;

import androidx.annotation.DrawableRes;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.core.content.ContextCompat;

import com.android.car.ui.sharedlibrary.oemapis.toolbar.ToolbarControllerOEMV1;

import com.chassis.car.ui.sharedlibrary.R;

import java.util.Objects;
import java.util.function.BiConsumer;
import java.util.function.Consumer;

class SearchController {

    @NonNull
    private final InputMethodManager mInputMethodManager;

    @NonNull
    private final ViewStub mStub;
    private View mInflatedView;
    private ImageView mSearchIconView;
    private OnPrivateImeCommandEditText mEditText;
    private View mCloseIcon;

    private int mStartPaddingWithoutIcon;
    private int mStartPadding;
    private int mEndPadding;

    private Drawable mSearchIcon;
    @Nullable
    private String mSearchQuery;
    private String mSearchHint;

    @Nullable
    private Consumer<String> mSearchListener = null;
    @Nullable
    private Runnable mSearchCompletedListener = null;

    @Nullable
    private Consumer<TextView> mSearchTextViewConsumer;
    @Nullable
    private BiConsumer<String, Bundle> mOnPrivateImeCommandListener;

    private final TextWatcher mTextWatcher = new TextWatcher() {
        @Override
        public void beforeTextChanged(CharSequence charSequence, int i, int i1, int i2) {
        }

        @Override
        public void onTextChanged(CharSequence charSequence, int i, int i1, int i2) {
        }

        @Override
        public void afterTextChanged(Editable editable) {
            onSearch(editable.toString());
        }
    };

    SearchController(@NonNull ViewStub searchStub) {
        mInputMethodManager = Objects.requireNonNull((InputMethodManager)
                searchStub.getContext().getSystemService(Context.INPUT_METHOD_SERVICE));
        mStub = Objects.requireNonNull(searchStub);
        mSearchIcon = getDrawable(R.drawable.icon_search);
        mSearchHint = mStub.getContext().getString(R.string.toolbar_default_search_hint);
    }

    public void setSearchTextViewConsumer(@Nullable Consumer<TextView> textViewConsumer) {
        mSearchTextViewConsumer = textViewConsumer;
        if (mEditText != null && textViewConsumer != null) {
            mSearchTextViewConsumer.accept(mEditText);
        }
    }

    public void setOnPrivateImeCommandListener(
            @Nullable BiConsumer<String, Bundle> onPrivateImeCommandListener) {
        mOnPrivateImeCommandListener = onPrivateImeCommandListener;
        if (mEditText != null) {
            mEditText.setOnPrivateImeCommandListener(mOnPrivateImeCommandListener);
        }
    }

    public void setSearchMode(int searchMode) {
        if (searchMode != ToolbarControllerOEMV1.SEARCH_MODE_DISABLED && mInflatedView == null) {
            mInflatedView = mStub.inflate();
            mSearchIconView = mInflatedView.requireViewById(R.id.toolbar_search_icon);
            mEditText = mInflatedView.requireViewById(R.id.toolbar_search_bar);
            mCloseIcon = mInflatedView.requireViewById(R.id.toolbar_search_close);
            if (mSearchTextViewConsumer != null) {
                mSearchTextViewConsumer.accept(mEditText);
            }
            mEditText.setOnPrivateImeCommandListener(mOnPrivateImeCommandListener);

            mInflatedView.setVisibility(View.GONE); // So our later check if it's showing will pass
            mSearchIconView.setImageDrawable(mSearchIcon);
            mEditText.setSaveEnabled(false);
            mEditText.setText(mSearchQuery);
            mEditText.setHint(mSearchHint);

            mEditText.setOnClickListener(view -> mInputMethodManager.showSoftInput(view, 0));
            mEditText.setOnFocusChangeListener(
                    (view, hasFocus) -> {
                        if (!hasFocus) {
                            mInputMethodManager.hideSoftInputFromWindow(view.getWindowToken(), 0);
                        }
                    });
            mEditText.addTextChangedListener(mTextWatcher);
            mEditText.setOnEditorActionListener((v, actionId, event) -> {
                if (actionId == EditorInfo.IME_ACTION_DONE
                        || actionId == EditorInfo.IME_ACTION_SEARCH) {
                    mEditText.clearFocus();
                    if (mSearchCompletedListener != null) {
                        mSearchCompletedListener.run();
                    }
                } else if (isEnter(event)) {
                    if (event.getAction() == KeyEvent.ACTION_UP) {
                        // Note that we want to trigger search only on ACTION_UP, but want to return
                        // true for all actions for the relevant key event.
                        mEditText.clearFocus();
                        if (mSearchCompletedListener != null) {
                            mSearchCompletedListener.run();
                        }
                    }
                    return true;
                }
                return false;
            });
            mCloseIcon.setVisibility(TextUtils.isEmpty(mSearchQuery) ? View.GONE : View.VISIBLE);
            mCloseIcon.setOnClickListener(view -> {
                mEditText.getText().clear();
                mEditText.requestFocus();
                mInputMethodManager.showSoftInput(mEditText, 0);
            });

            Resources resources = mEditText.getContext().getResources();
            mStartPaddingWithoutIcon = mEditText.getPaddingStart();
            mStartPadding = resources.getDimensionPixelSize(
                    R.dimen.toolbar_search_icon_container_width);
            mEndPadding = resources.getDimensionPixelSize(
                    R.dimen.toolbar_search_close_icon_container_width);
            mEditText.setPaddingRelative(mStartPadding, 0, mEndPadding, 0);
        }

        boolean showingSearch = !mInflatedView.isShown()
                && searchMode != ToolbarControllerOEMV1.SEARCH_MODE_DISABLED;

        switch (searchMode) {
            case ToolbarControllerOEMV1.SEARCH_MODE_DISABLED:
                mInflatedView.setVisibility(View.GONE);
                break;
            case ToolbarControllerOEMV1.SEARCH_MODE_SEARCH:
                mInflatedView.setVisibility(View.VISIBLE);
                mEditText.setPaddingRelative(mStartPadding, 0, mEndPadding, 0);
                mEditText.setImeOptions(EditorInfo.IME_ACTION_SEARCH);
                mSearchIconView.setImageDrawable(mSearchIcon);
                mSearchIconView.setVisibility(View.VISIBLE);
                break;
            case ToolbarControllerOEMV1.SEARCH_MODE_EDIT:
                mInflatedView.setVisibility(View.VISIBLE);
                mEditText.setPaddingRelative(mStartPaddingWithoutIcon, 0, mEndPadding, 0);
                mEditText.setImeOptions(EditorInfo.IME_ACTION_DONE);
                mSearchIconView.setVisibility(View.GONE);
                break;
        }

        if (showingSearch && mEditText.isShown()) {
            mEditText.requestFocus();
            mInputMethodManager.showSoftInput(mEditText, 0);
        }
    }

    public void setSearchIcon(@Nullable Drawable drawable) {
        if (drawable == null) {
            mSearchIcon = getDrawable(R.drawable.icon_search);
        } else {
            mSearchIcon = drawable;
        }

        if (mSearchIconView != null) {
            mSearchIconView.setImageDrawable(mSearchIcon);
        }
    }

    public void setSearchHint(@Nullable String hint) {
        if (hint == null) {
            hint = mStub.getContext().getString(R.string.toolbar_default_search_hint);
        }
        mSearchHint = hint;

        if (mEditText != null) {
            mEditText.setHint(hint);
        }
    }

    public void setSearchQuery(@Nullable String query) {
        mSearchQuery = query;

        if (mEditText != null) {
            mEditText.setText(query);
        }
    }

    public void setSearchListener(@Nullable Consumer<String> listener) {
        mSearchListener = listener;
    }

    public void setSearchCompletedListener(@Nullable Runnable listener) {
        mSearchCompletedListener = listener;
    }

    private void onSearch(@Nullable String query) {
        mCloseIcon.setVisibility(TextUtils.isEmpty(query) ? View.GONE : View.VISIBLE);

        if (query == null) {
            query = "";
        }

        if (mSearchListener != null) {
            mSearchListener.accept(query);
        }
    }

    @NonNull
    private Drawable getDrawable(@DrawableRes int resId) {
        return Objects.requireNonNull(ContextCompat.getDrawable(mStub.getContext(), resId));
    }

    private boolean isEnter(KeyEvent event) {
        boolean result = false;
        if (event != null) {
            int keyCode = event.getKeyCode();
            result = keyCode == KeyEvent.KEYCODE_ENTER
                    || keyCode == KeyEvent.KEYCODE_NUMPAD_ENTER
                    || keyCode == KeyEvent.KEYCODE_SEARCH;
        }
        return result;
    }
}
