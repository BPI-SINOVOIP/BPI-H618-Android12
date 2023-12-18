package com.eink.launcher;

import android.app.Application;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.res.Configuration;
import android.util.Log;

import java.util.Locale;

public class LauncherApplication extends Application {
    private static boolean sIsScreenLarge;
    private static float sScreenDensity;

    public ContentModel mModel;
    public IconCache mIconCache;
    private Locale mCurrentLocale;

    @Override
    public void onCreate() {
        super.onCreate();

        final int screenSize = getResources().getConfiguration().screenLayout &
                Configuration.SCREENLAYOUT_SIZE_MASK;
        sIsScreenLarge = screenSize == Configuration.SCREENLAYOUT_SIZE_LARGE ||
                screenSize == Configuration.SCREENLAYOUT_SIZE_XLARGE;

        sScreenDensity = getResources().getDisplayMetrics().density;

        mIconCache = new IconCache(this);
        mModel = new ContentModel(this, mIconCache);

        IntentFilter filter = new IntentFilter(Intent.ACTION_PACKAGE_ADDED);
        filter.addAction(Intent.ACTION_PACKAGE_REMOVED);
        filter.addAction(Intent.ACTION_PACKAGE_CHANGED);
        filter.addDataScheme("package");
        registerReceiver(mModel, filter);
    }

    public ContentModel setLauncher(Launcher launcher) {
        mModel.initialize(launcher);
        return mModel;
    }

    public ContentModel getModel() {
        return mModel;
    }

    public IconCache getIconCache() {
        return mIconCache;
    }

    public static boolean isScreenLarge() {
        return sIsScreenLarge;
    }

    public static boolean isScreenLandscape(Context context) {
        return context.getResources().getConfiguration().orientation ==
                Configuration.ORIENTATION_LANDSCAPE;
    }

    public static float getScreenDensity() {
        return sScreenDensity;
    }

    public void setLanguage(Locale l) {
        mCurrentLocale = l;
    }

    public Locale getLanguage() {
        return mCurrentLocale;
    }
}
