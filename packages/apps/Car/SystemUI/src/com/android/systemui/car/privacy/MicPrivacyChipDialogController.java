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

package com.android.systemui.car.privacy;

import static android.os.UserHandle.USER_SYSTEM;

import android.Manifest;
import android.app.AlertDialog;
import android.content.Context;
import android.content.Intent;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.content.pm.UserInfo;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.Drawable;
import android.os.UserHandle;
import android.os.UserManager;
import android.permission.PermGroupUsage;
import android.permission.PermissionManager;
import android.text.TextUtils;
import android.util.Log;
import android.view.LayoutInflater;

import androidx.annotation.AnyThread;
import androidx.annotation.MainThread;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.WorkerThread;
import androidx.core.text.BidiFormatter;

import com.android.car.ui.AlertDialogBuilder;
import com.android.car.ui.CarUiLayoutInflaterFactory;
import com.android.car.ui.recyclerview.CarUiContentListItem;
import com.android.car.ui.recyclerview.CarUiListItem;
import com.android.car.ui.recyclerview.CarUiListItemAdapter;
import com.android.launcher3.icons.BitmapInfo;
import com.android.launcher3.icons.IconFactory;
import com.android.systemui.R;
import com.android.systemui.dagger.SysUISingleton;
import com.android.systemui.dagger.qualifiers.Background;
import com.android.systemui.dagger.qualifiers.Main;
import com.android.systemui.privacy.PrivacyDialog;
import com.android.systemui.privacy.PrivacyItemController;
import com.android.systemui.privacy.PrivacyType;
import com.android.systemui.privacy.logging.PrivacyLogger;
import com.android.systemui.settings.UserTracker;
import com.android.systemui.statusbar.phone.SystemUIDialog;

import java.util.ArrayList;
import java.util.Comparator;
import java.util.List;
import java.util.Map;
import java.util.Optional;
import java.util.concurrent.Executor;
import java.util.stream.Collectors;

import javax.inject.Inject;

/**
 * Dialog to show ongoing and recent microphone usage.
 */
@SysUISingleton
public class MicPrivacyChipDialogController {
    private static final String TAG = "MicPrivacyChipDialog";
    private static final String EMPTY_APP_NAME = "";

    private static final Map<String, PrivacyType> PERM_GROUP_TO_PRIVACY_TYPE_MAP =
            Map.of(Manifest.permission_group.CAMERA, PrivacyType.TYPE_CAMERA,
                    Manifest.permission_group.MICROPHONE, PrivacyType.TYPE_MICROPHONE,
                    Manifest.permission_group.LOCATION, PrivacyType.TYPE_LOCATION);


    private final Context mContext;
    private final PermissionManager mPermissionManager;
    private final UserTracker mUserTracker;
    private final Executor mBackgroundExecutor;
    private final Executor mUiExecutor;
    private final PrivacyLogger mPrivacyLogger;
    private final PackageManager mPackageManager;
    private final PrivacyItemController mPrivacyItemController;
    private final UserManager mUserManager;
    private final String mDialogTitle;
    private final String mPhoneCallTitle;

    private AlertDialog mDialog;

    @Inject
    public MicPrivacyChipDialogController(
            Context context,
            @Background Executor backgroundExecutor,
            @Main Executor uiExecutor,
            PermissionManager permissionManager,
            PackageManager packageManager,
            PrivacyItemController privacyItemController,
            UserTracker userTracker,
            PrivacyLogger privacyLogger) {
        mContext = context;
        mBackgroundExecutor = backgroundExecutor;
        mUiExecutor = uiExecutor;
        mPermissionManager = permissionManager;
        mPackageManager = packageManager;
        mPrivacyItemController = privacyItemController;
        mUserTracker = userTracker;
        mPrivacyLogger = privacyLogger;

        mUserManager = context.getSystemService(UserManager.class);
        mDialogTitle = context.getString(R.string.mic_privacy_chip_dialog_title_mic);
        mPhoneCallTitle = context.getString(R.string.ongoing_privacy_dialog_phonecall);
    }

    /**
     * Creates and shows {@link AlertDialog}.
     */
    @AnyThread
    public void show() {
        // TODO(b/185482811): Move this to where {@link Context} is created in Dagger.
        // We need to inject Chassis' LayoutInflater into our context
        // to use {@link AlertDialogBuilder}.
        LayoutInflater layoutInflater = LayoutInflater.from(mContext);
        if (layoutInflater.getFactory2() == null) {
            layoutInflater.setFactory2(new CarUiLayoutInflaterFactory());
        }

        dismissDialog();

        mBackgroundExecutor.execute(() -> {
            List<PrivacyDialog.PrivacyElement> items = createPrivacyElements();

            mUiExecutor.execute(() -> {
                List<PrivacyDialog.PrivacyElement> elements = filterAndSort(items);
                mDialog = createDialog(elements);
                addFlagsAndListenersForSystemUi();
                mPrivacyLogger.logShowDialogContents(elements);
                showDialog();
            });
        });
    }

    private List<PrivacyDialog.PrivacyElement> createPrivacyElements() {
        List<UserInfo> userInfos = mUserTracker.getUserProfiles();
        List<PermGroupUsage> permGroupUsages = getPermGroupUsages();
        mPrivacyLogger.logUnfilteredPermGroupUsage(permGroupUsages);
        List<PrivacyDialog.PrivacyElement> items = new ArrayList<>();

        permGroupUsages.forEach(usage -> {
            PrivacyType type =
                    verifyType(PERM_GROUP_TO_PRIVACY_TYPE_MAP.get(usage.getPermGroupName()));
            if (type == null) return;

            int userId = UserHandle.getUserId(usage.getUid());
            Optional<UserInfo> optionalUserInfo = userInfos.stream()
                    .filter(ui -> ui.id == userId)
                    .findFirst();
            if (!optionalUserInfo.isPresent() && userId != USER_SYSTEM) return;

            UserInfo userInfo =
                    optionalUserInfo.orElseGet(() -> mUserManager.getUserInfo(USER_SYSTEM));

            String appName = usage.isPhoneCall()
                    ? EMPTY_APP_NAME
                    : getLabelForPackage(usage.getPackageName(), usage.getUid());

            items.add(
                    new PrivacyDialog.PrivacyElement(
                            type,
                            usage.getPackageName(),
                            userId,
                            appName,
                            usage.getAttribution(),
                            usage.getLastAccess(),
                            usage.isActive(),
                            userInfo.isManagedProfile(),
                            usage.isPhoneCall())
            );
        });

        return items;
    }

    private AlertDialog createDialog(List<PrivacyDialog.PrivacyElement> elements) {
        return new AlertDialogBuilder(mContext)
                .setAdapter(createCarListAdapter(elements))
                .setTitle(mDialogTitle)
                .setPositiveButton(R.string.mic_privacy_chip_dialog_ok,
                        (dialog, which) -> {
                            mPrivacyLogger.logPrivacyDialogDismissed();
                            mDialog = null;
                        })
                .setOnDismissListener(dialog -> {
                    mPrivacyLogger.logPrivacyDialogDismissed();
                    mDialog = null;
                })
                .create();
    }

    @NonNull
    private CarUiListItemAdapter createCarListAdapter(
            @NonNull List<PrivacyDialog.PrivacyElement> elements) {
        List<CarUiListItem> carUiListItems = new ArrayList<>();

        elements.forEach(element -> {
            Optional<ApplicationInfo> applicationInfo = getApplicationInfo(element);
            if (!applicationInfo.isPresent()) return;

            carUiListItems.add(createCarUiContentListItem(mContext, applicationInfo.get(),
                    element.getPackageName(), element.getUserId(), element.getPhoneCall()));
        });

        return new CarUiListItemAdapter(carUiListItems);
    }

    private CarUiContentListItem createCarUiContentListItem(Context context,
            ApplicationInfo applicationInfo, String packageName, int userId, boolean isPhoneCall) {
        CarUiContentListItem item = new CarUiContentListItem(CarUiContentListItem.Action.NONE);

        item.setTitle(isPhoneCall
                ? mPhoneCallTitle
                : getAppLabel(applicationInfo, context));
        item.setIcon(getBadgedIcon(context, applicationInfo));
        if (!isPhoneCall) {
            item.setOnItemClickedListener(it -> startActivity(context, packageName, userId));
        }

        return item;
    }

    private Optional<ApplicationInfo> getApplicationInfo(PrivacyDialog.PrivacyElement element) {
        return getApplicationInfo(element.getPackageName(), element.getUserId());
    }

    private Optional<ApplicationInfo> getApplicationInfo(String packageName, int userId) {
        ApplicationInfo applicationInfo;
        try {
            applicationInfo = mPackageManager
                    .getApplicationInfoAsUser(packageName, /* flags= */ 0, userId);
            return Optional.of(applicationInfo);
        } catch (PackageManager.NameNotFoundException e) {
            Log.w(TAG, "Application info not found for: " + packageName);
            return Optional.empty();
        }
    }

    private String getAppLabel(@NonNull ApplicationInfo applicationInfo, @NonNull Context context) {
        return BidiFormatter.getInstance()
                .unicodeWrap(applicationInfo.loadSafeLabel(context.getPackageManager(),
                        /* ellipsizeDip= */ 0,
                        /* flags= */ TextUtils.SAFE_STRING_FLAG_TRIM
                                | TextUtils.SAFE_STRING_FLAG_FIRST_LINE)
                        .toString());
    }

    @NonNull
    private Drawable getBadgedIcon(@NonNull Context context,
            @NonNull ApplicationInfo appInfo) {
        UserHandle user = UserHandle.getUserHandleForUid(appInfo.uid);
        try (IconFactory iconFactory = IconFactory.obtain(context)) {
            BitmapInfo bitmapInfo =
                    iconFactory.createBadgedIconBitmap(
                            appInfo.loadUnbadgedIcon(context.getPackageManager()), user,
                            /* shrinkNonAdaptiveIcons= */ false);
            return new BitmapDrawable(context.getResources(), bitmapInfo.icon);
        }
    }

    @MainThread
    private void startActivity(Context context, String packageName, int userId) {
        UserHandle userHandle = UserHandle.of(userId);
        Intent intent = new Intent(Intent.ACTION_MANAGE_APP_PERMISSIONS);
        intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_MULTIPLE_TASK);
        intent.putExtra(Intent.EXTRA_PACKAGE_NAME, packageName);
        intent.putExtra(Intent.EXTRA_USER, userHandle);
        intent.addCategory(Intent.CATEGORY_DEFAULT);
        mPrivacyLogger.logStartSettingsActivityFromDialog(packageName, userId);
        context.startActivityAsUser(intent, userHandle);
        dismissDialog();
    }

    private void addFlagsAndListenersForSystemUi() {
        SystemUIDialog.setShowForAllUsers(mDialog, /* show= */ true);
        SystemUIDialog.applyFlags(mDialog);
        SystemUIDialog.setWindowOnTop(mDialog);
        SystemUIDialog.registerDismissListener(mDialog);
    }

    @WorkerThread
    private List<PermGroupUsage> getPermGroupUsages() {
        return mPermissionManager.getIndicatorAppOpUsageData();
    }

    private void dismissDialog() {
        if (mDialog == null) return;

        mDialog.dismiss();
    }

    private void showDialog() {
        if (mDialog == null) return;

        mDialog.show();
    }

    @WorkerThread
    private String getLabelForPackage(String packageName, int userId) {
        Optional<ApplicationInfo> applicationInfo = getApplicationInfo(packageName, userId);

        if (!applicationInfo.isPresent()) return packageName;

        return (String) applicationInfo.get().loadLabel(mPackageManager);
    }

    /**
     * If {@link PrivacyType} is available then returns the argument, or else returns {@code null}.
     */
    @Nullable
    private PrivacyType verifyType(PrivacyType type) {
        if ((type == PrivacyType.TYPE_CAMERA || type == PrivacyType.TYPE_MICROPHONE) &&
                mPrivacyItemController.getMicCameraAvailable()) {
            return type;
        } else if (type == PrivacyType.TYPE_LOCATION &&
                mPrivacyItemController.getLocationAvailable()) {
            return type;
        } else {
            return null;
        }
    }

    private List<PrivacyDialog.PrivacyElement> filterAndSort(
            List<PrivacyDialog.PrivacyElement> list) {
        return list.stream()
                .filter(it -> it.getType() == PrivacyType.TYPE_MICROPHONE)
                .sorted(new PrivacyElementComparator())
                .collect(Collectors.toList());
    }

    private static class PrivacyElementComparator
            implements Comparator<PrivacyDialog.PrivacyElement> {
        @Override
        public int compare(PrivacyDialog.PrivacyElement it1, PrivacyDialog.PrivacyElement it2) {
            if (it1.getActive() && !it2.getActive()) {
                return 1;
            } else if (!it1.getActive() && it2.getActive()) {
                return -1;
            } else {
                return Long.compare(it1.getLastActiveTimestamp(), it2.getLastActiveTimestamp());
            }
        }
    }
}
