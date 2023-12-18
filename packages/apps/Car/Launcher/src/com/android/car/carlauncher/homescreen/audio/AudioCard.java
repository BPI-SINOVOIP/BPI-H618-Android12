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

package com.android.car.carlauncher.homescreen.audio;

import android.os.SystemClock;
import android.util.Log;

import androidx.lifecycle.ViewModelProvider;

import com.android.car.carlauncher.R;
import com.android.car.carlauncher.homescreen.CardPresenter;
import com.android.car.carlauncher.homescreen.HomeCardFragment;
import com.android.car.carlauncher.homescreen.HomeCardModule;

import java.util.Arrays;
import java.util.Collections;

/**
 * Home screen card that displays audio related content
 */
public class AudioCard implements HomeCardModule {

    private static final String TAG = "HomeScreenAudioCard";

    private ViewModelProvider mViewModelProvider;
    private HomeAudioCardPresenter mAudioCardPresenter;
    private AudioFragment mAudioCardView;

    @Override
    public void setViewModelProvider(ViewModelProvider viewModelProvider) {
        mViewModelProvider = viewModelProvider;
    }

    @Override
    public int getCardResId() {
        return R.id.bottom_card;
    }

    @Override
    public CardPresenter getCardPresenter() {
        if (mAudioCardPresenter == null) {
            mAudioCardPresenter = new HomeAudioCardPresenter();
            if (mViewModelProvider == null) {
                Log.w(TAG, "No ViewModelProvider set. Cannot get MediaViewModel");
                mAudioCardPresenter.setModels(
                        Collections.unmodifiableList(Collections.singletonList(
                                new InCallModel(SystemClock.elapsedRealtimeClock()))));
            } else {
                mAudioCardPresenter.setModels(Collections.unmodifiableList(Arrays.asList(
                        mViewModelProvider.get(MediaViewModel.class),
                        new InCallModel(SystemClock.elapsedRealtimeClock()))));
            }
        }
        return mAudioCardPresenter;
    }

    @Override
    public HomeCardFragment getCardView() {
        if (mAudioCardView == null) {
            mAudioCardView = new AudioFragment();
            getCardPresenter().setView(mAudioCardView);
            mAudioCardView.setPresenter(getCardPresenter());
        }
        return mAudioCardView;
    }
}
