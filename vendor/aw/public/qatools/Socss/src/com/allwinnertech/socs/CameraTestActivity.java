/*
 * Copyright 2017 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.allwinnertech.socs;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.os.CountDownTimer;

import com.allwinnertech.socs.camera.Camera2Fragment;

import android.util.Log;
import android.widget.FrameLayout;
import android.widget.LinearLayout;
import android.widget.TextView;

import com.google.android.cameraview.CameraView;

public class CameraTestActivity extends Activity {

	private Context mContext;
	private CameraView mCameraView;
	private TextView mInfo;
	private String TAG = "CameraTestActivity";
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        mContext = (Context)this;
        setContentView(R.layout.activity_camera);
        mCameraView = (CameraView)findViewById(R.id.camera);
        mInfo = (TextView)findViewById(R.id.camerainfo);
        mCameraView.setFacing(CameraView.FACING_FRONT);
        mInfo.setText(R.string.camera_front);
        mCameraView.start();
        mSwitchTimer.start();
        mShowTimer.start();
    }

    private CountDownTimer mSwitchTimer = new CountDownTimer(3 * 1000, 3 * 1000) {

        @Override
        public void onTick(long arg0) {

        }

        @Override
        public void onFinish() {
            Log.d(TAG,"switch camera");
            mInfo.setText(R.string.camera_back);
            mCameraView.setFacing(CameraView.FACING_BACK);
        }
    };
    
    private CountDownTimer mShowTimer = new CountDownTimer(6 * 1000, 6 * 1000) {

        @Override
        public void onTick(long arg0) {

        }

        @Override
        public void onFinish() {
            finishTest();
        }
    };

    private void finishTest() {
        finish();
    }

    @Override
    public void finish() {
    	mCameraView.stop();
        mShowTimer.cancel();
        Intent returnIntend =new Intent();
        setResult(RESULT_OK,returnIntend);
        super.finish();
    };
}
