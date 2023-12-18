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
import androidx.leanback.widget.GuidanceStylist;
import androidx.leanback.widget.GuidedAction;
import androidx.fragment.app.Fragment;
import androidx.fragment.app.FragmentActivity;

import com.android.tv.settings.R;
import com.android.tv.settings.connectivity.util.State;
import com.android.tv.settings.connectivity.util.StateMachine;

import java.util.List;

/**
 * State responsible for showing the page when pppoe setup is invalid.
 */
public class PppoeSetupInvalidState implements State {
    private final FragmentActivity mActivity;
    private Fragment mFragment;

    public PppoeSetupInvalidState(FragmentActivity activity) {
        mActivity = activity;
    }

    @Override
    public void processForward() {
        mFragment = new PppoeSetupInvalidFragment();
        FragmentChangeListener listener = (FragmentChangeListener) mActivity;
        if (listener != null) {
            listener.onFragmentChange(mFragment, true);
        }
    }

    @Override
    public void processBackward() {
        mFragment = new PppoeSetupInvalidFragment();
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
     * Fragment that notifies the user pppoe setup is invalid.
     */
    public static class PppoeSetupInvalidFragment extends WifiConnectivityGuidedStepFragment {
        private StateMachine mStateMachine;
        private AdvancedOptionsFlowInfo mAdvancedOptionsFlowInfo;

        @Override
        public GuidanceStylist.Guidance onCreateGuidance(Bundle savedInstanceState) {
            return new GuidanceStylist.Guidance(
                    getString(R.string.pppoe_setup_invalid),
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

        @Override
        public void onCreateActions(List<GuidedAction> actions, Bundle savedInstanceState) {
            Context context = getActivity();
            actions.add(new GuidedAction.Builder(context)
                    .title(getString(R.string.pppoe_action_try_again))
                    .id(GuidedAction.ACTION_ID_CONTINUE)
                    .build());
        }

        @Override
        public void onGuidedActionClicked(GuidedAction action) {
            if (action.getId() == GuidedAction.ACTION_ID_CONTINUE) {
                mAdvancedOptionsFlowInfo.remove(AdvancedOptionsFlowInfo.PPPOE_INTERFACE);
                mAdvancedOptionsFlowInfo.remove(AdvancedOptionsFlowInfo.PPPOE_USERNAME);
                mAdvancedOptionsFlowInfo.remove(AdvancedOptionsFlowInfo.PPPOE_PASSWORD);
                mStateMachine.getListener().onComplete(StateMachine.CONTINUE);
            }
        }
    }
}
