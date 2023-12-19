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
package com.google.android.enterprise.connectedapps;

import android.content.Context;
import android.os.UserHandle;
import com.google.android.enterprise.connectedapps.exceptions.UnavailableProfileException;

/** A {@link UserConnector} is used to manage the connection between users. */
public interface UserConnector {
  /**
   * Start trying to connect to another user and start manually managing the connection.
   *
   * <p>This will mean that the connection will not be dropped automatically to save resources.
   *
   * <p>Must be called before interacting with synchronous cross-user methods.
   *
   * <p>If the connection can not be made, then no errors will be thrown and connections will
   * re-attempted indefinitely.
   *
   * @see #connect(UserHandle)
   * @see #stopManualConnectionManagement(UserHandle)
   */
  void startConnecting(UserHandle userHandle);

  /**
   * Attempt to connect to the user and start manually managing the connection.
   *
   * <p>This will mean that the connection will not be dropped automatically to save resources.
   *
   * <p>Must be called before interacting with synchronous cross-profile methods.
   *
   * <p>This must not be called from the main thread.
   *
   * @see #startConnecting(UserHandle)
   * @see #stopManualConnectionManagement(UserHandle)
   * @throws UnavailableProfileException If the connection cannot be made.
   */
  void connect(UserHandle userHandle) throws UnavailableProfileException;

  /**
   * Stop manual connection management.
   *
   * <p>This can be called after {@link #startConnecting(UserHandle)} to return connection
   * management responsibilities to the SDK.
   *
   * <p>You should not make any synchronous cross-profile calls after calling this method.
   */
  void stopManualConnectionManagement(UserHandle userHandle);

  /**
   * Return the {@link CrossProfileSender} being used for the connection to the user.
   *
   * <p>This API should only be used by generated code.
   */
  CrossProfileSender crossProfileSender(UserHandle userHandle);

  /**
   * Register a listener to be called when the user is connected or disconnected.
   *
   * <p>{@link #isConnected(UserHandle)} can be called to check if a connection is established.
   *
   * @see #unregisterConnectionListener(UserHandle, ConnectionListener)
   */
  void registerConnectionListener(UserHandle userHandle, ConnectionListener listener);

  /**
   * Unregister a listener registered using {@link #registerConnectionListener(UserHandle,
   * ConnectionListener)}.
   */
  void unregisterConnectionListener(UserHandle userHandle, ConnectionListener listener);

  /**
   * Register a listener to be called when a user becomes available or unavailable.
   *
   * <p>{@link #isAvailable(UserHandle)} can be called to check if a user is available.
   *
   * @see #unregisterAvailabilityListener(UserHandle, AvailabilityListener)
   */
  void registerAvailabilityListener(UserHandle userHandle, AvailabilityListener listener);

  /**
   * Unregister a listener registered using {@link #registerAvailabilityListener(UserHandle,
   * AvailabilityListener)}.
   */
  void unregisterAvailabilityListener(UserHandle userHandle, AvailabilityListener listener);

  /**
   * Return true if the user can be connected to.
   *
   * <p>If this returns true, then asynchronous calls should succeed. Synchronous calls will only
   * succeed if {@link #isConnected(UserHandle)} also returns true.
   */
  boolean isAvailable(UserHandle userHandle);

  /**
   * Return true if the user is connected.
   *
   * <p>If this returns true, then synchronous calls should succeed unless they are disconnected
   * before the call completes.
   */
  boolean isConnected(UserHandle userHandle);

  /**
   * Return an instance of {@link ConnectedAppsUtils} for dealing with the connection to the user.
   */
  ConnectedAppsUtils utils(UserHandle userHandle);

  Permissions permissions(UserHandle userHandle);

  /** Return the application context used by the user. */
  Context applicationContext(UserHandle userHandle);

  /**
   * Returns true if the connection to the user is being managed manually.
   *
   * <p>Use {@link #startConnecting(UserHandle)} to begin manual connection management, and {@link
   * #stopManualConnectionManagement(UserHandle)} to end it.
   */
  boolean isManuallyManagingConnection(UserHandle userHandle);
}
