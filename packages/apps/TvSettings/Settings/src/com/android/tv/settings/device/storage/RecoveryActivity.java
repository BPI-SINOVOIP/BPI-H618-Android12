/*
 * Copyright (C) 2015 The Android Open Source Project
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
 * limitations under the License
 */

package com.android.tv.settings.device.storage;

import android.annotation.Nullable;
import android.app.Activity;
import android.content.Context;
import android.app.ActivityManager;
import android.content.Intent;
import android.os.Bundle;
import android.os.PowerManager;
import android.util.Log;

import androidx.annotation.NonNull;
import androidx.leanback.app.GuidedStepFragment;
import androidx.leanback.widget.GuidanceStylist;
import androidx.leanback.widget.GuidedAction;

import com.android.tv.settings.R;

import java.util.List;

public class RecoveryActivity extends Activity {

    private static final String TAG = "RecoveryActivity";

    @Override
    protected void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        if (savedInstanceState == null) {
            GuidedStepFragment.addAsRoot(this, RecoveryFragment.newInstance(), android.R.id.content);
        }
    }

    public static class RecoveryFragment extends GuidedStepFragment {

        public static RecoveryFragment newInstance() {

            Bundle args = new Bundle();

            RecoveryFragment fragment = new RecoveryFragment();
            fragment.setArguments(args);
            return fragment;
        }

        @NonNull
        @Override
        public GuidanceStylist.Guidance onCreateGuidance(Bundle savedInstanceState) {
            return new GuidanceStylist.Guidance(
                    getString(R.string.device_recovery),
                    getString(R.string.recovery_mode_description),
                    null,
                    getContext().getDrawable(R.drawable.ic_settings_backup_restore_132dp));
        }

        @Override
        public void onCreateActions(@NonNull List<GuidedAction> actions,
                Bundle savedInstanceState) {
            actions.add(new GuidedAction.Builder(getContext())
                    .clickAction(GuidedAction.ACTION_ID_CANCEL)
                    .build());
            actions.add(new GuidedAction.Builder(getContext())
                    .clickAction(GuidedAction.ACTION_ID_OK)
                    .title(getString(R.string.device_recovery))
                    .build());
        }

        @Override
        public void onGuidedActionClicked(GuidedAction action) {
            if (action.getId() == GuidedAction.ACTION_ID_OK) {
                add(getFragmentManager(), RecoveryConfirmFragment.newInstance());
            } else if (action.getId() == GuidedAction.ACTION_ID_CANCEL) {
                getActivity().finish();
            } else {
                Log.wtf(TAG, "Unknown action clicked");
            }
        }
    }

    public static class RecoveryConfirmFragment extends GuidedStepFragment {

        public static RecoveryConfirmFragment newInstance() {

            Bundle args = new Bundle();

            RecoveryConfirmFragment fragment = new RecoveryConfirmFragment();
            fragment.setArguments(args);
            return fragment;
        }

        @NonNull
        @Override
        public GuidanceStylist.Guidance onCreateGuidance(Bundle savedInstanceState) {
            return new GuidanceStylist.Guidance(
                    getString(R.string.device_recovery),
                    getString(R.string.confirm_recovery_mode_description),
                    null,
                    getContext().getDrawable(R.drawable.ic_settings_backup_restore_132dp));
        }

        @Override
        public void onCreateActions(@NonNull List<GuidedAction> actions,
                Bundle savedInstanceState) {
            actions.add(new GuidedAction.Builder(getContext())
                    .clickAction(GuidedAction.ACTION_ID_CANCEL)
                    .build());
            actions.add(new GuidedAction.Builder(getContext())
                    .clickAction(GuidedAction.ACTION_ID_OK)
                    .title(getString(R.string.confirm_recovery_mode_device))
                    .build());
        }

        @Override
        public void onGuidedActionClicked(GuidedAction action) {
            if (action.getId() == GuidedAction.ACTION_ID_OK) {
                if (ActivityManager.isUserAMonkey()) {
                    Log.v(TAG, "Monkey tried to erase the device. Bad monkey, bad!");
                    getActivity().finish();
                } else {
                    PowerManager pm = (PowerManager) getContext().getSystemService(Context.POWER_SERVICE);
                    pm.reboot(PowerManager.REBOOT_RECOVERY);
                }
            } else if (action.getId() == GuidedAction.ACTION_ID_CANCEL) {
                getActivity().finish();
            } else {
                Log.wtf(TAG, "Unknown action clicked");
            }
        }
    }
}
