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

public class HomeHelperImpl extends AbstractAutoStandardAppHelper implements IAutoHomeHelper {

    private static final int UI_RESPONSE_WAIT_WC = 5000;

    public HomeHelperImpl(Instrumentation instr) {
        super(instr);
    }

    public boolean hasMapWidget() {
        return (findUiObject(
                        getResourceFromConfig(
                                AutoConfigConstants.HOME,
                                AutoConfigConstants.HOME_VIEW,
                                AutoConfigConstants.MAP_CARD))
                != null);
    }

    public boolean hasAssistantWidget() {
        return (findUiObject(
                        getResourceFromConfig(
                                AutoConfigConstants.HOME,
                                AutoConfigConstants.HOME_VIEW,
                                AutoConfigConstants.TOP_CARD))
                != null);
    }

    public boolean hasMediaWidget() {
        return (findUiObject(
                        getResourceFromConfig(
                                AutoConfigConstants.HOME,
                                AutoConfigConstants.HOME_VIEW,
                                AutoConfigConstants.BOTTOM_CARD))
                != null);
    }

    /** {@inheritDoc} */
    @Override
    public void open() {
        pressHome();
        waitForIdle();
    }

    /** {@inheritDoc} */
    @Override
    public String getPackage() {
        return getApplicationConfig(AutoConfigConstants.HOME_PACKAGE);
    }
}
