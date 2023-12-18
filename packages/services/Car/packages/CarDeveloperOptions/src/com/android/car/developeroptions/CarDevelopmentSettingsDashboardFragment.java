/*
 * Copyright (C) 2020 The Android Open Source Project
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

package com.android.car.developeroptions;

import static com.android.car.developeroptions.CarDevelopmentConstants.PREFERENCES_TO_REMOVE;
import static com.android.car.ui.core.CarUi.requireToolbar;

import android.content.ComponentName;
import android.content.Context;
import android.content.pm.PackageManager;
import android.os.Bundle;

import com.android.car.ui.toolbar.MenuItem;
import com.android.car.ui.toolbar.Toolbar;
import com.android.car.ui.toolbar.ToolbarController;
import com.android.settings.SettingsActivity;
import com.android.settings.development.DevelopmentSettingsDashboardFragment;
import com.android.settingslib.core.AbstractPreferenceController;

import java.util.ArrayList;
import java.util.List;

/**
 * Main fragment for CarDeveloperOptions - extends from {@link DevelopmentSettingsDashboardFragment}
 * and overrides switch bar behavior and removes any preferences contained in the
 * {@link PREFERENCES_TO_REMOVE} constant.
 */
public class CarDevelopmentSettingsDashboardFragment extends DevelopmentSettingsDashboardFragment {

    private ToolbarController mToolbar;

    @Override
    public void onActivityCreated(Bundle icicle) {
        super.onActivityCreated(icicle);
        mToolbar = getToolbar();
        if (mToolbar != null) {
            List<MenuItem> items = getToolbarMenuItems();
            mToolbar.setTitle(getPreferenceScreen().getTitle());
            mToolbar.setMenuItems(items);
            mToolbar.setNavButtonMode(Toolbar.NavButtonMode.BACK);
            mToolbar.setState(Toolbar.State.SUBPAGE);
        }
    }

    @Override
    protected List<AbstractPreferenceController> createPreferenceControllers(Context context) {
        List<AbstractPreferenceController> controllers = super.createPreferenceControllers(context);
        removeControllers(controllers);
        addHiddenControllers(context, controllers);
        return controllers;
    }

    @Override
    public void onStop() {
        super.onStop();
        boolean switchBarChecked = ((SettingsActivity) getActivity()).getSwitchBar().isChecked();
        if (!switchBarChecked && isDeveloperOptionsModuleEnabled()) {
            PackageManager pm = getContext().getPackageManager();
            pm.setComponentEnabledSetting(getActivity().getComponentName(),
                    PackageManager.COMPONENT_ENABLED_STATE_DISABLED,
                    PackageManager.DONT_KILL_APP);
            finish();
        }
    }

    protected ToolbarController getToolbar() {
        return requireToolbar(requireActivity());
    }

    protected List<MenuItem> getToolbarMenuItems() {
        return null;
    }

    private void removeControllers(List<AbstractPreferenceController> controllers) {
        List<AbstractPreferenceController> controllersToBeRemoved = new ArrayList<>();
        for (AbstractPreferenceController controller : controllers) {
            if (PREFERENCES_TO_REMOVE.contains(controller.getPreferenceKey())) {
                controllersToBeRemoved.add(controller);
            }
        }
        controllers.removeAll(controllersToBeRemoved);
    }

    private void addHiddenControllers(Context context,
            List<AbstractPreferenceController> controllers) {
        for (String key : PREFERENCES_TO_REMOVE) {
            controllers.add(new HiddenPreferenceController(context, key));
        }
    }

    private boolean isDeveloperOptionsModuleEnabled() {
        PackageManager pm = getContext().getPackageManager();
        ComponentName component = getActivity().getComponentName();
        int state = pm.getComponentEnabledSetting(component);
        return state == PackageManager.COMPONENT_ENABLED_STATE_ENABLED;
    }
}
