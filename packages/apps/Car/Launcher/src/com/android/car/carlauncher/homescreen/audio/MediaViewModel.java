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

import static android.car.media.CarMediaManager.MEDIA_SOURCE_MODE_PLAYBACK;

import android.app.Application;
import android.car.Car;
import android.content.Context;
import android.content.Intent;
import android.graphics.drawable.Drawable;
import android.util.Size;
import android.view.View;

import androidx.annotation.NonNull;
import androidx.lifecycle.AndroidViewModel;
import androidx.lifecycle.Observer;

import com.android.car.apps.common.imaging.ImageBinder;
import com.android.car.carlauncher.homescreen.HomeCardInterface;
import com.android.car.carlauncher.homescreen.ui.CardContent;
import com.android.car.carlauncher.homescreen.ui.CardHeader;
import com.android.car.carlauncher.homescreen.ui.DescriptiveTextWithControlsView;
import com.android.car.media.common.MediaItemMetadata;
import com.android.car.media.common.R;
import com.android.car.media.common.playback.PlaybackViewModel;
import com.android.car.media.common.source.MediaSource;
import com.android.car.media.common.source.MediaSourceViewModel;
import com.android.internal.annotations.VisibleForTesting;


/**
 * ViewModel for media. Uses both a {@link MediaSourceViewModel} and a {@link PlaybackViewModel}
 * for data on the audio source and audio metadata (such as song title), respectively.
 */
public class MediaViewModel extends AndroidViewModel implements HomeCardInterface.Model {

    private HomeCardInterface.Presenter mAudioPresenter;
    // MediaSourceViewModel is for the current or last played media app
    private MediaSourceViewModel mSourceViewModel;
    // PlaybackViewModel has the media's metadata
    private PlaybackViewModel mPlaybackViewModel;
    private Context mContext;

    private CardHeader mCardHeader;
    private CharSequence mAppName;
    private Drawable mAppIcon;
    private CharSequence mSongTitle;
    private CharSequence mArtistName;
    private ImageBinder<MediaItemMetadata.ArtworkRef> mAlbumArtBinder;
    private Drawable mAlbumImageBitmap;

    private Observer<Object> mMediaSourceObserver = x -> updateModel();
    private Observer<Object> mMetadataObserver = x -> updateModelMetadata();

    public MediaViewModel(Application application) {
        super(application);
    }

    @VisibleForTesting
    MediaViewModel(Application application, MediaSourceViewModel sourceViewModel,
            PlaybackViewModel playbackViewModel) {
        super(application);
        mSourceViewModel = sourceViewModel;
        mPlaybackViewModel = playbackViewModel;
    }

    @Override
    public void onCreate(@NonNull Context context) {
        if (mSourceViewModel == null) {
            mSourceViewModel = MediaSourceViewModel.get(getApplication(),
                    MEDIA_SOURCE_MODE_PLAYBACK);
        }
        if (mPlaybackViewModel == null) {
            mPlaybackViewModel = PlaybackViewModel.get(getApplication(),
                    MEDIA_SOURCE_MODE_PLAYBACK);
        }

        mContext = context;
        int max = context.getResources().getInteger(R.integer.media_items_bitmap_max_size_px);
        Size maxArtSize = new Size(max, max);
        mAlbumArtBinder = new ImageBinder<>(ImageBinder.PlaceholderType.FOREGROUND, maxArtSize,
                drawable -> {
                    mAlbumImageBitmap = drawable;
                    mAudioPresenter.onModelUpdated(this);
                });
        mSourceViewModel.getPrimaryMediaSource().observeForever(mMediaSourceObserver);
        mPlaybackViewModel.getMetadata().observeForever(mMetadataObserver);
        mAudioPresenter.onModelUpdated(this);
    }

    @Override
    protected void onCleared() {
        super.onCleared();
        mSourceViewModel.getPrimaryMediaSource().removeObserver(mMediaSourceObserver);
        mPlaybackViewModel.getMetadata().removeObserver(mMetadataObserver);
    }

    @Override
    public void onClick(View v) {
        v.getContext().startActivity(new Intent(Car.CAR_INTENT_ACTION_MEDIA_TEMPLATE));
    }

    /**
     * Sets the Presenter, which will handle updating the UI
     */
    @Override
    public void setPresenter(HomeCardInterface.Presenter presenter) {
        mAudioPresenter = presenter;
    }

    @Override
    public CardHeader getCardHeader() {
        return mCardHeader;
    }

    @Override
    public CardContent getCardContent() {
        return new DescriptiveTextWithControlsView(mAlbumImageBitmap, mSongTitle, mArtistName);
    }

    /**
     * Package private method to allow the {@link HomeAudioCardPresenter} to access the model to
     * initialize the {@link com.android.car.media.common.PlaybackControlsActionBar}
     */
    PlaybackViewModel getPlaybackViewModel() {
        return mPlaybackViewModel;
    }

    /**
     * Callback for the observer of the MediaSourceViewModel
     */
    private void updateModel() {
        MediaSource mediaSource = mSourceViewModel.getPrimaryMediaSource().getValue();
        if (mediaSourceChanged()) {
            if (mediaSource == null) {
                mAppName = null;
                mAppIcon = null;
                mCardHeader = null;
                clearMetadata();
            } else {
                mAppName = mediaSource.getDisplayName();
                mAppIcon = mediaSource.getIcon();
                mCardHeader = new CardHeader(mAppName, mAppIcon);
                updateMetadata();
            }
            mAudioPresenter.onModelUpdated(this);
        }
    }

    /**
     * Callback for the observer of the PlaybackViewModel
     */
    private void updateModelMetadata() {
        if (metadataChanged()) {
            updateMetadata();
            if (mCardHeader != null) {
                mAudioPresenter.onModelUpdated(this);
            }
        }
    }

    private void updateMetadata() {
        MediaItemMetadata metadata = mPlaybackViewModel.getMetadata().getValue();
        if (metadata == null) {
            clearMetadata();
        } else {
            mSongTitle = metadata.getTitle();
            mArtistName = metadata.getArtist();
            mAlbumArtBinder.setImage(mContext, metadata.getArtworkKey());
        }
    }

    private void clearMetadata() {
        mSongTitle = null;
        mArtistName = null;
        mAlbumArtBinder.setImage(mContext, /* newArtRef = */ null);
    }

    /**
     * Helper method to check for a change in the media's metadata
     */
    private boolean metadataChanged() {
        MediaItemMetadata metadata = mPlaybackViewModel.getMetadata().getValue();
        if (metadata == null && (mSongTitle != null || mArtistName != null)) {
            return true;
        }
        if (metadata != null && (mSongTitle != metadata.getTitle()
                || mArtistName != metadata.getArtist())) {
            return true;
        }
        return false;
    }

    /**
     * Helper method to check for a change in the media source
     */
    private boolean mediaSourceChanged() {
        MediaSource mediaSource = mSourceViewModel.getPrimaryMediaSource().getValue();
        if (mediaSource == null && (mAppName != null | mAppIcon != null)) {
            return true;
        }
        if (mediaSource != null && (mAppName != mediaSource.getDisplayName()
                || mAppIcon != mediaSource.getIcon())) {
            return true;
        }
        return false;
    }
}
