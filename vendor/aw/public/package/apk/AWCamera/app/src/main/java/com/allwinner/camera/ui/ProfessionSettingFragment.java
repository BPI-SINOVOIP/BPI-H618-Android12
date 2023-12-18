package com.allwinner.camera.ui;

import android.annotation.SuppressLint;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.preference.ListPreference;
import android.preference.Preference;
import android.preference.PreferenceFragment;
import android.preference.PreferenceManager;
import android.util.Log;

import com.allwinner.camera.R;
import com.allwinner.camera.data.CameraData;
import com.allwinner.camera.data.Contants;
import com.allwinner.camera.utils.PlatFormsUtils;

import java.util.List;

import static org.greenrobot.eventbus.EventBus.TAG;


public class ProfessionSettingFragment extends PreferenceFragment implements Preference.OnPreferenceChangeListener{
    private ListPreference mBackListPreference;
    private ListPreference mFrontListPreference;

    @SuppressLint("ResourceType")
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        // 加载xml资源文件
        super.onCreate(savedInstanceState);
        addPreferencesFromResource(R.layout.video_pref_general);//加载xml文件
        mBackListPreference = (ListPreference) findPreference(Contants.KEY_BACK_PICTURE_SCALE);
        List<String> backList = CameraData.getInstance().getDataList("picturesize"+CameraData.getInstance().getBackCameraId());
        Log.e(TAG,"backList:"+backList);
        String[] backArray = backList.toArray(new String[backList.size()]);
        mBackListPreference.setEntries(backArray);
        mBackListPreference.setEntryValues(backArray);
        mBackListPreference.setOnPreferenceChangeListener(this);
        mFrontListPreference = (ListPreference) findPreference(Contants.KEY_FRONT_PICTURE_SCALE);
        List<String> frontList = CameraData.getInstance().getDataList("picturesize"+CameraData.getInstance().getFrontCameraId());
        Log.e(TAG,"frontList:"+frontList);
        String[] frontArray = frontList.toArray(new String[frontList.size()]);
        mFrontListPreference.setEntries(frontArray);
        mFrontListPreference.setEntryValues(frontArray);
        mFrontListPreference.setOnPreferenceChangeListener(this);
    }

    @Override
    public void onResume() {
        super.onResume();
        SharedPreferences shp = PreferenceManager.getDefaultSharedPreferences(getContext());
        String backValue = shp.getString(Contants.KEY_BACK_PICTURE_SCALE, Contants.PICTURE_SCALE_DEFAULT_VALUE);
        mBackListPreference.setSummary(backValue);
        String frontValue = shp.getString(Contants.KEY_FRONT_PICTURE_SCALE, Contants.PICTURE_SCALE_DEFAULT_VALUE);
        mFrontListPreference.setSummary(frontValue);
    }

    @Override
    public boolean onPreferenceChange(Preference preference, Object newValue) {
        String key=preference.getKey();
        preference.setSummary(""+newValue);
        return true;

    }
}
