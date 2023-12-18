package com.allwinner.camera.ui;

import android.Manifest;
import android.annotation.SuppressLint;
import android.annotation.TargetApi;
import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.pm.PackageManager;
import android.graphics.Point;
import android.os.Build;
import android.os.Bundle;
import android.preference.ListPreference;
import android.preference.Preference;
import android.preference.PreferenceFragment;
import android.preference.PreferenceGroup;
import android.preference.PreferenceManager;
import android.preference.SwitchPreference;
import android.text.Editable;
import android.text.TextUtils;
import android.text.TextWatcher;
import android.util.Log;
import android.widget.Button;
import android.widget.EditText;
import android.widget.Toast;

import com.allwinner.camera.CameraActivity;
import com.allwinner.camera.PermissionActivity;
import com.allwinner.camera.R;
import com.allwinner.camera.data.CameraData;
import com.allwinner.camera.data.Contants;
import com.allwinner.camera.utils.LocationUtils;
import com.allwinner.camera.utils.PlatFormsUtils;
import com.allwinner.camera.utils.SoundPlayerUtil;
import com.allwinner.camera.utils.CameraUtils;

import java.util.List;

import static org.greenrobot.eventbus.EventBus.TAG;


public class AutoSettingFragment extends PreferenceFragment implements Preference.OnPreferenceChangeListener {
    private ListPreference mBackListPreference;
    private ListPreference mFrontListPreference;
    private SwitchPreference mPositionPreference;
    private final static String TAG = "AutoSettingFragment";
    private SwitchPreference mSoundPreference;
    private SwitchPreference mTimeWaterSignPreference;
    private ListPreference mModelWaterSignPreference;

    // for location permission result code
    private final static int LOCATION_REQUEST_CODE = 111;

    @SuppressLint("ResourceType")
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        super.onCreate(savedInstanceState);
        addPreferencesFromResource(R.layout.auto_pref_general);//加载xml文件
        mSoundPreference = (SwitchPreference) findPreference(Contants.KEY_SOUND_CAPTURE);
        mSoundPreference.setOnPreferenceChangeListener(this);
        mPositionPreference = (SwitchPreference) findPreference(Contants.KEY_SAVE_POSITION);
        mPositionPreference.setOnPreferenceChangeListener(this);
        mTimeWaterSignPreference = (SwitchPreference) findPreference(Contants.KEY_TIME_WATERSIGN);
        mTimeWaterSignPreference.setOnPreferenceChangeListener(this);
        mBackListPreference = (ListPreference) findPreference(Contants.KEY_BACK_PICTURE_SCALE);
        mFrontListPreference = (ListPreference) findPreference(Contants.KEY_FRONT_PICTURE_SCALE);
        mModelWaterSignPreference = (ListPreference) findPreference(Contants.KEY_MODEL_WATERSIGN);
        mModelWaterSignPreference.setOnPreferenceChangeListener(this);

        if(CameraData.getInstance().getHasBackCamera()) {
            mBackListPreference.setEnabled(true);
            List<String> backList = CameraData.getInstance().getDataList("picturesize" + CameraData.getInstance().getBackCameraId());
            Log.e(TAG, "backList:" + backList);
            String[] backArray = backList.toArray(new String[backList.size()]);
            mBackListPreference.setEntries(backArray);
            mBackListPreference.setEntryValues(backArray);
            mBackListPreference.setOnPreferenceChangeListener(this);

        }else{
            ((PreferenceGroup)findPreference("auto")).removePreference(mBackListPreference);
        }
        if(CameraData.getInstance().getHasFrontCamera()) {
            List<String> frontList = CameraData.getInstance().getDataList("picturesize" + CameraData.getInstance().getFrontCameraId());
            Log.e(TAG, "frontList:" + frontList + "CameraData.getInstance().getFrontCameraId():" + CameraData.getInstance().getFrontCameraId());
            String[] frontArray = frontList.toArray(new String[frontList.size()]);
            mFrontListPreference.setEntries(frontArray);
            mFrontListPreference.setEntryValues(frontArray);
            mFrontListPreference.setOnPreferenceChangeListener(this);
        }else{
            ((PreferenceGroup)findPreference("auto")).removePreference(mFrontListPreference);
        }

        if (CameraUtils.checkHasNotLocationPermission(getContext())) {
            mPositionPreference.setChecked(false);
        }

    }

    @TargetApi(Build.VERSION_CODES.M)
    @Override
    public void onResume() {
        super.onResume();
        SharedPreferences shp = PreferenceManager.getDefaultSharedPreferences(getContext());
        String backValue = shp.getString(Contants.KEY_BACK_PICTURE_SCALE, Contants.PICTURE_SCALE_DEFAULT_VALUE);
        mBackListPreference.setSummary(backValue);
        String frontValue = shp.getString(Contants.KEY_FRONT_PICTURE_SCALE, Contants.PICTURE_SCALE_DEFAULT_VALUE);
        mFrontListPreference.setSummary(frontValue);
        String modelWaterSignValue = shp.getString(Contants.KEY_MODEL_WATERSIGN, Contants.MODEL_WATERSIGN_DEFAULT_VALUE);
        if(modelWaterSignValue.equals(Contants.ModelWatersignType.TYPE_CUSTOM)){
            mModelWaterSignPreference.setSummary(CameraData.getInstance().getCustomText());
        }
        else {
            //获取ListPreference中的实体内容
            CharSequence[] entries=mModelWaterSignPreference.getEntries();
            //获取ListPreference中的实体内容的下标值
            int index=mModelWaterSignPreference.findIndexOfValue(modelWaterSignValue);
            //把listPreference中的摘要显示为当前ListPreference的实体内容中选择的那个项目
            mModelWaterSignPreference.setSummary(entries[index]);
        }
    }

    private void requesetLocationPermission() {
        Intent intent = new Intent(getContext(), PermissionActivity.class);
        intent.putExtra(PermissionActivity.LOCATION_PERMISSION, true);
        ((Activity) getContext()).startActivityForResult(intent, LOCATION_REQUEST_CODE);
    }

    @TargetApi(Build.VERSION_CODES.M)
    @Override
    public boolean onPreferenceChange(final Preference preference, final Object newValue) {
        String key = preference.getKey();
//        if(key.equals(Contants.KEY_BACK_PICTURE_SCALE ) || key.equals(Contants.KEY_FRONT_PICTURE_SCALE)) {
//            preference.setSummary(""+newValue);
//        }else
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
        if(preference instanceof ListPreference){
            ListPreference listPreference=(ListPreference)preference;
            CharSequence[] entries=listPreference.getEntries();
            int index=listPreference.findIndexOfValue((String)newValue);
            if (key.equals(Contants.KEY_MODEL_WATERSIGN)) {
                String modelType = (String) newValue;
                if (modelType.equals(Contants.ModelWatersignType.TYPE_CUSTOM)) {
                    final EditText et = new EditText(getContext());
                    et.setHint(R.string.custom_watersign_hint);
                    et.setPadding(20, 30, 0, 20);
                    Dialog dialog = new AlertDialog.Builder(getContext()).setTitle(getString(R.string.model_watersign))
                            .setIcon(android.R.drawable.sym_def_app_icon)
                            .setView(et)
                            .setCancelable(false)
                            .setPositiveButton(getString(R.string.save), new DialogInterface.OnClickListener() {
                                @Override
                                public void onClick(DialogInterface dialogInterface, int i) {
                                    //按下确定键后的事件
                                    String text = et.getText().toString();
                                    preference.setSummary(text);
                                    CameraData.getInstance().setCustomText(text);
                                }
                            }).setNegativeButton(getString(R.string.cancel), new DialogInterface.OnClickListener() {
                                @Override
                                public void onClick(DialogInterface dialogInterface, int i) {
                                    String summary = (String) mModelWaterSignPreference.getSummary();
                                    if(summary.equals(getString(R.string.off))){
                                        mModelWaterSignPreference.setValue(Contants.ModelWatersignType.TYPE_OFF);
                                    }else if(summary.equals(getString(R.string.defaults))){
                                        mModelWaterSignPreference.setValue(Contants.ModelWatersignType.TYPE_DEFAULT);
                                    }
                                }
                            }).show();
                    final Button btn = ((AlertDialog) dialog).getButton(DialogInterface.BUTTON_POSITIVE);
                    btn.setEnabled(false);
                    et.addTextChangedListener(new TextWatcher() {

                        @Override
                        public void onTextChanged(CharSequence s, int start, int before, int count) {
                            // TODO Auto-generated method stub

                        }

                        @Override
                        public void beforeTextChanged(CharSequence s, int start, int count, int after) {
                            // TODO Auto-generated method stub

                        }

                        @Override
                        public void afterTextChanged(Editable s) {
                            // TODO Auto-generated method stub
                            String name = s.toString().trim();
                            if (name.isEmpty())
                                btn.setEnabled(false);
                            else
                                btn.setEnabled(true);
                        }
                    });
                    return true;
                }
            }
            listPreference.setSummary(entries[index]);
        }
        return true;
    }
}
