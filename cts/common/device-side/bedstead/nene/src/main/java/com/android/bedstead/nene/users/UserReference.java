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

package com.android.bedstead.nene.users;

import static android.Manifest.permission.INTERACT_ACROSS_USERS_FULL;

import android.content.Intent;
import android.os.UserHandle;

import com.android.bedstead.nene.TestApis;
import com.android.bedstead.nene.exceptions.AdbException;
import com.android.bedstead.nene.exceptions.NeneException;
import com.android.bedstead.nene.permissions.PermissionContext;
import com.android.bedstead.nene.users.User.UserState;
import com.android.bedstead.nene.utils.ShellCommand;
import com.android.bedstead.nene.utils.ShellCommandUtils;
import com.android.compatibility.common.util.BlockingBroadcastReceiver;

import javax.annotation.Nullable;

/**
 * A representation of a User on device which may or may not exist.
 *
 * <p>To resolve the user into a {@link User}, see {@link #resolve()}.
 */
public abstract class UserReference implements AutoCloseable {

    private final TestApis mTestApis;
    private final int mId;

    UserReference(TestApis testApis, int id) {
        if (testApis == null) {
            throw new NullPointerException();
        }
        mTestApis = testApis;
        mId = id;
    }

    public final int id() {
        return mId;
    }

    /**
     * Get a {@link UserHandle} for the {@link #id()}.
     */
    public final UserHandle userHandle() {
        return UserHandle.of(mId);
    }

    /**
     * Get the current state of the {@link User} from the device, or {@code null} if the user does
     * not exist.
     */
    @Nullable
    public final User resolve() {
        return mTestApis.users().fetchUser(mId);
    }

    /**
     * Remove the user from the device.
     *
     * <p>If the user does not exist, or the removal fails for any other reason, a
     * {@link NeneException} will be thrown.
     */
    public final void remove() {
        // TODO(scottjonathan): There's a potential issue here as when the user is marked as
        //  "is removing" the DPC still can't be uninstalled because it's set as the profile owner.
        try {
            // Expected success string is "Success: removed user"
            ShellCommand.builder("pm remove-user")
                    .addOperand(mId)
                    .validate(ShellCommandUtils::startsWithSuccess)
                    .execute();
            mTestApis.users().waitForUserToNotExistOrMatch(this, User::isRemoving);
        } catch (AdbException e) {
            throw new NeneException("Could not remove user " + this, e);
        }
    }

    /**
     * Start the user.
     *
     * <p>After calling this command, the user will be in the {@link UserState#RUNNING_UNLOCKED}
     * state.
     *
     * <p>If the user does not exist, or the start fails for any other reason, a
     * {@link NeneException} will be thrown.
     */
    //TODO(scottjonathan): Deal with users who won't unlock
    public UserReference start() {
        try {
            // Expected success string is "Success: user started"
            ShellCommand.builder("am start-user")
                    .addOperand(mId)
                    .addOperand("-w")
                    .validate(ShellCommandUtils::startsWithSuccess)
                    .execute();
            User waitedUser = mTestApis.users().waitForUserToNotExistOrMatch(
                    this, (user) -> user.state() == UserState.RUNNING_UNLOCKED);
            if (waitedUser == null) {
                throw new NeneException("User does not exist " + this);
            }
        } catch (AdbException e) {
            throw new NeneException("Could not start user " + this, e);
        }

        return this;
    }

    /**
     * Stop the user.
     *
     * <p>After calling this command, the user will be in the {@link UserState#NOT_RUNNING} state.
     */
    public UserReference stop() {
        try {
            // Expects no output on success or failure - stderr output on failure
            ShellCommand.builder("am stop-user")
                    .addOperand("-f") // Force stop
                    .addOperand(mId)
                    .allowEmptyOutput(true)
                    .validate(String::isEmpty)
                    .execute();
            User waitedUser = mTestApis.users().waitForUserToNotExistOrMatch(
                    this, (user) -> user.state() == UserState.NOT_RUNNING);
            if (waitedUser == null) {
                throw new NeneException("User does not exist " + this);
            }
        } catch (AdbException e) {
            throw new NeneException("Could not stop user " + this, e);
        }

        return this;
    }

    /**
     * Make the user the foreground user.
     */
    public UserReference switchTo() {
        BlockingBroadcastReceiver broadcastReceiver =
                new BlockingBroadcastReceiver(mTestApis.context().instrumentedContext(),
                        Intent.ACTION_USER_FOREGROUND,
                        (intent) ->((UserHandle)
                                intent.getParcelableExtra(Intent.EXTRA_USER))
                                .getIdentifier() == mId);

        try {
            try (PermissionContext p =
                         mTestApis.permissions().withPermission(INTERACT_ACROSS_USERS_FULL)) {
                broadcastReceiver.registerForAllUsers();
            }

            // Expects no output on success or failure
            ShellCommand.builder("am switch-user")
                    .addOperand(mId)
                    .allowEmptyOutput(true)
                    .validate(String::isEmpty)
                    .execute();

            broadcastReceiver.awaitForBroadcast();
        } catch (AdbException e) {
            throw new NeneException("Could not switch to user", e);
        } finally {
            broadcastReceiver.unregisterQuietly();
        }

        return this;
    }

    @Override
    public boolean equals(Object obj) {
        if (!(obj instanceof UserReference)) {
            return false;
        }

        UserReference other = (UserReference) obj;

        return other.id() == id();
    }

    @Override
    public int hashCode() {
        return id();
    }

    /** See {@link #remove}. */
    @Override
    public void close() {
        remove();
    }
}
