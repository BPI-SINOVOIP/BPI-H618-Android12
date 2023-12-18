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

import android.graphics.Bitmap;
import android.graphics.drawable.Drawable;
import android.os.Bundle;
import android.util.Size;
import android.view.View;
import android.view.ViewStub;
import android.widget.Chronometer;
import android.widget.TextView;

import com.android.car.apps.common.BitmapUtils;
import com.android.car.apps.common.ImageUtils;
import com.android.car.carlauncher.R;
import com.android.car.carlauncher.homescreen.HomeCardFragment;
import com.android.car.carlauncher.homescreen.HomeCardInterface;
import com.android.car.carlauncher.homescreen.ui.CardContent;
import com.android.car.carlauncher.homescreen.ui.DescriptiveTextWithControlsView;


/**
 * {@link HomeCardInterface.View} for the audio card. Displays and controls the current audio source
 * such as the currently playing (or last played) media item or an ongoing phone call.
 */
public class AudioFragment extends HomeCardFragment {

    private HomeAudioCardPresenter mPresenter;
    private Chronometer mChronometer;
    private View mChronometerSeparator;
    private float mBlurRadius;

    // Views from card_content_media.xml, which is used only for the media card
    private View mMediaLayoutView;
    private TextView mMediaTitle;
    private TextView mMediaSubtitle;

    @Override
    public void setPresenter(HomeCardInterface.Presenter presenter) {
        super.setPresenter(presenter);
        mPresenter = (HomeAudioCardPresenter) presenter;
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        mBlurRadius = getResources().getFloat(R.dimen.card_background_image_blur_radius);
    }

    @Override
    public void updateContentViewInternal(CardContent content) {
        if (content.getType() == CardContent.HomeCardContentType.DESCRIPTIVE_TEXT_WITH_CONTROLS) {
            DescriptiveTextWithControlsView audioContent =
                    (DescriptiveTextWithControlsView) content;
            updateBackgroundImage(audioContent.getImage());
            if (audioContent.getCenterControl() == null) {
                updateMediaView(audioContent.getTitle(), audioContent.getSubtitle());
            } else {
                updateDescriptiveTextWithControlsView(audioContent.getTitle(),
                        audioContent.getSubtitle(),
                        /* optionalImage= */ null, audioContent.getLeftControl(),
                        audioContent.getCenterControl(), audioContent.getRightControl());
                updateAudioDuration(audioContent);
            }
        } else {
            super.updateContentViewInternal(content);
        }
    }

    @Override
    protected void hideAllViews() {
        super.hideAllViews();
        getCardBackground().setVisibility(View.GONE);
        getMediaLayoutView().setVisibility(View.GONE);
    }

    private Chronometer getChronometer() {
        if (mChronometer == null) {
            mChronometer = getDescriptiveTextWithControlsLayoutView().findViewById(
                    R.id.optional_timer);
            mChronometerSeparator = getDescriptiveTextWithControlsLayoutView().findViewById(
                    R.id.optional_timer_separator);
        }
        return mChronometer;
    }

    private View getMediaLayoutView() {
        if (mMediaLayoutView == null) {
            ViewStub stub = getRootView().findViewById(R.id.media_layout);
            mMediaLayoutView = stub.inflate();
            mMediaTitle = mMediaLayoutView.findViewById(R.id.primary_text);
            mMediaSubtitle = mMediaLayoutView.findViewById(R.id.secondary_text);
            View mediaControlBarView = mMediaLayoutView.findViewById(
                    R.id.media_playback_controls_bar);
            mPresenter.initializeControlsActionBar(mediaControlBarView);
        }
        return mMediaLayoutView;
    }


    private void updateBackgroundImage(Drawable image) {
        if (image != null && getCardSize() != null) {
            int maxDimen = Math.max(getCardSize().getWidth(), getCardSize().getHeight());
            Size scaledSize = new Size(maxDimen, maxDimen);
            Bitmap imageBitmap = BitmapUtils.fromDrawable(image, scaledSize);
            Bitmap blurredBackground = ImageUtils.blur(getContext(), imageBitmap, scaledSize,
                    mBlurRadius);

            getCardBackgroundImage().setImageBitmap(blurredBackground, /* showAnimation= */ true);
            getCardBackground().setVisibility(View.VISIBLE);
        }
    }

    private void updateMediaView(CharSequence title, CharSequence subtitle) {
        getMediaLayoutView().setVisibility(View.VISIBLE);
        mMediaTitle.setText(title);
        mMediaSubtitle.setText(subtitle);
    }

    private void updateAudioDuration(DescriptiveTextWithControlsView content) {
        if (content.getStartTime() > 0) {
            getChronometer().setVisibility(View.VISIBLE);
            getChronometer().setBase(content.getStartTime());
            getChronometer().start();
            mChronometerSeparator.setVisibility(View.VISIBLE);
        } else {
            getChronometer().setVisibility(View.GONE);
            mChronometerSeparator.setVisibility(View.GONE);
        }
    }
}
