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

package com.android.car.ui.appstyledview;

import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.FrameLayout;

import androidx.annotation.NonNull;
import androidx.recyclerview.widget.RecyclerView;

import com.android.car.ui.R;
import com.android.car.ui.appstyledview.AppStyledRecyclerViewAdapter.AppStyledRecyclerViewHolder;

/**
 * Adapter for app styled dialog.
 */
public class AppStyledRecyclerViewAdapter extends
        RecyclerView.Adapter<AppStyledRecyclerViewHolder> {

    private View mContent;

    public AppStyledRecyclerViewAdapter(View content) {
        mContent = content;
    }

    @NonNull
    @Override
    public AppStyledRecyclerViewHolder onCreateViewHolder(@NonNull ViewGroup parent, int viewType) {
        LayoutInflater inflator = LayoutInflater.from(parent.getContext());
        View view = inflator.inflate(R.layout.car_ui_app_styled_view_item, parent, false);
        return new AppStyledRecyclerViewHolder(view);
    }

    @Override
    public void onBindViewHolder(@NonNull AppStyledRecyclerViewHolder holder, int position) {
        FrameLayout frameLayout = holder.mView.findViewById(R.id.car_ui_app_styled_item);
        frameLayout.addView(mContent);
    }

    @Override
    public int getItemCount() {
        return 1;
    }

    /**
     * Holds views for each element in the list.
     */
    public static class AppStyledRecyclerViewHolder extends RecyclerView.ViewHolder {

        View mView;

        AppStyledRecyclerViewHolder(@NonNull View itemView) {
            super(itemView);
            mView = itemView;
        }
    }
}

