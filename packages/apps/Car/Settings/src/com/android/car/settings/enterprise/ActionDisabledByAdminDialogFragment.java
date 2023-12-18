/*
 * Copyright (C) 2021 The Android Open Source Project
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

package com.android.car.settings.enterprise;

import android.annotation.Nullable;
import android.annotation.UserIdInt;
import android.app.Dialog;
import android.app.admin.DevicePolicyManager;
import android.content.ComponentName;
import android.content.Context;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.graphics.drawable.Drawable;
import android.os.Bundle;
import android.os.Process;
import android.os.UserHandle;
import android.util.IconDrawableFactory;

import com.android.car.settings.R;
import com.android.car.settings.common.Logger;
import com.android.car.ui.AlertDialogBuilder;
import com.android.car.ui.preference.CarUiDialogFragment;
import com.android.settingslib.RestrictedLockUtils;
import com.android.settingslib.RestrictedLockUtils.EnforcedAdmin;
import com.android.settingslib.RestrictedLockUtilsInternal;
import com.android.settingslib.enterprise.ActionDisabledByAdminController;
import com.android.settingslib.enterprise.ActionDisabledByAdminControllerFactory;

/**
 * Shows a dialog explaining that an action is not enabled due to restrictions imposed by an active
 * device administrator.
 */
// TODO(b/186905050): add unit tests
// TODO(b/188836559): move most of this class' logic to settingslib
public final class ActionDisabledByAdminDialogFragment extends CarUiDialogFragment {

    public static final String DISABLED_BY_ADMIN_CONFIRM_DIALOG_TAG =
            "com.android.car.settings.applications.DisabledByAdminConfirmDialog";

    private static final String TAG = ActionDisabledByAdminDialogFragment.class.getSimpleName();
    private static final Logger LOG = new Logger(TAG);

    private static final String EXTRA_RESTRICTION = TAG + "_restriction";
    private static final String EXTRA_USER_ID = TAG + "_userId";

    private String mRestriction;

    @UserIdInt
    private int mUserId;

    private ActionDisabledByAdminController mActionDisabledByAdminController;

    /**
     * Gets the dialog for the given user and restriction.
     */
    public static ActionDisabledByAdminDialogFragment newInstance(String restriction,
            @UserIdInt int userId) {
        ActionDisabledByAdminDialogFragment instance = new ActionDisabledByAdminDialogFragment();
        instance.mRestriction = restriction;
        instance.mUserId = userId;
        return instance;
    }

    @Override
    public Dialog onCreateDialog(Bundle savedInstanceState) {
        if (savedInstanceState != null) {
            mRestriction = savedInstanceState.getString(EXTRA_RESTRICTION);
            mUserId = savedInstanceState.getInt(EXTRA_USER_ID);
        }
        return initialize(getContext()).create();
    }

    @Override
    public void onSaveInstanceState(Bundle outState) {
        super.onSaveInstanceState(outState);

        outState.putString(EXTRA_RESTRICTION, mRestriction);
        outState.putInt(EXTRA_USER_ID, mUserId);
    }

    @Override
    protected void onDialogClosed(boolean positiveResult) {
    }

    private AlertDialogBuilder initialize(Context context) {
        EnforcedAdmin enforcedAdmin = RestrictedLockUtilsInternal
                .checkIfRestrictionEnforced(context, mRestriction, mUserId);

        AlertDialogBuilder builder = new AlertDialogBuilder(context)
                .setPositiveButton(R.string.okay, /* listener= */ null);
        mActionDisabledByAdminController = ActionDisabledByAdminControllerFactory
                .createInstance(context, mRestriction, new DeviceAdminStringProviderImpl(context),
                        context.getUser());
        mActionDisabledByAdminController.initialize(
                new ActionDisabledLearnMoreButtonLauncherImpl(builder));
        if (enforcedAdmin != null) {
            mActionDisabledByAdminController.updateEnforcedAdmin(enforcedAdmin, mUserId);
            mActionDisabledByAdminController.setupLearnMoreButton(context);
        }
        initializeDialogViews(context, builder, enforcedAdmin,
                getEnforcementAdminUserId(enforcedAdmin));
        return builder;
    }

    // NOTE: methods below were copied from phone Settings
    // (com.android.settings.enterprise.ActionDisabledByAdminDialogHelper), but adjusted to
    // use a AlertDialogBuilder directly, instead of an Activity hosting a dialog.

    private static @UserIdInt int getEnforcementAdminUserId(@Nullable EnforcedAdmin admin) {
        return admin == null || admin.user == null ? UserHandle.USER_NULL
                : admin.user.getIdentifier();
    }

    private void initializeDialogViews(Context context, AlertDialogBuilder builder,
            @Nullable EnforcedAdmin enforcedAdmin, @UserIdInt int userId) {
        ComponentName admin = null;

        if (enforcedAdmin != null) {
            admin = enforcedAdmin.component;
            if (admin == null) {
                return;
            }

            mActionDisabledByAdminController.updateEnforcedAdmin(enforcedAdmin, userId);
        }

        if (isNotCurrentUserOrProfile(context, admin, userId)) {
            admin = null;
        }
        setAdminSupportIcon(context, builder, admin, userId);
        setAdminSupportTitle(context, builder, mRestriction);

        if (enforcedAdmin != null) {
            setAdminSupportDetails(context, builder, enforcedAdmin);
        }
    }

    private boolean isNotCurrentUserOrProfile(Context context, ComponentName admin,
            @UserIdInt int userId) {
        return !RestrictedLockUtilsInternal.isAdminInCurrentUserOrProfile(context, admin)
                || !RestrictedLockUtils.isCurrentUserOrProfile(context, userId);
    }

    private void setAdminSupportIcon(Context context, AlertDialogBuilder builder,
            ComponentName admin, @UserIdInt int userId) {
        if (isNotCurrentUserOrProfile(context, admin, userId)) {
            builder.setIcon(context.getDrawable(com.android.internal.R.drawable.ic_info));
        } else {
            Drawable badgedIcon = getBadgedIcon(
                    IconDrawableFactory.newInstance(context),
                    context.getPackageManager(),
                    admin.getPackageName(),
                    userId);
            builder.setIcon(badgedIcon);
        }
    }

    private void setAdminSupportTitle(Context context, AlertDialogBuilder builder,
            String restriction) {
        builder.setTitle(mActionDisabledByAdminController.getAdminSupportTitle(restriction));
    }

    private void setAdminSupportDetails(Context context, AlertDialogBuilder builder,
            @Nullable EnforcedAdmin enforcedAdmin) {
        if (enforcedAdmin == null || enforcedAdmin.component == null) {
            LOG.i("setAdminSupportDetails(): no admin on " + enforcedAdmin);
            return;
        }
        CharSequence supportMessage = null;
        if (!RestrictedLockUtilsInternal.isAdminInCurrentUserOrProfile(context,
                enforcedAdmin.component) || !RestrictedLockUtils.isCurrentUserOrProfile(
                        context, getEnforcementAdminUserId(enforcedAdmin))) {
            enforcedAdmin.component = null;
        } else {
            if (enforcedAdmin.user == null) {
                enforcedAdmin.user = UserHandle.of(UserHandle.myUserId());
            }
            if (UserHandle.isSameApp(Process.myUid(), Process.SYSTEM_UID)) {
                supportMessage = context.getSystemService(DevicePolicyManager.class)
                        .getShortSupportMessageForUser(enforcedAdmin.component,
                                getEnforcementAdminUserId(enforcedAdmin));
            }
        }
        CharSequence supportContentString =
                mActionDisabledByAdminController.getAdminSupportContentString(
                        context, supportMessage);
        if (supportContentString != null) {
            builder.setMessage(supportContentString);
        }
    }

    // Copied from com.android.settings.Utils
    private static Drawable getBadgedIcon(IconDrawableFactory iconDrawableFactory,
            PackageManager packageManager, String packageName, int userId) {
        try {
            ApplicationInfo appInfo = packageManager.getApplicationInfoAsUser(
                    packageName, PackageManager.GET_META_DATA, userId);
            return iconDrawableFactory.getBadgedIcon(appInfo, userId);
        } catch (PackageManager.NameNotFoundException e) {
            return packageManager.getDefaultActivityIcon();
        }
    }
}
