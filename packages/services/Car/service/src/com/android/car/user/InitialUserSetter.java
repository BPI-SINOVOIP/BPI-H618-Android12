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

import static android.car.userlib.UserHalHelper.userFlagsToString;

import static com.android.car.internal.ExcludeFromCodeCoverageGeneratedReport.DUMP_INFO;

import android.annotation.IntDef;
import android.annotation.NonNull;
import android.annotation.Nullable;
import android.annotation.UserIdInt;
import android.app.ActivityManager;
import android.app.IActivityManager;
import android.car.settings.CarSettings;
import android.car.userlib.UserHalHelper;
import android.content.Context;
import android.content.pm.UserInfo;
import android.hardware.automotive.vehicle.V2_0.UserFlags;
import android.os.RemoteException;
import android.os.Trace;
import android.os.UserHandle;
import android.os.UserManager;
import android.provider.Settings;
import android.sysprop.CarProperties;
import android.util.Pair;
import android.util.Slog;
import android.util.TimingsTraceLog;

import com.android.car.CarLog;
import com.android.car.internal.ExcludeFromCodeCoverageGeneratedReport;
import com.android.car.internal.common.UserHelperLite;
import com.android.internal.annotations.VisibleForTesting;
import com.android.internal.util.Preconditions;
import com.android.internal.widget.LockPatternUtils;

import java.io.PrintWriter;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Iterator;
import java.util.List;
import java.util.function.Consumer;

/**
 * Helper used to set the initial Android user on boot or when resuming from RAM.
 */
final class InitialUserSetter {

    private static final String TAG = CarLog.tagFor(InitialUserSetter.class);

    private static final boolean DBG = false;
    private static final int BOOT_USER_NOT_FOUND = -1;

    /**
     * Sets the initial user using the default behavior.
     *
     * <p>The default behavior is:
     *
     * <ol>
     *  <li>On first boot, it creates and switches to a new user.
     *  <li>Otherwise, it will switch to either:
     *  <ol>
     *   <li>User defined by {@code android.car.systemuser.bootuseroverrideid} (when it was
     * constructed with such option enabled).
     *   <li>Last active user (as defined by
     * {@link android.provider.Settings.Global.LAST_ACTIVE_USER_ID}.
     *  </ol>
     * </ol>
     */
    public static final int TYPE_DEFAULT_BEHAVIOR = 0;

    /**
     * Switches to the given user, falling back to {@link #fallbackDefaultBehavior(String)} if it
     * fails.
     */
    public static final int TYPE_SWITCH = 1;

    /**
     * Creates a new user and switches to it, falling back to
     * {@link #fallbackDefaultBehavior(String) if any of these steps fails.
     *
     * @param name (optional) name of the new user
     * @param halFlags user flags as defined by Vehicle HAL ({@code UserFlags} enum).
     */
    public static final int TYPE_CREATE = 2;

    /**
     * Creates a new guest user and switches to it, if current user is unlocked guest user.
     * Does not fallback if any of these steps fails. falling back to
     * {@link #fallbackDefaultBehavior(String) if any of these steps fails
     */
    public static final int TYPE_REPLACE_GUEST = 3;

    @IntDef(prefix = { "TYPE_" }, value = {
            TYPE_DEFAULT_BEHAVIOR,
            TYPE_SWITCH,
            TYPE_CREATE,
            TYPE_REPLACE_GUEST
    })
    @Retention(RetentionPolicy.SOURCE)
    public @interface InitialUserInfoType { }

    private final Context mContext;

    // TODO(b/150413304): abstract AM / UM into interfaces, then provide local and remote
    // implementation (where local is implemented by ActivityManagerInternal / UserManagerInternal)
    private final UserManager mUm;
    private final CarUserService mCarUserService;
    private final LockPatternUtils mLockPatternUtils;

    private final String mNewUserName;
    private final String mNewGuestName;

    private final Consumer<UserInfo> mListener;

    InitialUserSetter(@NonNull Context context, @NonNull CarUserService carUserService,
            @NonNull Consumer<UserInfo> listener) {
        this(context, carUserService, listener, /* newGuestName= */ null);
    }

    InitialUserSetter(@NonNull Context context, @NonNull CarUserService carUserService,
            @NonNull Consumer<UserInfo> listener, @Nullable String newGuestName) {
        this(context, UserManager.get(context), carUserService, listener,
                new LockPatternUtils(context),
                context.getString(com.android.internal.R.string.owner_name), newGuestName);
    }

    @VisibleForTesting
    InitialUserSetter(@NonNull Context context, @NonNull UserManager um,
            @NonNull CarUserService carUserService, @NonNull Consumer<UserInfo> listener,
            @NonNull LockPatternUtils lockPatternUtils, @Nullable String newUserName,
            @Nullable String newGuestName) {
        mContext = context;
        mUm = um;
        mCarUserService = carUserService;
        mListener = listener;
        mLockPatternUtils = lockPatternUtils;
        mNewUserName = newUserName;
        mNewGuestName = newGuestName;
    }

    /**
     * Builder for {@link InitialUserInfo} objects.
     *
     */
    public static final class Builder {

        private final @InitialUserInfoType int mType;
        private boolean mReplaceGuest;
        private @UserIdInt int mSwitchUserId;
        private @Nullable String mNewUserName;
        private int mNewUserFlags;
        private boolean mSupportsOverrideUserIdProperty;
        private @Nullable String mUserLocales;

        /**
         * Constructor for the given type.
         *
         * @param type {@link #TYPE_DEFAULT_BEHAVIOR}, {@link #TYPE_SWITCH},
         * {@link #TYPE_CREATE} or {@link #TYPE_REPLACE_GUEST}.
         */
        public Builder(@InitialUserInfoType int type) {
            Preconditions.checkArgument(type == TYPE_DEFAULT_BEHAVIOR || type == TYPE_SWITCH
                    || type == TYPE_CREATE || type == TYPE_REPLACE_GUEST, "invalid builder type");
            mType = type;
        }

        /**
         * Sets the id of the user to be switched to.
         *
         * @throws IllegalArgumentException if builder is not for {@link #TYPE_SWITCH}.
         */
        @NonNull
        public Builder setSwitchUserId(@UserIdInt int userId) {
            Preconditions.checkArgument(mType == TYPE_SWITCH, "invalid builder type: " + mType);
            mSwitchUserId = userId;
            return this;
        }

        /**
         * Sets whether the current user should be replaced when it's a guest.
         */
        @NonNull
        public Builder setReplaceGuest(boolean value) {
            mReplaceGuest = value;
            return this;
        }

        /**
         * Sets the name of the new user being created.
         *
         * @throws IllegalArgumentException if builder is not for {@link #TYPE_CREATE}.
         */
        @NonNull
        public Builder setNewUserName(@Nullable String name) {
            Preconditions.checkArgument(mType == TYPE_CREATE, "invalid builder type: " + mType);
            mNewUserName = name;
            return this;
        }

        /**
         * Sets the flags (as defined by {@link android.hardware.automotive.vehicle.V2_0.UserFlags})
         * of the new user being created.
         *
         * @throws IllegalArgumentException if builder is not for {@link #TYPE_CREATE}.
         */
        @NonNull
        public Builder setNewUserFlags(int flags) {
            Preconditions.checkArgument(mType == TYPE_CREATE, "invalid builder type: " + mType);
            mNewUserFlags = flags;
            return this;
        }

        /**
         * Sets whether the {@link CarProperties#boot_user_override_id()} should be taking in
         * account when using the default behavior.
         */
        @NonNull
        public Builder setSupportsOverrideUserIdProperty(boolean value) {
            mSupportsOverrideUserIdProperty = value;
            return this;
        }

        /**
         * Sets the system locales for the initial user (when it's created).
         */
        @NonNull
        public Builder setUserLocales(@Nullable String userLocales) {
            mUserLocales = userLocales;
            return this;
        }

        /**
         * Builds the object.
         */
        @NonNull
        public InitialUserInfo build() {
            return new InitialUserInfo(this);
        }
    }

    /**
     * Object used to define the properties of the initial user (which can then be set by
     * {@link InitialUserSetter#set(InitialUserInfo)});
     */
    public static final class InitialUserInfo {
        public final @InitialUserInfoType int type;
        public final boolean replaceGuest;
        public final @UserIdInt int switchUserId;
        public final @Nullable String newUserName;
        public final int newUserFlags;
        public final boolean supportsOverrideUserIdProperty;
        public @Nullable String userLocales;

        private InitialUserInfo(@NonNull Builder builder) {
            type = builder.mType;
            switchUserId = builder.mSwitchUserId;
            replaceGuest = builder.mReplaceGuest;
            newUserName = builder.mNewUserName;
            newUserFlags = builder.mNewUserFlags;
            supportsOverrideUserIdProperty = builder.mSupportsOverrideUserIdProperty;
            userLocales = builder.mUserLocales;
        }

        @Override
        public String toString() {
            StringBuilder string = new StringBuilder("InitialUserInfo[type=");
            switch(type) {
                case TYPE_DEFAULT_BEHAVIOR:
                    string.append("DEFAULT_BEHAVIOR");
                    break;
                case TYPE_REPLACE_GUEST:
                    string.append("REPLACE_GUEST");
                    break;
                case TYPE_SWITCH:
                    string.append("SWITCH").append(",userId=").append(switchUserId);
                    break;
                case TYPE_CREATE:
                    string.append("CREATE").append(",flags=")
                            .append(UserHalHelper.userFlagsToString(newUserFlags));
                    if (newUserName != null) {
                        string.append(",name=" + UserHelperLite.safeName(newUserName));
                    }
                    if (userLocales != null) {
                        string.append(",locales=").append(userLocales);
                    }
                    break;
                default:
                    string.append("UNKNOWN:").append(type);
            }
            if (replaceGuest) string.append(",replaceGuest");
            if (supportsOverrideUserIdProperty) string.append(",supportsOverrideUserIdProperty");

            return string.append(']').toString();
        }
    }

    /**
     * Sets the initial user.
     */
    public void set(@NonNull InitialUserInfo info) {
        Preconditions.checkArgument(info != null, "info cannot be null");

        switch (info.type) {
            case TYPE_DEFAULT_BEHAVIOR:
                executeDefaultBehavior(info, /* fallback= */ false);
                break;
            case TYPE_SWITCH:
                try {
                    switchUser(info, /* fallback= */ true);
                } catch (Exception e) {
                    fallbackDefaultBehavior(info, /* fallback= */ true,
                            "Exception switching user: " + e);
                }
                break;
            case TYPE_CREATE:
                try {
                    createAndSwitchUser(info, /* fallback= */ true);
                } catch (Exception e) {
                    fallbackDefaultBehavior(info, /* fallback= */ true,
                            "Exception createUser user with name "
                                    + UserHelperLite.safeName(info.newUserName) + " and flags "
                                    + UserHalHelper.userFlagsToString(info.newUserFlags) + ": "
                                    + e);
                }
                break;
            case TYPE_REPLACE_GUEST:
                try {
                    replaceUser(info, /* fallback= */ true);
                } catch (Exception e) {
                    fallbackDefaultBehavior(info, /* fallback= */ true,
                            "Exception replace guest user: " + e);
                }
                break;
            default:
                throw new IllegalArgumentException("invalid InitialUserInfo type: " + info.type);
        }
    }

    private void replaceUser(InitialUserInfo info, boolean fallback) {
        int currentUserId = ActivityManager.getCurrentUser();
        UserInfo currentUser = mUm.getUserInfo(currentUserId);

        UserInfo newUser = replaceGuestIfNeeded(currentUser);
        if (newUser == null) {
            fallbackDefaultBehavior(info, fallback,
                    "could not replace guest " + currentUser.toFullString());
            return;
        }

        switchUser(new Builder(TYPE_SWITCH)
                .setSwitchUserId(newUser.id)
                .build(), fallback);

        if (newUser.id != currentUser.id) {
            Slog.i(TAG, "Removing old guest " + currentUser.id);
            if (!mUm.removeUser(currentUser.id)) {
                Slog.w(TAG, "Could not remove old guest " + currentUser.id);
            }
        }
    }

    private void executeDefaultBehavior(@NonNull InitialUserInfo info, boolean fallback) {
        if (!hasInitialUser()) {
            if (DBG) Slog.d(TAG, "executeDefaultBehavior(): no initial user, creating it");
            createAndSwitchUser(new Builder(TYPE_CREATE)
                    .setNewUserName(mNewUserName)
                    .setNewUserFlags(UserFlags.ADMIN)
                    .setSupportsOverrideUserIdProperty(info.supportsOverrideUserIdProperty)
                    .setUserLocales(info.userLocales)
                    .build(), fallback);
        } else {
            if (DBG) Slog.d(TAG, "executeDefaultBehavior(): switching to initial user");
            int userId = getInitialUser(info.supportsOverrideUserIdProperty);
            switchUser(new Builder(TYPE_SWITCH)
                    .setSwitchUserId(userId)
                    .setSupportsOverrideUserIdProperty(info.supportsOverrideUserIdProperty)
                    .setReplaceGuest(info.replaceGuest)
                    .build(), fallback);
        }
    }

    @VisibleForTesting
    void fallbackDefaultBehavior(@NonNull InitialUserInfo info, boolean fallback,
            @NonNull String reason) {
        if (!fallback) {
            // Only log the error
            Slog.w(TAG, reason);
            // Must explicitly tell listener that initial user could not be determined
            notifyListener(/*initialUser= */ null);
            return;
        }
        Slog.w(TAG, "Falling back to default behavior. Reason: " + reason);
        executeDefaultBehavior(info, /* fallback= */ false);
    }

    private void switchUser(@NonNull InitialUserInfo info, boolean fallback) {
        int userId = info.switchUserId;
        boolean replaceGuest = info.replaceGuest;

        if (DBG) {
            Slog.d(TAG, "switchUser(): userId=" + userId + ", replaceGuest=" + replaceGuest
                    + ", fallback=" + fallback);
        }

        UserInfo user = mUm.getUserInfo(userId);
        if (user == null) {
            fallbackDefaultBehavior(info, fallback, "user with id " + userId + " doesn't exist");
            return;
        }

        UserInfo actualUser = user;

        if (user.isGuest() && replaceGuest) {
            actualUser = replaceGuestIfNeeded(user);

            if (actualUser == null) {
                fallbackDefaultBehavior(info, fallback, "could not replace guest "
                        + user.toFullString());
                return;
            }
        }

        int actualUserId = actualUser.id;

        unlockSystemUserIfNecessary(actualUserId);

        int currentUserId = ActivityManager.getCurrentUser();
        if (actualUserId != currentUserId) {
            if (!startForegroundUser(actualUserId)) {
                fallbackDefaultBehavior(info, fallback,
                        "am.switchUser(" + actualUserId + ") failed");
                return;
            }
            setLastActiveUser(actualUser.id);
        }
        notifyListener(actualUser);

        if (actualUserId != userId) {
            Slog.i(TAG, "Removing old guest " + userId);
            if (!mUm.removeUser(userId)) {
                Slog.w(TAG, "Could not remove old guest " + userId);
            }
        }
    }

    private void unlockSystemUserIfNecessary(@UserIdInt int userId) {
        // If system user is the only user to unlock, it will be handled when boot is complete.
        if (userId != UserHandle.USER_SYSTEM) {
            unlockSystemUser();
        }
    }

    /**
     * Check if the user is a guest and can be replaced.
     */
    public boolean canReplaceGuestUser(UserInfo user) {
        if (!user.isGuest()) return false;

        if (mLockPatternUtils.isSecure(user.id)) {
            if (DBG) {
                Slog.d(TAG, "replaceGuestIfNeeded(), skipped, since user "
                        + user.id + " has secure lock pattern");
            }
            return false;
        }

        return true;
    }

    /**
     * Replaces {@code user} by a new guest, if necessary.
     *
     * <p>If {@code user} is not a guest, it doesn't do anything and returns the same user.
     *
     * <p>Otherwise, it marks the current guest for deletion, creates a new one, and returns the
     * new guest (or {@code null} if a new guest could not be created).
     */

    @VisibleForTesting
    @Nullable
    UserInfo replaceGuestIfNeeded(@NonNull UserInfo user) {
        Preconditions.checkArgument(user != null, "user cannot be null");

        if (!canReplaceGuestUser(user)) {
            return user;
        }

        Slog.i(TAG, "Replacing guest (" + user.toFullString() + ")");

        int halFlags = UserFlags.GUEST;
        if (user.isEphemeral()) {
            halFlags |= UserFlags.EPHEMERAL;
        } else {
            // TODO(b/150413515): decide whether we should allow it or not. Right now we're
            // just logging, as UserManagerService will automatically set it to ephemeral if
            // platform is set to do so.
            Slog.w(TAG, "guest being replaced is not ephemeral: " + user.toFullString());
        }

        if (!mUm.markGuestForDeletion(user.id)) {
            // Don't need to recover in case of failure - most likely create new user will fail
            // because there is already a guest
            Slog.w(TAG, "failed to mark guest " + user.id + " for deletion");
        }

        Pair<UserInfo, String> result = createNewUser(new Builder(TYPE_CREATE)
                .setNewUserName(mNewGuestName)
                .setNewUserFlags(halFlags)
                .build());

        String errorMessage = result.second;
        if (errorMessage != null) {
            Slog.w(TAG, "could not replace guest " + user.toFullString() + ": " + errorMessage);
            return null;
        }

        return result.first;
    }

    private void createAndSwitchUser(@NonNull InitialUserInfo info, boolean fallback) {
        Pair<UserInfo, String> result = createNewUser(info);
        String reason = result.second;
        if (reason != null) {
            fallbackDefaultBehavior(info, fallback, reason);
            return;
        }

        switchUser(new Builder(TYPE_SWITCH)
                .setSwitchUserId(result.first.id)
                .setSupportsOverrideUserIdProperty(info.supportsOverrideUserIdProperty)
                .build(), fallback);
    }

    /**
     * Creates a new user.
     *
     * @return on success, first element is the new user; on failure, second element contains the
     * error message.
     */
    @NonNull
    private Pair<UserInfo, String> createNewUser(@NonNull InitialUserInfo info) {
        String name = info.newUserName;
        int halFlags = info.newUserFlags;

        if (DBG) {
            Slog.d(TAG, "createUser(name=" + UserHelperLite.safeName(name) + ", flags="
                    + userFlagsToString(halFlags) + ")");
        }

        if (UserHalHelper.isSystem(halFlags)) {
            return new Pair<>(null, "Cannot create system user");
        }

        if (UserHalHelper.isAdmin(halFlags)) {
            boolean validAdmin = true;
            if (UserHalHelper.isGuest(halFlags)) {
                Slog.w(TAG, "Cannot create guest admin");
                validAdmin = false;
            }
            if (UserHalHelper.isEphemeral(halFlags)) {
                Slog.w(TAG, "Cannot create ephemeral admin");
                validAdmin = false;
            }
            if (!validAdmin) {
                return new Pair<>(null, "Invalid flags for admin user");
            }
        }
        // TODO(b/150413515): decide what to if HAL requested a non-ephemeral guest but framework
        // sets all guests as ephemeral - should it fail or just warn?

        int flags = UserHalHelper.toUserInfoFlags(halFlags);
        String type = UserHalHelper.isGuest(halFlags) ? UserManager.USER_TYPE_FULL_GUEST
                : UserManager.USER_TYPE_FULL_SECONDARY;

        if (DBG) {
            Slog.d(TAG, "calling am.createUser((name=" + UserHelperLite.safeName(name) + ", type="
                    + type + ", flags=" + UserInfo.flagsToString(flags) + ")");
        }

        UserInfo userInfo = mCarUserService.createUserEvenWhenDisallowed(name, type, flags);
        if (userInfo == null) {
            return new Pair<>(null, "createUser(name=" + UserHelperLite.safeName(name) + ", flags="
                    + userFlagsToString(halFlags) + "): failed to create user");
        }

        if (DBG) Slog.d(TAG, "user created: " + userInfo.id);

        if (info.userLocales != null) {
            if (DBG) {
                Slog.d(TAG, "setting locale for user " + userInfo.id + " to " + info.userLocales);
            }
            Settings.System.putStringForUser(mContext.getContentResolver(),
                    Settings.System.SYSTEM_LOCALES, info.userLocales, userInfo.id);
        }

        return new Pair<>(userInfo, null);
    }

    @VisibleForTesting
    void unlockSystemUser() {
        Slog.i(TAG, "unlocking system user");
        IActivityManager am = ActivityManager.getService();

        TimingsTraceLog t = new TimingsTraceLog(TAG, Trace.TRACE_TAG_SYSTEM_SERVER);
        t.traceBegin("UnlockSystemUser");
        try {
            // This is for force changing state into RUNNING_LOCKED. Otherwise unlock does not
            // update the state and USER_SYSTEM unlock happens twice.
            t.traceBegin("am.startUser");
            boolean started = am.startUserInBackground(UserHandle.USER_SYSTEM);
            t.traceEnd();
            if (!started) {
                Slog.w(TAG, "could not restart system user in foreground; trying unlock instead");
                t.traceBegin("am.unlockUser");
                boolean unlocked = am.unlockUser(UserHandle.USER_SYSTEM, /* token= */ null,
                        /* secret= */ null, /* listener= */ null);
                t.traceEnd();
                if (!unlocked) {
                    Slog.w(TAG, "could not unlock system user neither");
                    return;
                }
            }
        } catch (RemoteException e) {
            // should not happen for local call.
            Slog.wtf("RemoteException from AMS", e);
        } finally {
            t.traceEnd();
        }
    }

    @VisibleForTesting
    boolean startForegroundUser(@UserIdInt int userId) {
        if (UserHelperLite.isHeadlessSystemUser(userId)) {
            // System User doesn't associate with real person, can not be switched to.
            return false;
        }
        try {
            return ActivityManager.getService().startUserInForegroundWithListener(userId, null);
        } catch (RemoteException e) {
            Slog.w(TAG, "failed to start user " + userId, e);
            return false;
        }
    }

    private void notifyListener(@Nullable UserInfo initialUser) {
        if (DBG) Slog.d(TAG, "notifyListener(): " + initialUser);
        mListener.accept(initialUser);
    }

    /**
     * Dumps it state.
     */
    @ExcludeFromCodeCoverageGeneratedReport(reason = DUMP_INFO)
    public void dump(@NonNull PrintWriter writer) {
        writer.println("InitialUserSetter");
        String indent = "  ";
        writer.printf("%smNewUserName: %s\n", indent, mNewUserName);
        writer.printf("%smNewGuestName: %s\n", indent, mNewGuestName);
    }

    /**
     * Sets the last active user.
     */
    public void setLastActiveUser(@UserIdInt int userId) {
        if (UserHelperLite.isHeadlessSystemUser(userId)) {
            if (DBG) Slog.d(TAG, "setLastActiveUser(): ignoring headless system user " + userId);
            return;
        }
        setUserIdGlobalProperty(CarSettings.Global.LAST_ACTIVE_USER_ID, userId);

        // TODO(b/155918094): change method to receive a UserInfo instead
        UserInfo user = mUm.getUserInfo(userId);
        if (user == null) {
            Slog.w(TAG, "setLastActiveUser(): user " + userId + " doesn't exist");
            return;
        }
        if (!user.isEphemeral()) {
            setUserIdGlobalProperty(CarSettings.Global.LAST_ACTIVE_PERSISTENT_USER_ID, userId);
        }
    }

    private void setUserIdGlobalProperty(@NonNull String name, @UserIdInt int userId) {
        if (DBG) Slog.d(TAG, "setting global property " + name + " to " + userId);

        Settings.Global.putInt(mContext.getContentResolver(), name, userId);
    }

    /**
     * Gets the user id for the initial user to boot into. This is only applicable for headless
     * system user model. This method checks for a system property and will only work for system
     * apps.
     *
     * This method checks for the initial user via three mechanisms in this order:
     * <ol>
     *     <li>Check for a boot user override via {@link CarProperties#boot_user_override_id()}</li>
     *     <li>Check for the last active user in the system</li>
     *     <li>Fallback to the smallest user id that is not {@link UserHandle.USER_SYSTEM}</li>
     * </ol>
     *
     * If any step fails to retrieve the stored id or the retrieved id does not exist on device,
     * then it will move onto the next step.
     *
     * @return user id of the initial user to boot into on the device, or
     * {@link UserHandle#USER_NULL} if there is no user available.
     */
    @VisibleForTesting
    int getInitialUser(boolean usesOverrideUserIdProperty) {

        List<Integer> allUsers = userInfoListToUserIdList(getAllUsers());

        if (allUsers.isEmpty()) {
            return UserHandle.USER_NULL;
        }

        //TODO(b/150416512): Check if it is still supported, if not remove it.
        if (usesOverrideUserIdProperty) {
            int bootUserOverride = CarProperties.boot_user_override_id()
                    .orElse(BOOT_USER_NOT_FOUND);

            // If an override user is present and a real user, return it
            if (bootUserOverride != BOOT_USER_NOT_FOUND
                    && allUsers.contains(bootUserOverride)) {
                Slog.i(TAG, "Boot user id override found for initial user, user id: "
                        + bootUserOverride);
                return bootUserOverride;
            }
        }

        // If the last active user is not the SYSTEM user and is a real user, return it
        int lastActiveUser = getUserIdGlobalProperty(CarSettings.Global.LAST_ACTIVE_USER_ID);
        if (allUsers.contains(lastActiveUser)) {
            Slog.i(TAG, "Last active user loaded for initial user: " + lastActiveUser);
            return lastActiveUser;
        }
        resetUserIdGlobalProperty(CarSettings.Global.LAST_ACTIVE_USER_ID);

        int lastPersistentUser = getUserIdGlobalProperty(
                CarSettings.Global.LAST_ACTIVE_PERSISTENT_USER_ID);
        if (allUsers.contains(lastPersistentUser)) {
            Slog.i(TAG, "Last active, persistent user loaded for initial user: "
                    + lastPersistentUser);
            return lastPersistentUser;
        }
        resetUserIdGlobalProperty(CarSettings.Global.LAST_ACTIVE_PERSISTENT_USER_ID);

        // If all else fails, return the smallest user id
        int returnId = Collections.min(allUsers);
        // TODO(b/158101909): the smallest user id is not always the initial user; a better approach
        // would be looking for the first ADMIN user, or keep track of all last active users (not
        // just the very last)
        Slog.w(TAG, "Last active user (" + lastActiveUser + ") not found. Returning smallest user "
                + "id instead: " + returnId);
        return returnId;
    }

    /**
     * Gets all the users that can be brought to the foreground on the system.
     *
     * @return List of {@code UserInfo} for users that associated with a real person.
     */
    private List<UserInfo> getAllUsers() {
        if (UserManager.isHeadlessSystemUserMode()) {
            return getAllUsersExceptSystemUserAndSpecifiedUser(UserHandle.USER_SYSTEM);
        } else {
            return mUm.getAliveUsers();
        }
    }

    /**
     * Gets all the users except system user and the one with userId passed in.
     *
     * @param userId of the user not to be returned.
     * @return All users other than system user and user with userId.
     */
    private List<UserInfo> getAllUsersExceptSystemUserAndSpecifiedUser(@UserIdInt int userId) {
        List<UserInfo> users = mUm.getAliveUsers();

        for (Iterator<UserInfo> iterator = users.iterator(); iterator.hasNext(); ) {
            UserInfo userInfo = iterator.next();
            if (userInfo.id == userId || userInfo.id == UserHandle.USER_SYSTEM) {
                // Remove user with userId from the list.
                iterator.remove();
            }
        }
        return users;
    }

    /**
     * Checks whether the device has an initial user that can be switched to.
     */
    public boolean hasInitialUser() {
        List<UserInfo> allUsers = getAllUsers();
        for (int i = 0; i < allUsers.size(); i++) {
            UserInfo user = allUsers.get(i);
            if (user.isManagedProfile()) continue;

            return true;
        }
        return false;
    }

    private static List<Integer> userInfoListToUserIdList(List<UserInfo> allUsers) {
        ArrayList<Integer> list = new ArrayList<>(allUsers.size());
        for (int i = 0; i < allUsers.size(); i++) {
            list.add(allUsers.get(i).id);
        }
        return list;
    }

    private void resetUserIdGlobalProperty(@NonNull String name) {
        if (DBG) Slog.d(TAG, "resetting global property " + name);

        Settings.Global.putInt(mContext.getContentResolver(), name, UserHandle.USER_NULL);
    }

    private int getUserIdGlobalProperty(@NonNull String name) {
        int userId = Settings.Global.getInt(mContext.getContentResolver(), name,
                UserHandle.USER_NULL);
        if (DBG) Slog.d(TAG, "getting global property " + name + ": " + userId);

        return userId;
    }
}
