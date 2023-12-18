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

package com.android.car.carlauncher.homescreen.assistive;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNull;
import static org.mockito.Mockito.any;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.reset;
import static org.mockito.Mockito.verify;

import android.car.projection.ProjectionStatus;
import android.testing.TestableContext;

import androidx.test.InstrumentationRegistry;

import com.android.car.carlauncher.homescreen.HomeCardInterface;
import com.android.car.carlauncher.homescreen.ui.DescriptiveTextView;

import org.junit.After;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;

import java.util.Collections;

@RunWith(JUnit4.class)
public class ProjectionModelTest {

    @Rule
    public TestableContext mContext = new TestableContext(InstrumentationRegistry.getContext());

    private static final String PROJECTING_DEVICE_NAME = "projecting device name";
    private static final String NONPROJECTING_DEVICE_NAME = "non-projecting device name";
    private static final ProjectionStatus.MobileDevice PROJECTING_DEVICE =
            ProjectionStatus.MobileDevice.builder(0, PROJECTING_DEVICE_NAME).setProjecting(
                    true).build();
    private static final ProjectionStatus.MobileDevice NONPROJECTING_DEVICE =
            ProjectionStatus.MobileDevice.builder(0, NONPROJECTING_DEVICE_NAME).setProjecting(
                    false).build();

    private final ProjectionStatus mInactiveProjectionStatus = ProjectionStatus.builder(
            mContext.getPackageName(), ProjectionStatus.PROJECTION_STATE_INACTIVE).build();
    private final ProjectionStatus mProjectingDeviceProjectionStatus = ProjectionStatus.builder(
            mContext.getPackageName(),
            ProjectionStatus.PROJECTION_STATE_READY_TO_PROJECT).addMobileDevice(
            PROJECTING_DEVICE).build();
    private final ProjectionStatus mNonProjectingDeviceProjectionStatus = ProjectionStatus.builder(
            mContext.getPackageName(), ProjectionStatus.PROJECTION_STATE_READY_TO_PROJECT)
            .addMobileDevice(NONPROJECTING_DEVICE).build();

    private ProjectionModel mModel;

    @Mock
    private HomeCardInterface.Presenter mPresenter;

    @Before
    public void setUp() {
        MockitoAnnotations.initMocks(this);
        mModel = new ProjectionModel();
        mModel.setPresenter(mPresenter);
        mModel.onCreate(mContext);
        reset(mPresenter);
    }

    @After
    public void tearDown() {
        mModel.onDestroy(mContext);
    }

    @Test
    public void noChange_doesNotCallPresenter() {
        verify(mPresenter, never()).onModelUpdated(any());
        assertNull(mModel.getCardHeader());
        assertNull(mModel.getCardContent());
    }

    @Test
    public void changeProjectionStatusToProjectingDevice_callsPresenter() {
        sendProjectionStatus(mProjectingDeviceProjectionStatus);

        verify(mPresenter).onModelUpdated(mModel);
        DescriptiveTextView content = (DescriptiveTextView) mModel.getCardContent();
        assertEquals(content.getSubtitle(), PROJECTING_DEVICE_NAME);
    }

    @Test
    public void changeProjectionStatusToNonProjectingDevice_callsPresenter() {
        sendProjectionStatus(mNonProjectingDeviceProjectionStatus);

        verify(mPresenter).onModelUpdated(mModel);
        DescriptiveTextView content = (DescriptiveTextView) mModel.getCardContent();
        assertEquals(content.getSubtitle(), NONPROJECTING_DEVICE_NAME);
    }

    @Test
    public void changeProjectionStatusToInactive_callsPresenter() {
        sendProjectionStatus(mProjectingDeviceProjectionStatus);
        reset(mPresenter);

        sendProjectionStatus(mInactiveProjectionStatus);

        verify(mPresenter).onModelUpdated(mModel);
        assertNull(mModel.getCardHeader());
        assertNull(mModel.getCardContent());
    }

    private void sendProjectionStatus(ProjectionStatus status) {
        reset(mPresenter);
        mModel.onProjectionStatusChanged(
                status.getState(),
                status.getPackageName(),
                Collections.singletonList(status));
    }
}
