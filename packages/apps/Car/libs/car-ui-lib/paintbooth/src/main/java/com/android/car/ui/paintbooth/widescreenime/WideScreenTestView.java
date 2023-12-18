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

package com.android.car.ui.paintbooth.widescreenime;

import android.os.Bundle;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;
import androidx.recyclerview.widget.LinearLayoutManager;

import com.android.car.ui.baselayout.Insets;
import com.android.car.ui.baselayout.InsetsChangedListener;
import com.android.car.ui.core.CarUi;
import com.android.car.ui.paintbooth.R;
import com.android.car.ui.paintbooth.caruirecyclerview.RecyclerViewAdapter;
import com.android.car.ui.recyclerview.CarUiRecyclerView;
import com.android.car.ui.toolbar.MenuItem;
import com.android.car.ui.toolbar.NavButtonMode;
import com.android.car.ui.toolbar.SearchConfig;
import com.android.car.ui.toolbar.SearchMode;
import com.android.car.ui.toolbar.ToolbarController;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

/**
 * Activity that the custom view inflated in IME window.
 */
public class WideScreenTestView extends AppCompatActivity implements InsetsChangedListener {

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.car_ui_recycler_view_activity);

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

        toolbar.setMenuItems(Collections.singletonList(MenuItem.builder(this)
                .setToSearch()
                .setOnClickListener(i -> {
                    isSearching[0] = true;
                    toolbar.setSearchMode(SearchMode.SEARCH);
                    if (toolbar.getSearchCapabilities().canShowSearchResultsView()) {
                        toolbar.setSearchConfig(SearchConfig.builder()
                                .setSearchResultsView(findViewById(R.id.list)).build());
                    }
                }).build()));

        CarUiRecyclerView recyclerView = findViewById(R.id.list);
        recyclerView.setLayoutManager(new LinearLayoutManager(this));

        RecyclerViewAdapter adapter = new RecyclerViewAdapter(generateSampleData());
        recyclerView.setAdapter(adapter);
    }

    private List<String> generateSampleData() {
        List<String> data = new ArrayList<>();
        for (int i = 0; i <= 100; i++) {
            data.add(getString(R.string.test_data) + i);
        }
        return data;
    }

    @Override
    public void onCarUiInsetsChanged(@NonNull Insets insets) {
        requireViewById(R.id.list)
                .setPadding(0, insets.getTop(), 0, insets.getBottom());
        requireViewById(android.R.id.content)
                .setPadding(insets.getLeft(), 0, insets.getRight(), 0);
    }
}
