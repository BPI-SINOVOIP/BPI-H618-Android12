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

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNull;
import static org.mockito.Mockito.any;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.reset;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.graphics.drawable.Drawable;

import androidx.lifecycle.MutableLiveData;
import androidx.test.core.app.ApplicationProvider;

import com.android.car.arch.common.testing.InstantTaskExecutorRule;
import com.android.car.carlauncher.homescreen.HomeCardInterface;
import com.android.car.carlauncher.homescreen.ui.CardHeader;
import com.android.car.carlauncher.homescreen.ui.DescriptiveTextWithControlsView;
import com.android.car.media.common.MediaItemMetadata;
import com.android.car.media.common.playback.PlaybackViewModel;
import com.android.car.media.common.source.MediaSource;
import com.android.car.media.common.source.MediaSourceViewModel;

import org.junit.After;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;

@RunWith(JUnit4.class)
public class MediaViewModelTest {

    private static final CharSequence APP_NAME = "test app name";
    private static final Drawable APP_ICON = null;
    private static final CharSequence SONG_TITLE = "test song title";
    private static final CharSequence ARTIST_NAME = "test artist name";

    private MediaViewModel mMediaViewModel;
    private MutableLiveData<MediaSource> mLiveMediaSource = new MutableLiveData<>();
    private MutableLiveData<MediaItemMetadata> mLiveMetadata = new MutableLiveData<>();

    @Mock
    private MediaSourceViewModel mSourceViewModel;
    @Mock
    private PlaybackViewModel mPlaybackViewModel;
    @Mock
    private MediaSource mMediaSource;
    @Mock
    private MediaItemMetadata mMetadata;
    @Mock
    private HomeCardInterface.Presenter mPresenter;

    // The tests use the MediaViewModel's observers. To avoid errors with invoking observeForever
    // on a background thread, this rule configures LiveData to execute each task synchronously.
    @Rule
    public final InstantTaskExecutorRule mTaskExecutorRule = new InstantTaskExecutorRule();

    @Before
    public void setUp() {
        MockitoAnnotations.initMocks(this);
        mMediaViewModel = new MediaViewModel(ApplicationProvider.getApplicationContext(),
                mSourceViewModel, mPlaybackViewModel);
        when(mSourceViewModel.getPrimaryMediaSource()).thenReturn(mLiveMediaSource);
        when(mPlaybackViewModel.getMetadata()).thenReturn(mLiveMetadata);
        mMediaViewModel.setPresenter(mPresenter);
        mMediaViewModel.onCreate(ApplicationProvider.getApplicationContext());
        reset(mPresenter);
    }

    @After
    public void after() {
        mMediaViewModel.onCleared();
    }

    @Test
    public void noChange_doesNotCallPresenter() {
        verify(mPresenter, never()).onModelUpdated(any());
        assertNull(mMediaViewModel.getCardHeader());
        DescriptiveTextWithControlsView content =
                (DescriptiveTextWithControlsView) mMediaViewModel.getCardContent();
        assertNull(content.getTitle());
        assertNull(content.getSubtitle());
    }

    @Test
    public void changeSourceAndMetadata_updatesModel() {
        when(mMediaSource.getDisplayName()).thenReturn(APP_NAME);
        when(mMediaSource.getIcon()).thenReturn(APP_ICON);
        when(mMetadata.getArtist()).thenReturn(ARTIST_NAME);
        when(mMetadata.getTitle()).thenReturn(SONG_TITLE);

        mLiveMediaSource.setValue(mMediaSource);
        mLiveMetadata.setValue(mMetadata);

        // Model is updated exactly twice: once when source is set (null metadata)
        // and again when the metadata is set
        verify(mPresenter, times(2)).onModelUpdated(mMediaViewModel);
        CardHeader header = mMediaViewModel.getCardHeader();
        assertEquals(header.getCardTitle(), APP_NAME);
        assertNull(header.getCardIcon());
        DescriptiveTextWithControlsView content =
                (DescriptiveTextWithControlsView) mMediaViewModel.getCardContent();
        assertEquals(content.getTitle(), SONG_TITLE);
        assertEquals(content.getSubtitle(), ARTIST_NAME);
    }

    @Test
    public void changeSourceOnly_updatesModel() {
        when(mMediaSource.getDisplayName()).thenReturn(APP_NAME);
        when(mMediaSource.getIcon()).thenReturn(APP_ICON);

        mLiveMediaSource.setValue(mMediaSource);

        verify(mPresenter).onModelUpdated(mMediaViewModel);
        CardHeader header = mMediaViewModel.getCardHeader();
        assertEquals(header.getCardTitle(), APP_NAME);
        assertNull(header.getCardIcon());
        DescriptiveTextWithControlsView content =
                (DescriptiveTextWithControlsView) mMediaViewModel.getCardContent();
        assertNull(content.getTitle());
        assertNull(content.getSubtitle());
    }

    @Test
    public void changeMetadataOnly_doesNotCallPresenter() {
        when(mMetadata.getArtist()).thenReturn(ARTIST_NAME);
        when(mMetadata.getTitle()).thenReturn(SONG_TITLE);

        mLiveMetadata.setValue(mMetadata);

        verify(mPresenter, never()).onModelUpdated(any());
        assertNull(mMediaViewModel.getCardHeader());
        DescriptiveTextWithControlsView content =
                (DescriptiveTextWithControlsView) mMediaViewModel.getCardContent();
        assertEquals(content.getTitle(), SONG_TITLE);
        assertEquals(content.getSubtitle(), ARTIST_NAME);
    }
}

