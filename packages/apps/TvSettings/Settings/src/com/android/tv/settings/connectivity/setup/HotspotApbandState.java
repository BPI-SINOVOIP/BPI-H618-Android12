/*
 * Copyright (C) 2017 The Android Open Source Project
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

package com.android.tv.settings.connectivity.setup;

//import static android.net.wifi.WifiConfiguration.AP_BAND_2GHZ;
//import static android.net.wifi.WifiConfiguration.AP_BAND_5GHZ;

import androidx.lifecycle.ViewModelProviders;
import android.content.Context;
import android.net.wifi.SoftApConfiguration;
import android.os.Bundle;
import androidx.annotation.VisibleForTesting;
import androidx.leanback.widget.GuidanceStylist;
import androidx.leanback.widget.GuidedAction;
import androidx.fragment.app.Fragment;
import androidx.fragment.app.FragmentActivity;

import com.android.tv.settings.R;
import com.android.tv.settings.connectivity.util.State;
import com.android.tv.settings.connectivity.util.StateMachine;
import com.android.tv.settings.connectivity.util.WifiHotspotParameter;

import java.util.List;

/**
 * State responsible for choosing ap band.
 */
public class HotspotApbandState implements State {
    private final FragmentActivity mActivity;
    private Fragment mFragment;

    public HotspotApbandState(FragmentActivity activity) {
        mActivity = activity;
    }

    @Override
    public void processForward() {
        mFragment = new HotspotApbandFragment();
        FragmentChangeListener listener = (FragmentChangeListener) mActivity;
        if (listener != null) {
            listener.onFragmentChange(mFragment, true);
        }
    }

    @Override
    public void processBackward() {
        mFragment = new HotspotApbandFragment();
        FragmentChangeListener listener = (FragmentChangeListener) mActivity;
        if (listener != null) {
            listener.onFragmentChange(mFragment, false);
        }
    }

    @Override
    public Fragment getFragment() {
        return mFragment;
    }

    /**
     * Fragment that needs user to select ap band.
     */
    public static class HotspotApbandFragment extends WifiConnectivityGuidedStepFragment {
        static final long HOTSPOT_AP_Band_2GHZ = 100023;
        static final long HOTSPOT_AP_Band_5GHZ = 100024;
        private StateMachine mStateMachine;
        private UserChoiceInfo mUserChoiceInfo;

        @Override
        public GuidanceStylist.Guidance onCreateGuidance(Bundle savedInstanceState) {
            return new GuidanceStylist.Guidance(
                    getString(R.string.hotspot_apband_title),
                    null,
                    null,
                    null);
        }

        @Override
        public void onCreate(Bundle savedInstanceState) {
            mUserChoiceInfo = ViewModelProviders
                    .of(getActivity())
                    .get(UserChoiceInfo.class);
            mStateMachine = ViewModelProviders
                    .of(getActivity())
                    .get(StateMachine.class);
            super.onCreate(savedInstanceState);
        }

        @Override
        public void onCreateActions(List<GuidedAction> actions, Bundle savedInstanceState) {
            Context context = getActivity();
            actions.add(new GuidedAction.Builder(context)
                    .title(R.string.hotspot_apband_2G)
                    .id(HOTSPOT_AP_Band_2GHZ)
                    .build());
            actions.add(new GuidedAction.Builder(context)
                    .title(R.string.hotspot_apband_5G)
                    .id(HOTSPOT_AP_Band_5GHZ)
                    .build());
        }

        @Override
        public void onGuidedActionClicked(GuidedAction action) {
            if (action.getId() == HOTSPOT_AP_Band_2GHZ) {
                WifiHotspotParameter.setApbnadType(SoftApConfiguration.BAND_2GHZ);
            } else if (action.getId() == HOTSPOT_AP_Band_5GHZ) {
                WifiHotspotParameter.setApbnadType(SoftApConfiguration.BAND_5GHZ);

            }
                mStateMachine.getListener().onComplete(StateMachine.CONTINUE);
        }
    }
}
