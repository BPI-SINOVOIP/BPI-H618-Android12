/*
 * Copyright 2019 The Android Open Source Project
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

package com.android.car.ui.paintbooth.caruirecyclerview;

import android.app.Activity;
import android.content.Context;
import android.os.Bundle;
import android.widget.Toast;

import androidx.annotation.NonNull;
import androidx.recyclerview.widget.RecyclerView;

import com.android.car.ui.FocusArea;
import com.android.car.ui.baselayout.Insets;
import com.android.car.ui.baselayout.InsetsChangedListener;
import com.android.car.ui.core.CarUi;
import com.android.car.ui.paintbooth.R;
import com.android.car.ui.recyclerview.CarUiContentListItem;
import com.android.car.ui.recyclerview.CarUiHeaderListItem;
import com.android.car.ui.recyclerview.CarUiListItem;
import com.android.car.ui.recyclerview.CarUiRecyclerView;
import com.android.car.ui.toolbar.NavButtonMode;
import com.android.car.ui.toolbar.ToolbarController;

import java.util.ArrayList;

/**
 * Activity that shows {@link CarUiRecyclerView} with sample {@link CarUiContentListItem} entries
 */
public class CarUiListItemActivity extends Activity implements InsetsChangedListener {

    private final ArrayList<CarUiListItem> mData = new ArrayList<>();
    private RecyclerView.Adapter<? extends RecyclerView.ViewHolder> mAdapter;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.car_ui_recycler_view_activity);

        ToolbarController toolbar = CarUi.requireToolbar(this);
        toolbar.setTitle(getTitle());
        toolbar.setNavButtonMode(NavButtonMode.BACK);

        CarUiRecyclerView recyclerView = findViewById(R.id.list);
        mAdapter = CarUi.createListItemAdapter(this, generateSampleData());
        recyclerView.setAdapter(mAdapter);
    }

    private ArrayList<CarUiListItem> generateSampleData() {
        Context context = this;

        CarUiHeaderListItem header = new CarUiHeaderListItem(getString(R.string.first_header));
        mData.add(header);

        CarUiContentListItem item = new CarUiContentListItem(CarUiContentListItem.Action.NONE);
        item.setTitle(getString(R.string.test_title));
        item.setBody(getString(R.string.test_body));
        mData.add(item);

        item = new CarUiContentListItem(CarUiContentListItem.Action.NONE);
        item.setTitle(getString(R.string.test_title_no_body));
        mData.add(item);

        header = new CarUiHeaderListItem(getString(R.string.random_header),
                getString(R.string.header_with_body));
        mData.add(header);

        item = new CarUiContentListItem(CarUiContentListItem.Action.NONE);
        item.setBody(getString(R.string.test_body_no_title));
        mData.add(item);

        item = new CarUiContentListItem(CarUiContentListItem.Action.NONE);
        item.setTitle(getString(R.string.test_title));
        item.setIcon(getDrawable(R.drawable.ic_launcher));
        mData.add(item);

        item = new CarUiContentListItem(CarUiContentListItem.Action.NONE);
        item.setTitle(getString(R.string.test_title));
        item.setBody(getString(R.string.test_body));
        item.setIcon(getDrawable(R.drawable.ic_launcher));
        mData.add(item);

        item = new CarUiContentListItem(CarUiContentListItem.Action.NONE);
        item.setTitle(getString(R.string.title_with_content_icon));
        item.setPrimaryIconType(CarUiContentListItem.IconType.CONTENT);
        item.setIcon(getDrawable(R.drawable.ic_sample_logo));
        mData.add(item);

        item = new CarUiContentListItem(CarUiContentListItem.Action.NONE);
        item.setTitle(getString(R.string.test_title));
        item.setBody(getString(R.string.with_avatar_icon));
        item.setIcon(getDrawable(R.drawable.ic_sample_logo));
        item.setPrimaryIconType(CarUiContentListItem.IconType.AVATAR);

        item = new CarUiContentListItem(CarUiContentListItem.Action.NONE);
        item.setTitle(getString(R.string.test_title));
        item.setBody(getString(R.string.display_toast_on_click));
        item.setIcon(getDrawable(R.drawable.ic_launcher));
        item.setOnItemClickedListener(item1 -> {
            Toast.makeText(context, "Item clicked", Toast.LENGTH_SHORT).show();
        });
        mData.add(item);

        item = new CarUiContentListItem(CarUiContentListItem.Action.CHECK_BOX);
        item.setIcon(getDrawable(R.drawable.ic_launcher));
        item.setTitle(getString(R.string.title_item_with_checkbox));
        item.setBody(getString(R.string.toast_on_selection_changed));
        item.setOnCheckedChangeListener(
                (listItem, isChecked) -> Toast.makeText(context,
                        "Item checked state is: " + isChecked, Toast.LENGTH_SHORT).show());
        mData.add(item);

        item = new CarUiContentListItem(CarUiContentListItem.Action.CHECK_BOX);
        item.setIcon(getDrawable(R.drawable.ic_launcher));
        item.setEnabled(false);
        item.setTitle(getString(R.string.title_with_disabled_checkbox));
        item.setBody(getString(R.string.click_should_have_no_effect));
        item.setOnCheckedChangeListener(
                (listItem, isChecked) -> Toast.makeText(context,
                        "Item checked state is: " + isChecked, Toast.LENGTH_SHORT).show());
        mData.add(item);

        item = new CarUiContentListItem(CarUiContentListItem.Action.SWITCH);
        item.setIcon(getDrawable(R.drawable.ic_launcher));
        item.setBody(getString(R.string.body_item_with_switch));
        item.setOnItemClickedListener(item1 -> {
            Toast.makeText(context, "Click on item with switch", Toast.LENGTH_SHORT).show();
        });
        mData.add(item);

        item = new CarUiContentListItem(CarUiContentListItem.Action.CHECK_BOX);
        item.setIcon(getDrawable(R.drawable.ic_launcher));
        item.setTitle(getString(R.string.title_item_with_checkbox));
        item.setBody(getString(R.string.item_initially_checked));
        item.setChecked(true);
        mData.add(item);

        CarUiContentListItem radioItem1 = new CarUiContentListItem(
                CarUiContentListItem.Action.RADIO_BUTTON);
        CarUiContentListItem radioItem2 = new CarUiContentListItem(
                CarUiContentListItem.Action.RADIO_BUTTON);

        radioItem1.setTitle(getString(R.string.title_item_with_radio_button));
        radioItem1.setBody(getString(R.string.item_initially_checked));
        radioItem1.setChecked(false);
        radioItem1.setOnCheckedChangeListener((listItem, isChecked) -> {
            if (isChecked) {
                radioItem2.setChecked(false);
                mAdapter.notifyItemChanged(mData.indexOf(radioItem2));
            }
        });
        mData.add(radioItem1);

        radioItem2.setIcon(getDrawable(R.drawable.ic_launcher));
        radioItem2.setTitle(getString(R.string.item_mutually_exclusive_with_item_above));
        radioItem2.setChecked(true);
        radioItem2.setOnCheckedChangeListener((listItem, isChecked) -> {
            if (isChecked) {
                radioItem1.setChecked(false);
                mAdapter.notifyItemChanged(mData.indexOf(radioItem1));
            }
        });
        mData.add(radioItem2);

        item = new CarUiContentListItem(CarUiContentListItem.Action.ICON);
        item.setIcon(getDrawable(R.drawable.ic_launcher));
        item.setTitle(getString(R.string.test_title));
        item.setBody(getString(R.string.random_body_text_with_action_divider));
        item.setActionDividerVisible(true);
        item.setSupplementalIcon(getDrawable(R.drawable.ic_launcher));
        item.setChecked(true);
        mData.add(item);

        item = new CarUiContentListItem(CarUiContentListItem.Action.ICON);
        item.setIcon(getDrawable(R.drawable.ic_launcher));
        item.setTitle(getString(R.string.null_supplement_icon));
        item.setChecked(true);
        mData.add(item);

        item = new CarUiContentListItem(CarUiContentListItem.Action.ICON);
        item.setTitle(getString(R.string.supplemental_icon_with_listener));
        item.setPrimaryIconType(CarUiContentListItem.IconType.CONTENT);
        item.setIcon(getDrawable(R.drawable.ic_launcher));
        item.setBody(getString(R.string.test_body));
        item.setOnItemClickedListener(v -> Toast.makeText(context, "Clicked item",
                Toast.LENGTH_SHORT).show());
        item.setSupplementalIcon(getDrawable(R.drawable.ic_launcher),
                v -> Toast.makeText(context, "Clicked supplemental icon",
                        Toast.LENGTH_SHORT).show());
        item.setChecked(true);
        mData.add(item);

        item = new CarUiContentListItem(CarUiContentListItem.Action.CHEVRON);
        item.setTitle(getString(R.string.item_with_chevron));
        item.setBody(getString(R.string.test_body));
        mData.add(item);

        return mData;
    }

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
}
