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
import androidx.leanback.widget.GuidedActionsStylist;
import androidx.fragment.app.Fragment;
import androidx.fragment.app.FragmentActivity;
import android.view.View;
import android.net.PppoeManager;
import android.widget.RelativeLayout;
import android.widget.EditText;

import com.android.tv.settings.R;
import com.android.tv.settings.connectivity.util.AdvancedOptionsFlowUtil;
import com.android.tv.settings.connectivity.util.State;
import com.android.tv.settings.connectivity.util.StateMachine;
import com.android.tv.settings.connectivity.util.AdvancedOptionsFlowUtil;
import com.android.tv.settings.connectivity.NetworkFragment;
import com.android.tv.settings.connectivity.util.SettingsInputAddressEditText;

import java.util.List;

/**
 * State responsible for setting pppoe password.
 */
public class PppoePasswordState implements State {
    private final FragmentActivity mActivity;
    private Fragment mFragment;
    private static PppoeManager mPppoeManager;
    private static String ifname;
    private static String password;

    public PppoePasswordState(FragmentActivity activity) {
        mActivity = activity;
    }

    @Override
    public void processForward() {
        mFragment = new PppoePasswordFragment();
        FragmentChangeListener listener = (FragmentChangeListener) mActivity;
        if (listener != null) {
            listener.onFragmentChange(mFragment, true);
        }
    }

    @Override
    public void processBackward() {
        mFragment = new PppoePasswordFragment();
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
     * Fragment that needs user to enter password.
     */
    public static class PppoePasswordFragment extends WifiConnectivityGuidedStepFragment {
        private StateMachine mStateMachine;
        private AdvancedOptionsFlowInfo mAdvancedOptionsFlowInfo;
        private GuidedAction mAction;
        private RelativeLayout mRlRoot;
        private SettingsInputAddressEditText mEtTitle;

        @Override
        public GuidanceStylist.Guidance onCreateGuidance(Bundle savedInstanceState) {
            return new GuidanceStylist.Guidance(
                    getString(R.string.pppoe_setting_password),
                    getString(R.string.pppoe_password_description),
                    null,
                    null);
        }

        @Override
        public void onCreate(Bundle savedInstanceState) {
            mPppoeManager = (PppoeManager)getActivity().getSystemService(Context.PPPOE_SERVICE);
            mAdvancedOptionsFlowInfo = ViewModelProviders
                    .of(getActivity())
                    .get(AdvancedOptionsFlowInfo.class);
            mStateMachine = ViewModelProviders
                    .of(getActivity())
                    .get(StateMachine.class);
            super.onCreate(savedInstanceState);
        }


        @Override
        public GuidedActionsStylist onCreateActionsStylist() {
            return new GuidedActionsStylist() {
                @Override
                public int onProvideItemLayoutId() {
                    return R.layout.setup_text_input_item;
                }

                @Override
                public void onBindViewHolder(ViewHolder vh, GuidedAction action) {
                    super.onBindViewHolder(vh, action);
                    mRlRoot = (RelativeLayout) vh.itemView.findViewById(
                        R.id.rl_root_setup_text_input);
                    mEtTitle = (SettingsInputAddressEditText) vh.itemView.findViewById(
                        R.id.guidedactions_item_title);
                    mEtTitle.setLeaderView(mRlRoot);
                    mEtTitle.moveCursorToLast();
                }
            };
        }

        @Override
        public void onCreateActions(List<GuidedAction> actions, Bundle savedInstanceState) {
            String title = "";
            ifname = mPppoeManager.getPppoeInterfaceName();
            List<String> login = mPppoeManager.getPppoeUserInfo(ifname);
            if (login != null && (!login.isEmpty())) {
                password = login.get(1);
            } else {
                password = null;
            }
            if (AdvancedOptionsFlowUtil.pppoepassword != "") {
                title = AdvancedOptionsFlowUtil.pppoepassword;
            } else {
                if (password != null && (!password.isEmpty()))
                    title = password;
            }
            Context context = getActivity();
            mAction = new GuidedAction.Builder(context)
                    .title(title)
                    .editable(true)
                    .id(GuidedAction.ACTION_ID_CONTINUE)
                    .build();
            actions.add(mAction);
        }

        @Override
        public void onViewCreated(View view, Bundle savedInstanceState) {
            openInEditMode(mAction);
        }

        @Override
        public long onGuidedActionEditedAndProceed(GuidedAction action) {
            if (action.getId() == GuidedAction.ACTION_ID_CONTINUE) {
                mAdvancedOptionsFlowInfo.put(AdvancedOptionsFlowInfo.PPPOE_PASSWORD,
                        action.getTitle());
                int pppoeSetupResult = AdvancedOptionsFlowUtil.processPppoeSetup(
                        getActivity());
                if (pppoeSetupResult == 0) {
                    mStateMachine.getListener().onComplete(StateMachine.ADVANCED_FLOW_COMPLETE);
                } else {
                    mStateMachine.getListener().onComplete(StateMachine.PPPOE_SETUP_INVALID);
                }
            }
            return action.getId();
        }
    }
}
