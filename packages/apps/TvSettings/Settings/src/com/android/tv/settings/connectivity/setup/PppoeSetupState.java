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
import androidx.annotation.NonNull;
import androidx.leanback.widget.GuidanceStylist;
import androidx.leanback.widget.GuidedAction;
import androidx.fragment.app.Fragment;
import androidx.fragment.app.FragmentActivity;
import android.text.TextUtils;
import android.view.View;

import com.android.tv.settings.R;
import com.android.tv.settings.connectivity.util.AdvancedOptionsFlowUtil;
import com.android.tv.settings.connectivity.util.State;
import com.android.tv.settings.connectivity.util.StateMachine;

import java.io.File;
import java.util.List;

/**
 * State responsible for choosing pppoe interface settings.
 */
public class PppoeSetupState implements State {
    private final FragmentActivity mActivity;
    private Fragment mFragment;

    public PppoeSetupState(FragmentActivity activity) {
        mActivity = activity;
    }

    @Override
    public void processForward() {
        mFragment = new PppoeSetupFragment();
        FragmentChangeListener listener = (FragmentChangeListener) mActivity;
        if (listener != null) {
            listener.onFragmentChange(mFragment, true);
        }
    }

    @Override
    public void processBackward() {
        mFragment = new PppoeSetupFragment();
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
     * Fragment that makes user determine Pppoe interface.
     */
    public static class PppoeSetupFragment extends WifiConnectivityGuidedStepFragment {
        private static final int PPPOE_ACTION_ETH0  = 100007;
        private static final int PPPOE_ACTION_WLAN0 = 100008;
        private static final int PPPOE_ACTION_ETH1  = 100009;
        private StateMachine mStateMachine;
        private AdvancedOptionsFlowInfo mAdvancedOptionsFlowInfo;

        @NonNull
        @Override
        public GuidanceStylist.Guidance onCreateGuidance(Bundle savedInstanceState) {
            return new GuidanceStylist.Guidance(
                    getString(R.string.pppoe_settings),
                    null,
                    null,
                    null);
        }

        @Override
        public void onCreate(Bundle savedInstanceState) {
            mAdvancedOptionsFlowInfo = ViewModelProviders
                    .of(getActivity())
                    .get(AdvancedOptionsFlowInfo.class);
            mStateMachine = ViewModelProviders
                    .of(getActivity())
                    .get(StateMachine.class);
            super.onCreate(savedInstanceState);
        }

        private Boolean isIfaceExist(String ifname) {
            String netDevPath = "/sys/class/net/";
            File file = new File(netDevPath);
            File[] tempList = file.listFiles();
            for (int i = 0; i < tempList.length; i++) {
                if (tempList[i].toString().endsWith(ifname)) {
                    return true;
                }
            }
            return false;
        }

        @Override
        public void onCreateActions(@NonNull List<GuidedAction> actions,
                Bundle savedInstanceState) {
            Context context = getActivity();
            if (isIfaceExist("eth0")) {
                actions.add(new GuidedAction.Builder(context)
                        .title(getString(R.string.pppoe_eth0))
                        .id(PPPOE_ACTION_ETH0)
                        .build());
            }

            if (isIfaceExist("eth1")) {
                actions.add(new GuidedAction.Builder(context)
                        .title(getString(R.string.pppoe_eth1))
                        .id(PPPOE_ACTION_ETH1)
                        .build());
            }

            if (isIfaceExist("wlan0")) {
                actions.add(new GuidedAction.Builder(context)
                        .title(getString(R.string.pppoe_wlan0))
                        .id(PPPOE_ACTION_WLAN0)
                        .build());
            }

        }

        @Override
        public void onGuidedActionClicked(GuidedAction action) {
            mAdvancedOptionsFlowInfo.put(AdvancedOptionsFlowInfo.IP_SETTINGS, action.getTitle());
            if (action.getId() == PPPOE_ACTION_WLAN0) {
                mAdvancedOptionsFlowInfo.put(AdvancedOptionsFlowInfo.PPPOE_INTERFACE, "wlan0");
                mStateMachine.getListener().onComplete(StateMachine.CONTINUE);
            } else if (action.getId() == PPPOE_ACTION_ETH0) {
                mAdvancedOptionsFlowInfo.put(AdvancedOptionsFlowInfo.PPPOE_INTERFACE, "eth0");
                mStateMachine.getListener().onComplete(StateMachine.CONTINUE);
            } else if (action.getId() == PPPOE_ACTION_ETH1) {
                mAdvancedOptionsFlowInfo.put(AdvancedOptionsFlowInfo.PPPOE_INTERFACE, "eth1");
                mStateMachine.getListener().onComplete(StateMachine.CONTINUE);
            }
        }
    }
}
