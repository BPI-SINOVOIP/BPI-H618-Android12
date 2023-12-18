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

package com.android.settings.system;

import androidx.preference.Preference;

import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.os.PowerManager;
import android.os.UserManager;
import android.text.TextUtils;

import com.android.settings.R;
import com.android.settings.Utils;
import com.android.settings.core.PreferenceControllerMixin;
import com.android.settingslib.core.AbstractPreferenceController;

public class RebootRecoveryPreferenceController extends AbstractPreferenceController
        implements PreferenceControllerMixin, DialogInterface.OnClickListener, DialogInterface.OnDismissListener {
    private final UserManager mUm;
    private Context mContext;
    private AlertDialog mRebootDialog;

    public RebootRecoveryPreferenceController(Context context) {
        super(context);
        mUm = (UserManager) context.getSystemService(Context.USER_SERVICE);
        mContext = context;
    }

    @Override
    public boolean handlePreferenceTreeClick(Preference preference) {
        if (!TextUtils.equals(preference.getKey(), getPreferenceKey())) {
            return false;
        }

        if (mRebootDialog == null) {
            mRebootDialog = new AlertDialog.Builder(mContext)
                .setTitle(R.string.reboot_recovery_title)
                .setMessage(R.string.reboot_recovery_desc)
                .setPositiveButton(R.string.reboot_recovery_button, this)
                .setNegativeButton(android.R.string.cancel, null)
                .setOnDismissListener(this)
                .show();
        }
        return true;
    }

    @Override
    public boolean isAvailable() {
        return mUm.isAdminUser() || Utils.isDemoUser(mContext);
    }

    @Override
    public String getPreferenceKey() {
        return "reboot_recovery";
    }

    @Override
    public void onDismiss(DialogInterface dialog) {
        if (mRebootDialog == dialog) {
            mRebootDialog = null;
        }
    }

    @Override
    public void onClick(DialogInterface dialog, int which) {
        if (mRebootDialog != dialog) {
            return;
        }

        PowerManager pm = (PowerManager) mContext.getSystemService(Context.POWER_SERVICE);
        pm.reboot(PowerManager.REBOOT_RECOVERY);
    }
}
