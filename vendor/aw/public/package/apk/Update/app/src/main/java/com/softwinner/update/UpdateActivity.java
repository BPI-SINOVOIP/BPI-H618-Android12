package com.softwinner.update;

import android.app.Activity;
import android.app.Service;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.content.res.Configuration;
import android.content.res.Resources;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.os.Build;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.os.IBinder;
import android.os.Message;
import android.util.Log;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.view.animation.Animation;
import android.view.animation.AnimationUtils;
import android.view.animation.TranslateAnimation;
import android.widget.ImageButton;
import android.widget.ImageView;
import android.widget.RelativeLayout;
import android.widget.TextView;
import android.widget.Toast;

import com.bigbigcloud.devicehive.api.BbcDevice;
import com.bigbigcloud.devicehive.api.IUiCallback;
import com.bigbigcloud.devicehive.entity.Device;
import com.bigbigcloud.devicehive.entity.UpdateInfoResponse;
import com.bigbigcloud.devicehive.json.GsonFactory;
import com.bigbigcloud.devicehive.service.BbcDeviceService;
import com.bigbigcloud.devicehive.service.Property;
import com.google.gson.JsonObject;
import com.softwinner.shared.CircleRotate;
import com.softwinner.shared.FileSearch;
import com.softwinner.shared.StorageHelper;

import java.io.File;
import java.io.FilenameFilter;
import java.text.SimpleDateFormat;
import java.util.Arrays;
import java.util.List;

public class UpdateActivity extends Activity implements OnClickListener {
    /**
     * Called when the activity is first created.
     */
    private String TAG = "UpdateActivity";

    public static final String ACTION_CHECK = "com.softwinner.update.ACTION_CHECK";

    /**
     * private ServiceConnection mCheckConn = new ServiceConnection() {
     *
     * @Override public void onServiceConnected(ComponentName name, IBinder service) {
     * mServiceBinder = (CheckBinder) service;
     * }
     * @Override public void onServiceDisconnected(ComponentName name) {
     * <p>
     * }
     * };
     **/

    private RelativeLayout onlineBtn, localeBtn, btnLayout, checkLayout, rootLayout;
    private ImageView newVerImg;
    private ImageButton checkBtn;
    private TextView localTv, onlineTv, currentVerTv, lastTimeTv;
    private int zipNum;

    private Preferences mPreference;
    private boolean isBind = false, isShow = false;
    private Animation showAnimation, hideAnimation;
    private int checkbtnX, checkbtnY, checkbtnW, checkbtnH;
    private CircleRotate mCircleRotate = null;

    private Configuration mConfiguration;
    private Resources res;
    private boolean hasChecked = false;

    private BbcDevice bbcDevice;
    private Device device;
    private Property property;
    private BbcDeviceService.ServiceBinder serviceBinder;
    private ServiceConnection connection = new ServiceConnection() {
        @Override
        public void onServiceConnected(ComponentName name, IBinder service) {
            Log.e(TAG, "onServiceConnected !!!");
            serviceBinder = (BbcDeviceService.ServiceBinder) service;
            if (serviceBinder.getBbcDevice() == null) {
                Log.e(TAG, " onServiceConnected and bbcDevice is null");
            }
        }

        @Override
        public void onServiceDisconnected(ComponentName name) {
            Log.e(TAG, "onServiceDisconnected !!!");
        }
    };
    private UpdateInfoResponse updateInfoResponse;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.main_activity);

        rootLayout = (RelativeLayout) findViewById(R.id.root);
        onlineBtn = (RelativeLayout) findViewById(R.id.update_online);
        localeBtn = (RelativeLayout) findViewById(R.id.update_local);
        btnLayout = (RelativeLayout) findViewById(R.id.menu_field);
        checkLayout = (RelativeLayout) findViewById(R.id.check_layout);
        checkBtn = (ImageButton) findViewById(R.id.checkout_btn);
        newVerImg = (ImageView) findViewById(R.id.new_img);
        localTv = (TextView) findViewById(R.id.locale_txt);
        onlineTv = (TextView) findViewById(R.id.version_txt);
        currentVerTv = (TextView) findViewById(R.id.current_version_tv);
        lastTimeTv = (TextView) findViewById(R.id.last_time_tv);
        onlineBtn.setOnClickListener(this);
        localeBtn.setOnClickListener(this);
        checkBtn.setOnClickListener(this);
        onlineBtn.setClickable(false);
        localeBtn.setClickable(false);
        newVerImg.setVisibility(View.INVISIBLE);

//        mCheckThread = new QueryCheck();
        mPreference = new Preferences(this);

        long time = Long.valueOf(Build.TIME);
        SimpleDateFormat formatter = new SimpleDateFormat("yyyy/MM/dd");

        showAnimation = AnimationUtils.loadAnimation(this, R.anim.main_button_show);
        hideAnimation = AnimationUtils.loadAnimation(this, R.anim.main_button_exit);

        res = getResources();
        hasChecked = false;
        bindService(new Intent(this, OtaService.class), connection, Service.BIND_AUTO_CREATE);
        property = new Property(this);
        device = UpdateDevice.getInstance(this);

        currentVerTv.setText(getResources().getString(R.string.current_version) + "  " + device.getFirmwareVersion());
        lastTimeTv.setText(getResources().getString(R.string.last_update_time) + "  " + formatter.format(time));
    }

    @Override
    public void onConfigurationChanged(Configuration newConfig) {
        if (hasChecked) {
            if (newConfig.orientation == Configuration.ORIENTATION_LANDSCAPE) {
                RelativeLayout.LayoutParams params = new RelativeLayout.LayoutParams(
                        checkLayout.getLayoutParams());
                int top = res.getInteger(R.integer.check_button_top_with_checked_land);
                params.setMargins(params.leftMargin, top, params.rightMargin, top + params.height);
                checkLayout.setLayoutParams(params);

                params = (RelativeLayout.LayoutParams) btnLayout.getLayoutParams();
                int height = res.getInteger(R.integer.menu_button_height_land);
                params.height = height;
                btnLayout.setLayoutParams(params);
            } else {
                RelativeLayout.LayoutParams params = new RelativeLayout.LayoutParams(
                        checkLayout.getLayoutParams());
                int top = res.getInteger(R.integer.check_button_top_with_checked_port);
                params.setMargins(params.leftMargin, top, params.rightMargin, top + params.height);
                checkLayout.setLayoutParams(params);

                params = (RelativeLayout.LayoutParams) btnLayout.getLayoutParams();
                int height = res.getInteger(R.integer.menu_button_height_port);
                params.height = height;
                btnLayout.setLayoutParams(params);
            }
        } else {
            onCreate(null);
        }
        super.onConfigurationChanged(newConfig);
    }

    @Override
    protected void onResume() {
        super.onResume();
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        /**
         if(data != null){
         Bundle bundle = data.getExtras();
         String file = bundle.getString("file");
         if (file != null) {
         final Dialog dlg = new Dialog(this);
         dlg.setTitle(R.string.confirm_update);
         LayoutInflater inflater = LayoutInflater.from(this);
         InstallPackage dlgView = (InstallPackage) inflater.inflate(R.layout.install_ota, null,
         false);
         dlgView.setPackagePath(file);
         dlg.setContentView(dlgView);
         dlg.findViewById(R.id.confirm_cancel).setOnClickListener(new View.OnClickListener() {
        @Override public void onClick(View v) {
        dlg.dismiss();
        }
        });
         dlg.setCancelable(false);
         dlg.show();
         }
         }
         **/
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        unbindService(connection);
//        if (isBind) {
//            unbindService(mConn);
//        }
    }

    @Override
    public void onClick(View view) {
        if (view == checkBtn) {
            checkbtnX = checkBtn.getLeft();
            checkbtnY = checkBtn.getTop();
            checkbtnW = checkBtn.getWidth();
            checkbtnH = checkBtn.getHeight();
            mCircleRotate = new CircleRotate(UpdateActivity.this);
            mCircleRotate.setPonit(checkbtnX + (float) checkbtnW / 2, checkbtnY + (float) checkbtnW / 2);
            mCircleRotate.setRadius((float) checkbtnW / 2);
            RelativeLayout.LayoutParams lp1 = new RelativeLayout.LayoutParams
                    (ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT);
            lp1.addRule(RelativeLayout.ALIGN_PARENT_TOP);
            lp1.addRule(RelativeLayout.ALIGN_PARENT_LEFT);
            checkLayout.addView(mCircleRotate, lp1);
            mCircleRotate.spin();
//            if (isBind) {
//                unbindService(mConn);
//                isBind = false;
//            }
            checkBtn.setEnabled(false);
            Thread mThread = new Thread() {
                @Override
                public void run() {
                    FileNameSelector zipSelector = new FileNameSelector("zip");
                    StorageHelper sh = new StorageHelper(UpdateActivity.this);
                    List<String> mps = sh.getMountPoints();
                    zipNum = 0;
                    for (String mp : mps) {
                        File file = new File(mp);
                        File[] files =  file.listFiles(zipSelector);
                        zipNum += files.length;
                    }
//                    File file = new File(Environment.getExternalStorageDirectory().getPath());
//                    File mFiles[] = file.listFiles(new FileNameSelector("zip"));
//                    zipNum = mFiles == null ? 0 : mFiles.length;
                    Message msg = new Message();
                    msg.what = 0;
                    mHandler.sendMessage(msg);
                }
            };
            mThread.start();
            if (!checkInternet()) {
                showMainButton();
                onlineTv.setText(R.string.net_error);
                Toast.makeText(UpdateActivity.this, UpdateActivity.this.getString(R.string.net_error), Toast.LENGTH_SHORT).show();
                return;
            }
//            Intent mIntent = new Intent(UpdateActivity.this, UpdateService.class);
//            bindService(mIntent, mConn, Service.BIND_AUTO_CREATE);
//            isBind = true;
//            mCheckThread.start();
            bbcDevice = serviceBinder.getBbcDevice();
            if (bbcDevice != null) {
                Log.e(TAG, "inquireForUpdate= " + property.getDeviceGuid());
                bbcDevice.inquireForUpdate(property.getDeviceGuid(), device, new IUiCallback() {
                    @Override
                    public void onSuccess(JsonObject jsonObject) {
                        Log.e(TAG, "onSuccess: " + jsonObject);
                        if (jsonObject != null && !jsonObject.isJsonNull()) {
                            UpdateInfoResponse infoResponse = GsonFactory.createGson().fromJson(jsonObject, UpdateInfoResponse.class);
                            onUpdateInfo(infoResponse);
                        } else {
                            onUpdateInfo(null);
                        }
                    }

                    @Override
                    public void onError(int i, String s) {
                        Log.e(TAG, "onError: " + i + ", " + s);
                        showMainButton();
                    }

                    @Override
                    public void onCancel() {
                        Log.e(TAG, "onCancel!");
                        showMainButton();
                    }
                });
            }
        } else if (view == onlineBtn) {
            Intent downloadIntent = new Intent(UpdateActivity.this, DownloadPackageActivity.class);
            downloadIntent.putExtra("name", updateInfoResponse.getDetailDesc());
            UpdateActivity.this.startActivity(downloadIntent);
        } else if (view == localeBtn) {
            Intent intent = new Intent(UpdateActivity.this, FileSearch.class);
            UpdateActivity.this.startActivity(intent);
        }

    }

    public void onUpdateInfo(UpdateInfoResponse info) {
        if (info == null) {
            // Toast.makeText(this, " 没查到更新信息 !!", Toast.LENGTH_LONG).show();
            showMainButton();
            return;
        }
        //关于升级的方式的具体处理，开发者自己完成
        switch (info.getPubSetting().getUpdate_type()) {
            case 0:
                Log.d(TAG, " 未定义升级方式！");
                break;
            case 1:
                Log.d(TAG, " 这是静默升级！");
                break;
            case 2:
                Log.d(TAG, "用户确认升级");
                break;
            case 3:
                Log.d(TAG, "强制升级！！");
                break;
        }
        updateInfoResponse = info;
        mPreference.setDownloadTarget(Configs.DOWNLOAD_PATH);
        mPreference.setDownloadURL(info.getPackUrl());
        mPreference.setMd5(info.getPackMD5());
        mPreference.setPackageDescriptor(info.getDetailDesc());
        mPreference.setDownloadSize(info.getPackSize());
        onlineBtn.setClickable(true);
        showMainButton();
        onlineTv.setText(updateInfoResponse.getNewVersion());
        // Toast.makeText(this, "发现新版本，请下载更新包", Toast.LENGTH_LONG).show();
    }

    Handler mHandler = new Handler() {
        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case 0:
                    localTv.setText(String.format(
                            getResources().getString(
                                    R.string.package_number_txt), zipNum));
                    localeBtn.setClickable(true);
                    break;
                default:
                    break;
            }
        }

    };

    public class FileNameSelector implements FilenameFilter {
        String extension = ".";

        public FileNameSelector(String fileExtensionNoDot) {
            extension += fileExtensionNoDot;
        }

        public boolean accept(File dir, String name) {
            return name.endsWith(extension);
        }
    }

    private boolean checkInternet() {
        ConnectivityManager cm = (ConnectivityManager) UpdateActivity.this
                .getSystemService(Context.CONNECTIVITY_SERVICE);
        NetworkInfo info = cm.getActiveNetworkInfo();
        if (info != null && info.isConnected()) {
            return true;
        } else {
            return false;
        }
    }

    private void showMainButton() {
        mCircleRotate.stopSpinning();
        checkLayout.removeView(mCircleRotate);
        if (!isShow) {
            //MarginLayoutParams margin=new MarginLayoutParams(checkLayout.getLayoutParams());
            //margin.setMargins(margin.leftMargin,80, margin.rightMargin, 80+margin.height);
            //RelativeLayout.LayoutParams layoutParams = new RelativeLayout.LayoutParams(margin);
            //checkLayout.setLayoutParams(layoutParams);
            move();
            btnLayout.setVisibility(View.VISIBLE);
            btnLayout.startAnimation(showAnimation);
            isShow = true;
        }
        hasChecked = true;
        checkBtn.setEnabled(true);
        checkBtn.setBackgroundResource(R.drawable.check_succ_button_style);
    }


    private void move() {
        mConfiguration = this.getResources().getConfiguration();
        if (mConfiguration.orientation == Configuration.ORIENTATION_LANDSCAPE) {
            Animation mTranslateAnimation = new TranslateAnimation(0, 0, 0, res.getInteger(R.integer.check_button_anime_y_land));
            mTranslateAnimation.setDuration(300);
            mTranslateAnimation
                    .setAnimationListener(new Animation.AnimationListener() {
                        public void onAnimationStart(Animation animation) {

                        }

                        public void onAnimationEnd(Animation animation) {
                            RelativeLayout.LayoutParams params = new RelativeLayout.LayoutParams(
                                    checkLayout.getLayoutParams());
                            int top = 0;
                            if (mConfiguration.orientation == Configuration.ORIENTATION_LANDSCAPE) {
                                top = res.getInteger(R.integer.check_button_top_with_checked_land);
                            } else {
                                top = res.getInteger(R.integer.check_button_top_with_checked_port);
                            }
                            params.setMargins(params.leftMargin, top, params.rightMargin, top + params.height);
                            checkLayout.clearAnimation();
                            checkLayout.setLayoutParams(params);
                        }

                        public void onAnimationRepeat(Animation animation) {
                        }
                    });
            checkLayout.startAnimation(mTranslateAnimation);
        } else {
            Animation mTranslateAnimation = new TranslateAnimation(0, 0, 0, res.getInteger(R.integer.check_button_anime_y_port));
            mTranslateAnimation.setDuration(300);
            mTranslateAnimation
                    .setAnimationListener(new Animation.AnimationListener() {
                        public void onAnimationStart(Animation animation) {

                        }

                        public void onAnimationEnd(Animation animation) {
                            RelativeLayout.LayoutParams params = new RelativeLayout.LayoutParams(
                                    checkLayout.getLayoutParams());
                            int top = 0;
                            if (mConfiguration.orientation == Configuration.ORIENTATION_LANDSCAPE) {
                                top = res.getInteger(R.integer.check_button_top_with_checked_land);
                            } else {
                                top = res.getInteger(R.integer.check_button_top_with_checked_port);
                            }
                            params.setMargins(params.leftMargin, top, params.rightMargin, top + params.height);
                            checkLayout.clearAnimation();
                            checkLayout.setLayoutParams(params);
                        }

                        public void onAnimationRepeat(Animation animation) {
                        }
                    });
            checkLayout.startAnimation(mTranslateAnimation);
        }
    }
}



