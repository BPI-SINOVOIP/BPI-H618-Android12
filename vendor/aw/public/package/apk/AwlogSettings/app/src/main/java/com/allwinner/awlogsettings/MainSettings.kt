package com.softwinner.awlogsettings

import android.os.Bundle
import android.os.SystemProperties
import androidx.preference.Preference
import androidx.preference.PreferenceFragmentCompat
import androidx.preference.SwitchPreferenceCompat

class MainSettings : PreferenceFragmentCompat(), Preference.OnPreferenceChangeListener {

    override fun onCreatePreferences(savedInstanceState: Bundle?, rootKey: String?) {
        setPreferencesFromResource(R.xml.awlog_settings, rootKey)
        findPreference<Preference>("awlog_enabled")?.onPreferenceChangeListener = this
    }

    override fun onResume() {
        super.onResume()
        val enable = SystemProperties.getBoolean("persist.debug.logpersistd", false)
        findPreference<SwitchPreferenceCompat>("awlog_enabled")?.setChecked(enable)
    }

    override fun onBindPreferences() {
        activity?.setTitle(R.string.app_name)
    }

    override fun onPreferenceChange(preference: Preference?, newValue: Any?): Boolean {
        SystemProperties.set("persist.debug.logpersistd", newValue.toString())
        return true
    }

}