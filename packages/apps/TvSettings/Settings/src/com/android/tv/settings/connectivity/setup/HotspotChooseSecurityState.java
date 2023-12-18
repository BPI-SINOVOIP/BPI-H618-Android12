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

import androidx.lifecycle.ViewModelProviders;
import android.content.Context;
import android.os.Bundle;
import androidx.annotation.VisibleForTesting;
import androidx.leanback.widget.GuidanceStylist;
import androidx.leanback.widget.GuidedAction;
import androidx.fragment.app.Fragment;
import androidx.fragment.app.FragmentActivity;

import com.android.tv.settings.R;
import com.android.tv.settings.connectivity.util.State;
import com.android.tv.settings.connectivity.util.StateMachine;
import android.net.wifi.WifiConfiguration;
import com.android.tv.settings.connectivity.util.WifiHotspotParameter;

import java.util.List;

/**
 * State responsible for choosing hotspot security type.
 */
public class HotspotChooseSecurityState implements State {
    private final FragmentActivity mActivity;
    private Fragment mFragment;

    public HotspotChooseSecurityState(FragmentActivity activity) {
        mActivity = activity;
    }

    @Override
    public void processForward() {
        mFragment = new HotspotChooseSecurityFragment();
        FragmentChangeListener listener = (FragmentChangeListener) mActivity;
        if (listener != null) {
            listener.onFragmentChange(mFragment, true);
        }
    }

    @Override
    public void processBackward() {
        mFragment = new HotspotChooseSecurityFragment();
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
     * Fragment that needs user to select security type.
     */
    public static class HotspotChooseSecurityFragment extends WifiConnectivityGuidedStepFragment {
        @VisibleForTesting
        static final long HOTSPOT_SECURITY_TYPE_WPA2_PSK = 100021;
        @VisibleForTesting
        static final long HOTSPOT_SECURITY_TYPE_NONE = 100022;
        private StateMachine mStateMachine;
        private UserChoiceInfo mUserChoiceInfo;

        @Override
        public GuidanceStylist.Guidance onCreateGuidance(Bundle savedInstanceState) {
            return new GuidanceStylist.Guidance(
                    getString(R.string.security_type),
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
                    .title(R.string.hotspot_security_type_wpa2_psk)
                    .id(HOTSPOT_SECURITY_TYPE_WPA2_PSK)
                    .build());
            actions.add(new GuidedAction.Builder(context)
                    .title(R.string.hotspot_security_type_none)
                    .id(HOTSPOT_SECURITY_TYPE_NONE)
                    .build());
        }

        @Override
        public void onGuidedActionClicked(GuidedAction action) {
            if (action.getId() == HOTSPOT_SECURITY_TYPE_WPA2_PSK) {
                WifiHotspotParameter.setSecurityType(WifiConfiguration.KeyMgmt.WPA2_PSK);
            } else if (action.getId() == HOTSPOT_SECURITY_TYPE_NONE) {
                WifiHotspotParameter.setSecurityType(WifiConfiguration.KeyMgmt.NONE);
            }
                mStateMachine.getListener().onComplete(StateMachine.CONTINUE);
        }
    }
}
