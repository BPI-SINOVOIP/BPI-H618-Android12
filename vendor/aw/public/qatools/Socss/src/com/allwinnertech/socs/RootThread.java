package com.allwinnertech.socs;

import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.IOException;

import android.os.Handler;
import android.os.Message;
import android.os.SystemClock;
import android.util.Log;

public class RootThread extends Thread {
    private static final String TAG = "RootThread";
    private String mCmd = "";
    private Handler mHandler;
    private Process mP;
    private String line;
    private String mResult;

    public RootThread(String cmd,Handler handler){
        mCmd = cmd;
        mHandler = handler;
    }

    @Override
    public void run(){
        this.setPriority(MAX_PRIORITY);
        SysIntf.runRootCmd("/system/bin/ethpkt");
        Message msg = mHandler.obtainMessage();
        msg.what = 0x02;
        mHandler.sendMessage(msg);

    }

}
