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
import android.text.TextUtils;
import android.net.ConnectivityManager;
import android.net.wifi.WifiManager;
import android.net.wifi.WifiConfiguration;
import android.widget.RelativeLayout;
import android.widget.EditText;

import com.android.tv.settings.R;
import com.android.tv.settings.connectivity.util.State;
import com.android.tv.settings.connectivity.util.StateMachine;
import com.android.tv.settings.connectivity.NetworkFragment;
import com.android.tv.settings.connectivity.util.WifiHotspotParameter;
import com.android.tv.settings.connectivity.util.SettingsInputAddressEditText;

import java.util.List;

/**
 * State responsible for modify hotspot password.
 */
public class HotspotPasswordState implements State {
    private final FragmentActivity mActivity;
    private Fragment mFragment;
    private static WifiManager mWifiManager;
    private String mPassword;

    public HotspotPasswordState(FragmentActivity activity) {
        mActivity = activity;
    }

    @Override
    public void processForward() {
        mFragment = new HotspotPasswordFragment();
        FragmentChangeListener listener = (FragmentChangeListener) mActivity;
        if (listener != null) {
            listener.onFragmentChange(mFragment, true);
        }
    }

    @Override
    public void processBackward() {
        mFragment = new HotspotPasswordFragment();
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
     * Fragment that needs user to enter name.
     */
    public static class HotspotPasswordFragment extends WifiConnectivityGuidedStepFragment {
        private UserChoiceInfo mUserChoiceInfo;
        private StateMachine mStateMachine;
        private GuidedAction mAction;
        private RelativeLayout mRlRoot;
        private SettingsInputAddressEditText mEtTitle;

        @Override
        public GuidanceStylist.Guidance onCreateGuidance(Bundle savedInstanceState) {
            return new GuidanceStylist.Guidance(
                    getString(R.string.hotspot_password_title),
                    null,
                    null,
                    null);
        }

        @Override
        public void onCreate(Bundle savedInstanceState) {
            mWifiManager = (WifiManager)getActivity().getSystemService(Context.WIFI_SERVICE);
            mUserChoiceInfo = ViewModelProviders
                    .of(getActivity())
                    .get(UserChoiceInfo.class);
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
            final WifiConfiguration config = mWifiManager.getWifiApConfiguration();
            if (config == null || (config.getAuthType() == WifiConfiguration.KeyMgmt.WPA2_PSK
                    && TextUtils.isEmpty(config.preSharedKey))) {
                title = "12345678";
            } else {
                if (WifiHotspotParameter.hotspotpassword != "") {
                    title = WifiHotspotParameter.hotspotpassword;
                } else {
                    title = config.preSharedKey;
                }
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
                mUserChoiceInfo.put(UserChoiceInfo.HOTSPOTPASSWORD,
                        action.getTitle().toString());
                int setPasswordResult = WifiHotspotParameter.setHotspotPassword(getActivity());
                if (setPasswordResult == 0) {
                    mStateMachine.getListener().onComplete(StateMachine.CONTINUE);
                } else {
                    mStateMachine.getListener().onComplete(StateMachine.SOFTAP_PASSWORD_INVALID);
                }
            }
            return action.getId();
        }
    }
}
