package com.allwinner.camera;


import android.content.Intent;
import android.os.Bundle;
import android.preference.PreferenceFragment;
import android.support.v7.app.AppCompatActivity;
import android.util.Log;
import android.view.MenuItem;
import android.view.Window;

import com.allwinner.camera.data.CameraData;
import com.allwinner.camera.data.Contants;
import com.allwinner.camera.ui.AutoSettingFragment;
import com.allwinner.camera.ui.BaseModeUI;
import com.allwinner.camera.ui.ProfessionSettingFragment;
import com.allwinner.camera.ui.SquareSettingFragment;
import com.allwinner.camera.ui.VideoSettingFragment;

public class SettingFragmentActivity extends AppCompatActivity  {
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        requestWindowFeature(Window.FEATURE_NO_TITLE);
        if (savedInstanceState == null) {
            PreferenceFragment settingFragment = getFragment(CameraData.getInstance().getCurrentModeType());
            getFragmentManager().beginTransaction()
                    .add(android.R.id.content,settingFragment)
                    .commit();
        }
        android.support.v7.app.ActionBar actionBar = getSupportActionBar();
        if(actionBar != null){
            actionBar.setHomeButtonEnabled(true);
            actionBar.setDisplayHomeAsUpEnabled(true);
        }
    }
    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        switch (item.getItemId()) {
            case android.R.id.home:
                this.finish(); // back button
                return true;
        }
        return super.onOptionsItemSelected(item);
    }
    public PreferenceFragment getFragment(Contants.ModeType type) {
        switch (type) {
            case SquareMode:
                return new SquareSettingFragment();
            case ProfessionMode:
                return new ProfessionSettingFragment();
            case AutoMode:
                return new AutoSettingFragment();
            case VideMode:
                return new VideoSettingFragment();

            default:
                return new AutoSettingFragment();
        }

    }


}
