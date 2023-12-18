package com.allwinner.camera.ui;

import android.annotation.SuppressLint;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.preference.ListPreference;
import android.preference.Preference;
import android.preference.PreferenceFragment;
import android.preference.PreferenceGroup;
import android.preference.PreferenceManager;
import android.util.Log;

import com.allwinner.camera.R;
import com.allwinner.camera.data.CameraData;
import com.allwinner.camera.data.Contants;
import com.allwinner.camera.utils.PlatFormsUtils;

import java.util.List;

import static org.greenrobot.eventbus.EventBus.TAG;


public class VideoSettingFragment extends PreferenceFragment implements Preference.OnPreferenceChangeListener {
    private ListPreference mBackListPreference;
    private ListPreference mFrontListPreference;
    private final static String TAG = "VideoSettingFragment";

    @SuppressLint("ResourceType")
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        super.onCreate(savedInstanceState);
        addPreferencesFromResource(R.layout.video_pref_general);//加载xml文件
        mBackListPreference = (ListPreference) findPreference("back_video_scale");
        mFrontListPreference = (ListPreference) findPreference("front_video_scale");
        if(CameraData.getInstance().getHasBackCamera()) {
            List<String> backList = CameraData.getInstance().getDataList("videoquality" + CameraData.getInstance().getBackCameraId());
            Log.e(TAG, "backList:" + backList);
            String[] backArray = backList.toArray(new String[backList.size()]);
            mBackListPreference.setEntries(backArray);
            mBackListPreference.setEntryValues(backArray);
            mBackListPreference.setOnPreferenceChangeListener(this);
        }else{
            ((PreferenceGroup)findPreference("video")).removePreference(mBackListPreference);
        }
        if(CameraData.getInstance().getHasFrontCamera()) {
            List<String> frontList = CameraData.getInstance().getDataList("videoquality" + CameraData.getInstance().getFrontCameraId());
            Log.e(TAG, "frontList:" + frontList);
            String[] frontArray = frontList.toArray(new String[frontList.size()]);
            mFrontListPreference.setEntries(frontArray);
            mFrontListPreference.setEntryValues(frontArray);
            mFrontListPreference.setOnPreferenceChangeListener(this);
        }else{
            ((PreferenceGroup)findPreference("video")).removePreference(mFrontListPreference);
        }
    }

    @Override
    public void onResume() {
        super.onResume();
        SharedPreferences shp = PreferenceManager.getDefaultSharedPreferences(getContext());
        String backValue = shp.getString("back_video_scale", Contants.VIDEO_SCALE_DEFAULT_VALUE);
        mBackListPreference.setSummary(backValue);
        String frontValue = shp.getString("front_video_scale", Contants.VIDEO_SCALE_DEFAULT_VALUE);
        mFrontListPreference.setSummary(frontValue);
    }

    @Override
    public boolean onPreferenceChange(Preference preference, Object newValue) {
        preference.setSummary("" + newValue);
        return true;
    }
}
