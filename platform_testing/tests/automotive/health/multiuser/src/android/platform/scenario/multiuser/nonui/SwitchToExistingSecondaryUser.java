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

package android.platform.scenario.multiuser;

import android.content.pm.UserInfo;
import android.os.SystemClock;
import android.platform.helpers.MultiUserHelper;
import android.platform.test.scenario.annotation.Scenario;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

/**
 * This test will switch to an existing secondary non-guest user from default initial user.
 *
 * <p>It should be running under user 0, otherwise instrumentation may be killed after user
 * switched.
 */
@Scenario
@RunWith(JUnit4.class)
public class SwitchToExistingSecondaryUser {

    private final MultiUserHelper mMultiUserHelper = MultiUserHelper.getInstance();
    private int mTargetUserId;

    @Before
    public void setup() throws Exception {
    /*
    TODO: Create setup util API
     */
        UserInfo targetUser = mMultiUserHelper
            .getUserByName(MultiUserConstants.SECONDARY_USER_NAME);
        if (targetUser == null) {
            // Create new user and switch to it for the first time
            mTargetUserId = mMultiUserHelper
                .createUser(MultiUserConstants.SECONDARY_USER_NAME, false);
            // In order to skip reporting the duration for the first time a user is created,
            // always switch to newly created user for the first time it is created during setup.
            mMultiUserHelper.switchAndWaitForStable(
                mTargetUserId, MultiUserConstants.WAIT_FOR_IDLE_TIME_MS);
        }
        UserInfo currentUser = mMultiUserHelper.getCurrentForegroundUserInfo();
        if (currentUser.id != MultiUserConstants.DEFAULT_INITIAL_USER) {
            SystemClock.sleep(MultiUserConstants.WAIT_FOR_IDLE_TIME_MS);
            mMultiUserHelper.switchAndWaitForStable(
                MultiUserConstants.DEFAULT_INITIAL_USER, MultiUserConstants.WAIT_FOR_IDLE_TIME_MS);
        }
    }

    @Test
    public void testSwitch() throws Exception {
        mMultiUserHelper.switchToUserId(mTargetUserId);
    }
}
