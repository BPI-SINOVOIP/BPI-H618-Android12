package com.allwinner.camera.modes;

import android.content.Context;
import android.os.Bundle;
import android.support.annotation.Nullable;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import com.allwinner.camera.R;
import com.allwinner.camera.data.Contants;


public class PanoModeFragment extends BaseModeFragment {
    private static final  String TAG = "PanoModeFragment";

    @Override
    public View onCreateView(LayoutInflater inflater, @Nullable ViewGroup container, Bundle savedInstanceState) {
        return inflater.inflate(R.layout.panomodefragment,container,false);
    }


    @Override
    public void onAttach(Context context) {
        super.onAttach(context);
        Log.i(TAG,"onAttach");
    }

    @Override
    public void onDetach() {
        super.onDetach();
        Log.i(TAG,"onDetach");
    }


    @Override
    public void onStart() {
        super.onStart();
        Log.i(TAG,"onStart");
    }

    @Override
    public void onResume() {
        super.onResume();
        Log.i(TAG,"onResume");
    }

    @Override
    public void onPause() {
        super.onPause();
        Log.i(TAG,"onPause");
    }


    @Override
    public void onStop() {
        super.onStop();
        Log.i(TAG,"onStop");
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        Log.i(TAG,"onDestroy");
    }

    @Override
    public void setUserVisibleHint(boolean isVisibleToUser) {
        super.setUserVisibleHint(isVisibleToUser);
        Log.i(TAG,"setUserVisibleHint: " + mIsVisibleToUser);
    }
    @Override
    public void setCurrentMode() {
        getUiManager().setCurrentMode(Contants.ModeType.PanoMode);
    }
    @Override
    public void initView(View rootView) {
        getUiManager().initModeView(rootView,Contants.ModeType.PanoMode);
    }

}
