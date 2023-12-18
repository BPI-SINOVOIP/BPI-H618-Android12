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

package com.android.car.settings.profiles;

import static android.content.pm.UserInfo.FLAG_INITIALIZED;

import static com.android.car.settings.profiles.ProfilesDialogProvider.ANY_PROFILE;
import static com.android.car.settings.profiles.ProfilesDialogProvider.KEY_PROFILE_TYPE;
import static com.android.car.settings.profiles.ProfilesDialogProvider.LAST_ADMIN;
import static com.android.car.settings.profiles.RemoveProfileHandler.REMOVE_PROFILE_DIALOG_TAG;

import static com.google.common.truth.Truth.assertThat;

import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.content.Context;
import android.content.pm.UserInfo;
import android.os.Bundle;
import android.os.UserManager;

import androidx.test.annotation.UiThreadTest;
import androidx.test.core.app.ApplicationProvider;
import androidx.test.ext.junit.runners.AndroidJUnit4;

import com.android.car.settings.common.ConfirmationDialogFragment;
import com.android.car.settings.common.FragmentController;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;

@RunWith(AndroidJUnit4.class)
public class RemoveProfileHandlerTest {
    private static final String TEST_PROFILE_NAME = "Test Profile Name";

    private Context mContext = ApplicationProvider.getApplicationContext();
    private RemoveProfileHandler mRemoveProfileHandler;

    @Mock
    private FragmentController mMockFragmentController;
    @Mock
    private ProfileHelper mMockProfileHelper;
    @Mock
    private UserManager mMockUserManager;

    @Before
    public void setUp() {
        MockitoAnnotations.initMocks(this);
        mRemoveProfileHandler = new RemoveProfileHandler(
                mContext, mMockProfileHelper, mMockUserManager, mMockFragmentController);
    }

    @Test
    public void userNotRestricted_canRemoveProfile() {
        UserInfo userInfo = new UserInfo(/* id= */ 10, TEST_PROFILE_NAME, FLAG_INITIALIZED);
        mRemoveProfileHandler.setUserInfo(userInfo);
        when(mMockProfileHelper.isCurrentProcessUser(userInfo)).thenReturn(false);
        assertThat(mRemoveProfileHandler.canRemoveProfile(userInfo)).isTrue();
    }

    @Test
    public void userRestricted_cannotRemoveProfile() {
        UserInfo userInfo = new UserInfo(/* id= */ 10, TEST_PROFILE_NAME, FLAG_INITIALIZED);
        mRemoveProfileHandler.setUserInfo(userInfo);
        when(mMockProfileHelper.isCurrentProcessUser(userInfo)).thenReturn(false);
        when(mMockUserManager.hasUserRestriction(UserManager.DISALLOW_REMOVE_USER))
                .thenReturn(true);

        assertThat(mRemoveProfileHandler.canRemoveProfile(userInfo)).isFalse();
    }

    @Test
    public void viewingSystemUser_cannotRemoveProfile() {
        UserInfo userInfo = new UserInfo(/* id= */ 0, TEST_PROFILE_NAME, FLAG_INITIALIZED);
        mRemoveProfileHandler.setUserInfo(userInfo);
        when(mMockProfileHelper.isCurrentProcessUser(userInfo)).thenReturn(false);
        assertThat(mRemoveProfileHandler.canRemoveProfile(userInfo)).isFalse();
    }

    @Test
    public void isDemoUser_cannotRemoveProfile() {
        UserInfo userInfo = new UserInfo(/* id= */ 10, TEST_PROFILE_NAME, FLAG_INITIALIZED);
        mRemoveProfileHandler.setUserInfo(userInfo);
        when(mMockUserManager.isDemoUser()).thenReturn(true);
        assertThat(mRemoveProfileHandler.canRemoveProfile(userInfo)).isFalse();
    }

    @Test
    public void hasPreviousDeleteDialog_dialogListenerSet() {
        UserInfo userInfo = new UserInfo(/* id= */ 10, TEST_PROFILE_NAME, FLAG_INITIALIZED);
        mRemoveProfileHandler.setUserInfo(userInfo);
        ConfirmationDialogFragment dialog = new ConfirmationDialogFragment.Builder(
                mContext).build();
        when(mMockFragmentController.findDialogByTag(REMOVE_PROFILE_DIALOG_TAG)).thenReturn(dialog);

        mRemoveProfileHandler.resetListeners();

        assertThat(dialog.getConfirmListener()).isNotNull();
    }

    @Test
    public void showConfirmRemoveProfileDialog_showsConfirmationDialog() {
        UserInfo userInfo = new UserInfo(/* id= */ 10, TEST_PROFILE_NAME, FLAG_INITIALIZED);
        mRemoveProfileHandler.setUserInfo(userInfo);
        when(mMockProfileHelper.isCurrentProcessUser(userInfo)).thenReturn(false);

        mRemoveProfileHandler.showConfirmRemoveProfileDialog();

        verify(mMockFragmentController).showDialog(any(ConfirmationDialogFragment.class),
                eq(REMOVE_PROFILE_DIALOG_TAG));
    }

    @Test
    public void onDeleteConfirmed_removeProfile() {
        UserInfo userInfo = new UserInfo(/* id= */ 10, TEST_PROFILE_NAME, FLAG_INITIALIZED);
        mRemoveProfileHandler.setUserInfo(userInfo);
        when(mMockProfileHelper.isCurrentProcessUser(userInfo)).thenReturn(false);

        Bundle arguments = new Bundle();
        arguments.putString(KEY_PROFILE_TYPE, ANY_PROFILE);
        mRemoveProfileHandler.mRemoveConfirmListener.onConfirm(arguments);

        verify(mMockProfileHelper).removeProfile(mContext, userInfo);
    }

    @Test
    @UiThreadTest
    public void onDeleteConfirmed_lastAdmin_launchChooseNewAdminFragment() {
        UserInfo userInfo = new UserInfo(/* id= */ 10, TEST_PROFILE_NAME, FLAG_INITIALIZED);
        mRemoveProfileHandler.setUserInfo(userInfo);
        when(mMockProfileHelper.isCurrentProcessUser(userInfo)).thenReturn(false);

        Bundle arguments = new Bundle();
        arguments.putString(KEY_PROFILE_TYPE, LAST_ADMIN);
        mRemoveProfileHandler.mRemoveConfirmListener.onConfirm(arguments);

        verify(mMockFragmentController).launchFragment(any(ChooseNewAdminFragment.class));
    }
}
