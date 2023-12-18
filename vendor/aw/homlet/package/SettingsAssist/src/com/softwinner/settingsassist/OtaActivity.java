package com.softwinner.settingsassist;
import java.io.BufferedReader;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.security.GeneralSecurityException;
import java.text.SimpleDateFormat;
import java.util.Date;

import java.util.regex.Matcher;
import java.util.regex.Pattern;
import java.util.Scanner;


import android.os.Bundle;
import android.annotation.SuppressLint;
import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.util.Log;
import android.view.View;
import android.view.WindowManager;
import android.widget.EditText;
import android.widget.Button;
import android.widget.TextView;
import android.widget.Toast;
import android.os.Handler;
import android.os.Message;
import android.os.RecoverySystem;
import android.preference.PreferenceManager;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.SharedPreferences;

import android.os.AsyncTask;
import android.app.ProgressDialog;

public class OtaActivity extends Activity implements
RecoverySystem.ProgressListener {
    // GUI
    private Dialog alertDialog;
    private ProgressDialog mProgressDialog;
    Context mContext;
    // constant
    private static final int RECOVERY_PROGRESS = 1;

    private static final String TAG = "RepeatOTA";
    @SuppressLint("SdCardPath")
    private static final String PACKAGE_PATH = "/cache/update.zip";
    @SuppressLint("SdCardPath")
    private static final String LAST_INSTALL_LOG_PATH = "/cache/recovery/last_install";

    private static final String USB_PACKAGE_PATH = "/mnt/media_rw/([^\\s]+)";
    private static String REAL_USB_PACKAGE_PATH = null;
    public String updateUSBPath;//U盘中的update.zip路径
    int total_length = 0;//update.zip文件长度

    File updateZipF = null;//从U盘拷贝到flash的升级包
    File updateZipU = null;//u盘中的升级包

    class MyTask extends AsyncTask<Integer, Integer, String>{
        @Override
        protected String doInBackground(Integer... params){
            Log.e(TAG, "start do in back ground");
            int proLen = 100;
            InputStream is = null;
            OutputStream os = null;

            updateZipF = new File(PACKAGE_PATH);
            if(updateZipF.exists()) {
                updateZipF.delete();
            }

            updateZipU = new File(updateUSBPath);
            if(updateZipU != null){
                if(updateZipU.exists() && updateZipU.canRead()){
                    try{
                        is = new FileInputStream(updateZipU);
                        os = new FileOutputStream(updateZipF);
                        byte[] buffer = new byte[1024];
                        total_length = is.available();
                        int length = 0;
                        int sum = 0;
                        while((length = is.read(buffer)) > 0){
                            sum = sum + length;
                            os.write(buffer, 0 ,length);
                            if((sum / 1024 % 5) == 0){
                                float percent = (float)(sum)/total_length;
                                int count = (int)(percent * proLen);
                                publishProgress(count);
                            }
                        }
                    }
                    catch(IOException e){
                        e.printStackTrace();
                    }
                    finally{
                        try{
                            if(is != null){
                                is.close();
                            }
                            if(os != null){
                                os.close();
                            }
                        }
                        catch(IOException e){
                            e.printStackTrace();
                        }
                    }
                }
            }
            else{
                Log.e(TAG, "the updateU is null");
            }
            return "Task compleged";
        }
        @Override
        protected void onPostExecute(String result){
            if(!updateZipF.exists() || !updateZipF.canRead()) {
                Dialog alertDialog = new AlertDialog.Builder(OtaActivity.this)
                    .setTitle(R.string.ota_error_title)
                    .setMessage(R.string.ota_error)
                    .setPositiveButton(
                            R.string.ota_accept,
                            new DialogInterface.OnClickListener() {
                                @Override
                                public void onClick(DialogInterface dialog,
                                    int which) {
                                }
                            }).create();
                alertDialog.show();
                mProgressDialog.dismiss();
                return;
            }

            Log.i(TAG, "/cache/update.zip ret = " + updateZipF.exists() + " ,can read? = " + updateZipF.canRead());
            Log.i(TAG, "USB update.zip ret = " + updateZipU.exists() + " ,can read? = " + updateZipU.canRead());

            //mProgressDialog.setProgress(100);
            mProgressDialog.dismiss();
            if(updateZipF.length() == total_length) {
                //拷贝文件成功，进行ota升级
                startOTAUpgrade();
            }else {
                Toast.makeText(mContext, "升级包update.zip拷贝失败!", Toast.LENGTH_LONG).show();
                finish();
            }
        }
        @Override
        protected void onPreExecute(){
            //初始化拷贝update.zip对话框
            mProgressDialog = new ProgressDialog(mContext);
            mProgressDialog.setTitle("OTA升级");
            mProgressDialog.setMessage("正在拷贝升级文件");
            mProgressDialog.setCancelable(false);
            mProgressDialog.setMax(100);
            mProgressDialog.setProgressStyle(ProgressDialog.STYLE_HORIZONTAL);
            mProgressDialog.show();
        }
        @Override
        protected void onProgressUpdate(Integer... values){
            mProgressDialog.setProgress(values[0]);
        }
    }

    /*
     * Start OTA procedure
     */
    private boolean startOTAUpgrade() {

        File OTAPackage = null;
        OTAPackage = new File(PACKAGE_PATH);
        if (OTAPackage.exists()) {
            try {
                Log.e(TAG, "start to verifyPackage and reboot recovery!!!");
                RecoverySystem.verifyPackage(OTAPackage, this, null);
                RecoverySystem.installPackage(this, OTAPackage);
            } catch (IOException e) {
                // TODO Auto-generated catch block
                e.printStackTrace();
                return false;
            } catch (GeneralSecurityException e) {
                // TODO Auto-generated catch block
                e.printStackTrace();
                return false;
            }
            return true;
        } else {
            Log.e(TAG, "OTA package does not exist");
            return false;
        }
    }


    @SuppressLint("HandlerLeak")
    private Handler myHandler = new Handler() {

        public void handleMessage(Message msg) {
            switch (msg.what) {
                //handle message
            }
            super.handleMessage(msg);
        }
    };

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        mContext = this;
        Intent intent = getIntent();
        updateUSBPath = intent.getStringExtra("updateUSBPath");
        new MyTask().execute(100);
    }

    @Override
    public void onProgress(int progress) {
        Message message = new Message();
        message.what = RECOVERY_PROGRESS;
        message.arg1 = progress;
        myHandler.sendMessage(message);
    }
}
