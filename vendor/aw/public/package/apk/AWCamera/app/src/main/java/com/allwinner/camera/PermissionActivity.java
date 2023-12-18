package com.allwinner.camera;

import android.Manifest;
import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.PackageManager;
import android.content.res.Configuration;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.os.Environment;
import android.provider.MediaStore;
import android.support.annotation.RequiresApi;
import android.util.Log;
import android.view.KeyEvent;


import com.allwinner.camera.data.Contants;
import com.allwinner.camera.utils.SharedPreferencesUtils;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;

public class PermissionActivity extends Activity {

    private static final String TAG = "PermissionsActivity";

    private static int PERMISSION_REQUEST_CODE = 1;
    private static int RESULT_CODE_OK = 1;
    private static int RESULT_CODE_FAILED = 2;

    private int mIndexPermissionRequestCamera;
    private int mIndexPermissionRequestMicrophone;
    private int mIndexPermissionRequestLocation;
    private int mIndexPermissionRequestStorage;
    private int mIndexPermissionRequestWriteStorage;
    private boolean mShouldRequestCameraPermission;
    private boolean mShouldRequestMicrophonePermission;
    private boolean mShouldRequestLocationPermission;
    private boolean mShouldRequestFineLocationPermission;
    private boolean mShouldRequestStoragePermission;
    private boolean mShouldRequestWriteStoragePermission;
    private int mNumPermissionsToRequest;
    private boolean mFlagHasCameraPermission;
    private boolean mFlagHasMicrophonePermission;
    private boolean mFlagHasStoragePermission = true;
    private boolean mFlagHasStorageWritePermission = true;
    private boolean mForAddPermission = false;

    static {
//        System.loadLibrary("opencv_java3");
       // System.loadLibrary("jni_panoramiclib");
    }
    public final static String LOCATION_PERMISSION = "local_permission";
    public final static String LOCATION_PERMISSION_RESULT = "local_permission_result";
    public final static String LOCATION_PERMISSION_REQUESST  = "location_permission_request";
    public static int RESULT_CODE = 110;

    private AlertDialog mDialog;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        Log.e(TAG, "onCreate");
        setContentView(R.layout.permissions);
        Configuration config = getResources().getConfiguration();
        int smallestScreenWidth = config.smallestScreenWidthDp;
        Log.e(TAG, "smallestScreenWidth:" + smallestScreenWidth);
        Intent intent = getIntent();
        if (intent != null) {
            boolean getLocationPermission = intent.getBooleanExtra(LOCATION_PERMISSION, false);
            if (getLocationPermission) {
                mForAddPermission = true;
            }
        }
    }

    @RequiresApi(api = Build.VERSION_CODES.M)
    @Override
    protected void onResume() {
        super.onResume();
        mNumPermissionsToRequest = 0;
        Log.e(TAG, "onResume");
        checkPermissions();
    }

    @Override
    protected void onDestroy() {
        if(mDialog != null) {
            mDialog.dismiss();
        }
        super.onDestroy();
        Log.v(TAG, "onDestroy: unregistering receivers");

        //  unregisterReceiver(mShutdownReceiver);
    }

    @RequiresApi(api = Build.VERSION_CODES.M)
    private void checkPermissions() {
        if (checkSelfPermission(Manifest.permission.CAMERA)
                != PackageManager.PERMISSION_GRANTED) {
            mNumPermissionsToRequest++;
            mShouldRequestCameraPermission = true;
        } else {
            mFlagHasCameraPermission = true;
        }

        if (checkSelfPermission(Manifest.permission.RECORD_AUDIO)
                != PackageManager.PERMISSION_GRANTED) {
            mNumPermissionsToRequest++;
            mShouldRequestMicrophonePermission = true;
        } else {
            mFlagHasMicrophonePermission = true;
        }
/*
        if (checkSelfPermission(Manifest.permission.READ_EXTERNAL_STORAGE)
                != PackageManager.PERMISSION_GRANTED) {
            mNumPermissionsToRequest++;
            mShouldRequestStoragePermission = true;
        } else {
            mFlagHasStoragePermission = true;
        }
        if (checkSelfPermission(Manifest.permission.WRITE_EXTERNAL_STORAGE)
                != PackageManager.PERMISSION_GRANTED) {
            mNumPermissionsToRequest++;
            mShouldRequestWriteStoragePermission = true;
        } else {
            mFlagHasStorageWritePermission = true;
        }
*/
        if (checkSelfPermission(Manifest.permission.ACCESS_COARSE_LOCATION)
                != PackageManager.PERMISSION_GRANTED) {
            mNumPermissionsToRequest++;
            mShouldRequestLocationPermission = true;
        }
        if (checkSelfPermission(Manifest.permission.ACCESS_FINE_LOCATION)
                != PackageManager.PERMISSION_GRANTED) {
            mNumPermissionsToRequest++;
            mShouldRequestFineLocationPermission = true;
        }
        /*
        if (checkSelfPermission(Manifest.permission.WRITE_MEDIA_STORAGE)
                != PackageManager.PERMISSION_GRANTED) {
            mNumPermissionsToRequest++;
            mShouldRequestStoragePermission = true;
        } else {
            mFlagHasStoragePermission = true;
        }
        if (checkSelfPermission(Manifest.permission.READ_MEDIA_STORAGE)
                != PackageManager.PERMISSION_GRANTED) {
            mNumPermissionsToRequest++;
            mShouldRequestStoragePermission = true;
        } else {
            mFlagHasStoragePermission = true;
        }*/
        if (mNumPermissionsToRequest != 0) {
            Boolean hasSeenPermissionsDialogs = (Boolean) SharedPreferencesUtils.getParam(getApplicationContext(), Contants.KEY_HAS_SEEN_PERMISSIONS_DIALOGS, false);
            Boolean hasLocationReject = (Boolean) SharedPreferencesUtils.getParam(this, LOCATION_PERMISSION_REQUESST, false);
            if (!hasSeenPermissionsDialogs || !hasLocationReject) {
                buildPermissionsRequest();
            } else {
                // Permissions dialog has already been shown,  and we're still missing permissions.
                handlePermissionsFailure();
            }
        } else {
            handlePermissionsSuccess();
        }
    }

    @RequiresApi(api = Build.VERSION_CODES.M)
    private void buildPermissionsRequest() {
        String[] permissionsToRequest = new String[mNumPermissionsToRequest];
        int permissionsRequestIndex = 0;

        if (mShouldRequestCameraPermission) {
            permissionsToRequest[permissionsRequestIndex] = Manifest.permission.CAMERA;
            mIndexPermissionRequestCamera = permissionsRequestIndex;
            permissionsRequestIndex++;
        }
        if (mShouldRequestMicrophonePermission) {
            permissionsToRequest[permissionsRequestIndex] = Manifest.permission.RECORD_AUDIO;
            mIndexPermissionRequestMicrophone = permissionsRequestIndex;
            permissionsRequestIndex++;
        }
        if (mShouldRequestStoragePermission) {
            permissionsToRequest[permissionsRequestIndex] = Manifest.permission.READ_EXTERNAL_STORAGE;
            mIndexPermissionRequestStorage = permissionsRequestIndex;
            permissionsRequestIndex++;
        }
        if (mShouldRequestWriteStoragePermission) {
            permissionsToRequest[permissionsRequestIndex] = Manifest.permission.WRITE_EXTERNAL_STORAGE;
            mIndexPermissionRequestWriteStorage = permissionsRequestIndex;
            permissionsRequestIndex++;
        }
        if (mShouldRequestLocationPermission) {
            permissionsToRequest[permissionsRequestIndex] = Manifest.permission.ACCESS_COARSE_LOCATION;
            mIndexPermissionRequestLocation = permissionsRequestIndex;
        }
        if (mShouldRequestFineLocationPermission) {
            permissionsToRequest[permissionsRequestIndex] = Manifest.permission.ACCESS_FINE_LOCATION;
            mIndexPermissionRequestLocation = permissionsRequestIndex;
        }
        Log.v(TAG, "requestPermissions count: " + permissionsToRequest.length);
        requestPermissions(permissionsToRequest, PERMISSION_REQUEST_CODE);
    }

    @Override
    public void onRequestPermissionsResult(int requestCode,
                                           String permissions[], int[] grantResults) {
        Log.v(TAG, "onPermissionsResult counts: " + permissions.length + ":" + grantResults.length);
        /*mSettingsManager.set(
                SettingsManager.SCOPE_GLOBAL,
                Keys.KEY_HAS_SEEN_PERMISSIONS_DIALOGS,
                true);*/
        SharedPreferencesUtils.setParam(getApplicationContext(), Contants.KEY_HAS_SEEN_PERMISSIONS_DIALOGS, true);
        if (mShouldRequestCameraPermission) {
            if (grantResults.length > 0 && grantResults[mIndexPermissionRequestCamera] ==
                    PackageManager.PERMISSION_GRANTED) {
                mFlagHasCameraPermission = true;
            } else {
                handlePermissionsFailure();
            }
        }
        if (mShouldRequestMicrophonePermission) {
            if (grantResults.length > 0 && grantResults[mIndexPermissionRequestMicrophone] ==
                    PackageManager.PERMISSION_GRANTED) {
                mFlagHasMicrophonePermission = true;
            } else {
                handlePermissionsFailure();
            }
        }
        if (mShouldRequestStoragePermission) {
            if (grantResults.length > 0 && grantResults[mIndexPermissionRequestStorage] ==
                    PackageManager.PERMISSION_GRANTED) {
                mFlagHasStoragePermission = true;
            } else {
                handlePermissionsFailure();
            }
        }
        if (mShouldRequestWriteStoragePermission) {
            if (grantResults.length > 0 && grantResults[mIndexPermissionRequestWriteStorage] ==
                    PackageManager.PERMISSION_GRANTED) {
                mFlagHasStorageWritePermission = true;
            } else {
                handlePermissionsFailure();
            }
        }

        if (mShouldRequestLocationPermission) {
            if (grantResults.length > 0 && grantResults[mIndexPermissionRequestLocation] ==
                    PackageManager.PERMISSION_GRANTED) {
                // Do nothing
            } else {
                SharedPreferencesUtils.setParam(this, LOCATION_PERMISSION_REQUESST, true);
            }
        }

        if (mFlagHasCameraPermission && mFlagHasMicrophonePermission && mFlagHasStoragePermission && mFlagHasStorageWritePermission) {
            handlePermissionsSuccess();
        }
    }

    private void handlePermissionsSuccess() {
        //InitModelFiles();
        if (!mForAddPermission) {
            Intent intent = new Intent(this, CameraActivity.class);
            String action =  getIntent().getAction();
            if(action!= null) {
                intent.setAction(action);
            }
            if(getIntent().getExtras()!= null) {
                intent.putExtras(getIntent().getExtras());
            }
            startActivity(intent);
            overridePendingTransition(0, 0);
        }
        finish();
    }

    private void handlePermissionsFailure() {
       mDialog =  new AlertDialog.Builder(this).setTitle(getResources().getString(R.string.camera_error_title))
                .setMessage(getResources().getString(R.string.error_permissions))
                .setCancelable(false)
                .setOnKeyListener(new Dialog.OnKeyListener() {
                    @Override
                    public boolean onKey(DialogInterface dialog, int keyCode, KeyEvent event) {
                        if (keyCode == KeyEvent.KEYCODE_BACK) {
                            finish();
                        }
                        return true;
                    }
                })
                .setPositiveButton(getResources().getString(R.string.dialog_dismiss),
                        new DialogInterface.OnClickListener() {
                            @Override
                            public void onClick(DialogInterface dialog, int which) {
                                finish();
                            }
                        })
                .show();
    }
}
