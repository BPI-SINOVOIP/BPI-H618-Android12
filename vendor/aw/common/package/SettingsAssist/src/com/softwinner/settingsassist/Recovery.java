
package com.softwinner.settingsassist;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.security.GeneralSecurityException;

import android.app.Activity;
import android.content.ActivityNotFoundException;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.net.Uri;
import android.os.AsyncTask;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.os.PowerManager;

import com.softwinner.settingsassist.recovery.RecoveryUtils;

import android.os.RecoverySystem.ProgressListener;
import android.os.storage.StorageManager;
import android.os.storage.StorageVolume;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.ProgressBar;
import android.widget.TextView;
import android.widget.Toast;

import java.util.ArrayList;

public class Recovery extends Activity implements View.OnClickListener,
        RecoveryUtils.RecoveryProgress {
    
    private static final String TAG = "Recovery";

    private static final String CACHE_FILE = "/cache/update.zip";
    private static final boolean USE_CACHE = false;
    
    private static final int REQUEST_CODE_ALL_FILE = 0;
    
    private static final int RECOVERY_CPY_FAILED = 0;
    private static final int RECOVERY_VFY_FAILED = 1;

    private Button mBtnLeft;
    private Button mBtnRight;

    private ViewGroup mContentField;
    private ViewGroup mDescField;
    private ViewGroup mProgressField;
    private RecoveryUtils mRecoveryUtils;
    private File mRecoveryPkg;

    private static final int HANDLE_MSG_ERROR = 0;
    private static final int HANDLE_MSG_CPY_PROGRESS = 1;
    private static final int HANDLE_MSG_VFY_PROGRESS = 2;
    private Handler mHandler = new Handler() {
        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case HANDLE_MSG_ERROR:
                    int resid = 0;
                    switch (msg.arg1) {
                        case RECOVERY_CPY_FAILED:
                            setDescription(String.format(
                                    getString(R.string.recovery_copy_error_failed),
                                    mRecoveryPkg.getPath()));
                            break;
                        case RECOVERY_VFY_FAILED:
                            setDescription(String.format(
                                    getString(R.string.recovery_verify_error_failed),
                                    mRecoveryPkg.getPath()));
                            break;
                        default:
                            break;
                    }
                    mBtnRight.setEnabled(true);
                    mBtnLeft.setEnabled(true);
                    
                    break;
                case HANDLE_MSG_CPY_PROGRESS:
                    resid = R.string.recovery_copying;
                    setProgress(msg.arg1, resid);
                    break;
                case HANDLE_MSG_VFY_PROGRESS:
                    resid = R.string.recovery_verifying;
                    if(msg.arg1 < 100)
                        setProgress(msg.arg1, resid);
                    else 
                        setDescription(R.string.recovery_ready_to_reboot);
                    break;
            }
        }
    };
    
    private void setDescription(String str){
        View field = mContentField.findViewById(R.id.desc_field);
        if (field == null) {
            mContentField.removeAllViews();
            mContentField.addView(mDescField);
        }
        TextView desc = (TextView) mDescField.findViewById(R.id.text_description);
        desc.setText(str);
    }

    private void setDescription(int resid) {
        setDescription(getString(resid));
    }

    private void setProgress(int progress, int resid) {
        View field = mContentField.findViewById(R.id.progress_field);
        if (field == null) {
            mContentField.removeAllViews();
            mContentField.addView(mProgressField);
        }
        TextView desc = (TextView) mProgressField.findViewById(R.id.text_description);
        ProgressBar progressBar = (ProgressBar) mProgressField.findViewById(R.id.progress);
        String str = getString(resid) +  "...";
        desc.setText(str);
        progressBar.setProgress(progress);
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.recovery);

        getWindow().setCloseOnTouchOutside(false);

        mContentField = (ViewGroup) findViewById(R.id.content_field);
        mBtnLeft = (Button) findViewById(R.id.cancel);
        mBtnRight = (Button) findViewById(R.id.ok);
        mDescField = (ViewGroup) 
                LayoutInflater.from(this).inflate(R.layout.recovery_text_des, null);
        mProgressField = (ViewGroup) 
                LayoutInflater.from(this).inflate(R.layout.recovery_progress, null);
        
        mBtnRight.setText(R.string.recovery_button_select);
        mBtnRight.setOnClickListener(this);
        mBtnLeft.setOnClickListener(this);
        setDescription(R.string.recovery_des);
        
        mRecoveryUtils = new RecoveryUtils(this);
    }

    public void startActivityForResultSafely(Intent intent, int requestCode) {
        try {
            startActivityForResult(intent, requestCode);
        } catch (ActivityNotFoundException e) {
            Toast.makeText(this, R.string.recovery_activity_not_found, Toast.LENGTH_SHORT).show();
        } catch (SecurityException e) {
            Toast.makeText(this, R.string.recovery_activity_not_found, Toast.LENGTH_SHORT).show();
        }
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if(requestCode == REQUEST_CODE_ALL_FILE && resultCode == RESULT_OK){
            Uri uri = data.getData();
            if(uri != null && uri.getScheme().equals("file") ){
                File pkg = new File(uri.getPath());
                mRecoveryPkg = pkg;
                new Thread(new Runnable(){
                    @Override
                    public void run() {
                        mRecoveryUtils.recoverySystem(mRecoveryPkg, Recovery.this);
                    }
                }).start();
                mBtnRight.setEnabled(false);
                mBtnLeft.setEnabled(false);
            }
        }
    }

    @Override
    public void onClick(View v) {
        if (v.equals(mBtnRight)) {
            Intent i = new Intent("com.softwinner.action.GET_FILE");
            startActivityForResultSafely(i, REQUEST_CODE_ALL_FILE);
        } else if (v.equals(mBtnLeft)) {
            finish();
        }
    }

    @Override
    public void onVerifyProgress(int progress) {
        Message msg = mHandler.obtainMessage();
        msg.what = HANDLE_MSG_VFY_PROGRESS;
        msg.arg1 = progress;
        mHandler.sendMessage(msg);
    }

    @Override
    public void onVerifyFailed(int errorCode, Object object) {
        Log.v(TAG,"onVerifyFailed");
        Message msg = mHandler.obtainMessage();
        msg.what = HANDLE_MSG_ERROR;
        msg.arg1 = RECOVERY_VFY_FAILED;
        mHandler.sendMessage(msg);
    }

    @Override
    public void onCopyProgress(int progress) {
        Message msg = mHandler.obtainMessage();
        msg.what = HANDLE_MSG_CPY_PROGRESS;
        msg.arg1 = progress;
        mHandler.sendMessage(msg);
    }

    @Override
    public void onCopyFailed(int errorCode, Object object) {
        Message msg = mHandler.obtainMessage();
        msg.what = HANDLE_MSG_ERROR;
        msg.arg1 = RECOVERY_CPY_FAILED;
        mHandler.sendMessage(msg);
    }
}
