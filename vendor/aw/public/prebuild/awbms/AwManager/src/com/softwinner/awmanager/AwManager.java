package com.softwinner.awmanager;

import android.app.Activity;
import android.content.ComponentName;
import android.content.Intent;
import android.content.pm.ActivityInfo;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import android.graphics.drawable.ColorDrawable;
import android.graphics.drawable.Drawable;
import android.os.Bundle;
import android.os.Handler;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.BaseAdapter;
import android.widget.ImageView;
import android.widget.ListView;
import android.widget.Spinner;
import android.widget.Switch;
import android.widget.TextView;

import android.aw.BackgroundManager;

import java.util.ArrayList;
import java.util.List;

public class AwManager extends Activity implements AdapterView.OnItemClickListener, AdapterView.OnItemSelectedListener {
    private AwManagerApp mApp;
    private PackageManager mPackageManager;
    private Handler mHandler;
    private View mMain;
    private Spinner mLimitSpinner;
    private ArrayAdapter<String> mLimitAdapter;
    private ListView mAppList;
    private MyAdapter mAdapter;
    private List<MyData> mAppData;
    private TextView mAppNone;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.aw_manager);

        mApp = (AwManagerApp) getApplication();
        mPackageManager = getPackageManager();
        mHandler = new Handler();

        mMain = findViewById(R.id.main);
        mLimitSpinner = (Spinner) findViewById(R.id.spinner);
        mLimitAdapter = new ArrayAdapter<String>(this, R.layout.limit_spinner_item);
        mLimitAdapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        int limit = BackgroundManager.getLimitBackgroundCount();
        for (int i = 0; i < limit + 3; i++) {
            mLimitAdapter.add(getString(R.string.manager_limit, i));
        }
        mLimitSpinner.setAdapter(mLimitAdapter);
        mLimitSpinner.setOnItemSelectedListener(this);
        mAppList = (ListView) findViewById(R.id.app_list);
        mAdapter = new MyAdapter();
        mAppList.setAdapter(mAdapter);
        mAppList.setOnItemClickListener(this);
        mAppNone = (TextView) findViewById(R.id.app_none);
    }

    @Override
    protected void onResume() {
        super.onResume();
        if (mApp.enable()) {
            int limit = BackgroundManager.getLimitBackgroundCount();
            List<String> systemWhitelist = BackgroundManager.getSystemWhitelist();
            List<String> userWhitelist = BackgroundManager.getUserWhitelist();
            mLimitSpinner.setSelection(limit, false);
            reloadAppList(systemWhitelist, userWhitelist);
        }
        refreshVisble();
    }

    private void reloadAppList(List<String> systemWhitelist, List<String> userWhitelist) {
        List<MyData> dataList = new ArrayList<>();
        Intent mainIntent = new Intent(Intent.ACTION_MAIN, null);
        mainIntent.addCategory(Intent.CATEGORY_LAUNCHER);
        List<ResolveInfo> lists = mPackageManager.queryIntentActivities(mainIntent, 0);
        for (ResolveInfo app : lists) {
            String packageName = app.activityInfo.packageName;
            boolean skip = false;
            if (systemWhitelist != null) {
                for (String pkg : systemWhitelist) {
                    if (packageName.startsWith(pkg)) {
                        skip = true;
                        break;
                    }
                }
            }
            if (skip) continue;
            ComponentName cn = new ComponentName(packageName, app.activityInfo.name);
            MyData data = new MyData();
            dataList.add(data);
            try {
                ActivityInfo info = mPackageManager.getActivityInfo(cn, 0);
                data.pkg = packageName;
                data.title = info.loadLabel(mPackageManager);
                data.icon = info.loadIcon(mPackageManager);
            } catch (PackageManager.NameNotFoundException e) {
                e.printStackTrace();
            }
            if (userWhitelist != null) {
                data.checked = userWhitelist.contains(packageName);
            } else {
                data.checked = false;
            }
        }
        mAppData = dataList;
        mAdapter.notifyDataSetInvalidated();
    }

    @Override
    public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
        MyData data = mAdapter.getItem(position);
        if (data != null)
            data.checked = !data.checked;
        updateUserList();
        mAdapter.notifyDataSetChanged();
    }

    private Runnable setUserListRunnable = new Runnable() {

        @Override
        public void run() {
            List<String> user = new ArrayList<>();
            if (mAppData != null) {
                for (MyData data : mAppData) {
                    if (data.checked)
                        user.add(data.pkg);
                }
            }
            BackgroundManager.setUserWhitelist(user);
        }
    };

    private void updateUserList() {
        mHandler.removeCallbacks(setUserListRunnable);
        mHandler.postDelayed(setUserListRunnable, 1000);
    }

    @Override
    public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
        setLimit(position);
    }

    @Override
    public void onNothingSelected(AdapterView<?> parent) {

    }

    private void setLimit(int limit) {
        BackgroundManager.setLimitBackgroundCount(limit);
        refreshVisble();
    }

    private void refreshVisble() {
        if (mApp.enable()) {
            mMain.setVisibility(View.VISIBLE);
            int limit = BackgroundManager.getLimitBackgroundCount();
            if (limit > 0 && mAppData != null && mAppData.size() > 0) {
                mAppList.setVisibility(View.VISIBLE);
                mAppNone.setVisibility(View.GONE);
            } else if (limit >= 0) {
                mAppList.setVisibility(View.GONE);
                mAppNone.setVisibility(View.VISIBLE);
            } else {
                mMain.setVisibility(View.GONE);
            }
        } else {
            mMain.setVisibility(View.GONE);
        }
    }

    private class MyData {
        public String pkg;
        public CharSequence title;
        public Drawable icon;
        public boolean checked;
    }

    private class MyAdapter extends BaseAdapter {

        @Override
        public int getCount() {
            return mAppData != null ? mAppData.size() : 0;
        }

        @Override
        public MyData getItem(int position) {
            return mAppData != null ? mAppData.get(position) : null;
        }

        @Override
        public long getItemId(int position) {
            return position;
        }

        @Override
        public View getView(int position, View convertView, ViewGroup container) {
            if (convertView == null) {
                convertView = getLayoutInflater().inflate(R.layout.app_item, container, false);
            }

            MyData data = getItem(position);
            if (data != null) {
                ((ImageView) convertView.findViewById(android.R.id.icon)).setImageDrawable(data.icon != null ? data.icon : new ColorDrawable(0));
                ((TextView) convertView.findViewById(android.R.id.title)).setText(data.title);
                ((Switch) convertView.findViewById(R.id.switchWidget)).setChecked(data.checked);
                ((TextView) convertView.findViewById(R.id.widget_text1)).setText(
                        data.checked ? R.string.manager_allowed : R.string.manager_disallowed);
            }
            return convertView;
        }
    }
}

