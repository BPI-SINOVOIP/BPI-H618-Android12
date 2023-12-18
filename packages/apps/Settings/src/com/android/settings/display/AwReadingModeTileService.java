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

package com.android.settings.display;

import android.app.StatusBarManager;
import android.content.Intent;
import android.os.IBinder;
import android.service.quicksettings.Tile;
import android.service.quicksettings.TileService;
import android.util.Log;

import vendor.display.DisplayOutputManager;

public class AwReadingModeTileService extends TileService {
    private static final String TAG = "AwReadingModeTileService";
    private DisplayOutputManager mManager;
    private static final int DISPLAY0 = 0;

    public AwReadingModeTileService() {
        mManager = DisplayOutputManager.getInstance();
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
        //getQsTile().setLabel(getString(R.string.reading_mode));
        //getQsTile().setIcon(Icon.createWithResource(this, R.drawable.icon_tile_reading_mode));
        updateState();
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
        mManager.setReadingMode(DISPLAY0, !mManager.getReadingMode(DISPLAY0));
        updateState();
    }

    private void updateState() {

        getQsTile().setState(mManager.getReadingMode(DISPLAY0) ? Tile.STATE_ACTIVE : Tile.STATE_INACTIVE);
        getQsTile().updateTile();
    }

}
