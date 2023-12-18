/*
 * Copyright (C) 2021 The Android Open Source Project
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
package android.platform.test.rule;

import static android.content.res.Configuration.ORIENTATION_LANDSCAPE;

import static org.junit.Assert.assertEquals;

import android.os.RemoteException;

import org.junit.runner.Description;

/**
 * Locks landscape orientation before running a test and goes back to natural orientation
 * afterwards.
 */
public class LandscapeOrientationRule extends TestWatcher {

    @Override
    protected void starting(Description description) {
        try {
            getUiDevice().setOrientationNatural();
            int currentOrientation = getContext().getResources().getConfiguration().orientation;
            if (currentOrientation != ORIENTATION_LANDSCAPE) { // ORIENTATION_PORTRAIT
                getUiDevice().setOrientationLeft();
                int rotatedOrientation = getContext().getResources().getConfiguration().orientation;
                assertEquals(
                        "Orientation should be landscape",
                        ORIENTATION_LANDSCAPE,
                        rotatedOrientation);
            }
        } catch (RemoteException e) {
            String message = "RemoteException when forcing landscape rotation on the device";
            throw new RuntimeException(message, e);
        }
    }

    @Override
    protected void finished(Description description) {
        try {
            if (!getUiDevice().isNaturalOrientation()) {
                getUiDevice().setOrientationNatural();
            }
            getUiDevice().unfreezeRotation();
        } catch (RemoteException e) {
            String message = "RemoteException when restoring natural rotation of the device";
            throw new RuntimeException(message, e);
        }
    }
}
