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
package com.android.car.ui;

import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.recyclerview.widget.RecyclerView;
import androidx.recyclerview.widget.RecyclerView.Adapter;

import com.android.car.rotary.test.R;
import com.android.car.ui.TestAdapter.TestViewHolder;

import java.util.ArrayList;
import java.util.List;

public final class TestAdapter extends Adapter<TestViewHolder> {

    private final List<String> mItems;

    public TestAdapter(int numItems) {
        mItems = new ArrayList<>();
        for (int i = 0; i < numItems; i++) {
            mItems.add("Item " + i);
        }
    }

    @Override
    public TestViewHolder onCreateViewHolder(@NonNull ViewGroup parent,
            int viewType) {
        View layout = LayoutInflater.from(parent.getContext())
                .inflate(R.layout.test_car_ui_recycler_view_list_item, parent, false);
        return new TestViewHolder(layout);
    }

    @Override
    public void onBindViewHolder(TestViewHolder holder, int position) {
        holder.bind(mItems.get(position));
    }

    @Override
    public int getItemCount() {
        return mItems.size();
    }

    static class TestViewHolder extends RecyclerView.ViewHolder {

        TestViewHolder(@NonNull View itemView) {
            super(itemView);
        }

        void bind(CharSequence text) {
            TextView textView = itemView.requireViewById(R.id.textTitle);
            textView.setText(text);
        }
    }
}
