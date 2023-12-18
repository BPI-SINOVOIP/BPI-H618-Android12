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

package com.android.car.ui.paintbooth.widescreenime;

import static android.view.inputmethod.EditorInfo.IME_FLAG_NO_EXTRACT_UI;

import static com.android.car.ui.imewidescreen.CarUiImeWideScreenController.ADD_DESC_TITLE_TO_CONTENT_AREA;
import static com.android.car.ui.imewidescreen.CarUiImeWideScreenController.ADD_DESC_TO_CONTENT_AREA;
import static com.android.car.ui.imewidescreen.CarUiImeWideScreenController.ADD_ERROR_DESC_TO_INPUT_AREA;
import static com.android.car.ui.imewidescreen.CarUiImeWideScreenController.REQUEST_RENDER_CONTENT_AREA;
import static com.android.car.ui.imewidescreen.CarUiImeWideScreenController.WIDE_SCREEN_ACTION;
import static com.android.car.ui.imewidescreen.CarUiImeWideScreenController.WIDE_SCREEN_EXTRACTED_TEXT_ICON_RES_ID;

import android.content.Context;
import android.graphics.drawable.Drawable;
import android.os.Bundle;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.View.OnFocusChangeListener;
import android.view.ViewGroup;
import android.view.inputmethod.InputMethodManager;
import android.widget.EditText;
import android.widget.Toast;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.content.ContextCompat;
import androidx.recyclerview.widget.RecyclerView;

import com.android.car.ui.FocusArea;
import com.android.car.ui.baselayout.Insets;
import com.android.car.ui.baselayout.InsetsChangedListener;
import com.android.car.ui.core.CarUi;
import com.android.car.ui.imewidescreen.CarUiImeSearchListItem;
import com.android.car.ui.paintbooth.R;
import com.android.car.ui.recyclerview.CarUiContentListItem;
import com.android.car.ui.recyclerview.CarUiRecyclerView;
import com.android.car.ui.toolbar.MenuItem;
import com.android.car.ui.toolbar.NavButtonMode;
import com.android.car.ui.toolbar.SearchConfig;
import com.android.car.ui.toolbar.SearchMode;
import com.android.car.ui.toolbar.ToolbarController;

import java.util.ArrayList;
import java.util.List;

/**
 * Activity that shows different scenarios for wide screen ime.
 */
public class WideScreenImeActivity extends AppCompatActivity implements InsetsChangedListener {
    private static final String TAG = "PaintBooth";

    private final List<MenuItem> mMenuItems = new ArrayList<>();
    private final List<ListElement> mWidescreenItems = new ArrayList<>();
    private final List<CarUiImeSearchListItem> mSearchItems = new ArrayList<>();

    private InputMethodManager mInputMethodManager;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.car_ui_recycler_view_activity);

        mInputMethodManager = (InputMethodManager)
                getSystemService(Context.INPUT_METHOD_SERVICE);

        ToolbarController toolbar = CarUi.requireToolbar(this);
        toolbar.setTitle(getTitle());
        toolbar.setNavButtonMode(NavButtonMode.BACK);
        toolbar.setLogo(R.drawable.ic_launcher);
        boolean[] isSearching = new boolean[]{false};
        toolbar.registerBackListener(
                () -> {
                    if (isSearching[0]) {
                        toolbar.setSearchMode(SearchMode.DISABLED);
                        isSearching[0] = false;
                        return true;
                    }
                    return false;
                });

        CarUiContentListItem.OnClickListener mainClickListener = i -> {
            CharSequence title = i.getTitle() != null
                    ? i.getTitle().getPreferredText()
                    : null;
            Toast.makeText(this, "Item clicked! " + title, Toast.LENGTH_SHORT).show();
            Log.i(TAG, "Item clicked! " + title);
        };
        CarUiContentListItem.OnClickListener secondaryClickListener = i -> {
            Toast.makeText(this, "Item's secondary action clicked!", Toast.LENGTH_SHORT).show();
            Log.i(TAG, "Item's secondary action clicked!");
        };

        Drawable icon = ContextCompat.getDrawable(this, R.drawable.ic_launcher);

        for (int i = 1; i <= 100; i++) {
            CarUiImeSearchListItem item = new CarUiImeSearchListItem(
                    CarUiContentListItem.Action.ICON);
            item.setTitle("Title " + i);
            item.setBody("Sub title " + i);
            item.setIcon(icon);
            item.setSupplementalIcon(icon, secondaryClickListener);
            item.setOnItemClickedListener(mainClickListener);

            mSearchItems.add(item);
        }

        mMenuItems.add(MenuItem.builder(this)
                .setToSearch()
                .setOnClickListener(i -> {
                    isSearching[0] = true;
                    toolbar.setSearchMode(SearchMode.SEARCH);
                    if (toolbar.getSearchCapabilities().canShowSearchResultItems()) {
                        toolbar.setSearchConfig(SearchConfig.builder()
                                .setSearchResultItems(mSearchItems)
                                .setSearchResultsInputViewIcon(ContextCompat.getDrawable(
                                        this, R.drawable.car_ui_icon_search))
                                .build());
                    }
                })
                .build());

        toolbar.setMenuItems(mMenuItems);

        mWidescreenItems.add(new EditTextElement("Default Input Edit Text field", null));
        mWidescreenItems.add(
                new EditTextElement("Add Desc to content area", this::addDescToContentArea));
        mWidescreenItems.add(new EditTextElement("Hide the content area", this::hideContentArea));
        mWidescreenItems.add(new EditTextElement("Hide extraction view", this::hideExtractionView));

        mWidescreenItems.add(
                new EditTextElement("Add icon to extracted view", this::addIconToExtractedView));
        mWidescreenItems.add(new EditTextElement("Add error message to content area",
                this::addErrorDescToContentArea));

        CarUiRecyclerView recyclerView = requireViewById(R.id.list);
        recyclerView.setAdapter(mAdapter);
    }

    private void addIconToExtractedView(View view, boolean hasFocus) {
        if (!hasFocus) {
            return;
        }

        Bundle bundle = new Bundle();
        bundle.putInt(WIDE_SCREEN_EXTRACTED_TEXT_ICON_RES_ID, R.drawable.car_ui_icon_edit);
        mInputMethodManager.sendAppPrivateCommand(view, WIDE_SCREEN_ACTION, bundle);
    }

    private void addErrorDescToContentArea(View view, boolean hasFocus) {
        if (!hasFocus) {
            return;
        }

        Bundle bundle = new Bundle();
        bundle.putString(ADD_ERROR_DESC_TO_INPUT_AREA, "Some error message");
        bundle.putString(ADD_DESC_TITLE_TO_CONTENT_AREA, "Title");
        bundle.putString(ADD_DESC_TO_CONTENT_AREA, "Description provided by the application");
        mInputMethodManager.sendAppPrivateCommand(view, WIDE_SCREEN_ACTION, bundle);
    }

    private void hideExtractionView(View view, boolean hasFocus) {
        if (!hasFocus) {
            return;
        }

        EditText editText = (EditText) view;
        editText.setImeOptions(IME_FLAG_NO_EXTRACT_UI);

        Bundle bundle = new Bundle();
        bundle.putBoolean(REQUEST_RENDER_CONTENT_AREA, false);
        mInputMethodManager.sendAppPrivateCommand(view, WIDE_SCREEN_ACTION, bundle);
    }

    private void addDescToContentArea(View view, boolean hasFocus) {
        if (!hasFocus) {
            return;
        }

        Bundle bundle = new Bundle();
        bundle.putString(ADD_DESC_TITLE_TO_CONTENT_AREA, "Title");
        bundle.putString(ADD_DESC_TO_CONTENT_AREA, "Description provided by the application");
        mInputMethodManager.sendAppPrivateCommand(view, WIDE_SCREEN_ACTION, bundle);
    }

    private void hideContentArea(View view, boolean hasFocus) {
        if (!hasFocus) {
            return;
        }

        Bundle bundle = new Bundle();
        bundle.putBoolean(REQUEST_RENDER_CONTENT_AREA, false);
        mInputMethodManager.sendAppPrivateCommand(view, WIDE_SCREEN_ACTION, bundle);
    }

    private abstract static class ViewHolder extends RecyclerView.ViewHolder {

        ViewHolder(@NonNull View itemView) {
            super(itemView);
        }

        public abstract void bind(ListElement element);
    }

    private static class EditTextViewHolder extends ViewHolder {

        private final EditText mEditText;

        EditTextViewHolder(@NonNull View itemView) {
            super(itemView);
            mEditText = itemView.requireViewById(R.id.edit_text);
        }

        @Override
        public void bind(ListElement e) {
            if (!(e instanceof EditTextElement)) {
                throw new IllegalArgumentException("Expected an EditTextElement");
            }
            EditTextElement element = (EditTextElement) e;
            mEditText.setText(element.getText());
            mEditText.setOnFocusChangeListener(element.getOnFocusChangeListener());
        }
    }

    private final RecyclerView.Adapter<ViewHolder> mAdapter =
            new RecyclerView.Adapter<ViewHolder>() {
                @Override
                public int getItemCount() {
                    return mWidescreenItems.size();
                }

                @Override
                public ViewHolder onCreateViewHolder(ViewGroup parent, int viewType) {
                    LayoutInflater inflater = LayoutInflater.from(parent.getContext());
                    if (viewType == ListElement.TYPE_EDIT_TEXT) {
                        return new EditTextViewHolder(
                                inflater.inflate(R.layout.edit_text_list_item, parent, false));
                    } else {
                        throw new IllegalArgumentException("Unknown viewType: " + viewType);
                    }
                }

                @Override
                public void onBindViewHolder(@NonNull ViewHolder holder, int position) {
                    holder.bind(mWidescreenItems.get(position));
                }

                @Override
                public int getItemViewType(int position) {
                    return mWidescreenItems.get(position).getType();
                }
            };

    @Override
    public void onCarUiInsetsChanged(@NonNull Insets insets) {
        FocusArea focusArea = requireViewById(R.id.focus_area);
        focusArea.setBoundsOffset(0, insets.getTop(), 0, insets.getBottom());
        focusArea.setHighlightPadding(0, insets.getTop(), 0, insets.getBottom());
        requireViewById(R.id.list)
                .setPadding(0, insets.getTop(), 0, insets.getBottom());
        requireViewById(android.R.id.content)
                .setPadding(insets.getLeft(), 0, insets.getRight(), 0);
    }

    private abstract static class ListElement {

        static final int TYPE_EDIT_TEXT = 0;

        private final String mText;

        ListElement(String text) {
            mText = text;
        }

        String getText() {
            return mText;
        }

        abstract int getType();
    }

    private static class EditTextElement extends ListElement {

        private final OnFocusChangeListener mListener;

        EditTextElement(String text, OnFocusChangeListener listener) {
            super(text);
            mListener = listener;
        }

        OnFocusChangeListener getOnFocusChangeListener() {
            return mListener;
        }

        @Override
        int getType() {
            return TYPE_EDIT_TEXT;
        }
    }
}
