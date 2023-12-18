package com.softwinner.awlogsettings

import android.os.Bundle
import android.os.SystemProperties
import androidx.preference.*

class SCPLogSettings : PreferenceFragmentCompat(), Preference.OnPreferenceChangeListener {

    override fun onCreatePreferences(savedInstanceState: Bundle?, rootKey: String?) {
        setPreferencesFromResource(R.xml.scp_log_settings, rootKey)
        findPreference<Preference>("scp_log_enabled")?.onPreferenceChangeListener = this
    }

    override fun onBindPreferences() {
        activity?.setTitle(R.string.scp_log)
    }

    override fun onResume() {
        super.onResume()
        val enable = SystemProperties.getBoolean("debug.scp_log.enable", false)
        val bootEnable = SystemProperties.getBoolean("persist.debug.scp_log.enable", false)
        findPreference<SwitchPreferenceCompat>("scp_log_enabled")?.isChecked = enable
        findPreference<SwitchPreferenceCompat>("scp_log_boot_enabled")?.isChecked = bootEnable
    }

    override fun onPreferenceChange(preference: Preference?, newValue: Any?): Boolean {
        when (preference?.key) {
            "scp_log_enabled" -> {
                val enable = newValue as Boolean
                val bootEnable = findPreference<SwitchPreferenceCompat>("scp_log_boot_enabled")?.isChecked!!
                if (bootEnable)
                    SystemProperties.set("persist.debug.scp_log.enable", newValue.toString())
                else
                    SystemProperties.set("debug.scp_log.enable", newValue.toString())
                if (!enable) {
                    findPreference<SwitchPreferenceCompat>("scp_log_boot_enabled")?.isChecked = false
                    SystemProperties.set("persist.debug.scp_log.enable", newValue.toString())
                }
            }
        }
        return true
    }
}