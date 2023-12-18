package com.softwinner.awlogsettings

import android.os.Bundle
import android.os.SystemProperties
import androidx.preference.*

class CrashdumpSettings : PreferenceFragmentCompat(), Preference.OnPreferenceChangeListener {

    override fun onCreatePreferences(savedInstanceState: Bundle?, rootKey: String?) {
        setPreferencesFromResource(R.xml.crashdump_settings, rootKey)
        findPreference<Preference>("crashdump_enabled")?.onPreferenceChangeListener = this
    }

    override fun onBindPreferences() {
        activity?.setTitle(R.string.crashdump)
    }

    override fun onResume() {
        super.onResume()
        val enable = SystemProperties.getBoolean("debug.crashdump.enable", false)
        val bootEnable = SystemProperties.getBoolean("persist.debug.crashdump.enable", false)
        findPreference<SwitchPreferenceCompat>("crashdump_enabled")?.isChecked = enable
        findPreference<SwitchPreferenceCompat>("crashdump_boot_enabled")?.isChecked = bootEnable
    }

    override fun onPreferenceChange(preference: Preference?, newValue: Any?): Boolean {
        when (preference?.key) {
            "crashdump_enabled" -> {
                val enable = newValue as Boolean
                val bootEnable = findPreference<SwitchPreferenceCompat>("crashdump_boot_enabled")?.isChecked!!
                if (bootEnable)
                    SystemProperties.set("persist.debug.crashdump.enable", newValue.toString())
                else
                    SystemProperties.set("debug.crashdump.enable", newValue.toString())
                if (!enable) {
                    findPreference<SwitchPreferenceCompat>("crashdump_boot_enabled")?.isChecked = false
                    SystemProperties.set("persist.debug.crashdump.enable", newValue.toString())
                }
            }
        }
        return true
    }
}