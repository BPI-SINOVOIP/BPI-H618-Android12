/*
 * Copyright (C) 2015 The Android Open Source Project
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

package com.android.camera.settings;

import com.android.camera.app.AppController;
import com.android.camera.CameraActivity;
import com.android.camera.one.OneCamera;
import com.android.camera2.R;

import android.content.res.Resources;

/**
 * Handles the camera facing setting for a particular scope stored in
 * SharedPreferences keyed by Keys.KEY_CAMERA_ID.
 */
public class CameraFacingSetting {
    private final SettingsManager mSettingsManager;

    private final String mSettingScope;

    private final String mCameraFacingSettingKey;

    private final int mCameraFacingBackValue;
    private final int mCameraFacingFrontValue;
    private final int mCameraFacingExternalValue;
    private final int mCameraFacingDefaultValue;

    public CameraFacingSetting(
            Resources resources,
            SettingsManager settingsManager,
            String moduleSettingScope,
            AppController appController) {
        mSettingsManager = settingsManager;

        mSettingScope = SettingsManager.getModuleSettingScope(moduleSettingScope);

        mCameraFacingSettingKey = Keys.KEY_CAMERA_ID;
        mCameraFacingDefaultValue =
                Integer.parseInt(resources.getString(R.string.pref_camera_id_default));

        if (appController.getCameraProvider().getCharacteristics(mCameraFacingDefaultValue).
                isFacingFront()) {
            mCameraFacingFrontValue    = 1;
            mCameraFacingBackValue     = 0;
            mCameraFacingExternalValue = 0;
        }
        else if(appController.getCameraProvider().getCharacteristics(mCameraFacingDefaultValue).
                isFacingBack()) {
            mCameraFacingBackValue     = 1;
            mCameraFacingFrontValue    = 0;
            mCameraFacingExternalValue = 0;
        }
        else{
            mCameraFacingBackValue     = 0;
            mCameraFacingFrontValue    = 0;
            mCameraFacingExternalValue = 1;
        }
    }

    @Override
    public String toString() {
        return isFacingBack() ? "Back Camera" :
            (isFacingFront() ? "Front Camera" : "External Camera");
    }

    /**
     * Sets the default value for the camera facing setting.
     */
    public void setDefault() {
        mSettingsManager.setDefaults(
                Keys.KEY_CAMERA_ID,
                mCameraFacingDefaultValue,
                new int[]{mCameraFacingBackValue, mCameraFacingFrontValue});
    }

    /**
     * Whether the back camera should be opened.
     *
     * @return Whether the back camera should be opened.
     */
    public boolean isFacingBack() {
        return getCameraFacing() == OneCamera.Facing.BACK;
    }

    /**
     * Whether the front camera should be opened.
     *
     * @return Whether the front camera should be opened.
     */
    public boolean isFacingFront() {
        return getCameraFacing() == OneCamera.Facing.FRONT;
    }


    public boolean isFacingExternal() {
        return getCameraFacing() == OneCamera.Facing.EXTERNAL;
    }

    /**
     * Gets the current camera facing in the setting.
     *
     * @return The current camera facing in the setting.
     */
    public OneCamera.Facing getCameraFacing() {
        final int cameraId = mSettingsManager.getInteger(mSettingScope, mCameraFacingSettingKey);
        if (cameraId == mCameraFacingBackValue) {
            return OneCamera.Facing.BACK;
        } else if (cameraId == mCameraFacingFrontValue) {
            return OneCamera.Facing.FRONT;
        } else if (cameraId == mCameraFacingExternalValue) {
            return OneCamera.Facing.EXTERNAL;
        } else {
            return getDefaultCameraFacing();
        }
    }

    /**
     * Sets the camera facing setting.
     *
     * @param cameraFacing The new camera facing.
     */
    public void setCameraFacing(OneCamera.Facing cameraFacing) {
        final int cameraId = (cameraFacing == OneCamera.Facing.BACK) ?
                mCameraFacingBackValue :
                ((cameraFacing == OneCamera.Facing.FRONT) ?
                 mCameraFacingFrontValue: mCameraFacingExternalValue);
        mSettingsManager.set(mSettingScope, mCameraFacingSettingKey, cameraId);
    }

    private OneCamera.Facing getDefaultCameraFacing() {
        if (mCameraFacingDefaultValue == mCameraFacingBackValue) {
            return OneCamera.Facing.BACK;
        }
        if (mCameraFacingDefaultValue == mCameraFacingFrontValue) {
            return OneCamera.Facing.FRONT;
        } else {
            return OneCamera.Facing.EXTERNAL;
        }
    }
}
