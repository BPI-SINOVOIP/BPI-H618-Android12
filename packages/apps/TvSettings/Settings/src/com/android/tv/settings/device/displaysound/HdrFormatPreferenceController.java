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

package com.android.tv.settings.device.displaysound;

import android.content.Context;
import android.hardware.display.DisplayManager;

import androidx.preference.Preference;
import androidx.preference.SwitchPreference;

import com.android.settingslib.core.AbstractPreferenceController;

import java.util.Arrays;
import java.util.Set;
import java.util.stream.Collectors;

/**
 * Controller for the hdr formats switch preferences.
 */
public class HdrFormatPreferenceController extends AbstractPreferenceController {

    private final int mHdrType;
    private final DisplayManager mDisplayManager;

    public HdrFormatPreferenceController(
            Context context, int hdrType, DisplayManager displayManager) {
        super(context);
        mHdrType = hdrType;
        mDisplayManager = displayManager;
    }

    @Override
    public boolean isAvailable() {
        return true;
    }

    @Override
    public String getPreferenceKey() {
        return HdrFormatSelectionFragment.KEY_HDR_FORMAT_PREFIX + mHdrType;
    }

    @Override
    public void updateState(Preference preference) {
        super.updateState(preference);
        if (preference.getKey().equals(getPreferenceKey())) {
            preference.setEnabled(getPreferenceEnabledState());
            ((SwitchPreference) preference).setChecked(getPreferenceCheckedState());
        }
    }

    @Override
    public boolean handlePreferenceTreeClick(Preference preference) {
        if (preference.getKey().equals(getPreferenceKey())) {
            onPreferenceClicked((SwitchPreference) preference);
        }
        return super.handlePreferenceTreeClick(preference);
    }

    /**
     * @return checked state of a HDR format switch based displayManager.
     */
    private boolean getPreferenceCheckedState() {
        if (!mDisplayManager.areUserDisabledHdrTypesAllowed()) {
            return !(toSet(mDisplayManager.getUserDisabledHdrTypes()).contains(mHdrType));
        }
        return true;
    }

    /** @return true if the format checkboxes should be enabled, i.e. in manual mode. */
    private boolean getPreferenceEnabledState() {
        return !mDisplayManager.areUserDisabledHdrTypesAllowed();
    }

    /**
     * Handler for when this particular format preference is clicked.
     */
    private void onPreferenceClicked(SwitchPreference preference) {
        final boolean enabled = preference.isChecked();

        Set<Integer> disabledHdrTypes = toSet(mDisplayManager.getUserDisabledHdrTypes());
        if (enabled) {
            disabledHdrTypes.remove(mHdrType);
        } else {
            disabledHdrTypes.add(mHdrType);
        }
        mDisplayManager.setUserDisabledHdrTypes(toArray(disabledHdrTypes));
    }

    private int[] toArray(Set<Integer> set) {
        return set.stream().mapToInt(Integer::intValue).toArray();
    }

    private Set<Integer> toSet(int[] array) {
        return Arrays.stream(array).boxed().collect(Collectors.toSet());
    }
}
