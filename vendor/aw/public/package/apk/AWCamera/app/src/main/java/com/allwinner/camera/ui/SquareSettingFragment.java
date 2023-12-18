package com.allwinner.camera.ui;

import android.Manifest;
import android.annotation.SuppressLint;
import android.app.Activity;
import android.app.AlertDialog;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.pm.PackageManager;
import android.os.Bundle;
import android.preference.ListPreference;
import android.preference.Preference;
import android.preference.PreferenceFragment;
import android.preference.PreferenceManager;
import android.preference.SwitchPreference;
import android.widget.Toast;

import com.allwinner.camera.PermissionActivity;
import com.allwinner.camera.R;
import com.allwinner.camera.data.Contants;
import com.allwinner.camera.utils.CameraUtils;
import com.allwinner.camera.utils.LocationUtils;
import com.allwinner.camera.utils.PlatFormsUtils;

public class SquareSettingFragment extends PreferenceFragment implements Preference.OnPreferenceChangeListener {
    private ListPreference mBackListPreference;
    private ListPreference mFrontListPreference;

    private SwitchPreference mPositionPreference;

    // for location permission result code
    private final static int LOCATION_REQUEST_CODE = 111;

    @SuppressLint("ResourceType")
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        // 加载xml资源文件
        super.onCreate(savedInstanceState);

        addPreferencesFromResource(R.layout.square_pref_general);//加载xml文件
        mPositionPreference = (SwitchPreference) findPreference(Contants.KEY_SAVE_POSITION);
        mPositionPreference.setOnPreferenceChangeListener(this);
        if (CameraUtils.checkHasNotLocationPermission(getContext())) {
            mPositionPreference.setChecked(false);
        }
    }

    @Override
    public void onResume() {
        super.onResume();
    }

    private void requesetLocationPermission() {
        Intent intent = new Intent(getContext(), PermissionActivity.class);
        intent.putExtra(PermissionActivity.LOCATION_PERMISSION, true);
        ((Activity) getContext()).startActivityForResult(intent, LOCATION_REQUEST_CODE);
    }

    @Override
    public boolean onPreferenceChange(Preference preference, Object newValue) {
        String key = preference.getKey();
        if (key.equals(Contants.KEY_SAVE_POSITION)) {
            boolean value = (boolean) newValue;
            if (!value) return true;
            if (getContext().checkSelfPermission(Manifest.permission.ACCESS_FINE_LOCATION) != PackageManager.PERMISSION_GRANTED ||
                getContext().checkSelfPermission(Manifest.permission.ACCESS_COARSE_LOCATION) != PackageManager.PERMISSION_GRANTED) {
                requesetLocationPermission();
                return false;
            }

            if(!LocationUtils.getInstance(getContext()).isLocationProviderEnabled()){
                new AlertDialog.Builder(getContext())
                    .setTitle(getString(R.string.location_setting))
                    .setMessage(getContext().getString(R.string.location_open_wifi_or_gps_tips))
                    .setCancelable(false)
                    .setPositiveButton(getContext().getString(R.string.location_setting), (dialog, which) -> {
                        getContext().startActivity(new Intent(android.provider.Settings.ACTION_LOCATION_SOURCE_SETTINGS));
                    })
                    .setNegativeButton(getContext().getString(R.string.location_got_it), (dialog, which) -> {
                        dialog.dismiss();
                    })
                    .create()
                    .show();
                return false;
            }
        }
        return true;
    }
}
