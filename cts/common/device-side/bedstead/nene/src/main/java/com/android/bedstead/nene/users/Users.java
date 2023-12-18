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

import static android.os.Build.VERSION.SDK_INT;
import static android.os.Process.myUserHandle;

import static com.android.bedstead.nene.users.UserType.MANAGED_PROFILE_TYPE_NAME;
import static com.android.bedstead.nene.users.UserType.SECONDARY_USER_TYPE_NAME;
import static com.android.bedstead.nene.users.UserType.SYSTEM_USER_TYPE_NAME;

import android.os.Build;
import android.os.UserHandle;

import androidx.annotation.CheckResult;
import androidx.annotation.Nullable;

import com.android.bedstead.nene.TestApis;
import com.android.bedstead.nene.exceptions.AdbException;
import com.android.bedstead.nene.exceptions.AdbParseException;
import com.android.bedstead.nene.exceptions.NeneException;
import com.android.bedstead.nene.utils.ShellCommand;
import com.android.compatibility.common.util.PollingCheck;

import java.util.Collection;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Iterator;
import java.util.Map;
import java.util.Set;
import java.util.concurrent.atomic.AtomicReference;
import java.util.function.Function;
import java.util.stream.Collectors;

public final class Users {

    static final int SYSTEM_USER_ID = 0;
    private static final long WAIT_FOR_USER_TIMEOUT_MS = 1000 * 60;

    private Map<Integer, User> mCachedUsers = null;
    private Map<String, UserType> mCachedUserTypes = null;
    private Set<UserType> mCachedUserTypeValues = null;
    private final AdbUserParser mParser;
    private final TestApis mTestApis;

    public Users(TestApis testApis) {
        mTestApis = testApis;
        mParser = AdbUserParser.get(mTestApis, SDK_INT);
    }

    /** Get all {@link User}s on the device. */
    public Collection<User> all() {
        fillCache();

        return mCachedUsers.values();
    }

    /** Get a {@link UserReference} for the user running the current test process. */
    public UserReference instrumented() {
        return find(myUserHandle());
    }

    /** Get a {@link UserReference} for the system user. */
    public UserReference system() {
        return find(0);
    }

    /** Get a {@link UserReference} by {@code id}. */
    public UserReference find(int id) {
        return new UnresolvedUser(mTestApis, id);
    }

    /** Get a {@link UserReference} by {@code userHandle}. */
    public UserReference find(UserHandle userHandle) {
        return new UnresolvedUser(mTestApis, userHandle.getIdentifier());
    }

    @Nullable
    User fetchUser(int id) {
        // TODO(scottjonathan): fillCache probably does more than we need here -
        //  can we make it more efficient?
        fillCache();

        return mCachedUsers.get(id);
    }

    /** Get all supported {@link UserType}s. */
    public Set<UserType> supportedTypes() {
        ensureSupportedTypesCacheFilled();
        return mCachedUserTypeValues;
    }

    /** Get a {@link UserType} with the given {@code typeName}, or {@code null} */
    public UserType supportedType(String typeName) {
        ensureSupportedTypesCacheFilled();
        return mCachedUserTypes.get(typeName);
    }

    /**
     * Find all users which have the given {@link UserType}.
     */
    public Set<UserReference> findUsersOfType(UserType userType) {
        if (userType == null) {
            throw new NullPointerException();
        }

        if (userType.baseType().contains(UserType.BaseType.PROFILE)) {
            throw new NeneException("Cannot use findUsersOfType with profile type " + userType);
        }

        return all().stream()
                .filter(u -> u.type().equals(userType))
                .collect(Collectors.toSet());
    }

    /**
     * Find a single user which has the given {@link UserType}.
     *
     * <p>If there are no users of the given type, {@code Null} will be returned.
     *
     * <p>If there is more than one user of the given type, {@link NeneException} will be thrown.
     */
    @Nullable
    public UserReference findUserOfType(UserType userType) {
        Set<UserReference> users = findUsersOfType(userType);

        if (users.isEmpty()) {
            return null;
        } else if (users.size() > 1) {
            throw new NeneException("findUserOfType called but there is more than 1 user of type "
                    + userType + ". Found: " + users);
        }

        return users.iterator().next();
    }

    /**
     * Find all users which have the given {@link UserType} and the given parent.
     */
    public Set<UserReference> findProfilesOfType(UserType userType, UserReference parent) {
        if (userType == null || parent == null) {
            throw new NullPointerException();
        }

        if (!userType.baseType().contains(UserType.BaseType.PROFILE)) {
            throw new NeneException("Cannot use findProfilesOfType with non-profile type "
                    + userType);
        }

        return all().stream()
                .filter(u -> parent.equals(u.parent())
                        && u.type().equals(userType))
                .collect(Collectors.toSet());
    }

    /**
     * Find all users which have the given {@link UserType} and the given parent.
     *
     * <p>If there are no users of the given type and parent, {@code Null} will be returned.
     *
     * <p>If there is more than one user of the given type and parent, {@link NeneException} will
     * be thrown.
     */
    @Nullable
    public UserReference findProfileOfType(UserType userType, UserReference parent) {
        Set<UserReference> profiles = findProfilesOfType(userType, parent);

        if (profiles.isEmpty()) {
            return null;
        } else if (profiles.size() > 1) {
            throw new NeneException("findProfileOfType called but there is more than 1 user of "
                    + "type " + userType + " with parent " + parent + ". Found: " + profiles);
        }

        return profiles.iterator().next();
    }

    private void ensureSupportedTypesCacheFilled() {
        if (mCachedUserTypes != null) {
            // SupportedTypes don't change so don't need to be refreshed
            return;
        }
        if (SDK_INT < Build.VERSION_CODES.R) {
            mCachedUserTypes = new HashMap<>();
            mCachedUserTypes.put(MANAGED_PROFILE_TYPE_NAME, managedProfileUserType());
            mCachedUserTypes.put(SYSTEM_USER_TYPE_NAME, systemUserType());
            mCachedUserTypes.put(SECONDARY_USER_TYPE_NAME, secondaryUserType());
            mCachedUserTypeValues = new HashSet<>();
            mCachedUserTypeValues.addAll(mCachedUserTypes.values());
            return;
        }

        fillCache();
    }

    private UserType managedProfileUserType() {
        UserType.MutableUserType managedProfileMutableUserType = new UserType.MutableUserType();
        managedProfileMutableUserType.mName = MANAGED_PROFILE_TYPE_NAME;
        managedProfileMutableUserType.mBaseType = Set.of(UserType.BaseType.PROFILE);
        managedProfileMutableUserType.mEnabled = true;
        managedProfileMutableUserType.mMaxAllowed = -1;
        managedProfileMutableUserType.mMaxAllowedPerParent = 1;
        return new UserType(managedProfileMutableUserType);
    }

    private UserType systemUserType() {
        UserType.MutableUserType managedProfileMutableUserType = new UserType.MutableUserType();
        managedProfileMutableUserType.mName = SYSTEM_USER_TYPE_NAME;
        managedProfileMutableUserType.mBaseType =
                Set.of(UserType.BaseType.FULL, UserType.BaseType.SYSTEM);
        managedProfileMutableUserType.mEnabled = true;
        managedProfileMutableUserType.mMaxAllowed = -1;
        managedProfileMutableUserType.mMaxAllowedPerParent = -1;
        return new UserType(managedProfileMutableUserType);
    }

    private UserType secondaryUserType() {
        UserType.MutableUserType managedProfileMutableUserType = new UserType.MutableUserType();
        managedProfileMutableUserType.mName = SECONDARY_USER_TYPE_NAME;
        managedProfileMutableUserType.mBaseType = Set.of(UserType.BaseType.FULL);
        managedProfileMutableUserType.mEnabled = true;
        managedProfileMutableUserType.mMaxAllowed = -1;
        managedProfileMutableUserType.mMaxAllowedPerParent = -1;
        return new UserType(managedProfileMutableUserType);
    }

    /**
     * Create a new user.
     */
    @CheckResult
    public UserBuilder createUser() {
        return new UserBuilder(mTestApis);
    }

    /**
     * Get a {@link UserReference} to a user who does not exist.
     */
    public UserReference nonExisting() {
        fillCache();
        int id = 0;

        while (mCachedUsers.get(id) != null) {
            id++;
        }

        return new UnresolvedUser(mTestApis, id);
    }

    private void fillCache() {
        try {
            // TODO: Replace use of adb on supported versions of Android
            String userDumpsysOutput = ShellCommand.builder("dumpsys user").execute();
            AdbUserParser.ParseResult result = mParser.parse(userDumpsysOutput);

            mCachedUsers = result.mUsers;
            if (result.mUserTypes != null) {
                mCachedUserTypes = result.mUserTypes;
            } else {
                ensureSupportedTypesCacheFilled();
            }

            Iterator<Map.Entry<Integer, User>> iterator = mCachedUsers.entrySet().iterator();

            while (iterator.hasNext()) {
                Map.Entry<Integer, User> entry = iterator.next();

                if (entry.getValue().isRemoving()) {
                    // We don't expose users who are currently being removed
                    iterator.remove();
                    continue;
                }

                User.MutableUser mutableUser = entry.getValue().mMutableUser;

                if (SDK_INT < Build.VERSION_CODES.R) {
                    if (entry.getValue().id() == SYSTEM_USER_ID) {
                        mutableUser.mType = supportedType(SYSTEM_USER_TYPE_NAME);
                        mutableUser.mIsPrimary = true;
                    } else if (entry.getValue().hasFlag(User.FLAG_MANAGED_PROFILE)) {
                        mutableUser.mType =
                                supportedType(MANAGED_PROFILE_TYPE_NAME);
                        mutableUser.mIsPrimary = false;
                    } else {
                        mutableUser.mType =
                                supportedType(SECONDARY_USER_TYPE_NAME);
                        mutableUser.mIsPrimary = false;
                    }
                }

                if (SDK_INT < Build.VERSION_CODES.S) {
                    if (mutableUser.mType.baseType()
                            .contains(UserType.BaseType.PROFILE)) {
                        // We assume that all profiles before S were on the System User
                        mutableUser.mParent = find(SYSTEM_USER_ID);
                    }
                }
            }

            mCachedUserTypeValues = new HashSet<>();
            mCachedUserTypeValues.addAll(mCachedUserTypes.values());

        } catch (AdbException | AdbParseException e) {
            throw new RuntimeException("Error filling cache", e);
        }
    }

    /**
     * Block until the user with the given {@code userReference} exists and is in the correct state.
     *
     * <p>If this cannot be met before a timeout, a {@link NeneException} will be thrown.
     */
    User waitForUserToMatch(UserReference userReference, Function<User, Boolean> userChecker) {
        return waitForUserToMatch(userReference, userChecker, /* waitForExist= */ true);
    }

    /**
     * Block until the user with the given {@code userReference} to not exist or to be in the
     * correct state.
     *
     * <p>If this cannot be met before a timeout, a {@link NeneException} will be thrown.
     */
    @Nullable
    User waitForUserToNotExistOrMatch(
            UserReference userReference, Function<User, Boolean> userChecker) {
        return waitForUserToMatch(userReference, userChecker, /* waitForExist= */ false);
    }

    @Nullable
    private User waitForUserToMatch(
            UserReference userReference, Function<User, Boolean> userChecker,
            boolean waitForExist) {
        // TODO(scottjonathan): This is pretty heavy because we resolve everything when we know we
        //  are throwing away everything except one user. Optimise
        try {
            AtomicReference<User> returnUser = new AtomicReference<>();
            PollingCheck.waitFor(WAIT_FOR_USER_TIMEOUT_MS, () -> {
                User user = userReference.resolve();
                returnUser.set(user);
                if (user == null) {
                    return !waitForExist;
                }
                return userChecker.apply(user);
            });
            return returnUser.get();
        } catch (AssertionError e) {
            User user = userReference.resolve();

            if (user == null) {
                throw new NeneException(
                        "Timed out waiting for user state for user "
                                + userReference + ". User does not exist.", e);
            }
            throw new NeneException(
                    "Timed out waiting for user state, current state " + user, e
            );
        }
    }
}
