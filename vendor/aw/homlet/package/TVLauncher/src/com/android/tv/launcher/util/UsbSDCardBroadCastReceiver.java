/*
 * Copyright (C) 2014 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.android.tv.launcher.util;
import com.android.tv.launcher.device.storage.StorageActivity;
import com.android.tv.launcher.R;

import android.R.string;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.os.Environment;
import android.util.Log;
import android.widget.Toast;
import android.os.storage.StorageManager;
import android.os.storage.StorageVolume;

public class UsbSDCardBroadCastReceiver{
    public static interface Callbacks {
        public void onUSBMounted(String path);
        public void onUSBEject(String path);
        public void onSDCardMounted(String path);
        public void onSDCardEject(String path);
    }
    public static boolean USBMounted = false; //在BrowseInfo那边用，标识是否需要重新构造图标（修改语言之后 ，返回图标过来SD图标消失了)
    public static boolean SDCardMounted = false;
    private static final String TAG = "UsbSDCardBroadCastReceiver";
    private static UsbSDCardBroadCastReceiver sInstance;
    private static Callbacks mCallbacks;
    private static Context mContext;
    //单例
    public static UsbSDCardBroadCastReceiver getInstance(Callbacks callbacks,Context context) {
            if (sInstance == null) {
                sInstance = new UsbSDCardBroadCastReceiver();
            }
            mCallbacks = callbacks;  //这个参数需要多次初始化，在返回主界面的时候，此类要重新加载一次。
            mContext = context;
            return sInstance;
    }
    private UsbSDCardBroadCastReceiver(){
    }
    public static class UsbSDCardBroadCast extends BroadcastReceiver{
            private static Toast mToast = null;
            private static final String SD_CARD = "/storage/0000-0000";
            @Override
            public void onReceive(Context context, Intent intent) {
		if(mContext==null){
			return;
		}
                // TODO Auto-generated method stub
                String sdcard="/storage/emulated/0";
                String action = intent.getAction();
                String path = intent.getData().getPath();
                String tip = null;
                boolean sdorusb = false;
                if(path.equals(sdcard)) //内置存储卡的mount不用通知
                    return;

                StorageManager manager = (StorageManager) mContext.getSystemService(Context.STORAGE_SERVICE);
                StorageVolume[] volumes = manager.getVolumeList();
                for (StorageVolume sv:volumes) {
                      Log.d("IKE",sv.getId() + " " + sv.getPath());
                      if(sv.getPath().equals(path)){
                          if(sv.getId().startsWith("public:179")){
                             sdorusb = true;
                          }else if(sv.getId().startsWith("public:8")){
                             Log.d("IKE","usb device plug in");
                             sdorusb = false;
                          }
                      }
                }

                if(action.equals(Intent.ACTION_MEDIA_MOUNTED)){
                    if(sdorusb){ //SD卡
                        tip = mContext.getString(R.string.aw_sdcard_mounted_toast_string); 
                        SDCardMounted = true;
                        mCallbacks.onSDCardMounted(path);
                    }
                    else{  //USB设备,会有多个
                        tip = mContext.getString(R.string.aw_usb_mounted_toast_string);
                        mCallbacks.onUSBMounted(path);
                    }
                }else if(action.equals(Intent.ACTION_MEDIA_EJECT)){
                    if(sdorusb) {
                        tip = mContext.getString(R.string.aw_sdcard_eject_toast_string);  
                        SDCardMounted = false;
                        mCallbacks.onSDCardEject(path);
                    }
                    else{  //USB设备
                        tip = mContext.getString(R.string.aw_usb_eject_toast_string);
                        mCallbacks.onUSBEject(path);
                    }
                }
                if(mToast == null)
                    mToast = Toast.makeText(context, tip, Toast.LENGTH_SHORT);
                else
                    mToast.setText(tip);
                mToast.show();
            }
    }
}

