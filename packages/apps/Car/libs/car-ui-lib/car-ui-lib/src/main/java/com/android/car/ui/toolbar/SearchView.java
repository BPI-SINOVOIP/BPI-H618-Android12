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
package com.android.car.ui.toolbar;

import static com.android.car.ui.utils.CarUiUtils.requireViewByRefId;

import android.content.Context;
import android.graphics.drawable.Drawable;
import android.text.Editable;
import android.text.TextUtils;
import android.text.TextWatcher;
import android.util.AttributeSet;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.View;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputMethodManager;
import android.widget.EditText;
import android.widget.ImageView;

import androidx.annotation.NonNull;
import androidx.constraintlayout.widget.ConstraintLayout;

import com.android.car.ui.R;

import java.util.Collections;
import java.util.Set;
import java.util.function.Consumer;

/**
 * A search view used by {@link Toolbar}.
 */
@SuppressWarnings("AndroidJdkLibsChecker")
public class SearchView extends ConstraintLayout {

    private final InputMethodManager mInputMethodManager;
    private final SearchWidescreenController mSearchWidescreenController;
    private final ImageView mIcon;
    private final EditText mSearchText;
    private final View mCloseIcon;
    private final int mStartPaddingWithoutIcon;
    private final int mStartPadding;
    private final int mEndPadding;

    private Set<Consumer<String>> mSearchListeners = Collections.emptySet();
    private Set<Runnable> mSearchCompletedListeners =
            Collections.emptySet();
    private Set<Toolbar.OnSearchListener> mDeprecatedSearchListeners = Collections.emptySet();
    private Set<Toolbar.OnSearchCompletedListener> mDeprecatedSearchCompletedListeners =
            Collections.emptySet();
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

    private boolean mIsPlainText = false;

    public SearchView(Context context) {
        this(context, null);
    }

    public SearchView(Context context, AttributeSet attrs) {
        this(context, attrs, 0);
    }

    public SearchView(Context context, AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);

        mInputMethodManager = getContext().getSystemService(InputMethodManager.class);

        LayoutInflater inflater = LayoutInflater.from(context);
        inflater.inflate(R.layout.car_ui_toolbar_search_view, this, true);

        mSearchText = requireViewByRefId(this, R.id.car_ui_toolbar_search_bar);
        mIcon = requireViewByRefId(this, R.id.car_ui_toolbar_search_icon);
        mCloseIcon = requireViewByRefId(this, R.id.car_ui_toolbar_search_close);

        mCloseIcon.setOnClickListener(view -> {
            if (view.isFocused()) {
                mSearchText.requestFocus();
                mInputMethodManager.showSoftInput(mSearchText, 0);
            }
            mSearchText.getText().clear();
        });
        mCloseIcon.setVisibility(View.GONE);

        mStartPaddingWithoutIcon = mSearchText.getPaddingStart();
        mStartPadding = context.getResources().getDimensionPixelSize(
                R.dimen.car_ui_toolbar_search_search_icon_container_width);
        mEndPadding = context.getResources().getDimensionPixelSize(
                R.dimen.car_ui_toolbar_search_close_icon_container_width);

        mSearchText.setSaveEnabled(false);
        mSearchText.setPaddingRelative(mStartPadding, 0, mEndPadding, 0);

        mSearchText.setOnClickListener((view) -> mInputMethodManager.showSoftInput(view, 0));

        mSearchText.setOnFocusChangeListener(
                (view, hasFocus) -> {
                    if (!hasFocus) {
                        mInputMethodManager.hideSoftInputFromWindow(view.getWindowToken(), 0);
                    }
                });

        mSearchText.addTextChangedListener(mTextWatcher);

        mSearchText.setOnEditorActionListener((v, actionId, event) -> {
            if (actionId == EditorInfo.IME_ACTION_DONE
                    || actionId == EditorInfo.IME_ACTION_SEARCH) {
                notifyQuerySubmit();
            } else if (isEnter(event)) {
                if (event.getAction() == KeyEvent.ACTION_UP) {
                    // Note that we want to trigger search only on ACTION_UP, but want to return
                    // true for all actions for the relevant key event.
                    notifyQuerySubmit();
                }
                return true;
            }
            return false;
        });

        mSearchWidescreenController = new SearchWidescreenController(context);
        mSearchWidescreenController.setTextView(mSearchText);
        if (mSearchText instanceof CarUiEditText) {
            ((CarUiEditText) mSearchText).setOnPrivateImeCommandListener(
                    mSearchWidescreenController.getOnPrivateImeCommandListener());
        }
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

    private void notifyQuerySubmit() {
        mSearchText.clearFocus();
        for (Runnable listener : mSearchCompletedListeners) {
            listener.run();
        }
        for (Toolbar.OnSearchCompletedListener listener : mDeprecatedSearchCompletedListeners) {
            listener.onSearchCompleted();
        }
    }

    private boolean mWasShown = false;

    @Override
    public void onVisibilityChanged(@NonNull View changedView, int visibility) {
        super.onVisibilityChanged(changedView, visibility);

        boolean isShown = isShown();
        if (isShown && !mWasShown) {
            boolean hasQuery = mSearchText.getText().length() > 0;
            mCloseIcon.setVisibility(hasQuery ? View.VISIBLE : View.GONE);
            mSearchText.requestFocus();
            mInputMethodManager.showSoftInput(mSearchText, 0);
        }
        mWasShown = isShown;
    }

    /**
     * Sets the {@link SearchConfig} full of data to show in the IME.
     *
     * @see SearchWidescreenController#setSearchConfig(SearchConfig)
     */
    public void setSearchConfig(SearchConfig config) {
        mSearchWidescreenController.setSearchConfig(config);
    }

    /**
     * Sets a listener for the search text changing.
     */
    public void setSearchListeners(
            Set<Toolbar.OnSearchListener> deprecatedListeners,
            Set<Consumer<String>> listeners) {
        mSearchListeners = listeners;
        mDeprecatedSearchListeners = deprecatedListeners;
    }

    /**
     * Sets a listener for the user completing their search, for example by clicking the
     * enter/search button on the keyboard.
     */
    public void setSearchCompletedListeners(
            Set<Toolbar.OnSearchCompletedListener> deprecatedListeners,
            Set<Runnable> listeners) {
        mSearchCompletedListeners = listeners;
        mDeprecatedSearchCompletedListeners = deprecatedListeners;
    }

    /**
     * Sets the search hint
     *
     * @param hint A CharSequence of the search hint.
     */
    public void setHint(CharSequence hint) {
        mSearchText.setHint(hint);
    }

    /**
     * Sets a custom icon to display in the search box.
     */
    public void setIcon(Drawable d) {
        if (d == null) {
            mIcon.setImageResource(R.drawable.car_ui_icon_search);
        } else {
            mIcon.setImageDrawable(d);
        }
    }

    /**
     * Sets whether or not the search bar should look like a regular text box instead of a search
     * box.
     */
    public void setPlainText(boolean plainText) {
        if (plainText != mIsPlainText) {
            if (plainText) {
                mSearchText.setPaddingRelative(mStartPaddingWithoutIcon, 0, mEndPadding, 0);
                mSearchText.setImeOptions(EditorInfo.IME_ACTION_DONE);
                mIcon.setVisibility(View.GONE);
            } else {
                mSearchText.setPaddingRelative(mStartPadding, 0, mEndPadding, 0);
                mSearchText.setImeOptions(EditorInfo.IME_ACTION_SEARCH);
                mIcon.setVisibility(View.VISIBLE);
            }
            mIsPlainText = plainText;

            // Needed to detect changes to imeOptions
            mInputMethodManager.restartInput(mSearchText);
        }
    }

    private void onSearch(String query) {
        mCloseIcon.setVisibility(TextUtils.isEmpty(query) ? View.GONE : View.VISIBLE);

        for (Consumer<String> listener : mSearchListeners) {
            listener.accept(query);
        }
        for (Toolbar.OnSearchListener listener : mDeprecatedSearchListeners) {
            listener.onSearch(query);
        }
    }

    /**
     * Sets the text being searched.
     */
    public void setSearchQuery(String query) {
        mSearchText.setText(query);
        mSearchText.setSelection(mSearchText.getText().length());
    }
}
