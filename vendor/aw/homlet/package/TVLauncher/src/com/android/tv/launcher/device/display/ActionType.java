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

package com.android.tv.launcher.device.display;

import com.android.tv.launcher.ActionBehavior;
import com.android.tv.launcher.ActionKey;
import com.android.tv.launcher.R;
import com.android.tv.launcher.dialog.old.Action;

import android.content.res.Resources;
import android.util.Log;

import java.util.ArrayList;

/**
 * The different possible action types (screens).
 */
enum ActionType {
    /*
     * Display
     */
    DISPLAY_OVERVIEW(R.string.captions_display), //"显示"
    DISPLAY_RESOLUTION(R.string.captions_display_resolution), //"分辨率"
    DISPLAY_ZOOM(R.string.captions_display_zoom), //"缩放"
    DISPLAY_SET_WALLPAPER(R.string.captions_display_wallpaper), //"壁纸设置"
    DISPLAY_PIP_SETTINGS(R.string.captions_picture_in_picture), //"画中画设置"
        DISPLAY_PIP_SETTINGS_POS(R.string.captions_picture_in_picture_position), //"画中画位置"
        DISPLAY_PIP_SETTINGS_SIZE(R.string.captions_picture_in_picture_size); //"画中画大小" 


    private final int mTitleResource;
    private final int mDescResource;

    private ActionType(int titleResource) {
        mTitleResource = titleResource;
        mDescResource = 0;
    }

    private ActionType(int titleResource, int descResource) {
        mTitleResource = titleResource;
        mDescResource = descResource;
    }

    String getTitle(Resources resources) {
        return resources.getString(mTitleResource);
    }

    String getDescription(Resources resources) {
        if (mDescResource != 0) {
            return resources.getString(mDescResource);
        }
        return null;
    }

    Action toAction(Resources resources) {
        return toAction(resources, getDescription(resources));
    }

    Action toAction(Resources resources, String description) {
        return new Action.Builder()
                .key(getKey(this, ActionBehavior.INIT))
                .title(getTitle(resources))
                .description(description)
                .build();
    }

    Action toInfo(Resources resources, String description) {
        return new Action.Builder()
                .key(getKey(this, ActionBehavior.INIT))
                .title(getTitle(resources))
                .description(description)
                .enabled(false)
                .build();
    }

    Action toInfo(Resources resources, int descResource) {
        return toInfo(resources, resources.getString(descResource));
    }

    Action toAction(Resources resources, int descResource) {
        return new Action.Builder()
                .key(getKey(this, ActionBehavior.INIT))
                .title(getTitle(resources))
                .description(resources.getString(descResource))
                .build();
    }

    private String getKey(ActionType t, ActionBehavior b) {
        String tmp = new ActionKey<ActionType, ActionBehavior>(t, b).getKey();
        return tmp;
    }
}
