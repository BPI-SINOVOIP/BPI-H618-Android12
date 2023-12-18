package com.allwinnertech.socs;

import android.app.Activity;
import android.graphics.Color;
import android.graphics.Rect;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.util.Log;
import android.view.View;
import android.webkit.WebView;
import android.widget.TextView;

public class MainTestActivity extends Activity {

    protected static final int UPDATE_ETH_RESULT = 0x01;
    protected static final int UPDATE_ETH_FINISH = 0x02;
    protected static final String TAG = "MainTestActivity";
    TextView mCpuid_platform;
    TextView mCpuid_chipid;
    TextView mCpuid_ictype;
    TextView mCpuid_reason;
    TextView mCpuid_custtype;
    TextView mEthernetCmd;
    WebView mEthRes;
    View mCpuiditem;
    View mEthiditem;
    Handler mHandler = new Handler(){
        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
            case UPDATE_ETH_RESULT:
                break;
            case UPDATE_ETH_FINISH:
                if (mEthernetCmd != null) {
                    Log.d(TAG,"load eth result url");
                    mEthRes.loadUrl("file:///sdcard/eth_result.html");
                }
                break;
            }
        }
    };

    void hideCpuidTestItem(boolean hide){
        mCpuiditem = this.findViewById(R.id.cpuiditem);
        if(hide)
            mCpuiditem.setVisibility(View.GONE);
        else
            mCpuiditem.setVisibility(View.VISIBLE);
    }
    void initAndStartCpuidTestItem(){
        mCpuid_platform = (TextView) this.findViewById(R.id.platform);
        mCpuid_chipid = (TextView) this.findViewById(R.id.chipid);
        mCpuid_ictype = (TextView) this.findViewById(R.id.ictype);
        mCpuid_reason = (TextView) this.findViewById(R.id.reason);
        mCpuid_custtype = (TextView) this.findViewById(R.id.custtype);
        final ChipidInfo cpuid = new ChipidInfo();
        /*Thread cpuinfoThread = new Thread(){
            @Override
            public void run(){

            }
        };
        cpuinfoThread.start();*/
        String[] info = cpuid.getChipType();
        Log.d(TAG,"mCpuid_chipid = " + info[1]);
        Log.d(TAG,"mCpuid_reason = " +info[3]);
        mCpuid_platform.setText(info[0]);
        mCpuid_chipid.setText(info[1]);
        mCpuid_ictype.setText(info[2]);
        mCpuid_ictype.setTextColor(Color.RED);
        mCpuid_reason.setText(info[3]);
        mCpuid_custtype.setText(info[4]);
    }


    void initAndStartEthTestItem(){
        mEthernetCmd = (TextView) this.findViewById(R.id.ethoutput);
        mEthRes = (WebView) this.findViewById(R.id.ethresult);
        Log.d("SysIntf","start run ethpkt");
        RootThread rt = new RootThread("/system/bin/ethpkt",mHandler);
        rt.start();
    }
    void hideEthTestItem(boolean hide){
        mEthiditem = this.findViewById(R.id.ethitem);
        if(hide)
            mEthiditem.setVisibility(View.GONE);
        else
            mEthiditem.setVisibility(View.VISIBLE);
    }
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main_test);
        TestsConfig tc = new TestsConfig();
        tc.loadAllConfigFromFile(this.getFilesDir() + "/config.json");
        if(tc.getEnableCpuidTest()){
            hideCpuidTestItem(false);
            initAndStartCpuidTestItem();
        }else{
            hideCpuidTestItem(true);
        }
        if(tc.getEnableEthTest()){
            hideEthTestItem(false);
            initAndStartEthTestItem();
        }else{
            hideEthTestItem(true);
        }

    }

}
