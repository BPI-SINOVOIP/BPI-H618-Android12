/*
 * Copyright (C) 2020 The Android Open Source Project
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

package com.android.car.user;

import android.annotation.NonNull;
import android.annotation.Nullable;
import android.content.pm.UserInfo;
import android.os.UserManager;
import android.sysprop.CarProperties;
import android.util.EventLog;
import android.util.Slog;
import android.util.SparseBooleanArray;

import com.android.car.CarLog;
import com.android.car.internal.common.EventLogTags;
import com.android.internal.annotations.VisibleForTesting;

import java.util.ArrayList;
import java.util.List;

/**
 * Manages the pre-created users.
 *
 * <p>
 * It creates (and remove) new pre-created users based on the values of {@link CarProperties} and
 * the number of existing pre-created users.
 */
public final class UserPreCreator {

    private static final String TAG = CarLog.tagFor(UserPreCreator.class);

    private final UserManager mUserManager;

    UserPreCreator(UserManager userManager) {
        mUserManager = userManager;
    }

    /**
     * Manages the required number of pre-created users.
     */
    public void managePreCreatedUsers() {
        // First gets how many pre-createad users are defined by the OEM
        int numberRequestedGuests = CarProperties.number_pre_created_guests().orElse(0);
        int numberRequestedUsers = CarProperties.number_pre_created_users().orElse(0);
        EventLog.writeEvent(EventLogTags.CAR_USER_SVC_PRE_CREATION_REQUESTED, numberRequestedUsers,
                numberRequestedGuests);
        Slog.d(TAG, "managePreCreatedUsers(): OEM asked for " + numberRequestedGuests
                + " guests and " + numberRequestedUsers + " users");

        if (numberRequestedGuests < 0 || numberRequestedUsers < 0) {
            Slog.w(TAG, "preCreateUsers(): invalid values provided by OEM; "
                    + "number_pre_created_guests=" + numberRequestedGuests
                    + ", number_pre_created_users=" + numberRequestedUsers);
            return;
        }

        // Then checks how many exist already
        List<UserInfo> allUsers = mUserManager.getUsers(/* excludePartial= */ true,
                /* excludeDying= */ true, /* excludePreCreated= */ false);

        int allUsersSize = allUsers.size();
        Slog.d(TAG, "preCreateUsers: total users size is " + allUsersSize);

        int numberExistingGuests = 0;
        int numberExistingUsers = 0;

        // List of pre-created users that were not properly initialized. Typically happens when
        // the system crashed / rebooted before they were fully started.
        SparseBooleanArray invalidPreCreatedUsers = new SparseBooleanArray();

        // List of all pre-created users - it will be used to remove unused ones (when needed)
        SparseBooleanArray existingPrecreatedUsers = new SparseBooleanArray();

        // List of extra pre-created users and guests - they will be removed
        List<Integer> extraPreCreatedUsers = new ArrayList<>();

        for (int i = 0; i < allUsersSize; i++) {
            UserInfo user = allUsers.get(i);
            if (!user.preCreated) continue;
            if (!user.isInitialized()) {
                Slog.w(TAG, "Found invalid pre-created user that needs to be removed: "
                        + user.toFullString());
                invalidPreCreatedUsers.append(user.id, /* notUsed=*/ true);
                continue;
            }
            boolean isGuest = user.isGuest();
            existingPrecreatedUsers.put(user.id, isGuest);
            if (isGuest) {
                numberExistingGuests++;
                if (numberExistingGuests > numberRequestedGuests) {
                    extraPreCreatedUsers.add(user.id);
                }
            } else {
                numberExistingUsers++;
                if (numberExistingUsers > numberRequestedUsers) {
                    extraPreCreatedUsers.add(user.id);
                }
            }
        }
        Slog.i(TAG, "managePreCreatedUsers(): system already has " + numberExistingGuests
                + " pre-created guests," + numberExistingUsers + " pre-created users, and these"
                + " invalid users: " + invalidPreCreatedUsers
                + " and these extra pre-created users: " + extraPreCreatedUsers);

        int numberGuestsToAdd = numberRequestedGuests - numberExistingGuests;
        int numberUsersToAdd = numberRequestedUsers - numberExistingUsers;
        int numberGuestsToRemove = numberExistingGuests - numberRequestedGuests;
        int numberUsersToRemove = numberExistingUsers - numberRequestedUsers;
        int numberInvalidUsersToRemove = invalidPreCreatedUsers.size();

        EventLog.writeEvent(EventLogTags.CAR_USER_SVC_PRE_CREATION_STATUS,
                numberExistingUsers, numberUsersToAdd, numberUsersToRemove,
                numberExistingGuests, numberGuestsToAdd, numberGuestsToRemove,
                numberInvalidUsersToRemove);

        if (numberGuestsToAdd == 0 && numberUsersToAdd == 0 && numberInvalidUsersToRemove == 0) {
            Slog.i(TAG, "managePreCreatedUsers(): everything in sync");
            return;
        }

        if (numberUsersToAdd > 0) {
            preCreateUsers(numberUsersToAdd, /* isGuest= */ false);
        }
        if (numberGuestsToAdd > 0) {
            preCreateUsers(numberGuestsToAdd, /* isGuest= */ true);
        }

        int totalNumberToRemove = extraPreCreatedUsers.size();
        Slog.d(TAG, "Must delete " + totalNumberToRemove + " pre-created users");
        if (totalNumberToRemove > 0) {
            int[] usersToRemove = new int[totalNumberToRemove];
            for (int i = 0; i < totalNumberToRemove; i++) {
                usersToRemove[i] = extraPreCreatedUsers.get(i);
            }
            removePreCreatedUsers(usersToRemove);
        }

        if (numberInvalidUsersToRemove > 0) {
            for (int i = 0; i < numberInvalidUsersToRemove; i++) {
                int userId = invalidPreCreatedUsers.keyAt(i);
                Slog.w(TAG, "removing invalid pre-created user " + userId);
                mUserManager.removeUser(userId);
            }
        }
    }

    private void preCreateUsers(int size, boolean isGuest) {
        String msg = isGuest ? "preCreateGuests-" + size : "preCreateUsers-" + size;
        Slog.d(TAG, "preCreateUsers: " + msg);
        for (int i = 1; i <= size; i++) {
            UserInfo preCreated = preCreateUsers(isGuest);
            if (preCreated == null) {
                Slog.w(TAG, "Could not pre-create" + (isGuest ? " guest" : "")
                        + " user #" + i);
                continue;
            }
        }
    }

    @VisibleForTesting
    @Nullable
    UserInfo preCreateUsers(boolean isGuest) {
        String traceMsg = "pre-create" + (isGuest ? "-guest" : "-user");
        // NOTE: we want to get rid of UserManagerHelper, so let's call UserManager directly
        String userType =
                isGuest ? UserManager.USER_TYPE_FULL_GUEST : UserManager.USER_TYPE_FULL_SECONDARY;
        UserInfo user = null;
        try {
            user = mUserManager.preCreateUser(userType);
            if (user == null) {
                logPrecreationFailure(traceMsg, /* cause= */ null);
            }
        } catch (Exception e) {
            logPrecreationFailure(traceMsg, e);
        }
        return user;
    }

    private void removePreCreatedUsers(int[] usersToRemove) {
        for (int userId : usersToRemove) {
            Slog.i(TAG, "removing pre-created user with id " + userId);
            mUserManager.removeUser(userId);
        }
    }

    /**
     * Logs proper message when user pre-creation fails (most likely because there are too many).
     */
    @VisibleForTesting
    void logPrecreationFailure(@NonNull String operation, @Nullable Exception cause) {
        int maxNumberUsers = UserManager.getMaxSupportedUsers();
        int currentNumberUsers = mUserManager.getUserCount();
        String message = new StringBuilder(operation.length() + 100)
                .append(operation).append(" failed. Number users: ").append(currentNumberUsers)
                .append(" Max: ").append(maxNumberUsers).toString();
        if (cause == null) {
            Slog.w(TAG, message);
        } else {
            Slog.w(TAG, message, cause);
        }
    }
}
