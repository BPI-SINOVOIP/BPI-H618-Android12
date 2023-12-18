/*
 * Copyright (C) 2011 The Android Open Source Project
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

package com.softwinner.screenshot;

import android.app.StatusBarManager;
import android.content.Intent;
import android.os.Handler;
import android.os.IBinder;
import android.service.quicksettings.Tile;
import android.service.quicksettings.TileService;
import android.util.Log;
import com.android.internal.util.ScreenshotHelper;

import static android.view.WindowManager.TAKE_SCREENSHOT_FULLSCREEN;
import static android.view.WindowManager.ScreenshotSource.SCREENSHOT_OTHER;

public class ScreenshotTileService extends TileService {
    private static final String TAG = "ScreenshotTileService";

    private static final String SYSUI_PACKAGE = "com.android.systemui";
    private static final String SYSUI_SCREENSHOT_SERVICE =
            "com.android.systemui.screenshot.TakeScreenshotService";
    private static final String SYSUI_SCREENSHOT_ERROR_RECEIVER =
            "com.android.systemui.screenshot.ScreenshotServiceErrorReceiver";
    private static final int DELAY = 500;
    private Handler mHandler;
    private ScreenshotHelper mScreenshotHelper;

    public ScreenshotTileService() {
        mHandler = new Handler();
    }

    @Override
    public IBinder onBind(Intent intent) {
        Log.i(TAG, "onBind");
        return super.onBind(intent);
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        Log.i(TAG, "onDestroy");
    }

    @Override
    public void onTileAdded() {
        Log.i(TAG, "onTileAdded");
    }

    @Override
    public void onTileRemoved() {
        Log.i(TAG, "onTileRemoved");
    }

    @Override
    public void onStartListening() {
        Log.i(TAG, "onStartListening");
        getQsTile().setLabel(getString(R.string.label_tile_screenshot));
        //getQsTile().setIcon(Icon.createWithResource(this, R.drawable.icon_tile_screenshot));
        getQsTile().setState(Tile.STATE_ACTIVE);
        getQsTile().updateTile();
    }

    @Override
    public void onStopListening() {
        Log.i(TAG, "onStopListening");
    }

    @Override
    public void onClick() {
        Log.i(TAG, "onClick");
        StatusBarManager statusBarManager = (StatusBarManager) this
                .getSystemService(android.app.Service.STATUS_BAR_SERVICE);
        statusBarManager.collapsePanels();
        mHandler.removeCallbacks(mScreenshotRunnable);
        mHandler.postDelayed(mScreenshotRunnable, DELAY);
    }

    private class ScreenshotRunnable implements Runnable {
        @Override
        public void run() {
            if (mScreenshotHelper == null)
                mScreenshotHelper = new ScreenshotHelper(getApplicationContext());
            mScreenshotHelper.takeScreenshot(TAKE_SCREENSHOT_FULLSCREEN, true, true, SCREENSHOT_OTHER, mHandler, null);
        }
    }

    private final ScreenshotRunnable mScreenshotRunnable = new ScreenshotRunnable();

}
