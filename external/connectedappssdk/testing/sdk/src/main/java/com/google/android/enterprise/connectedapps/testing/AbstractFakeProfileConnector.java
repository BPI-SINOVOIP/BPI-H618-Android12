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
package com.google.android.enterprise.connectedapps.testing;

import android.content.Context;
import com.google.android.enterprise.connectedapps.AvailabilityListener;
import com.google.android.enterprise.connectedapps.ConnectedAppsUtils;
import com.google.android.enterprise.connectedapps.ConnectionListener;
import com.google.android.enterprise.connectedapps.CrossProfileSender;
import com.google.android.enterprise.connectedapps.Permissions;
import com.google.android.enterprise.connectedapps.ProfileConnector;
import com.google.android.enterprise.connectedapps.annotations.CustomProfileConnector.ProfileType;
import com.google.android.enterprise.connectedapps.exceptions.UnavailableProfileException;
import java.util.HashSet;
import java.util.Set;

/**
 * A fake {@link ProfileConnector} for use in tests.
 *
 * <p>This should be extended to make it compatible with a specific {@link ProfileConnector}
 * interface.
 */
public abstract class AbstractFakeProfileConnector implements ProfileConnector {

  enum WorkProfileState {
    DOES_NOT_EXIST,
    TURNED_OFF,
    TURNED_ON
  }

  private final Context applicationContext;
  private final ProfileType primaryProfileType;
  private ProfileType runningOnProfile = ProfileType.PERSONAL;
  private WorkProfileState workProfileState = WorkProfileState.DOES_NOT_EXIST;
  private boolean isConnected = false;
  private boolean hasPermissionToMakeCrossProfileCalls = true;
  private boolean isManuallyManagingConnection = false;

  private final Set<ConnectionListener> connectionListeners = new HashSet<>();
  private final Set<AvailabilityListener> availabilityListeners = new HashSet<>();

  public AbstractFakeProfileConnector(Context context, ProfileType primaryProfileType) {
    if (context == null || primaryProfileType == null) {
      throw new NullPointerException();
    }
    this.applicationContext = context.getApplicationContext();
    this.primaryProfileType = primaryProfileType;
  }

  /**
   * Simulate running on a particular profile type.
   *
   * <p>If {@code currentProfile} is {@link ProfileType#WORK} and a work profile does not exist or
   * is not turned on, then a work profile will be created and turned on.
   *
   * @see #runningOnProfile
   */
  public void setRunningOnProfile(ProfileType currentProfile) {
    if (currentProfile == ProfileType.WORK && workProfileState != WorkProfileState.TURNED_ON) {
      turnOnWorkProfile();
    }
    this.runningOnProfile = currentProfile;
  }

  /**
   * Get the current profile type being simulated.
   *
   * @see #setRunningOnProfile(ProfileType)
   */
  public ProfileType runningOnProfile() {
    return runningOnProfile;
  }

  /**
   * Simulate the creation of a work profile.
   *
   * <p>The new work profile will be turned off by default.
   */
  public void createWorkProfile() {
    if (workProfileState != WorkProfileState.DOES_NOT_EXIST) {
      return;
    }
    this.workProfileState = WorkProfileState.TURNED_OFF;
  }

  /**
   * Remove a simulated work profile.
   *
   * <p>The simulated work profile will be turned off first.
   */
  public void removeWorkProfile() {
    if (workProfileState == WorkProfileState.DOES_NOT_EXIST) {
      return;
    }

    turnOffWorkProfile();
    this.workProfileState = WorkProfileState.DOES_NOT_EXIST;
  }

  /**
   * Simulate a work profile being turned on.
   *
   * <p>If no simulated work profile exists, then it will be created.
   */
  public void turnOnWorkProfile() {
    if (workProfileState == WorkProfileState.TURNED_ON) {
      return;
    }
    if (workProfileState == WorkProfileState.DOES_NOT_EXIST) {
      createWorkProfile();
    }
    workProfileState = WorkProfileState.TURNED_ON;
    notifyAvailabilityChanged();
  }

  /**
   * Simulate a work profile being turned off.
   *
   * <p>If no simulated work profile exists, then it will be created.
   *
   * <p>This fake will also be set to simulate running on the personal profile for future calls.
   */
  public void turnOffWorkProfile() {
    if (workProfileState == WorkProfileState.DOES_NOT_EXIST) {
      createWorkProfile();
    }
    setRunningOnProfile(ProfileType.PERSONAL);
    if (workProfileState == WorkProfileState.TURNED_OFF) {
      return;
    }

    if (isConnected) {
      isConnected = false;
      notifyConnectionChanged();
    }

    workProfileState = WorkProfileState.TURNED_OFF;
    notifyAvailabilityChanged();
  }

  /**
   * Force the connector to be "automatically" connected.
   *
   * <p>This call should only be used by the SDK and should not be called in tests. If you want to
   * connect manually, use {@link #startConnecting()}, or for automatic management just make the
   * asynchronous call directly.
   *
   * @hide
   */
  public void automaticallyConnect() {
    if (isAvailable() && !isConnected) {
      isConnected = true;
      notifyConnectionChanged();
    }
  }

  /**
   * Disconnect after an automatic connection.
   *
   * <p>In reality, this timeout happens some arbitrary time of no interaction with the other
   * profile.
   *
   * <p>If {@link #isManuallyManagingConnection()} is true, then this will do nothing.
   */
  public void timeoutConnection() {
    if (isManuallyManagingConnection) {
      return;
    }

    if (isConnected) {
      isConnected = false;
      notifyConnectionChanged();
    }
  }

  @Override
  public void startConnecting() {
    isManuallyManagingConnection = true;
    automaticallyConnect();
  }

  /**
   * This fake does not enforce the requirement that calls to {@link #connect()} do not occur on the
   * UI Thread.
   */
  @Override
  public void connect() throws UnavailableProfileException {
    if (!isAvailable()) {
      throw new UnavailableProfileException("No profile available");
    }

    isManuallyManagingConnection = true;
    automaticallyConnect();
  }

  /**
   * Stop manually managing the connection and ensure that the connector is disconnected.
   */
  public void disconnect() {
    stopManualConnectionManagement();
    timeoutConnection();
  }

  @Override
  public void stopManualConnectionManagement() {
    isManuallyManagingConnection = false;
  }

  /** Unsupported by the fake so always returns {@code null}. */
  @Override
  public CrossProfileSender crossProfileSender() {
    return null;
  }

  @Override
  public void registerConnectionListener(ConnectionListener listener) {
    connectionListeners.add(listener);
  }

  @Override
  public void unregisterConnectionListener(ConnectionListener listener) {
    connectionListeners.remove(listener);
  }

  private void notifyConnectionChanged() {
    for (ConnectionListener listener : connectionListeners) {
      listener.connectionChanged();
    }
  }

  @Override
  public void registerAvailabilityListener(AvailabilityListener listener) {
    availabilityListeners.add(listener);
  }

  @Override
  public void unregisterAvailabilityListener(AvailabilityListener listener) {
    availabilityListeners.remove(listener);
  }

  private void notifyAvailabilityChanged() {
    for (AvailabilityListener listener : availabilityListeners) {
      listener.availabilityChanged();
    }
  }

  @Override
  public boolean isAvailable() {
    return (runningOnProfile == ProfileType.WORK || workProfileState == WorkProfileState.TURNED_ON);
  }

  @Override
  public boolean isConnected() {
    return isConnected;
  }

  @Override
  public ConnectedAppsUtils utils() {
    return new FakeConnectedAppsUtils(this, primaryProfileType);
  }

  @Override
  public Permissions permissions() {
    return new FakePermissions(this);
  }

  /** Not supported by the fake so returns null. */
  @Override
  public Context applicationContext() {
    return applicationContext;
  }

  @Override
  public boolean isManuallyManagingConnection() {
    return isManuallyManagingConnection;
  }

  /**
   * Set whether or not the app has been given the appropriate permission to make cross-profile
   * calls.
   */
  public void setHasPermissionToMakeCrossProfileCalls(
      boolean hasPermissionToMakeCrossProfileCalls) {
    this.hasPermissionToMakeCrossProfileCalls = hasPermissionToMakeCrossProfileCalls;
  }

  boolean hasPermissionToMakeCrossProfileCalls() {
    return hasPermissionToMakeCrossProfileCalls;
  }
}
