/*
 * Copyright (C) 2019 The Android Open Source Project
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

package com.android.tv.settings.help;

import static android.content.Context.ACCESSIBILITY_SERVICE;

import static com.android.tv.settings.overlay.FlavorUtils.FLAVOR_CLASSIC;
import static com.android.tv.settings.overlay.FlavorUtils.FLAVOR_TWO_PANEL;
import static com.android.tv.settings.overlay.FlavorUtils.FLAVOR_VENDOR;
import static com.android.tv.settings.overlay.FlavorUtils.FLAVOR_X;
import static com.android.tv.settings.util.InstrumentationUtils.logEntrySelected;

import android.app.tvsettings.TvSettingsEnums;
import android.content.pm.ResolveInfo;
import android.os.Bundle;
import android.view.accessibility.AccessibilityManager;

import androidx.annotation.Keep;
import androidx.leanback.widget.VerticalGridView;
import androidx.preference.Preference;

import com.android.tv.settings.MainFragment;
import com.android.tv.settings.R;
import com.android.tv.settings.SettingsPreferenceFragment;
import com.android.tv.settings.overlay.FlavorUtils;

/**
 * The "Help & feedback" screen in TV settings.
 */
@Keep
public class HelpFragment extends SettingsPreferenceFragment {

    private static final String KEY_SEND_FEEDBACK = "feedback";
    private static final String KEY_HELP = "help_center";

    private int getPreferenceScreenResId() {
        switch (FlavorUtils.getFlavor(getContext())) {
            case FLAVOR_CLASSIC:
            case FLAVOR_TWO_PANEL:
                return R.xml.help_and_feedback;
            case FLAVOR_X:
            case FLAVOR_VENDOR:
                return R.xml.help_and_feedback_x;
            default:
                return R.xml.help_and_feedback;
        }
    }

    @Override
    public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
        setPreferencesFromResource(getPreferenceScreenResId(), null);

        final Preference sendFeedbackPref = findPreference(KEY_SEND_FEEDBACK);
        if (sendFeedbackPref != null) {
            sendFeedbackPref.setVisible(!FlavorUtils.getFeatureFactory(getContext())
                    .getBasicModeFeatureProvider().isBasicMode(getContext()));
        }

        final Preference helpPref = findPreference(KEY_HELP);
        if (helpPref != null) {
            final ResolveInfo info = MainFragment.systemIntentIsHandled(getActivity(),
                    helpPref.getIntent());
            helpPref.setVisible(info != null);
        }
    }

    @Override
    public boolean onPreferenceTreeClick(Preference preference) {
        AccessibilityManager am =
                (AccessibilityManager) getContext().getSystemService(ACCESSIBILITY_SERVICE);
        boolean isAccessibilityEnabled = am.isEnabled();
        VerticalGridView listView = (VerticalGridView) getListView();
        if (!listView.getChildAt(listView.getSelectedPosition()).isAccessibilityFocused()
                && isAccessibilityEnabled) {
            return true;
        }
        switch (preference.getKey()) {
            case KEY_SEND_FEEDBACK:
                logEntrySelected(TvSettingsEnums.FEEDBACK_SEND);
                return super.onPreferenceTreeClick(preference);
            case KEY_HELP:
                FlavorUtils.getFeatureFactory(getActivity()).getSupportFeatureProvider()
                        .startSupport(getActivity());
                return true;
            default:
                return super.onPreferenceTreeClick(preference);
        }
    }

    @Override
    protected int getPageId() {
        return TvSettingsEnums.FEEDBACK;
    }
}
