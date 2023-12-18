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

import java.util.Calendar;

import android.app.Activity;
import android.os.Bundle;
import android.graphics.drawable.Drawable;
import android.icu.text.SimpleDateFormat;
import androidx.leanback.app.BrowseFragment;
import androidx.leanback.widget.ListRow;
import androidx.leanback.widget.ListRowPresenter;
import androidx.leanback.widget.ClassPresenterSelector;
import androidx.leanback.widget.ObjectAdapter;
import android.text.format.DateFormat;
import android.util.Log;
import android.view.KeyEvent;
import android.view.View;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
/**
 * Activity showing a menu of settings.
 */
public abstract class MenuActivity extends Activity {
    private static final String TAG = "MenuActivity";
    private static final String TAG_BROWSE_FRAGMENT = "browse_fragment";
    private IntentFilter mIntentFilter; //接收时间变化
    private BrowseFragment mBrowseFragment;
    protected abstract String getBrowseTitle();
    protected abstract Drawable getBadgeImage();
    protected abstract BrowseInfoFactory getBrowseInfoFactory();

    private BroadcastReceiver mTimeIntentReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            if(mBrowseFragment != null)
            {
                getDateAndTime();
            }
        }
    };

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        mBrowseFragment = (BrowseFragment) getFragmentManager().findFragmentByTag(
                TAG_BROWSE_FRAGMENT);
        mBrowseFragment = new BrowseFragment();
         /*   当搜索按钮没有回调的时候，就是它不显示的时候
              mBrowseFragment.setOnSearchClickedListener(new View.OnClickListener() {
                @Override
                public void onClick(View view) { //点击搜索按钮

                    //Intent intent = new Intent(this, SearchActivity.class);
                    //startActivity(intent);
                }
            }); */
        getFragmentManager().beginTransaction()
                    .replace(android.R.id.content, mBrowseFragment, TAG_BROWSE_FRAGMENT)
                    .commit();

        ClassPresenterSelector rowPresenterSelector = new ClassPresenterSelector();
        rowPresenterSelector.addClassPresenter(ListRow.class, new ListRowPresenter());

        ObjectAdapter adapter = getBrowseInfoFactory().getRows();
        adapter.setPresenterSelector(rowPresenterSelector);

        mBrowseFragment.setAdapter(adapter);
        updateBrowseParams();

        mIntentFilter = new IntentFilter();
        mIntentFilter.addAction(Intent.ACTION_TIME_TICK);
        mIntentFilter.addAction(Intent.ACTION_TIME_CHANGED);
        mIntentFilter.addAction(Intent.ACTION_TIMEZONE_CHANGED);
        mIntentFilter.addAction(Intent.ACTION_DATE_CHANGED);
        registerReceiver(mTimeIntentReceiver, mIntentFilter);
    }

    protected void updateBrowseParams() {
        getDateAndTime();
        mBrowseFragment.setBadgeDrawable(getBadgeImage());
        mBrowseFragment.setHeadersState(BrowseFragment.HEADERS_DISABLED);//ENABLE---按向左键会出现一个菜单
    }

    @Override
    protected void onPause() {
        // TODO Auto-generated method stub
        super.onPause();
        unregisterReceiver(mTimeIntentReceiver);
    }
    @Override
    protected void onResume() {
        // TODO Auto-generated method stub
        super.onResume();
        getDateAndTime();
        registerReceiver(mTimeIntentReceiver, mIntentFilter);
    }

    private void getDateAndTime(){
        final Calendar now = Calendar.getInstance();
        //java的这个FULL模式可以满足显示年月日及星期的要求，支持多语言
        java.text.DateFormat dateFormat = java.text.DateFormat.getDateInstance(java.text.DateFormat.FULL);
        String mNowDate  = dateFormat.format(now.getTime());
        //android的这个可以满足只显示时和分的要求，支持多语言，24小时格式，时区变化等
        java.text.DateFormat timeFormat = DateFormat.getTimeFormat(this);
        String mNowTime = timeFormat.format(now.getTime());
        mBrowseFragment.setTitle(mNowDate +"  "+ mNowTime);
    }
}
