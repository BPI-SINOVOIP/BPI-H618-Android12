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

public interface IAutoProfileHelper extends IAppHelper {

    /**
     * Setup expectation: Profiles and Accounts setting is open.
     *
     * <p>This method is to add an a new user.
     */
    void addProfile();

    /**
     * Setup expectation: Profiles and Accounts setting is open.
     *
     * <p>This method is to add an a new user.
     */
    void addProfileQuickSettings(String userName);

    /**
     * Setup expectation: Profiles and Accounts setting is open.
     *
     * <p>This method is to delete an existing user.
     */
    void deleteProfile(String user);

    /**
     * Setup expectation: Profiles and Accounts setting is open.
     *
     * <p>This method is to delete user's own profile.
     */
    void deleteCurrentProfile();

    /**
     * Setup expectation: Profiles and Accounts setting is open.
     *
     * <p>This method is to check if user already exists.
     */
    boolean isProfilePresent(String userName);

    /**
     * Setup expectation: Profiles and Accounts setting is open.
     *
     * <p>This method is to switch between existing users.
     */
    void switchProfile(String userFrom, String userTo);

    /**
     * Setup expectation: Profiles and Accounts setting is open.
     *
     * <p>This method is to make an existing user admin.
     */
    void makeUserAdmin(String user);
}
