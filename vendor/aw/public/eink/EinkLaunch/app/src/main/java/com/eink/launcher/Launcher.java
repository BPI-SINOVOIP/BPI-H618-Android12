package com.eink.launcher;

import java.util.ArrayList;
import java.util.Locale;

import com.eink.launcher.PagedView.PageSwitchListener;
import com.softwinner.einklaunch.R;

import android.content.Context;
import android.os.Build;
import android.os.Bundle;
import android.os.LocaleList;
import android.app.Activity;
import android.content.ActivityNotFoundException;
import android.content.Intent;
import android.content.res.Configuration;
import android.util.Log;
import android.view.Menu;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.ImageView;
import android.widget.TextView;
import android.widget.Toast;

public class Launcher extends Activity
        implements ContentModel.Callbacks {

    private static final String TAG = "Launcher";
    private AppsPagedView mPageLayout;
    private ContentModel mModel;

    private ImageView mNextPage, mPrevPage;
    private TextView mCurPage, mTotalPage;

    private boolean mPaused = true;
    private boolean mOnResumeNeedsLoad;
    private boolean mNeedRefresh = false;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        /*add by A-gan set eink update strategy*/
        /*View.setEinkUpdateStrategy(View.EINK_DISPLAY_STRATEGY_ALL_FLIP_WITHOUT_LAST,
        View.EINK_DISPLAY_MODE_GC16_LOCAL, View.EINK_DISPLAY_MODE_GC16_FULL, View.EINK_DISPLAY_MODE_GC16_LOCAL);*/
        /*add end*/
        super.onCreate(savedInstanceState);

        Log.i(TAG, "onCreate");
        LauncherApplication app = ((LauncherApplication) getApplication());
        mModel = app.setLauncher(this);

        setContentView(R.layout.launcher_applist);

        setupView();

        Log.d(TAG, "onCreate locale:" + getResources().getConfiguration().getLocales().get(0));
        Locale locale;
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.N) {
            locale = getResources().getConfiguration().getLocales().get(0);
        } else {
            locale = getResources().getConfiguration().locale;
        }
        if (app.getLanguage() == null) {
            app.setLanguage(locale);
        }
        if (!app.getLanguage().equals(locale)) {
            app.setLanguage(locale);
            mModel.reset();
        }

        mModel.startLoader(this, true);
    }

    @Override
    protected void onPause() {
        // TODO Auto-generated method stub
        super.onPause();
        mPaused = true;
    }

    @Override
    protected void onResume() {
        /*add by A-gan set eink update strategy*/
        /*View.setEinkUpdateStrategy(View.EINK_DISPLAY_STRATEGY_ALL_FLIP_WITHOUT_LAST,
        View.EINK_DISPLAY_MODE_GC16_FULL, View.EINK_DISPLAY_MODE_GC16_FULL, View.EINK_DISPLAY_MODE_GC16_FULL);*/
        /*add end*/
        super.onResume();
        Log.i(TAG, "onResume");

        mPaused = false;

        if (mOnResumeNeedsLoad) {
            mModel.startLoader(this, true);
            mOnResumeNeedsLoad = false;
        }

        mPageLayout.loadAssociatedPages(mPageLayout.getCurrentPage());
    }

    @Override
    protected void onDestroy() {
        /*add by A-gan set eink update strategy*/
        /*View.setEinkUpdateStrategy(View.EINK_DISPLAY_STRATEGY_ALL_FLIP_WITHOUT_LAST,
        View.EINK_DISPLAY_MODE_GC16_LOCAL, View.EINK_DISPLAY_MODE_GC16_FULL, View.EINK_DISPLAY_MODE_GC16_LOCAL);*/
        /*add end*/
        super.onDestroy();
    }

    private void setupView() {
        showLoadingWait();

        mPageLayout = (AppsPagedView) findViewById(R.id.apps_content);

        mPrevPage = (ImageView) findViewById(R.id.page_prev);
        mNextPage = (ImageView) findViewById(R.id.page_next);
        mCurPage = (TextView) findViewById(R.id.cur_page_ind);
        mTotalPage = (TextView) findViewById(R.id.total_page_ind);

        mPageLayout.setPageSwitchListener(new PageSwitchListener() {

            @Override
            public void onPageSwitch(View newPage, int newPageIndex) {
                // TODO Auto-generated method stub
                mCurPage.setText(String.valueOf(newPageIndex + 1));
            }
        });

        OnClickListener listener = new OnClickListener() {

            @Override
            public void onClick(View v) {
                // TODO Auto-generated method stub
                switch (v.getId()) {
                    case R.id.page_prev:
                        mPageLayout.scrollLeft();
                        break;
                    case R.id.page_next:
                        mPageLayout.scrollRight();
                        break;
                }
            }
        };

        mPrevPage.setOnClickListener(listener);
        mNextPage.setOnClickListener(listener);
    }

    @Override
    public void onConfigurationChanged(Configuration newConfig) {
        super.onConfigurationChanged(newConfig);

        Log.d(TAG, "onConfigurationChanged newConfig:"+newConfig.toString());
        LocaleList list = newConfig.getLocales();
        if (!list.isEmpty()) {
            mModel.reset();
            mModel.startLoader(this, true);
            mNeedRefresh = true;
        }
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        // Inflate the menu; this adds items to the action bar if it is present.
        //	getMenuInflater().inflate(R.menu.activity_main, menu);
        return true;
    }

    public static boolean startActivitySafely(Context context, Intent intent, Object tag) {
        intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        try {
            context.startActivity(intent);
            return true;
        } catch (ActivityNotFoundException e) {
            Toast.makeText(context, "Activity not found", Toast.LENGTH_SHORT).show();
            Log.e(TAG, "Unable to launch. tag=" + tag + " intent=" + intent, e);
        } catch (SecurityException e) {
            Toast.makeText(context, "Activity not found", Toast.LENGTH_SHORT).show();
            Log.e(TAG, "Launcher does not have the permission to launch " + intent +
                    ". Make sure to create a MAIN intent-filter for the corresponding activity " +
                    "or use the exported attribute for this activity. "
                    + "tag=" + tag + " intent=" + intent, e);
        }
        return false;
    }

    @Override
    public boolean setLoadOnResume() {
        // TODO Auto-generated method stub
        if (mPaused) {
            mOnResumeNeedsLoad = true;
            return true;
        } else {
            return false;
        }
    }

    @Override
    public void bindAllApplications(final ArrayList<ApplicationInfo> apps) {
        // TODO Auto-generated method stub
        setLoadOnResume();
        //Log.d("debug","bindAllApplication");
        mPageLayout.post(new Runnable() {
            public void run() {
                if (mPageLayout != null) {
                    mPageLayout.setApps(apps);
                    if (mNeedRefresh) {
                        mPageLayout.refreshApps();
                        mNeedRefresh = false;
                    }
                }
                int count = mPageLayout.getPageCount();
                if (count <= 0) {
                    count = 1;
                }
                mTotalPage.setText(String.valueOf(count));

                hideLoadingWait();
            }
        });
    }

    @Override
    public void bindAppsAdded(ArrayList<ApplicationInfo> apps) {
        // TODO Auto-generated method stub
        setLoadOnResume();
        if (mPageLayout != null) {
            mPageLayout.addApps(apps);
        }
    }

    @Override
    public void bindAppsUpdated(ArrayList<ApplicationInfo> apps) {
        // TODO Auto-generated method stub
        setLoadOnResume();
        if (mPageLayout != null) {
            mPageLayout.updateApps(apps);
        }
    }

    @Override
    public void bindAppsRemoved(ArrayList<ApplicationInfo> apps,
                                boolean permanent) {
        // TODO Auto-generated method stub
        setLoadOnResume();
        if (mPageLayout != null) {
            mPageLayout.removeApps(apps);
        }
    }

    @Override
    public void bindPackagesUpdated() {
        // TODO Auto-generated method stub

    }

    public void showLoadingWait() {
        View v = findViewById(R.id.load_wait);
        if (v != null) {
            v.setVisibility(View.VISIBLE);
        }
    }

    public void hideLoadingWait() {
        View v = findViewById(R.id.load_wait);
        if (v != null) {
            v.setVisibility(View.GONE);
        }
    }
}
