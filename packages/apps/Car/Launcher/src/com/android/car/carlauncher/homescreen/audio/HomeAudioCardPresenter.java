/*
 * Copyright (C) 2020 Google Inc.
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

package com.android.car.carlauncher.homescreen.audio;

import android.view.View;

import com.android.car.carlauncher.homescreen.CardPresenter;
import com.android.car.carlauncher.homescreen.HomeCardInterface;
import com.android.car.media.common.PlaybackControlsActionBar;

import java.util.List;

/**
 * The {@link CardPresenter} for an audio card.
 *
 * For the audio card, the {@link AudioFragment} implements the View and displays information on
 * media from a {@link MediaViewModel}.
 */
public class HomeAudioCardPresenter extends CardPresenter {

    private HomeCardInterface.Model mCurrentModel;
    private List<HomeCardInterface.Model> mModelList;
    private MediaViewModel mMediaViewModel;

    @Override
    public void setModels(List<HomeCardInterface.Model> models) {
        mModelList = models;
    }

    /**
     * Called when the View is created
     */
    @Override
    public void onViewCreated() {
        for (HomeCardInterface.Model model : mModelList) {
            if (model.getClass() == MediaViewModel.class) {
                mMediaViewModel = (MediaViewModel) model;
            }
            model.setPresenter(this);
            model.onCreate(getFragment().requireContext());
        }
    }

    /**
     * Called when the View is destroyed
     */
    @Override
    public void onViewDestroyed() {
        if (mModelList != null) {
            for (HomeCardInterface.Model model : mModelList) {
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
     * Updates the View appropriately when a Model has new content.
     *
     * If the updated model has content, it is displayed, regardless of what is currently shown on
     * the card. Otherwise if the model on display is updating to empty content (eg. when a call
     * ends, the InCallModel header and content are updated to null), default to showing the media
     * model if it has content.
     */
    @Override
    public void onModelUpdated(HomeCardInterface.Model model) {
        // Null card header indicates the model has no content to display
        if (model.getCardHeader() == null) {
            if (mCurrentModel != null && model.getClass() == mCurrentModel.getClass()) {
                // If the model currently on display is updating to empty content, check if there
                // is media content to display. If there is no media content the super method is
                // called with empty content, which hides the card.
                if (mMediaViewModel != null && mMediaViewModel.getCardHeader() != null) {
                    mCurrentModel = mMediaViewModel;
                    super.onModelUpdated(mMediaViewModel);
                    return;
                }
            } else {
                // Otherwise, another model is already on display, so don't update with this
                // empty content since that would hide the card.
                return;
            }
        }
        mCurrentModel = model;
        super.onModelUpdated(model);
    }

    void initializeControlsActionBar(View actionBar) {
        // TODO(b/159452592): implement media control bar instead of using PlaybackControlsActionBar
        // The PlaybackControlsActionBar requires direct access to the PlaybackViewModel, which
        // directly connects a View to a Model. To fit the design of the home app, interactions
        // should be mediated by the HomeAudioCardPresenter instead. Using these existing classes
        // for now until the logic can be brought into the MediaViewModel.
        ((PlaybackControlsActionBar) actionBar).setModel(mMediaViewModel.getPlaybackViewModel(),
                getFragment().getViewLifecycleOwner());
    }
}
