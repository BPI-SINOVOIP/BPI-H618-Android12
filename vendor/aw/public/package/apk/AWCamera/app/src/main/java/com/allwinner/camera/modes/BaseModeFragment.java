package com.allwinner.camera.modes;


import android.app.Fragment;
import android.os.Bundle;
import android.util.Log;
import android.view.View;

import com.allwinner.camera.ui.UIManager;

public class BaseModeFragment extends Fragment implements ModeStatus{

    private UIManager mUiManager;
    protected boolean mIsVisibleToUser = false;
    protected boolean mIsPreviewStart = false;
    private final static String TAG = "BaseModeFragment";

    public void setUiManager(UIManager uiManager) {
        mUiManager = uiManager;
        Log.i(TAG,"setUiManager: " + mUiManager);
    }

    public UIManager getUiManager() {
        return mUiManager;
    }
    @Override
    public void onViewCreated(View view, Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);
        initView(view);
    }


    @Override
    public void onStart() {
        super.onStart();
    }

    @Override
    public void onResume() {
        super.onResume();
        Log.i(TAG, "onResume" + mIsVisibleToUser) ;
        if (mIsVisibleToUser) {
            onModeResume();
        }
    }

    @Override
    public void onPause() {
        super.onPause();
        Log.i(TAG, "onPause" + mIsVisibleToUser) ;
        if (mIsVisibleToUser) {
            onModePause();
        }
    }

    @Override
    public void onStop() {
        super.onStop();
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
    }

    @Override
    public void setUserVisibleHint(boolean isVisibleToUser) {
        super.setUserVisibleHint(isVisibleToUser);
        mIsVisibleToUser = isVisibleToUser;
        if (mIsVisibleToUser) {
            setCurrentMode();
        }
    }

    public void setCurrentMode(){

    }

    public void onModeResume(){
        getUiManager().onModeResume();
    }

    public void onModePause(){
        getUiManager().onModePause();
    }

    public void initView(View rootView){

    }



}
