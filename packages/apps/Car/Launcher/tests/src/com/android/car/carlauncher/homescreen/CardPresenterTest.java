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

package com.android.car.carlauncher.homescreen;

import static org.mockito.Mockito.any;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import com.android.car.carlauncher.homescreen.ui.CardHeader;
import com.android.car.carlauncher.homescreen.ui.DescriptiveTextView;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;

import java.util.List;

@RunWith(JUnit4.class)
public class CardPresenterTest {

    private static final CardHeader CARD_HEADER = new CardHeader("appName", /* cardIcon = */ null);
    private static final DescriptiveTextView CARD_CONTENT = new DescriptiveTextView(/* image = */
            null, "title", "subtitle");

    private CardPresenter mPresenter;

    @Mock
    private HomeCardInterface.View mView;
    @Mock
    private HomeCardInterface.Model mModel;

    @Before
    public void setUp() {
        MockitoAnnotations.initMocks(this);
        mPresenter = new CardPresenter() {
            @Override
            public void setModels(List<HomeCardInterface.Model> models) {
            }

            @Override
            public void onViewCreated() {
            }

            @Override
            public void onViewDestroyed() {
            }
        };
        mPresenter.setView(mView);
    }

    @Test
    public void onModelUpdated_nullModel_hidesFragment() {
        mPresenter.onModelUpdated(null);

        verify(mView).hideCard();
    }

    @Test
    public void onModelUpdated_validModel_updatesFragment() {
        when(mModel.getCardHeader()).thenReturn(CARD_HEADER);
        when(mModel.getCardContent()).thenReturn(CARD_CONTENT);

        mPresenter.onModelUpdated(mModel);

        verify(mView).updateHeaderView(CARD_HEADER);
        verify(mView).updateContentView(CARD_CONTENT);
    }

    @Test
    public void onModelUpdated_validHeaderNullContent_showsHeaderOnly() {
        when(mModel.getCardHeader()).thenReturn(CARD_HEADER);
        when(mModel.getCardContent()).thenReturn(null);

        mPresenter.onModelUpdated(mModel);

        verify(mView).updateHeaderView(CARD_HEADER);
        verify(mView, never()).updateContentView(any());
    }

    @Test
    public void onModelUpdated_nullHeaderValidContent_hidesFragment() {
        when(mModel.getCardHeader()).thenReturn(null);
        when(mModel.getCardContent()).thenReturn(CARD_CONTENT);

        mPresenter.onModelUpdated(mModel);

        verify(mView).hideCard();
        verify(mView, never()).updateContentView(CARD_CONTENT);
    }
}
