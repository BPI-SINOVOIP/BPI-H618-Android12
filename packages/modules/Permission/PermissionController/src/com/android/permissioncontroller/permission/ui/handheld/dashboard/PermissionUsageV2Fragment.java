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

package com.android.permissioncontroller.permission.ui.handheld.dashboard;

import static com.android.permissioncontroller.Constants.EXTRA_SESSION_ID;
import static com.android.permissioncontroller.Constants.INVALID_SESSION_ID;
import static com.android.permissioncontroller.PermissionControllerStatsLog.PERMISSION_USAGE_FRAGMENT_INTERACTION;
import static com.android.permissioncontroller.PermissionControllerStatsLog.PERMISSION_USAGE_FRAGMENT_INTERACTION__ACTION__SEE_OTHER_PERMISSIONS_CLICKED;
import static com.android.permissioncontroller.PermissionControllerStatsLog.PERMISSION_USAGE_FRAGMENT_INTERACTION__ACTION__SHOW_SYSTEM_CLICKED;
import static com.android.permissioncontroller.PermissionControllerStatsLog.write;

import static java.util.concurrent.TimeUnit.DAYS;

import android.Manifest;
import android.app.ActionBar;
import android.app.Activity;
import android.app.role.RoleManager;
import android.content.Context;
import android.os.Build;
import android.os.Bundle;
import android.util.ArrayMap;
import android.util.ArraySet;
import android.util.Log;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.RequiresApi;
import androidx.preference.Preference;
import androidx.preference.PreferenceCategory;
import androidx.preference.PreferenceGroupAdapter;
import androidx.preference.PreferenceScreen;
import androidx.recyclerview.widget.RecyclerView;

import com.android.permissioncontroller.R;
import com.android.permissioncontroller.permission.model.AppPermissionGroup;
import com.android.permissioncontroller.permission.model.AppPermissionUsage;
import com.android.permissioncontroller.permission.model.AppPermissionUsage.GroupUsage;
import com.android.permissioncontroller.permission.model.legacy.PermissionApps;
import com.android.permissioncontroller.permission.ui.handheld.PermissionUsageV2ControlPreference;
import com.android.permissioncontroller.permission.ui.handheld.SettingsWithLargeHeader;
import com.android.permissioncontroller.permission.utils.KotlinUtils;
import com.android.permissioncontroller.permission.utils.Utils;
import com.android.settingslib.HelpUtils;

import java.time.Instant;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Set;

/**
 * The main page for the privacy dashboard.
 */
@RequiresApi(Build.VERSION_CODES.S)
public class PermissionUsageV2Fragment extends SettingsWithLargeHeader implements
        PermissionUsages.PermissionsUsagesChangeCallback {
    private static final String LOG_TAG = "PermUsageV2Fragment";

    private static final int MENU_REFRESH = MENU_HIDE_SYSTEM + 1;

    /** TODO(ewol): Use the config setting to determine amount of time to show. */
    private static final long TIME_FILTER_MILLIS = DAYS.toMillis(1);

    private static final Map<String, Integer> PERMISSION_GROUP_ORDER = Map.of(
            Manifest.permission_group.LOCATION, 0,
            Manifest.permission_group.CAMERA, 1,
            Manifest.permission_group.MICROPHONE, 2
    );
    private static final int DEFAULT_ORDER = 3;

    // Pie chart in this screen will be the first child.
    // Hence we use PERMISSION_GROUP_ORDER + 1 here.
    private static final int PERMISSION_USAGE_INITIAL_EXPANDED_CHILDREN_COUNT =
            PERMISSION_GROUP_ORDER.size() + 1;
    private static final int EXPAND_BUTTON_ORDER = 999;

    private static final String KEY_SESSION_ID = "_session_id";
    private static final String SESSION_ID_KEY = PermissionUsageV2Fragment.class.getName()
            + KEY_SESSION_ID;

    private @NonNull PermissionUsages mPermissionUsages;
    private @Nullable List<AppPermissionUsage> mAppPermissionUsages = new ArrayList<>();

    private boolean mShowSystem;
    private boolean mHasSystemApps;
    private MenuItem mShowSystemMenu;
    private MenuItem mHideSystemMenu;
    private boolean mOtherExpanded;

    private ArrayMap<String, Integer> mGroupAppCounts = new ArrayMap<>();

    private boolean mFinishedInitialLoad;

    private @NonNull RoleManager mRoleManager;

    private PermissionUsageGraphicPreference mGraphic;

    /** Unique Id of a request */
    private long mSessionId;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        if (savedInstanceState != null) {
            mSessionId = savedInstanceState.getLong(SESSION_ID_KEY);
        } else {
            mSessionId = getArguments().getLong(EXTRA_SESSION_ID, INVALID_SESSION_ID);
        }

        mFinishedInitialLoad = false;

        // By default, do not show system app usages.
        mShowSystem = false;

        // Start out with 'other' permissions not expanded.
        mOtherExpanded = false;

        setLoading(true, false);
        setHasOptionsMenu(true);
        ActionBar ab = getActivity().getActionBar();
        if (ab != null) {
            ab.setDisplayHomeAsUpEnabled(true);
        }

        Context context = getPreferenceManager().getContext();
        mPermissionUsages = new PermissionUsages(context);
        mRoleManager = Utils.getSystemServiceSafe(context, RoleManager.class);

        reloadData();
    }

    @Override
    public RecyclerView.Adapter onCreateAdapter(PreferenceScreen preferenceScreen) {
        PreferenceGroupAdapter adapter =
                (PreferenceGroupAdapter) super.onCreateAdapter(preferenceScreen);

        adapter.registerAdapterDataObserver(new RecyclerView.AdapterDataObserver() {
            @Override
            public void onChanged() {
                updatePreferenceScreenAdvancedTitleAndSummary(preferenceScreen, adapter);
            }

            @Override
            public void onItemRangeInserted(int positionStart, int itemCount) {
                onChanged();
            }

            @Override
            public void onItemRangeRemoved(int positionStart, int itemCount) {
                onChanged();
            }

            @Override
            public void onItemRangeChanged(int positionStart, int itemCount) {
                onChanged();
            }

            @Override
            public void onItemRangeMoved(int fromPosition, int toPosition, int itemCount) {
                onChanged();
            }
        });

        updatePreferenceScreenAdvancedTitleAndSummary(preferenceScreen, adapter);
        return adapter;
    }

    private void updatePreferenceScreenAdvancedTitleAndSummary(PreferenceScreen preferenceScreen,
            PreferenceGroupAdapter adapter) {
        int count = adapter.getItemCount();
        if (count == 0) {
            return;
        }

        Preference preference = adapter.getItem(count - 1);

        // This is a hacky way of getting the expand button preference for advanced info
        if (preference.getOrder() == EXPAND_BUTTON_ORDER) {
            mOtherExpanded = false;
            preference.setTitle(R.string.perm_usage_adv_info_title);
            preference.setSummary(preferenceScreen.getSummary());
            preference.setLayoutResource(R.layout.expand_button_with_large_title);
            if (mGraphic != null) {
                mGraphic.setShowOtherCategory(false);
            }
        } else {
            mOtherExpanded = true;
            if (mGraphic != null) {
                mGraphic.setShowOtherCategory(true);
            }
        }
    }

    @Override
    public void onStart() {
        super.onStart();
        getActivity().setTitle(R.string.permission_usage_title);

    }

    @Override
    public void onCreateOptionsMenu(Menu menu, MenuInflater inflater) {
        super.onCreateOptionsMenu(menu, inflater);
        if (mHasSystemApps) {
            mShowSystemMenu = menu.add(Menu.NONE, MENU_SHOW_SYSTEM, Menu.NONE,
                    R.string.menu_show_system);
            mHideSystemMenu = menu.add(Menu.NONE, MENU_HIDE_SYSTEM, Menu.NONE,
                    R.string.menu_hide_system);
        }

        HelpUtils.prepareHelpMenuItem(getActivity(), menu, R.string.help_permission_usage,
                getClass().getName());
        MenuItem refresh = menu.add(Menu.NONE, MENU_REFRESH, Menu.NONE,
                R.string.permission_usage_refresh);
        refresh.setIcon(R.drawable.ic_refresh);
        refresh.setShowAsAction(MenuItem.SHOW_AS_ACTION_IF_ROOM);
        updateMenu();
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        switch (item.getItemId()) {
            case android.R.id.home:
                getActivity().finishAfterTransition();
                return true;
            case MENU_SHOW_SYSTEM:
                write(PERMISSION_USAGE_FRAGMENT_INTERACTION, mSessionId,
                        PERMISSION_USAGE_FRAGMENT_INTERACTION__ACTION__SHOW_SYSTEM_CLICKED);
                // fall through
            case MENU_HIDE_SYSTEM:
                mShowSystem = item.getItemId() == MENU_SHOW_SYSTEM;
                // We already loaded all data, so don't reload
                updateUI();
                updateMenu();
                break;
            case MENU_REFRESH:
                reloadData();
                break;
        }
        return super.onOptionsItemSelected(item);
    }

    private void updateMenu() {
        if (mHasSystemApps) {
            mShowSystemMenu.setVisible(!mShowSystem);
            mHideSystemMenu.setVisible(mShowSystem);
        }
    }

    @Override
    public void onPermissionUsagesChanged() {
        if (mPermissionUsages.getUsages().isEmpty()) {
            return;
        }
        mAppPermissionUsages = new ArrayList<>(mPermissionUsages.getUsages());
        updateUI();
    }

    @Override
    public int getEmptyViewString() {
        return R.string.no_permission_usages;
    }

    @Override
    public void onSaveInstanceState(Bundle outState) {
        super.onSaveInstanceState(outState);
        if (outState != null) {
            outState.putLong(SESSION_ID_KEY, mSessionId);
        }
    }

    private void updateUI() {
        if (mAppPermissionUsages.isEmpty() || getActivity() == null) {
            return;
        }
        Context context = getActivity();

        PreferenceScreen screen = getPreferenceScreen();
        if (screen == null) {
            screen = getPreferenceManager().createPreferenceScreen(context);
            setPreferenceScreen(screen);
        }
        screen.removeAll();

        if (mOtherExpanded) {
            screen.setInitialExpandedChildrenCount(Integer.MAX_VALUE);
        } else {
            screen.setInitialExpandedChildrenCount(
                    PERMISSION_USAGE_INITIAL_EXPANDED_CHILDREN_COUNT);
        }
        screen.setOnExpandButtonClickListener(() -> {
            write(PERMISSION_USAGE_FRAGMENT_INTERACTION, mSessionId,
                    PERMISSION_USAGE_FRAGMENT_INTERACTION__ACTION__SEE_OTHER_PERMISSIONS_CLICKED);
        });

        long curTime = System.currentTimeMillis();
        long startTime = Math.max(curTime - TIME_FILTER_MILLIS,
                Instant.EPOCH.toEpochMilli());

        mGroupAppCounts.clear();
        // Permission group to count mapping.
        Map<String, Integer> usages = new HashMap<>();
        List<AppPermissionGroup> permissionGroups = getOSPermissionGroups();
        for (int i = 0; i < permissionGroups.size(); i++) {
            usages.put(permissionGroups.get(i).getName(), 0);
        }
        ArrayList<PermissionApps.PermissionApp> permApps = new ArrayList<>();

        Set<String> exemptedPackages = Utils.getExemptedPackages(mRoleManager);

        boolean seenSystemApp = extractPermissionUsage(exemptedPackages,
                usages, permApps, startTime);

        if (mHasSystemApps != seenSystemApp) {
            mHasSystemApps = seenSystemApp;
            getActivity().invalidateOptionsMenu();
        }

        mGraphic = new PermissionUsageGraphicPreference(context);
        screen.addPreference(mGraphic);
        mGraphic.setUsages(usages);

        // Add the preference header.
        PreferenceCategory category = new PreferenceCategory(context);
        screen.addPreference(category);

        Map<String, CharSequence> groupUsageNameToLabel = new HashMap<>();
        List<Map.Entry<String, Integer>> groupUsagesList = new ArrayList<>(usages.entrySet());
        int usagesEntryCount = groupUsagesList.size();
        for (int usageEntryIndex = 0; usageEntryIndex < usagesEntryCount; usageEntryIndex++) {
            Map.Entry<String, Integer> usageEntry = groupUsagesList.get(usageEntryIndex);
            groupUsageNameToLabel.put(usageEntry.getKey(),
                    KotlinUtils.INSTANCE.getPermGroupLabel(context, usageEntry.getKey()));
        }

        groupUsagesList.sort((e1, e2) -> comparePermissionGroupUsage(
                e1, e2, groupUsageNameToLabel));

        CharSequence advancedInfoSummary = getAdvancedInfoSummaryString(context, groupUsagesList);
        screen.setSummary(advancedInfoSummary);

        addUIContent(context, groupUsagesList, permApps, category);
    }

    private CharSequence getAdvancedInfoSummaryString(Context context,
            List<Map.Entry<String, Integer>> groupUsagesList) {
        int size = groupUsagesList.size();
        if (size <= PERMISSION_USAGE_INITIAL_EXPANDED_CHILDREN_COUNT - 1) {
            return "";
        }

        // case for 1 extra item in the advanced info
        if (size == PERMISSION_USAGE_INITIAL_EXPANDED_CHILDREN_COUNT) {
            String permGroupName = groupUsagesList
                    .get(PERMISSION_USAGE_INITIAL_EXPANDED_CHILDREN_COUNT - 1).getKey();
            return KotlinUtils.INSTANCE.getPermGroupLabel(context, permGroupName);
        }

        String permGroupName1 = groupUsagesList
                .get(PERMISSION_USAGE_INITIAL_EXPANDED_CHILDREN_COUNT - 1).getKey();
        String permGroupName2 = groupUsagesList
                .get(PERMISSION_USAGE_INITIAL_EXPANDED_CHILDREN_COUNT).getKey();
        CharSequence permGroupLabel1 = KotlinUtils
                .INSTANCE.getPermGroupLabel(context, permGroupName1);
        CharSequence permGroupLabel2 = KotlinUtils
                .INSTANCE.getPermGroupLabel(context, permGroupName2);

        // case for 2 extra items in the advanced info
        if (size == PERMISSION_USAGE_INITIAL_EXPANDED_CHILDREN_COUNT + 1) {
            return context.getResources().getString(R.string.perm_usage_adv_info_summary_2_items,
                    permGroupLabel1, permGroupLabel2);
        }

        // case for 3 or more extra items in the advanced info
        int numExtraItems = size - PERMISSION_USAGE_INITIAL_EXPANDED_CHILDREN_COUNT - 1;
        return context.getResources().getString(R.string.perm_usage_adv_info_summary_more_items,
                permGroupLabel1, permGroupLabel2, numExtraItems);
    }

    /**
     * Extract the permission usages from mAppPermissionUsages and put the extracted usages
     * into usages and permApps. Returns whether we have seen a system app during the process.
     *
     * TODO: theianchen
     * It's doing two things at the same method which is violating the SOLID principle.
     * We should fix this.
     *
     * @param exemptedPackages packages that are the role holders for exempted roles
     * @param usages an empty List that will be filled with permission usages.
     * @param permApps an empty List that will be filled with permission apps.
     * @return whether we have seen a system app.
     */
    private boolean extractPermissionUsage(Set<String> exemptedPackages,
            Map<String, Integer> usages,
            ArrayList<PermissionApps.PermissionApp> permApps,
            long startTime) {
        boolean seenSystemApp = false;
        int numApps = mAppPermissionUsages.size();
        for (int appNum = 0; appNum < numApps; appNum++) {
            AppPermissionUsage appUsage = mAppPermissionUsages.get(appNum);
            if (exemptedPackages.contains(appUsage.getPackageName())) {
                continue;
            }

            boolean used = false;
            List<GroupUsage> appGroups = appUsage.getGroupUsages();
            int numGroups = appGroups.size();
            for (int groupNum = 0; groupNum < numGroups; groupNum++) {
                GroupUsage groupUsage = appGroups.get(groupNum);
                String groupName = groupUsage.getGroup().getName();
                long lastAccessTime = groupUsage.getLastAccessTime();
                if (lastAccessTime == 0) {
                    Log.w(LOG_TAG,
                            "Unexpected access time of 0 for " + appUsage.getApp().getKey() + " "
                                    + groupUsage.getGroup().getName());
                    continue;
                }
                if (lastAccessTime < startTime) {
                    continue;
                }

                final boolean isSystemApp = !Utils.isGroupOrBgGroupUserSensitive(
                        groupUsage.getGroup());
                seenSystemApp = seenSystemApp || isSystemApp;

                // If not showing system apps, skip.
                if (!mShowSystem && isSystemApp) {
                    continue;
                }

                used = true;
                addGroupUser(groupName);

                usages.put(groupName, usages.getOrDefault(groupName, 0) + 1);
            }
            if (used) {
                permApps.add(appUsage.getApp());
                addGroupUser(null);
            }
        }

        return seenSystemApp;
    }

    /**
     * Use the usages and permApps that are previously constructed to add UI content to the page
     */
    private void addUIContent(Context context,
            List<Map.Entry<String, Integer>> usages,
            ArrayList<PermissionApps.PermissionApp> permApps,
            PreferenceCategory category) {
        new PermissionApps.AppDataLoader(context, () -> {
            for (int i = 0; i < usages.size(); i++) {
                Map.Entry<String, Integer> currentEntry = usages.get(i);
                PermissionUsageV2ControlPreference permissionUsagePreference =
                        new PermissionUsageV2ControlPreference(context, currentEntry.getKey(),
                                currentEntry.getValue(), mShowSystem, mSessionId);
                category.addPreference(permissionUsagePreference);
            }

            setLoading(false, true);
            mFinishedInitialLoad = true;
            setProgressBarVisible(false);

            Activity activity = getActivity();
            if (activity != null) {
                mPermissionUsages.stopLoader(activity.getLoaderManager());
            }
        }).execute(permApps.toArray(new PermissionApps.PermissionApp[0]));
    }

    private void addGroupUser(String app) {
        Integer count = mGroupAppCounts.get(app);
        if (count == null) {
            mGroupAppCounts.put(app, 1);
        } else {
            mGroupAppCounts.put(app, count + 1);
        }
    }

    /**
     * Reloads the data to show.
     */
    private void reloadData() {
        final long filterTimeBeginMillis = Math.max(System.currentTimeMillis()
                - TIME_FILTER_MILLIS, Instant.EPOCH.toEpochMilli());
        mPermissionUsages.load(null /*filterPackageName*/, null /*filterPermissionGroups*/,
                filterTimeBeginMillis, Long.MAX_VALUE, PermissionUsages.USAGE_FLAG_LAST
                        | PermissionUsages.USAGE_FLAG_HISTORICAL, getActivity().getLoaderManager(),
                false /*getUiInfo*/, false /*getNonPlatformPermissions*/, this /*callback*/,
                false /*sync*/);
        if (mFinishedInitialLoad) {
            setProgressBarVisible(true);
        }
    }

    private static int comparePermissionGroupUsage(@NonNull Map.Entry<String, Integer> first,
            @NonNull Map.Entry<String, Integer> second,
            Map<String, CharSequence> groupUsageNameToLabelMapping) {
        int firstPermissionOrder = PERMISSION_GROUP_ORDER
                .getOrDefault(first.getKey(), DEFAULT_ORDER);
        int secondPermissionOrder = PERMISSION_GROUP_ORDER
                .getOrDefault(second.getKey(), DEFAULT_ORDER);
        if (firstPermissionOrder != secondPermissionOrder) {
            return firstPermissionOrder - secondPermissionOrder;
        }

        return groupUsageNameToLabelMapping.get(first.getKey()).toString()
                .compareTo(groupUsageNameToLabelMapping.get(second.getKey()).toString());
    }

    /**
     * Get the permission groups declared by the OS.
     *
     * @return a list of the permission groups declared by the OS.
     */
    private @NonNull List<AppPermissionGroup> getOSPermissionGroups() {
        final List<AppPermissionGroup> groups = new ArrayList<>();
        final Set<String> seenGroups = new ArraySet<>();
        final int numGroups = mAppPermissionUsages.size();
        for (int i = 0; i < numGroups; i++) {
            final AppPermissionUsage appUsage = mAppPermissionUsages.get(i);
            final List<GroupUsage> groupUsages = appUsage.getGroupUsages();
            final int groupUsageCount = groupUsages.size();
            for (int j = 0; j < groupUsageCount; j++) {
                final GroupUsage groupUsage = groupUsages.get(j);
                if (Utils.isModernPermissionGroup(groupUsage.getGroup().getName())) {
                    if (seenGroups.add(groupUsage.getGroup().getName())) {
                        groups.add(groupUsage.getGroup());
                    }
                }
            }
        }
        return groups;
    }
}
