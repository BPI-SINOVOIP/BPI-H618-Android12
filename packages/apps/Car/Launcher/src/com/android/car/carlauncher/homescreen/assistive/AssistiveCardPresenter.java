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

package com.android.car.carlauncher.homescreen.assistive;

import android.view.View;

import com.android.car.carlauncher.homescreen.CardPresenter;
import com.android.car.carlauncher.homescreen.HomeCardInterface;

import java.util.List;

/**
 * The {@link CardPresenter} for an assistive card.
 */
public class AssistiveCardPresenter extends CardPresenter {

    private HomeCardInterface.Model mCurrentModel;
    private List<HomeCardInterface.Model> mModels;

    @Override
    public void setModels(List<HomeCardInterface.Model> models) {
        mModels = models;
    }

    /**
     * Called when the View is created
     */
    @Override
    public void onViewCreated() {
        for (HomeCardInterface.Model model : mModels) {
            model.setPresenter(this);
            model.onCreate(getFragment().requireContext());
        }
    }

    /**
     * Called when the View is destroyed
     */
    @Override
    public void onViewDestroyed() {
        if (mModels != null) {
            for (HomeCardInterface.Model model : mModels) {
                model.onDestroy(getFragment().requireContext());
            }
        }
    }

    /**
     * Called when the View is clicked
     */
    @Override
    public void onViewClicked(View v) {
        mCurrentModel.onClick(v);
    }

    /**
     * Called when a Model is updated.
     */
    @Override
    public void onModelUpdated(HomeCardInterface.Model model) {
        if (model.getCardHeader() == null) {
            if (mCurrentModel != null && model.getClass() == mCurrentModel.getClass()) {
                if (mModels != null) {
                    // Check if any other models have content to display
                    for (HomeCardInterface.Model candidate : mModels) {
                        if (candidate.getCardHeader() != null) {
                            mCurrentModel = candidate;
                            super.onModelUpdated(candidate);
                            return;
                        }
                    }
                }
            } else {
                // Otherwise, another model is already on display,
                return;
            }
        }
        mCurrentModel = model;
        super.onModelUpdated(model);
    }
}
