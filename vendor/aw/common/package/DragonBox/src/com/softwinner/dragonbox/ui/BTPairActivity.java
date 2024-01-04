package com.softwinner.dragonbox.ui;

import android.animation.AnimatorInflater;
import android.animation.ObjectAnimator;
import android.animation.ValueAnimator;
import android.app.AlertDialog;
import android.content.Context;
import android.os.Bundle;
import android.util.Log;
import android.view.KeyEvent;
import android.view.View;
import android.view.inputmethod.InputMethodManager;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.TextView;
import com.softwinner.dragonbox.manager.ScanGunManager;
import com.softwinner.dragonbox.jni.ReadPrivateJNI;
import android.widget.Toast;
import com.softwinner.dragonbox.receiver.BluetoothReceiver;
import android.content.IntentFilter;
import android.bluetooth.BluetoothHidHost;
import com.softwinner.dragonbox.platform.BTPairCallback;
import android.app.Activity;
import com.softwinner.dragonbox.DragonBoxApplication;
import android.content.Intent;
import com.softwinner.dragonbox.R;
import com.softwinner.dragonbox.entity.MES_RequestBean;
import com.softwinner.dragonbox.utils.NetUtil;
import com.softwinner.dragonbox.entity.NetReceivedResult;
import com.softwinner.dragonbox.testcase.CaseWifi;
import android.os.Handler;
import java.lang.Thread;
import android.os.Message;
import com.softwinner.dragonbox.utils.PreferenceUtil;

public class BTPairActivity extends Activity {

    private static final String TAG = "DragonBox-BTPairActivity";
    private static final int MES_UPLOAD_SUCCESS = 0;
    private static final int MES_UPLOAD_FAIL = 1;
    private static final int MES_SCAN_SUCCESS = 2;
    private static final int MES_PAIR_SUCCESS = 3;

    private ScanGunManager scanGunManager;
    private ValueAnimator anim;
    private TextView tv_scan_qrcode;
    private EditText et_show_mac;
    private LinearLayout ll_root;
    private ReadPrivateJNI mReadPrivateJNI = DragonBoxApplication.getReadPrivateJNI();
    private BluetoothReceiver mBluetoothReceiver;
    private AlertDialog sendBroadcastDialog;

    private String bluetooth_mac = "";

    private Handler mHandler =new Handler() {
        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case MES_UPLOAD_SUCCESS:
                    Log.d(TAG,"upload mes success!!!");
                    mReadPrivateJNI.nativeSetParameter("btpair", "pass");
                    showDialog(getString(R.string.bluetooth_pair_success_shutdown));
                    break;
                case MES_UPLOAD_FAIL:
                    showDialog(getString(R.string.btpair_upload_server_failed));
                    break;
                case MES_SCAN_SUCCESS:
                    showDialog(getString(R.string.please_press_bluetooth_remote_control_pair));
                    break;
                case MES_PAIR_SUCCESS:
                    showDialog(getString(R.string.pair_success_uploading_data));
                    break;
            }
        }
    };

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_btpair);
        tv_scan_qrcode = findViewById(R.id.tv_scan_qrcode);
        et_show_mac = findViewById(R.id.et_show_mac);
        ll_root = findViewById(R.id.ll_root);

        anim = (ObjectAnimator) AnimatorInflater.loadAnimator(this, R.animator.btpair_text_anim);
        anim.setTarget(tv_scan_qrcode);
        anim.start();

        //拦截扫码器回调,获取扫码内容
        scanGunManager = new ScanGunManager(new ScanGunManager.OnScanValueListener() {
            @Override
            public void onScanValue(String value) {
                Log.d(TAG, "扫描到的内容:"+value);
                anim.cancel();
                bluetooth_mac = value;
                et_show_mac.setText(value);
                mHandler.sendEmptyMessage(MES_SCAN_SUCCESS);
                //扫码完发送广播
                Intent intent = new Intent("com.tmalltv.btautopair.ACTION_SET_BD_ADDR");
                intent.putExtra("BD_ADDR",value);
                sendBroadcast(intent);
            }
        });

        ll_root.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                hideKeyboard(v);
            }
        });

        mBluetoothReceiver = new BluetoothReceiver();
        IntentFilter intentFilter = new IntentFilter();
        intentFilter.addAction(BluetoothHidHost.ACTION_CONNECTION_STATE_CHANGED);
        registerReceiver(mBluetoothReceiver, intentFilter);
        mBluetoothReceiver.setBTPairCallback(new BTPairCallback(){
            @Override
            public void onDeviceConnectedEvent(String os_mac){
                //防止特殊情况
                if(!bluetooth_mac.equals(os_mac)) {
                    os_mac = bluetooth_mac;
                }
                //1.将蓝牙遥控器的mac地址上传到MES系统。2.sst中写入btpair=pass 3.提示关机
                mHandler.sendEmptyMessage(MES_PAIR_SUCCESS);
                onBTPairSuccessEvent(os_mac);
            }

            @Override
            public void onDeviceDisConnectEvent(){

            }
        });
    }

	//监听键盘事件,除了返回事件都将它拦截,使用我们自定义的拦截器处理该事件
    @Override
    public boolean dispatchKeyEvent(KeyEvent event) {
        if (event.getKeyCode() != KeyEvent.KEYCODE_BACK) {
            scanGunManager.analysisKeyEvent(event);
            return true;
        }
        return super.dispatchKeyEvent(event);
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        unregisterReceiver(mBluetoothReceiver);
    }

    private void showDialog(String msg) {
        if(sendBroadcastDialog == null) {
            sendBroadcastDialog = new AlertDialog.Builder(this).create();
        }
        sendBroadcastDialog.setMessage(msg);
        sendBroadcastDialog.setCancelable(false);
        sendBroadcastDialog.show();
    }

    private void hideDialog() {
        if(sendBroadcastDialog!=null && sendBroadcastDialog.isShowing()) {
            sendBroadcastDialog.dismiss();
        }
    }

    public void hideKeyboard(View view){
        if(view == null) return;
        InputMethodManager imm = (InputMethodManager) view.getContext()
                .getSystemService(Context.INPUT_METHOD_SERVICE);
        if (imm != null) {
            imm.hideSoftInputFromWindow(view.getWindowToken(),0);
        }
    }

    private void onBTPairSuccessEvent(String os_mac){
        new Thread() {
            @Override
            public void run() {
                super.run();
                for(int i = 1; i <= 5; i++) {
                    NetReceivedResult netReceivedResult = uploadToMES(os_mac,"FIRST");
                    if(netReceivedResult.iCode == 200) {
                        mHandler.sendEmptyMessageDelayed(MES_UPLOAD_SUCCESS,1000);
                        return;
                    } else {
                        Log.d(TAG,"upload mes fail,retry after 1 second.");
                        try {
                            Thread.sleep(1000);
                        } catch (InterruptedException e) {
                            e.printStackTrace();
                        }
                    }
                }
                mHandler.sendEmptyMessage(MES_UPLOAD_FAIL);
            }
        }.start();
    }

    private NetReceivedResult uploadToMES(String os_mac,String step){
        MES_RequestBean requestBean = MES_RequestBean.getQuestBeanFromCaseWifi();
        requestBean.step = step;
        requestBean.stationKey = "617";
        requestBean.scpContent = getScpContent(os_mac);
        String urlStr = PreferenceUtil.getString(PreferenceUtil.KEY_UPLOAD_MES_URL,"");
        Log.d(TAG,"uploadToMES urlStr:"+urlStr+" ,os_mac:"+os_mac);
        return NetUtil.getURLContentByGet(urlStr,requestBean);
    }

    private String getScpContent(String os_mac){
        StringBuilder sb = new StringBuilder();
        sb.append("OS").append("%0B").append(os_mac);
        String ret = sb.toString();
        Log.d(TAG,"upload ret:"+ret);
        return ret;
    }
}