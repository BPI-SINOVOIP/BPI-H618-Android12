package com.softwinner.awlogsettings

import android.os.Bundle
import android.os.SystemProperties
import androidx.preference.*

class ATFLogSettings : PreferenceFragmentCompat(), Preference.OnPreferenceChangeListener {

    override fun onCreatePreferences(savedInstanceState: Bundle?, rootKey: String?) {
        setPreferencesFromResource(R.xml.atf_log_settings, rootKey)
        findPreference<Preference>("atf_log_enabled")?.onPreferenceChangeListener = this
    }

    override fun onBindPreferences() {
        activity?.setTitle(R.string.atf_log)
    }

    override fun onResume() {
        super.onResume()
        val enable = SystemProperties.getBoolean("debug.atf_log.enable", false)
        val bootEnable = SystemProperties.getBoolean("persist.debug.atf_log.enable", false)
        findPreference<SwitchPreferenceCompat>("atf_log_enabled")?.isChecked = enable
        findPreference<SwitchPreferenceCompat>("atf_log_boot_enabled")?.isChecked = bootEnable
    }

    override fun onPreferenceChange(preference: Preference?, newValue: Any?): Boolean {
        when (preference?.key) {
            "atf_log_enabled" -> {
                val enable = newValue as Boolean
                val bootEnable = findPreference<SwitchPreferenceCompat>("atf_log_boot_enabled")?.isChecked!!
                if (bootEnable)
                    SystemProperties.set("persist.debug.atf_log.enable", newValue.toString())
                else
                    SystemProperties.set("debug.atf_log.enable", newValue.toString())
                if (!enable) {
                    findPreference<SwitchPreferenceCompat>("atf_log_boot_enabled")?.isChecked = false
                    SystemProperties.set("persist.debug.atf_log.enable", newValue.toString())
                }
            }
        }
        return true
    }
}