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

package com.android.tv.launcher;

import android.accounts.Account;
import android.accounts.AccountManager;
import android.accounts.OnAccountsUpdateListener;
import android.content.ComponentName;
import android.graphics.drawable.Drawable;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.Bundle;
import android.util.Log;
import android.view.KeyEvent;
import android.view.Window;
import android.view.WindowInsets;
import android.view.WindowInsetsController;

import com.android.tv.launcher.accessories.BluetoothConnectionsManager;
import com.android.tv.launcher.connectivity.ConnectivityListener;

import java.util.Locale;

// AW:Added for BOOTEVENT
import android.os.SystemProperties;
import java.io.FileOutputStream;
import java.io.FileNotFoundException;
import java.io.IOException;


/**
 * Main settings which loads up the top level headers.
 */
public class MainSettings extends MenuActivity implements OnAccountsUpdateListener,
        ConnectivityListener.Listener {

	private static final String TAG = "MainSettings";
    private static MainSettings sInstance;

    private BrowseInfo mBrowseInfo;
    private AccountManager mAccountManager;
    private Locale mCurrentLocale;
    private ConnectivityListener mConnectivityListener;

    // AW:Added for BOOTEVENT
    private static boolean sBootEventenable = SystemProperties.getBoolean("persist.sys.bootevent", true);
    static void logBootEvent(String bootevent) {
        if (!sBootEventenable) {
            return ;
        }
        FileOutputStream fos =null;
        try {
            fos = new FileOutputStream("/proc/bootevent");
            fos.write(bootevent.getBytes());
            fos.flush();
        } catch (FileNotFoundException e) {
            Log.e("BOOTEVENT","Failure open /proc/bootevent,not found!",e);
        } catch (java.io.IOException e) {
            Log.e("BOOTEVENT","Failure open /proc/bootevent entry",e);
        } finally {
            if (fos != null) {
                try {
                    fos.close();
                } catch (IOException e) {
                    Log.e ("BOOTEVENT","Failure close /proc/bootevent entry",e);
                }
            }
        }
    }
    public static synchronized MainSettings getInstance() {
        return sInstance;
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        //AW:BOOTEVENT
        logBootEvent("TVLauncher:onCreate start");
        Log.i(TAG,"TVLauncher:onCreate start");
        mBrowseInfo = new BrowseInfo(this);
        mBrowseInfo.init();
        mCurrentLocale = Locale.getDefault();
        mAccountManager = AccountManager.get(this);
        super.onCreate(savedInstanceState);
        mConnectivityListener = new ConnectivityListener(this, this);
        logBootEvent("TVLauncher:onCreate end");
        Log.i(TAG,"TVLauncher:onCreate end");
    }

    @Override
    protected void onStart() {
        super.onStart();
        mAccountManager.addOnAccountsUpdatedListener(this, null, true);
        onAccountsUpdated(null);
        mConnectivityListener.start();
    }

    @Override
    protected void onResume() {
        // AW:BOOTEVENT
        logBootEvent("TVLauncher:onResume start");
        Log.i(TAG,"TVLauncher:onResume start");
        super.onResume();
        sInstance = this;
        updateAccessories();
        mBrowseInfo.checkForDeveloperOptionUpdate();
        // Update network item forcefully here
        // because mBrowseInfo doesn't have information regarding Ethernet availability.
        mBrowseInfo.updateWifi(mConnectivityListener.isEthernetAvailable());
        // Update InstalledAppList here
        // because mBrowseInfo may not update it when low memory
        mBrowseInfo.updateInstalledAppList();
        Log.i(TAG,"TVLauncher:onResume updateInstalledAppList()");
        // AW:BOOTEVENT Launcher displayed end,turn off bootevent
        logBootEvent("TVLauncher:onResume end");
        logBootEvent("0");
        Log.i(TAG,"TVLauncher:onResume end");
    }

    @Override
    protected void onPause() {
        sInstance = null;
        super.onPause();
        
    }

    @Override
    protected void onStop() {
        mAccountManager.removeOnAccountsUpdatedListener(this);
        super.onStop();
        
        mConnectivityListener.stop();
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        
        mAccountManager.removeOnAccountsUpdatedListener(this);
    }

    @Override
    public void onConnectivityChange(Intent intent) {
    	
        mBrowseInfo.updateWifi(mConnectivityListener.isEthernetAvailable());
    }

    @Override
    protected String getBrowseTitle() {
    	
        return getString(R.string.settings_app_name);
    }

    @Override
    protected Drawable getBadgeImage() {
    	
        return getResources().getDrawable(R.drawable.aw_logo);//全志的logo
    }

    @Override
    protected BrowseInfoFactory getBrowseInfoFactory() {
    	
        if (!mCurrentLocale.equals(Locale.getDefault())) {
            // the System Locale information has changed
            mCurrentLocale = Locale.getDefault();
            mBrowseInfo.rebuildInfo();
        }
        return mBrowseInfo;
    }

    @Override
    public void onAccountsUpdated(Account[] accounts) {
    	
        mBrowseInfo.updateAccounts();
    }

    public void updateAccessories() {
    	
        mBrowseInfo.updateAccessories();
    }

    @Override
    public boolean onKeyUp(int keyCode,KeyEvent event){
        switch(keyCode) {
            case KeyEvent.KEYCODE_MENU:
                Intent intent = new Intent();
                intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
                intent.setComponent(new ComponentName("com.android.tv.settings","com.android.tv.settings.MainSettings"));
                startActivity(intent);
                return true;
        }
        return super.onKeyUp(keyCode, event);
    }

    //屏蔽返回键，不然在主界面多次按返回，会重新加载主界面，导致u盘图标因没有收到插入消息而没有显示
    @Override
    public boolean onKeyDown(int keyCode,KeyEvent event){
        if(keyCode == KeyEvent.KEYCODE_BACK){
            return true;
        }
        return super.onKeyDown(keyCode, event);
    }

}
