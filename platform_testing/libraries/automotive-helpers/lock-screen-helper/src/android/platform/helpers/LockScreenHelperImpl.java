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

package android.platform.helpers;

import android.app.Instrumentation;
import android.os.SystemClock;
import android.support.test.uiautomator.By;
import android.support.test.uiautomator.BySelector;
import android.support.test.uiautomator.UiObject2;
import android.support.test.uiautomator.Until;

public class LockScreenHelperImpl extends AbstractAutoStandardAppHelper
        implements IAutoLockScreenHelper {
    private static final int DEFAULT_WAIT_TIME = 5000;
    private static final int THREE_SECOND_WAIT_TIME = 3000;

    private static final String UNLOCK_BY = "input text %s";

    private HelperAccessor<IAutoSecuritySettingsHelper> mSecuritySettingsHelper;
    private HelperAccessor<IAutoSettingHelper> mSettingHelper;

    public LockScreenHelperImpl(Instrumentation instr) {
        super(instr);
        mSecuritySettingsHelper = new HelperAccessor<>(IAutoSecuritySettingsHelper.class);
        mSettingHelper = new HelperAccessor<>(IAutoSettingHelper.class);
    }

    /** {@inheritDoc} */
    @Override
    public String getPackage() {
        return getApplicationConfig(AutoConfigConstants.LOCK_SCREEN_PACKAGE);
    }

    @Override
    public void lockScreenBy(LockType lockType, String credential) {
        if (lockType == LockType.PIN) {
            mSecuritySettingsHelper.get().setLockByPin(credential);
        } else {
            mSecuritySettingsHelper.get().setLockByPassword(credential);
        }
        pressPowerButton();
    }

    @Override
    public void unlockScreenBy(LockType lockType, String credential) {
        pressPowerButton();
        if (lockType == LockType.PIN) {
            unlockByPin(credential);
        } else {
            unlockByPassword(credential);
        }
    }

    private void unlockByPassword(String password) {
        executeShellCommand(String.format(UNLOCK_BY, password));
        SystemClock.sleep(THREE_SECOND_WAIT_TIME);
        pressEnter();
        AutoUtility.exitSuw();
        mSettingHelper.get().openSetting(AutoConfigConstants.SECURITY_SETTINGS);
    }

    private void unlockByPin(String pin) {
        selectPinOnPinPad(pin);
        UiObject2 enter_button =
                findUiObject(
                        getResourceFromConfig(
                                AutoConfigConstants.LOCK_SCREEN,
                                AutoConfigConstants.LOCK_SCREEN_VIEW,
                                AutoConfigConstants.ENTER_KEY));
        clickAndWaitForIdleScreen(enter_button);
        UiObject2 pinPad =
                findUiObject(
                        getResourceFromConfig(
                                AutoConfigConstants.LOCK_SCREEN,
                                AutoConfigConstants.LOCK_SCREEN_VIEW,
                                AutoConfigConstants.PIN_PAD));
        SystemClock.sleep(THREE_SECOND_WAIT_TIME);
    }

    private void selectPinOnPinPad(String pin) {
        int length = pin.length();
        for (int i = 0; i < length; i++) {
            char c = pin.charAt(i);
            String resourceId = "key" + c;
            BySelector number_selector =
                    By.res(
                            getApplicationConfig(AutoConfigConstants.LOCK_SCREEN_PACKAGE),
                            resourceId);
            UiObject2 number = mDevice.wait(Until.findObject(number_selector), DEFAULT_WAIT_TIME);
            number.click();
        }
    }
}
