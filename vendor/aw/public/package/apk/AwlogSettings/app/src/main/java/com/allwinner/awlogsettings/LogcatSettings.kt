package com.softwinner.awlogsettings

import android.os.Bundle
import android.os.SystemProperties
import androidx.preference.*

class LogcatSettings : PreferenceFragmentCompat(), Preference.OnPreferenceChangeListener {

    override fun onCreatePreferences(savedInstanceState: Bundle?, rootKey: String?) {
        setPreferencesFromResource(R.xml.logcat_settings, rootKey)
        findPreference<Preference>("logcat_enabled")?.onPreferenceChangeListener = this
        findPreference<Preference>("logcat_buffer_type")?.onPreferenceChangeListener = this
        findPreference<Preference>("logcat_single_log_size")?.onPreferenceChangeListener = this
        findPreference<Preference>("logcat_log_limit_count")?.onPreferenceChangeListener = this
    }

    override fun onBindPreferences() {
        activity?.setTitle(R.string.logcat)
    }

    override fun onResume() {
        super.onResume()
        val enable = SystemProperties.getBoolean("debug.logcat.enable", false)
        val bootEnable = SystemProperties.getBoolean("persist.debug.logcat.enable", false)
        val buffer = SystemProperties.get("persist.debug.logcat.buffer")
        val size = SystemProperties.get("persist.debug.logcat.size")
        val limit = SystemProperties.get("persist.debug.logcat.limit")
        findPreference<SwitchPreferenceCompat>("logcat_enabled")?.isChecked = enable
        findPreference<SwitchPreferenceCompat>("logcat_boot_enabled")?.isChecked = bootEnable
        findPreference<MultiSelectListPreference>("logcat_buffer_type")?.summary = buffer
        if (!buffer.isNullOrBlank())
            findPreference<MultiSelectListPreference>("logcat_buffer_type")?.values = buffer.split(",").toSet()
        findPreference<ListPreference>("logcat_single_log_size")?.value = size
        findPreference<ListPreference>("logcat_log_limit_count")?.value = limit
    }

    override fun onPreferenceChange(preference: Preference?, newValue: Any?): Boolean {
        when (preference?.key) {
            "logcat_enabled" -> {
                val enable = newValue as Boolean
                val bootEnable = findPreference<SwitchPreferenceCompat>("logcat_boot_enabled")?.isChecked!!
                if (bootEnable)
                    SystemProperties.set("persist.debug.logcat.enable", newValue.toString())
                else
                    SystemProperties.set("debug.logcat.enable", newValue.toString())
                if (!enable) {
                    findPreference<SwitchPreferenceCompat>("logcat_boot_enabled")?.isChecked = false
                    SystemProperties.set("persist.debug.logcat.enable", newValue.toString())
                }
            }
            "logcat_buffer_type" -> {
                android.util.Log.e("rogge", "newValue = " + newValue.toString())
                val buffer = (newValue as Set<*>).joinToString(",")
                (preference as MultiSelectListPreference).summary = buffer
                android.util.Log.e("rogge", "persist.debug.logcat.buffer = " + buffer)
                SystemProperties.set("persist.debug.logcat.buffer", buffer)
            }
            "logcat_single_log_size" -> {
                SystemProperties.set("persist.debug.logcat.size", newValue.toString())
            }
            "logcat_log_limit_count" -> {
                SystemProperties.set("persist.debug.logcat.limit", newValue.toString())
            }
        }
        return true
    }
}