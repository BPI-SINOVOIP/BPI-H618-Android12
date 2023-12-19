/*
 * Copyright 2021 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   https://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package com.google.android.enterprise.connectedapps.instrumented.utils;

import static java.nio.charset.StandardCharsets.UTF_8;

import android.content.Context;
import android.os.Build.VERSION;
import android.os.Build.VERSION_CODES;
import android.os.ParcelFileDescriptor;
import android.os.UserHandle;
import androidx.test.platform.app.InstrumentationRegistry;
import com.google.android.enterprise.connectedapps.ProfileConnector;
import com.google.android.enterprise.connectedapps.SharedTestUtilities;
import com.google.android.enterprise.connectedapps.instrumented.utils.ServiceCall.Parameter;
import com.google.android.enterprise.connectedapps.testing.ProfileAvailabilityPoll;
import java.io.FileInputStream;
import java.io.InputStream;
import java.util.NoSuchElementException;
import java.util.Scanner;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * Wrapper around {@link
 * com.google.android.enterprise.connectedapps.testing.InstrumentedTestUtilities} which adds
 * features needed only by the SDK.
 */
public class InstrumentedTestUtilities {

  private final ProfileConnector connector;
  private final Context context;
  private final com.google.android.enterprise.connectedapps.testing.InstrumentedTestUtilities
          instrumentedTestUtilities;

  private static final int S_REQUEST_QUIET_MODE_ENABLED_ID = 73;
  private static final int R_REQUEST_QUIET_MODE_ENABLED_ID = 72;
  private static final int REQUEST_QUIET_MODE_ENABLED_ID = 58;

  private static final String USER_ID_KEY = "USER_ID";
  private static final Parameter USER_ID_PARAMETER = new Parameter(USER_ID_KEY);

  private static final ServiceCall S_TURN_OFF_WORK_PROFILE_COMMAND =
          new ServiceCall("user", S_REQUEST_QUIET_MODE_ENABLED_ID)
                  .setUser(1000) // user 1000 has packageName "android"
                  .addStringParam("android") // callingPackage
                  .addBooleanParam(true) // enableQuietMode
                  .addIntParam(USER_ID_PARAMETER) // userId
                  .addIntParam(0) // target
                  .addIntParam(0); // flags

  private static final ServiceCall R_TURN_OFF_WORK_PROFILE_COMMAND =
          new ServiceCall("user", R_REQUEST_QUIET_MODE_ENABLED_ID)
                  .setUser(1000) // user 1000 has packageName "android"
                  .addStringParam("android") // callingPackage
                  .addBooleanParam(true) // enableQuietMode
                  .addIntParam(USER_ID_PARAMETER) // userId
                  .addIntParam(0) // target
                  .addIntParam(0); // flags

  private static final ServiceCall TURN_OFF_WORK_PROFILE_COMMAND =
          new ServiceCall("user", REQUEST_QUIET_MODE_ENABLED_ID)
                  .setUser(1000) // user 1000 has packageName "android"
                  .addStringParam("android") // callingPackage
                  .addBooleanParam(true) // enableQuietMode
                  .addIntParam(USER_ID_PARAMETER) // userId
                  .addIntParam(0); // target

  private static final ServiceCall S_TURN_ON_WORK_PROFILE_COMMAND =
          new ServiceCall("user", S_REQUEST_QUIET_MODE_ENABLED_ID)
                  .setUser(1000) // user 1000 has packageName "android"
                  .addStringParam("android") // callingPackage
                  .addBooleanParam(false) // enableQuietMode
                  .addIntParam(USER_ID_PARAMETER) // userId
                  .addIntParam(0) // target
                  .addIntParam(0); // flags

  private static final ServiceCall R_TURN_ON_WORK_PROFILE_COMMAND =
          new ServiceCall("user", R_REQUEST_QUIET_MODE_ENABLED_ID)
                  .setUser(1000) // user 1000 has packageName "android"
                  .addStringParam("android") // callingPackage
                  .addBooleanParam(false) // enableQuietMode
                  .addIntParam(USER_ID_PARAMETER) // userId
                  .addIntParam(0) // target
                  .addIntParam(0); // flags

  private static final ServiceCall TURN_ON_WORK_PROFILE_COMMAND =
          new ServiceCall("user", REQUEST_QUIET_MODE_ENABLED_ID)
                  .setUser(1000) // user 1000 has packageName "android"
                  .addStringParam("android") // callingPackage
                  .addBooleanParam(false) // enableQuietMode
                  .addIntParam(USER_ID_PARAMETER) // userId
                  .addIntParam(0); // target

  public InstrumentedTestUtilities(Context context, ProfileConnector connector) {
    this.context = context;
    this.connector = connector;
    this.instrumentedTestUtilities =
            new com.google.android.enterprise.connectedapps.testing.InstrumentedTestUtilities(
                    context, connector);
  }

  /**
   * See {@link
   * com.google.android.enterprise.connectedapps.testing.InstrumentedTestUtilities#hasWorkProfile()}.
   */
  public boolean hasWorkProfile() {
    return instrumentedTestUtilities.hasWorkProfile();
  }

  /**
   * See {@link
   * com.google.android.enterprise.connectedapps.testing.InstrumentedTestUtilities#getWorkProfileUserId()}.
   */
  public int getWorkProfileUserId() {
    return instrumentedTestUtilities.getWorkProfileUserId();
  }

  private UserHandle getWorkProfileUserHandle() {
    return SharedTestUtilities.getUserHandleForUserId(getWorkProfileUserId());
  }

  /**
   * See {@link
   * com.google.android.enterprise.connectedapps.testing.InstrumentedTestUtilities#ensureReadyForCrossProfileCalls()}.
   */
  public void ensureReadyForCrossProfileCalls() {
    instrumentedTestUtilities.ensureReadyForCrossProfileCalls();
    ensureWorkProfileTurnedOn();
  }

  /**
   * See {@link
   * com.google.android.enterprise.connectedapps.testing.InstrumentedTestUtilities#ensureNoWorkProfile()}.
   */
  public void ensureNoWorkProfile() {
    instrumentedTestUtilities.ensureNoWorkProfile();
  }

  public void removeUser(int userId) {
    runCommandWithOutput("pm remove-user " + userId);
  }

  public void installInUser(int userId) {
    runCommandWithOutput(
            "cmd package install-existing --user " + userId + " " + context.getPackageName());
  }

  /**
   * Grant the {@code INTERACT_ACROSS_USERS} permission if this app declares it.
   *
   * <p>This is required before cross-profile interaction will work.
   */
  public void grantInteractAcrossUsers() {
    // TODO(scottjonathan): Support INTERACT_ACROSS_PROFILES in these tests.
    runCommandWithOutput(
            "pm grant " + context.getPackageName() + " android.permission.INTERACT_ACROSS_USERS");
  }

  /**
   * See {@link
   * com.google.android.enterprise.connectedapps.testing.InstrumentedTestUtilities#ensureWorkProfileExists()}
   */
  public void ensureWorkProfileExists() {
    instrumentedTestUtilities.ensureWorkProfileExists();
  }

  /**
   * Create a work profile but do not install the test app.
   *
   * <p>This means that, as there is no profile owner, it will not be recognised as a work profile
   * by the SDK when running on that profile.
   */
  public void ensureWorkProfileExistsWithoutTestApp() {
    if (hasWorkProfile()) {
      if (!userHasPackageInstalled(getWorkProfileUserId(), context.getPackageName())) {
        return;
      }


      runCommandWithOutput("pm remove-user " + getWorkProfileUserId());

      // TODO(162219825): Try to remove the package

//      throw new IllegalStateException(
//              "There is already a work profile on the device with user id "
//                      + getWorkProfileUserId()
//                      + ".");
    }
    runCommandWithOutput("pm create-user --profileOf 0 --managed TestProfile123");
    int workProfileUserId = getWorkProfileUserId();
    startUser(workProfileUserId);
  }

  private static boolean userHasPackageInstalled(int userId, String packageName) {
    String expectedPackageLine = "package:" + packageName;
    String[] installedPackages =
            runCommandWithOutput("pm list packages --user " + userId).split("\n");
    for (String installedPackage : installedPackages) {
      if (installedPackage.equals(expectedPackageLine)) {
        return true;
      }
    }
    return false;
  }

  /** Ensure that the work profile is running. */
  public void ensureWorkProfileTurnedOn() {
    turnOnWorkProfileAndWait();
  }

  /** Ensure that the work profile is not running. */
  public void ensureWorkProfileTurnedOff() {
    turnOffWorkProfileAndWait();
  }

  /**
   * Turn off the work profile and block until it has been turned off.
   *
   * <p>This uses {@link ServiceCall} and so is only guaranteed to work correctly on AOSP.
   *
   * @see #turnOffWorkProfile()
   */
  public void turnOffWorkProfileAndWait() {
    turnOffWorkProfile();

    ProfileAvailabilityPoll.blockUntilProfileNotAvailable(context, getWorkProfileUserHandle());
  }

  // TODO(160147511): Remove use of service calls for versions after R
  /**
   * Turn off the work profile
   *
   * <p>This uses {@link ServiceCall} and so is only guaranteed to work correctly on AOSP.
   *
   * @see #turnOffWorkProfileAndWait()
   */
  public void turnOffWorkProfile() {
    if (VERSION.CODENAME.equals("S")) {
      runCommandWithOutput(
              S_TURN_OFF_WORK_PROFILE_COMMAND
                      .prepare()
                      .setInt(USER_ID_KEY, getWorkProfileUserId())
                      .getCommand());
    } else if (VERSION.SDK_INT == VERSION_CODES.R) {
      runCommandWithOutput(
              R_TURN_OFF_WORK_PROFILE_COMMAND
                      .prepare()
                      .setInt(USER_ID_KEY, getWorkProfileUserId())
                      .getCommand());
    } else if (VERSION.SDK_INT == VERSION_CODES.Q || VERSION.SDK_INT == VERSION_CODES.P) {
      runCommandWithOutput(
              TURN_OFF_WORK_PROFILE_COMMAND
                      .prepare()
                      .setInt(USER_ID_KEY, getWorkProfileUserId())
                      .getCommand());
    } else {
      throw new IllegalStateException("Cannot turn off work on this version of android");
    }
  }

  /**
   * Turn on the work profile and block until it has been turned on.
   *
   * <p>This uses {@link ServiceCall} and so is only guaranteed to work correctly on AOSP.
   *
   * @see #turnOnWorkProfile()
   */
  public void turnOnWorkProfileAndWait() {
    if (connector.isAvailable()) {
      return; // Already on
    }

    turnOnWorkProfile();

    ProfileAvailabilityPoll.blockUntilProfileRunningAndUnlocked(
            context, getWorkProfileUserHandle());
  }

  // TODO(160147511): Remove use of service calls for versions after R
  /**
   * Turn on the work profile and block until it has been turned on.
   *
   * <p>This uses {@link ServiceCall} and so is only guaranteed to work correctly on AOSP.
   *
   * @see #turnOnWorkProfileAndWait()
   */
  public void turnOnWorkProfile() {
    if (VERSION.CODENAME.equals("S")) {
      runCommandWithOutput(
              S_TURN_ON_WORK_PROFILE_COMMAND
                      .prepare()
                      .setInt(USER_ID_KEY, getWorkProfileUserId())
                      .getCommand());
    } else if (VERSION.SDK_INT == VERSION_CODES.R) {
      runCommandWithOutput(
              R_TURN_ON_WORK_PROFILE_COMMAND
                      .prepare()
                      .setInt(USER_ID_KEY, getWorkProfileUserId())
                      .getCommand());
    } else if (VERSION.SDK_INT == VERSION_CODES.Q || VERSION.SDK_INT == VERSION_CODES.P) {
      runCommandWithOutput(
              TURN_ON_WORK_PROFILE_COMMAND
                      .prepare()
                      .setInt(USER_ID_KEY, getWorkProfileUserId())
                      .getCommand());
    } else {
      throw new IllegalStateException("Cannot turn on work on this version of android");
    }
  }

  /**
   * See {@link
   * com.google.android.enterprise.connectedapps.testing.InstrumentedTestUtilities#waitForDisconnected()}.
   */
  public void waitForDisconnected() {
    instrumentedTestUtilities.waitForDisconnected();
  }

  /**
   * See {@link
   * com.google.android.enterprise.connectedapps.testing.InstrumentedTestUtilities#waitForConnected()}.
   */
  public void waitForConnected() {
    instrumentedTestUtilities.waitForConnected();
  }

  /**
   * Manually call {@link ProfileConnector#startConnecting()} and wait for connection to be
   * complete.
   */
  public void manuallyConnectAndWait() {
    connector.startConnecting();
    waitForConnected();
  }

  private static final Pattern CREATE_USER_PATTERN =
          Pattern.compile("Success: created user id (\\d+)");

  public int createUser(String username) {
    String output = runCommandWithOutput("pm create-user " + username);

    Matcher userMatcher = CREATE_USER_PATTERN.matcher(output);
    if (userMatcher.find()) {
      return Integer.parseInt(userMatcher.group(1));
    }

    throw new IllegalStateException("Could not create user. Output: " + output);
  }

  public void startUser(int userId) {
    UserHandle userHandle = SharedTestUtilities.getUserHandleForUserId(userId);
    InstrumentedTestUtilities.runCommandWithOutput("am start-user " + userId);
    ProfileAvailabilityPoll.blockUntilProfileRunningAndUnlocked(context, userHandle);
  }

  private static String runCommandWithOutput(String command) {
    // TODO: Log output so we can see why it's failing
    ParcelFileDescriptor p = runCommand(command);

    InputStream inputStream = new FileInputStream(p.getFileDescriptor());

    try (Scanner scanner = new Scanner(inputStream, UTF_8.name())) {
      return scanner.useDelimiter("\\A").next();
    } catch (NoSuchElementException e) {
      return "";
    }
  }

  private static ParcelFileDescriptor runCommand(String command) {
    return InstrumentationRegistry.getInstrumentation()
            .getUiAutomation()
            .executeShellCommand(command);
  }
}
