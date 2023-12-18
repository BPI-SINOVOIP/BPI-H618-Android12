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
import android.platform.helpers.exceptions.UnknownUiException;
import android.support.test.uiautomator.By;
import android.support.test.uiautomator.UiObject2;

import android.os.SystemClock;

public class ContactDetailsHelperImpl extends AbstractAutoStandardAppHelper
        implements IAutoDialContactDetailsHelper {
    private static final String LOG_TAG = DialHelperImpl.class.getSimpleName();

    public ContactDetailsHelperImpl(Instrumentation instr) {
        super(instr);
    }

    /** {@inheritDoc} */
    @Override
    public String getPackage() {
        return getApplicationConfig(AutoConfigConstants.DIAL_PACKAGE);
    }

    /** {@inheritDoc} */
    public void open() {
        pressHome();
        waitForIdle();
        String phone_launch_command =
                "am start -n " + getApplicationConfig(AutoConfigConstants.PHONE_ACTIVITY);
        executeShellCommand(phone_launch_command);
    }

    /** {@inheritDoc} */
    @Override
    public String getLauncherName() {
        return "Phone";
    }

    /** {@inheritDoc} */
    public void addRemoveFavoriteContact() {
        UiObject2 favoriteButton =
                findUiObject(
                        getResourceFromConfig(
                                AutoConfigConstants.PHONE,
                                AutoConfigConstants.CONTACTS_VIEW,
                                AutoConfigConstants.ADD_CONTACT_TO_FAVORITE));
        if (favoriteButton != null) {
            clickAndWaitForIdleScreen(favoriteButton);
        } else {
            throw new UnknownUiException("Unable to find add favorite button");
        }
    }

    /** {@inheritDoc} */
    public void makeCallFromDetailsPageByType(ContactType contactType) {
        UiObject2 number = null;
        switch (contactType) {
            case HOME:
                number =
                        findUiObject(
                                getResourceFromConfig(
                                        AutoConfigConstants.PHONE,
                                        AutoConfigConstants.CONTACTS_VIEW,
                                        AutoConfigConstants.CONTACT_TYPE_HOME));
                break;
            case WORK:
                number =
                        findUiObject(
                                getResourceFromConfig(
                                        AutoConfigConstants.PHONE,
                                        AutoConfigConstants.CONTACTS_VIEW,
                                        AutoConfigConstants.CONTACT_TYPE_WORK));
                break;
            case MOBILE:
                number =
                        findUiObject(
                                getResourceFromConfig(
                                        AutoConfigConstants.PHONE,
                                        AutoConfigConstants.CONTACTS_VIEW,
                                        AutoConfigConstants.CONTACT_TYPE_MOBILE));
                break;
            default:
                number = findUiObject(By.text("undefined"));
        }
        if (number != null) {
            clickAndWaitForIdleScreen(number.getParent());
        } else {
            throw new UnknownUiException("Unable to find number in contact details");
        }
    }

    /** {@inheritDoc} */
    public void closeDetailsPage() {
        // count is used to avoid infinite loop in case someone invokes
        // after exiting settings application
        int count = 5;
        while (count > 0
                && findUiObject(
                                By.clickable(true)
                                        .hasDescendant(
                                                getResourceFromConfig(
                                                        AutoConfigConstants.PHONE,
                                                        AutoConfigConstants.DIAL_PAD_VIEW,
                                                        AutoConfigConstants.DIAL_PAD_MENU)))
                        == null) {
            pressBack();
            SystemClock.sleep(3000); // to avoid stale object error
            count--;
        }
    }
}
