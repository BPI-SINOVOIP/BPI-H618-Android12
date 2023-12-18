package com.allwinnertech.socs;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

import org.json.JSONException;
import org.json.JSONObject;
import org.json.JSONStringer;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.ProgressDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.net.Uri;
import android.os.AsyncTask;
import android.os.Build;
import android.os.Bundle;
import android.provider.DocumentsContract;
import android.util.JsonReader;
import android.util.Log;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.Switch;
import android.widget.CompoundButton;
import android.widget.CompoundButton.OnCheckedChangeListener;
import android.widget.RadioButton;
import android.widget.TextView;
import android.Manifest.permission;
import android.content.pm.PackageManager;

import java.io.File;

public class ConfigActivity extends Activity {

    private String TAG = "ConfigActivity";
    private static final int READ_TFCARD_REQUEST_CODE = 38;
    private static final int READ_USBHOST_REQUEST_CODE = 42;
    private static final int READ_MAPDATA_REQUEST_CODE = 48;
    private static final int MY_PERMISSIONS_REQUEST = 42;

    private static final String LOCALMAPDATAPATH = "/storage/emulated/0";

    Context mContext;
    Switch mEnableDefConfig;
    Switch mAutoTestConfig;
    View mSelfConfig;
    Button mTfcard;
    Button mUsbhost;
    Switch mEnableEthtest;
    Switch mEnableCameraTest;
    Switch mEnableBluetoothTest;
    Switch mEnableWifiTest;
    Switch mEnableAutoMapTest;
    Switch mEnableCpuid;
    Switch mEnableGPSTest;
	Switch mEnableGPUTest;
    Button mSave;
    TextView mtfcard_video_path ;
    TextView musbhost_video_path;
    TestsConfig testconfg;
    Button mMapData;
    private String configFilePath;

    private String[] permissions = new String[] {
        permission.CAMERA,
        permission.READ_EXTERNAL_STORAGE,
        permission.ACCESS_COARSE_LOCATION,
        permission.ACCESS_FINE_LOCATION
    };

    List<String> mPermissionsList = new ArrayList<>();

    private boolean checkMapData() {
        File path = new File(LOCALMAPDATAPATH + "/amapauto8/data/map");
        if (path.exists()) {
            String[] list = path.list();
            if (list.length > 0)
                return true;
        }
        return false;
    }

    @Override
    public void onActivityResult(int requestCode, int resultCode,
            Intent resultData) {
        if (resultCode == Activity.RESULT_OK) {
            Uri uri = null;
            if (resultData != null) {
                uri = resultData.getData();
            }
            DocUtils du = new DocUtils();
            String realpath;
            switch(requestCode){
            case READ_TFCARD_REQUEST_CODE:
                Log.i(TAG, "tfcard Uri: " + uri.toString());
                realpath = du.getPath(mContext, uri);
                testconfg.tfcard_video = realpath;
                mtfcard_video_path.setText(testconfg.tfcard_video);
                break;
            case READ_USBHOST_REQUEST_CODE:
                Log.i(TAG, "usbhost Uri: " + uri.toString());
                realpath = du.getPath(mContext, uri);
                testconfg.usbhost_video = realpath;
                musbhost_video_path.setText(testconfg.usbhost_video);
                break;
            case READ_MAPDATA_REQUEST_CODE:
                Log.i(TAG, "Map data uri :" + uri.toString());
                realpath = du.getPath(mContext, uri);
                new AsyncTask<String, Void, Void>() {
                    private ProgressDialog preDialog;
                    @Override
                    protected void onPreExecute() {
                        preDialog = new ProgressDialog(mContext);
                        preDialog.setProgressStyle(ProgressDialog.STYLE_SPINNER);
                        preDialog.setTitle("Copy Map data");
                        preDialog.setMessage("copy map data to local storage");
                        preDialog.setIndeterminate(false);
                        preDialog.setCancelable(false);
                        preDialog.show();
                    }

                    @Override
                    protected Void doInBackground(String... params) {
                        SysIntf.runRootCmd("rm " + LOCALMAPDATAPATH + "/amapauto8 -rf");
                        SysIntf.runRootCmd("cp " + params[0] + " " + LOCALMAPDATAPATH + " -rf");
                        return null;
                    }

                    @Override
                    protected void onProgressUpdate(Void... progresses) {
                        // TODO
                    }

                    @Override
                    protected void onPostExecute(Void result) {
                        preDialog.dismiss();
                        if (checkMapData()) {
                            mMapData.setVisibility(View.GONE);
                        }
                    }
                }.execute(realpath);
                break;
            }
        }
    }

    private boolean checkPermissions() {
        mPermissionsList.clear();
        for (int i = 0; i < permissions.length; i++) {
            if (checkSelfPermission(permissions[i]) != PackageManager.PERMISSION_GRANTED)
                mPermissionsList.add(permissions[i]);
        }
        if (mPermissionsList.isEmpty())
            return true;
        return false;
    }

    private void requestAllPermissions() {
        String[] requests = mPermissionsList.toArray(new String[mPermissionsList.size()]);
        requestPermissions(requests, MY_PERMISSIONS_REQUEST);
    }

    @Override
    public void onRequestPermissionsResult(
            int requestCode, String permissions[], int[] grantResults) {
        switch (requestCode) {
            case MY_PERMISSIONS_REQUEST:
            {
                if (checkPermissions())
                    initConfig();
                else
                    requestAllPermissions();
                break;
            }
        }
    }


    private void initConfig(){
        setContentView(R.layout.activity_choose_config);
        mContext = this;
        configFilePath = mContext.getFilesDir() + "/config.json";
        testconfg = new TestsConfig();
        if (testconfg.checkConfigFileExist(configFilePath))
            testconfg.loadAllConfigFromFile(configFilePath);
        mEnableDefConfig = (Switch) this.findViewById(R.id.enable_default);
        mAutoTestConfig = (Switch) this.findViewById(R.id.auto_test_default);
        mSelfConfig = this.findViewById(R.id.self_config);
        if (testconfg.getHideConfig()) {
            mSelfConfig.setVisibility(View.INVISIBLE);
        } else {
            mSelfConfig.setVisibility(View.VISIBLE);
        }

        if (testconfg.getAutoTest()) {
            mAutoTestConfig.setChecked(true);
        } else {
            mAutoTestConfig.setChecked(false);
        }
        mTfcard = (Button) this.findViewById(R.id.tfcard);
        mTfcard.setOnClickListener(new OnClickListener(){

            @Override
            public void onClick(View arg0) {
                Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT);
                intent.addCategory(Intent.CATEGORY_OPENABLE);
                intent.setType("video/*");
                startActivityForResult(intent, READ_TFCARD_REQUEST_CODE);
            }

        });
        mtfcard_video_path = (TextView) this.findViewById(R.id.tfcard_video_path);
        mtfcard_video_path.setText(testconfg.tfcard_video);
        musbhost_video_path = (TextView) this.findViewById(R.id.usbhost_video_path);
        musbhost_video_path.setText(testconfg.usbhost_video);
        mUsbhost = (Button) this.findViewById(R.id.usbhost);
        mUsbhost.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View arg0) {
                Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT);
                intent.addCategory(Intent.CATEGORY_OPENABLE);
                intent.setType("video/*");
                startActivityForResult(intent, READ_USBHOST_REQUEST_CODE);
            }

        });
        mEnableEthtest = (Switch) this.findViewById(R.id.enable_ethtest);
        mEnableEthtest.setChecked(testconfg.enable_ethernet_test);
        mEnableEthtest.setOnCheckedChangeListener(new OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(CompoundButton arg0, boolean arg1) {
                if (arg1) {
                    testconfg.enable_ethernet_test = true;
                } else {
                    testconfg.enable_ethernet_test = false;
                }
            }

        });
        mEnableCameraTest = (Switch) this.findViewById(R.id.enable_cameratest);
        mEnableCameraTest.setChecked(testconfg.enable_camera);
        mEnableCameraTest.setOnCheckedChangeListener(new OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(CompoundButton arg0, boolean arg1) {
                if (arg1) {
                    testconfg.enable_camera = true;
                } else {
                    testconfg.enable_camera = false;
                }
            }

        });

        mEnableBluetoothTest = (Switch) this.findViewById(R.id.enable_bluetoothtest);
        mEnableBluetoothTest.setChecked(testconfg.enable_bluetooth);
        mEnableBluetoothTest.setOnCheckedChangeListener(new OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(CompoundButton arg0, boolean arg1) {
                if (arg1) {
                    testconfg.enable_bluetooth = true;
                } else {
                    testconfg.enable_bluetooth = false;
                }
            }

        });

        mEnableWifiTest = (Switch) this.findViewById(R.id.enable_wifitest);
        mEnableWifiTest.setChecked(testconfg.enable_wifi);
        mEnableWifiTest.setOnCheckedChangeListener(new OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(CompoundButton arg0, boolean arg1) {
                if (arg1) {
                    testconfg.enable_wifi = true;
                } else {
                    testconfg.enable_wifi = false;
                }
            }

        });

        mEnableGPSTest = (Switch) this.findViewById(R.id.enable_gpstest);
        mEnableGPSTest.setChecked(testconfg.enable_gps);
        mEnableGPSTest.setOnCheckedChangeListener(new OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(CompoundButton arg0, boolean arg1) {
                if (arg1) {
                    testconfg.enable_gps = true;
                } else {
                    testconfg.enable_gps = false;
                }
            }

        });

        mEnableGPUTest = (Switch) this.findViewById(R.id.enable_gputest);
        mEnableGPUTest.setChecked(testconfg.enable_gpu);
        mEnableGPUTest.setOnCheckedChangeListener(new OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(CompoundButton arg0, boolean arg1) {
                if (arg1) {
                    testconfg.enable_gpu = true;
                } else {
                    testconfg.enable_gpu = false;
                }
            }

        });

        mMapData = (Button) this.findViewById(R.id.automapdata);
        mMapData.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View arg0) {
                Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT_TREE);
                startActivityForResult(intent, READ_MAPDATA_REQUEST_CODE);
            }
        });

        mEnableAutoMapTest = (Switch) this.findViewById(R.id.enable_automaptest);
        if (!checkMapData()) {
            testconfg.enable_automap = false;
        } else {
            mMapData.setVisibility(View.GONE);
        }
        mEnableAutoMapTest.setChecked(testconfg.enable_automap);
        if (!mEnableAutoMapTest.isChecked()) {
            mMapData.setEnabled(false);
        }
        mEnableAutoMapTest.setOnCheckedChangeListener(new OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(CompoundButton arg0, boolean checked) {
                mMapData.setEnabled(checked);
                testconfg.enable_automap = checked;
            }
        });

        mEnableCpuid = (Switch) this.findViewById(R.id.enable_cpuid);
        mEnableCpuid.setChecked(testconfg.enable_cpuid_test);
        mEnableCpuid.setOnCheckedChangeListener(new OnCheckedChangeListener() {

            @Override
            public void onCheckedChanged(CompoundButton arg0, boolean arg1) {
                if (arg1) {
                    testconfg.enable_cpuid_test = true;
                } else {
                    testconfg.enable_cpuid_test =false;
                }
            }

        });
        mSave = (Button) this.findViewById(R.id.save);
        mSave.setOnClickListener(new OnClickListener() {

            @Override
            public void onClick(View arg0) {
                Log.d(TAG,"save config to file");
                if (mEnableAutoMapTest.isChecked() && !checkMapData()) {
                    AlertDialog.Builder builder = new AlertDialog.Builder(mContext);
                    builder.setTitle(R.string.MapConfig);
                    builder.setIcon(R.drawable.config);
                    builder.setMessage(R.string.MapMessage);
                    builder.setPositiveButton("确定", new DialogInterface.OnClickListener() {
                        @Override
                        public void onClick(DialogInterface dialog, int which) {
                            mEnableAutoMapTest.setChecked(false);
                            testconfg.enable_automap = false;
                        }
                    });
                    builder.create().show();
                    return;
                }
                testconfg.saveAllConfigToFile(configFilePath);
                finish();
            }

        });
        mEnableDefConfig.setOnCheckedChangeListener(new OnCheckedChangeListener(){

            @Override
            public void onCheckedChanged(CompoundButton arg0, boolean arg1) {

                if (arg1) {
                    testconfg.hide_config = true;
                    mSelfConfig.setVisibility(View.INVISIBLE);
                } else {
                    testconfg.hide_config = false;
                    mSelfConfig.setVisibility(View.VISIBLE);
                }
            }

        });
        mAutoTestConfig.setOnCheckedChangeListener(new OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(CompoundButton arg0, boolean arg1) {
                if (arg1) {
                    testconfg.setAutoTest(true);
                } else {
                    testconfg.setAutoTest(false);
                }
            }
        });
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        if (checkPermissions())
            initConfig();
        else
            requestAllPermissions();
    }
}
