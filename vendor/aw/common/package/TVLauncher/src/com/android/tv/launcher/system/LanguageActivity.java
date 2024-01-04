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

package com.android.tv.launcher.system;

import com.android.tv.launcher.R;
import com.android.tv.launcher.dialog.old.Action;
import com.android.tv.launcher.dialog.old.ActionAdapter;
import com.android.tv.launcher.dialog.old.ActionFragment;
import com.android.tv.launcher.dialog.old.ContentFragment;
import com.android.tv.launcher.dialog.old.DialogActivity;
import com.android.internal.app.LocalePicker;

import android.app.ActivityManagerNative;
import android.app.Fragment;
import android.content.Context;
import android.content.res.Configuration;
import android.content.res.Resources;
import android.net.wifi.WifiManager;
import android.os.Bundle;
import android.os.Handler;
import android.os.RemoteException;
import android.text.TextUtils;
import android.util.Log;
import android.view.KeyEvent;
import android.widget.ArrayAdapter;

import java.util.ArrayList;
import java.util.Comparator;
import java.util.Locale;
import java.util.MissingResourceException;

public class LanguageActivity extends DialogActivity implements ActionAdapter.Listener {

    private static final String TAG = "LanguageActivity";
    private static final String KEY_LOCALE = "locale";
 
    private final Handler mDelayHandler = new Handler();
    private Locale mNewLocale;
    private final Runnable mSetLanguageRunnable = new Runnable() {
        @Override
        public void run() {
            LocalePicker.updateLocale(mNewLocale);
            //setWifiCountryCode();这个会出现特别卡的情况，暂时不考虑这种情况
        }
    };
    
    private String mNowFocusItemKey; //标示当前选中的那个的key值，在相同key上点击不触发任何改变
    private String[] mLocaleNames = null;//保存所有名称
    private Fragment mContentFragment;
    private Resources mResources;
    private ArrayList<Action> mActions;
    private ArrayAdapter<LocalePicker.LocaleInfo> mLocales;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        
        makeContentFragment();
        getActions();
        setContentAndActionFragments(mContentFragment, ActionFragment.newInstance(mActions));
    }
    @Override
    public void onPause() {
        super.onPause();
        finish();
    }
  
    private void makeContentFragment() {
        mResources = getResources();
        mContentFragment = ContentFragment.newInstance(
                mResources.getString(R.string.system_language), null, null,
                R.drawable.ic_settings_language, getResources().getColor(R.color.icon_background));
    }
    private void getActions() {
        if(mActions == null)
            mActions = new ArrayList<Action>();
        else
            mActions.clear();
        
        if(mLocales == null)
            mLocales = LocalePicker.constructAdapter(this);
        
        Locale currentLocale;
        try {
            currentLocale = ActivityManagerNative.getDefault().getConfiguration().locale;
        } catch (RemoteException e) {
            currentLocale = null;
        }
        
        boolean newLocaleNames = false;
        if(mLocaleNames == null)
        {
            newLocaleNames = true;
            mLocaleNames = new String[mLocales.getCount()];
        }
        
        final String[] specialLocaleCodes = getResources().getStringArray(
                com.android.internal.R.array.special_locale_codes);
        final String[] specialLocaleNames = getResources().getStringArray(
                com.android.internal.R.array.special_locale_names);
        for(int i=0;i < mLocaleNames.length; i++)
        {
            Locale temp;
            temp = mLocales.getItem(i).getLocale();
            if(newLocaleNames)
            {
                mLocaleNames[i] = getDisplayName(temp, specialLocaleCodes, specialLocaleNames);
                if (mLocales.getItem(i).getLabel().contains("(")) {
                    String country = temp.getCountry();
                    if (!TextUtils.isEmpty(country)) {
                        mLocaleNames[i] = mLocaleNames[i] + " (" + temp.getCountry() + ")";
                    }
                }
                if (mLocaleNames[i].length() > 0) {
                    mLocaleNames[i] = mLocaleNames[i].substring(0, 1).toUpperCase(currentLocale) +
                            mLocaleNames[i].substring(1);
                }
            }
            
            
              
            StringBuilder sb = new StringBuilder();
            sb.append(KEY_LOCALE).append(i);
            boolean check = temp.equals(currentLocale);
            if(check)
            {
                mNowFocusItemKey = sb.toString();
                
            }
            mActions.add(new Action.Builder()
                    .key(sb.toString())
                    .title(mLocaleNames[i])
                    .checked(check)
                    .build());
        }
    }
    
    private static String getDisplayName(
            Locale l, String[] specialLocaleCodes, String[] specialLocaleNames) {
            String code = l.toString();
            for (int i = 0; i < specialLocaleCodes.length; i++) {
                if (specialLocaleCodes[i].equals(code)) {
                    return specialLocaleNames[i];
                }
            }
            return l.getDisplayName(l);
    }
    
    @Override
    public void onActionClicked(Action action) {
        String key = action.getKey();
        
        if(mNowFocusItemKey!=null&&mNowFocusItemKey.equals(key))
            return;
        if (key.contains(KEY_LOCALE)) {
            String s = key.substring(KEY_LOCALE.length());
            int i = Integer.parseInt(s);
            
            mNewLocale = mLocales.getItem(i).getLocale();
            mDelayHandler.removeCallbacks(mSetLanguageRunnable);
            mDelayHandler.post(mSetLanguageRunnable);
        }
    }

    @Override
    public void onConfigurationChanged(Configuration newConfig) {
        super.onConfigurationChanged(newConfig);
        
        makeContentFragment();
        setContentFragment(mContentFragment);
        
        getActions(); //refresh mActions
        Fragment fra = getActionFragment();
        if ((fra != null) && (fra instanceof ActionFragment)) {
          ActionFragment actFrag = (ActionFragment) fra;
          ((ActionAdapter) actFrag.getAdapter()).setActions(mActions);
        }
    }

    private void setWifiCountryCode() {
        String countryCode = Locale.getDefault().getCountry();
        WifiManager wifiMgr = (WifiManager) getSystemService(Context.WIFI_SERVICE);

        if (wifiMgr != null && !TextUtils.isEmpty(countryCode)) {
            //wifiMgr.setCountryCode(countryCode);
        }
    }
    
}
