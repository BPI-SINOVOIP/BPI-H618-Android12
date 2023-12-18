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
import static org.junit.Assert.assertNotNull;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.verify;

import android.content.Context;
import android.net.Uri;
import android.telecom.Call;

import androidx.test.core.app.ApplicationProvider;

import com.android.car.carlauncher.R;
import com.android.car.carlauncher.homescreen.HomeCardInterface;
import com.android.car.carlauncher.homescreen.ui.DescriptiveTextWithControlsView;
import com.android.car.telephony.common.TelecomUtils;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;

import java.time.Clock;

@RunWith(JUnit4.class)
public class InCallModelTest {

    private static final String PHONE_NUMBER = "01234567";
    private static final String DISPLAY_NAME = "Test Caller";
    private static final String INITIALS = "T";

    private InCallModel mInCallModel;
    private String mOngoingCallSecondaryText;
    private Context mContext;

    @Mock
    private HomeCardInterface.Presenter mPresenter;
    @Mock
    private Clock mClock;

    private Call mCall = null;

    @Before
    public void setUp() {
        MockitoAnnotations.initMocks(this);
        mContext = ApplicationProvider.getApplicationContext();
        mInCallModel = new InCallModel(mClock);
        mInCallModel.setPresenter(mPresenter);
        mInCallModel.onCreate(mContext);
        mOngoingCallSecondaryText =
                ApplicationProvider.getApplicationContext().getResources().getString(
                        R.string.ongoing_call_text);
    }

    @After
    public void tearDown() {
        mInCallModel.onDestroy(ApplicationProvider.getApplicationContext());
    }

    @Test
    public void noChange_doesNotCallPresenter() {
        verify(mPresenter, never()).onModelUpdated(any());
    }

    @Test
    public void onCallRemoved_callsPresenter() {
        mInCallModel.onCallRemoved(mCall);

        verify(mPresenter).onModelUpdated(mInCallModel);
    }

    @Test
    public void updateModelWithPhoneNumber_setsPhoneNumber() {
        mInCallModel.updateModelWithPhoneNumber(PHONE_NUMBER);

        verify(mPresenter).onModelUpdated(mInCallModel);
        DescriptiveTextWithControlsView content =
                (DescriptiveTextWithControlsView) mInCallModel.getCardContent();
        String formattedNumber = TelecomUtils.getFormattedNumber(
                ApplicationProvider.getApplicationContext(), PHONE_NUMBER);
        assertEquals(content.getTitle(), formattedNumber);
        assertEquals(content.getSubtitle(), mOngoingCallSecondaryText);
    }

    @Test
    public void updateModelWithContact_noAvatarUri_setsContactNameAndInitialsIcon() {
        TelecomUtils.PhoneNumberInfo phoneInfo = new TelecomUtils.PhoneNumberInfo(PHONE_NUMBER,
                DISPLAY_NAME, DISPLAY_NAME, INITIALS, /* avatarUri = */ null,
                /* typeLabel = */ null, /*  lookupKey = */ null);
        mInCallModel.updateModelWithContact(phoneInfo);

        verify(mPresenter).onModelUpdated(mInCallModel);
        DescriptiveTextWithControlsView content =
                (DescriptiveTextWithControlsView) mInCallModel.getCardContent();
        assertEquals(content.getTitle(), DISPLAY_NAME);
        assertEquals(content.getSubtitle(), mOngoingCallSecondaryText);
        assertNotNull(content.getImage());
    }

    @Test
    public void updateModelWithContact_invalidAvatarUri_setsContactNameAndInitialsIcon() {
        Uri invalidUri = new Uri.Builder().path("invalid uri path").build();
        TelecomUtils.PhoneNumberInfo phoneInfo = new TelecomUtils.PhoneNumberInfo(PHONE_NUMBER,
                DISPLAY_NAME, DISPLAY_NAME, INITIALS, invalidUri, /* typeLabel = */ null,
                /* lookupKey = */ null);
        mInCallModel.updateModelWithContact(phoneInfo);

        verify(mPresenter).onModelUpdated(mInCallModel);
        DescriptiveTextWithControlsView content =
                (DescriptiveTextWithControlsView) mInCallModel.getCardContent();
        assertEquals(content.getTitle(), DISPLAY_NAME);
        assertEquals(content.getSubtitle(), mOngoingCallSecondaryText);
        assertNotNull(content.getImage());
    }
}
