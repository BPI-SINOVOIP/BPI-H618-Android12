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
import com.google.android.enterprise.connectedapps.exceptions.UnavailableProfileException;

/** A {@link ProfileConnector} is used to manage the connection between profiles. */
public interface ProfileConnector {
  /**
   * Start trying to connect to the other profile and start manually managing the connection.
   *
   * <p>This will mean that the connection will not be dropped automatically to save resources.
   *
   * <p>Must be called before interacting with synchronous cross-profile methods.
   *
   * <p>If the connection can not be made, then no errors will be thrown and connections will
   * re-attempted indefinitely.
   *
   * @see #connect()
   * @see #stopManualConnectionManagement()
   */
  void startConnecting();

  /**
   * Attempt to connect to the other profile and start manually managing the connection.
   *
   * <p>This will mean that the connection will not be dropped automatically to save resources.
   *
   * <p>Must be called before interacting with synchronous cross-profile methods.
   *
   * <p>This must not be called from the main thread.
   *
   * @see #startConnecting()
   * @see #stopManualConnectionManagement()
   * @throws UnavailableProfileException If the connection cannot be made.
   */
  void connect() throws UnavailableProfileException;

  /**
   * Stop manual connection management.
   *
   * <p>This can be called after {@link #startConnecting()} to return connection management
   * responsibilities to the SDK.
   *
   * <p>You should not make any synchronous cross-profile calls after calling this method.
   */
  void stopManualConnectionManagement();

  /**
   * Return the {@link CrossProfileSender} being used for this connection.
   *
   * <p>This API should only be used by generated code.
   */
  CrossProfileSender crossProfileSender();

  /**
   * Register a listener to be called when a profile is connected or disconnected.
   *
   * <p>{@link #isConnected()} can be called to check if a connection is established.
   *
   * @see #unregisterConnectionListener(ConnectionListener)
   */
  void registerConnectionListener(ConnectionListener listener);

  /**
   * Unregister a listener registered using {@link #registerConnectionListener(
   * ConnectionListener)}.
   */
  void unregisterConnectionListener(ConnectionListener listener);

  /**
   * Register a listener to be called when a profile becomes available or unavailable.
   *
   * <p>{@link #isAvailable()} can be called to check if a profile is available.
   *
   * @see #unregisterAvailabilityListener(AvailabilityListener)
   */
  void registerAvailabilityListener(AvailabilityListener listener);

  /**
   * Unregister a listener registered using {@link #registerAvailabilityListener(
   * AvailabilityListener)}.
   */
  void unregisterAvailabilityListener(AvailabilityListener listener);

  /**
   * Return true if there is another profile which could be connected to.
   *
   * <p>If this returns true, then asynchronous calls should succeed. Synchronous calls will only
   * succeed if {@link #isConnected()} also returns true.
   */
  boolean isAvailable();

  /**
   * Return true if there is another profile connected.
   *
   * <p>If this returns true, then synchronous calls should succeed unless they are disconnected
   * before the call completes.
   */
  boolean isConnected();

  /** Return an instance of {@link ConnectedAppsUtils} for dealing with this connection. */
  ConnectedAppsUtils utils();

  Permissions permissions();

  /** Return the application context used by this connector. */
  Context applicationContext();

  /**
   * Returns true if this connection is being managed manually.
   *
   * <p>Use {@link #startConnecting()} to begin manual connection management, and {@link
   * #stopManualConnectionManagement()} to end it.
   */
  boolean isManuallyManagingConnection();
}
