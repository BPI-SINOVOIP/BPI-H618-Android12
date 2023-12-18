package com.softwinner.awlogsettings

import android.os.Bundle
import android.os.SystemProperties
import androidx.preference.*

class KernelLogSettings : PreferenceFragmentCompat(), Preference.OnPreferenceChangeListener {

    override fun onCreatePreferences(savedInstanceState: Bundle?, rootKey: String?) {
        setPreferencesFromResource(R.xml.kernel_log_settings, rootKey)
        findPreference<Preference>("kernel_log_enabled")?.onPreferenceChangeListener = this
        findPreference<Preference>("kernel_log_single_log_size")?.onPreferenceChangeListener = this
        findPreference<Preference>("kernel_log_log_limit_count")?.onPreferenceChangeListener = this
    }

    override fun onBindPreferences() {
        activity?.setTitle(R.string.kernel_log)
    }

    override fun onResume() {
        super.onResume()
        val enable = SystemProperties.getBoolean("debug.kernel_log.enable", false)
        val bootEnable = SystemProperties.getBoolean("persist.debug.kernel_log.enable", false)
        val size = SystemProperties.get("persist.debug.kernel_log.size")
        val limit = SystemProperties.get("persist.debug.kernel_log.limit")
        findPreference<SwitchPreferenceCompat>("kernel_log_enabled")?.isChecked = enable
        findPreference<SwitchPreferenceCompat>("kernel_log_boot_enabled")?.isChecked = bootEnable
        findPreference<ListPreference>("kernel_log_single_log_size")?.value = size
        findPreference<ListPreference>("kernel_log_log_limit_count")?.value = limit
    }

    override fun onPreferenceChange(preference: Preference?, newValue: Any?): Boolean {
        when (preference?.key) {
            "kernel_log_enabled" -> {
                val enable = newValue as Boolean
                val bootEnable = findPreference<SwitchPreferenceCompat>("kernel_log_boot_enabled")?.isChecked!!
                if (bootEnable)
                    SystemProperties.set("persist.debug.kernel_log.enable", newValue.toString())
                else
                    SystemProperties.set("debug.kernel_log.enable", newValue.toString())
                if (!enable) {
                    findPreference<SwitchPreferenceCompat>("kernel_log_boot_enabled")?.isChecked = false
                    SystemProperties.set("persist.debug.kernel_log.enable", newValue.toString())
                }
            }
            "kernel_log_single_log_size" -> {
                SystemProperties.set("persist.debug.kernel_log.size", newValue.toString())
            }
            "kernel_log_log_limit_count" -> {
                SystemProperties.set("persist.debug.kernel_log.limit", newValue.toString())
            }
        }
        return true
    }
}