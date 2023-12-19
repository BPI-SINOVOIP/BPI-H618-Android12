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
import com.google.android.enterprise.connectedapps.annotations.AvailabilityRestrictions;
import com.google.android.enterprise.connectedapps.exceptions.UnavailableProfileException;
import java.util.concurrent.ScheduledExecutorService;
import org.checkerframework.checker.nullness.qual.Nullable;

/** Standard implementation of {@link UserConnector}. */
public abstract class AbstractUserConnector
    implements UserConnector, ConnectionListener, AvailabilityListener {

  public AbstractUserConnector(Class<? extends UserConnector> userConnectorClass, Builder builder) {
    if (userConnectorClass == null || builder == null || builder.context == null) {
      throw new NullPointerException();
    }
  }

  @Override
  public void availabilityChanged() {}

  @Override
  public void connectionChanged() {}

  @Override
  public void startConnecting(UserHandle userHandle) {}

  @Override
  public void connect(UserHandle userHandle) throws UnavailableProfileException {}

  @Override
  public void stopManualConnectionManagement(UserHandle userHandle) {}

  @Override
  public CrossProfileSender crossProfileSender(UserHandle userHandle) {
    return null;
  }

  @Override
  public void registerConnectionListener(UserHandle userHandle, ConnectionListener listener) {}

  @Override
  public void unregisterConnectionListener(UserHandle userHandle, ConnectionListener listener) {}

  @Override
  public void registerAvailabilityListener(UserHandle userHandle, AvailabilityListener listener) {}

  @Override
  public void unregisterAvailabilityListener(
      UserHandle userHandle, AvailabilityListener listener) {}

  @Override
  public boolean isAvailable(UserHandle userHandle) {
    return false;
  }

  @Override
  public boolean isConnected(UserHandle userHandle) {
    return false;
  }

  @Override
  public ConnectedAppsUtils utils(UserHandle userHandle) {
    return null;
  }

  @Override
  public Permissions permissions(UserHandle userHandle) {
    return null;
  }

  @Override
  public Context applicationContext(UserHandle userHandle) {
    return null;
  }

  @Override
  public boolean isManuallyManagingConnection(UserHandle userHandle) {
    return false;
  }

  /** A builder for an {@link AbstractUserConnector}. */
  public static final class Builder {
    @Nullable ScheduledExecutorService scheduledExecutorService;
    @Nullable ConnectionBinder binder;
    @Nullable AvailabilityRestrictions availabilityRestrictions;
    Context context;
    String serviceClassName;

    public Builder setContext(Context context) {
      this.context = context;
      return this;
    }

    public Builder setScheduledExecutorService(ScheduledExecutorService scheduledExecutorService) {
      this.scheduledExecutorService = scheduledExecutorService;
      return this;
    }

    public Builder setBinder(ConnectionBinder binder) {
      this.binder = binder;
      return this;
    }

    public Builder setServiceClassName(String serviceClassName) {
      this.serviceClassName = serviceClassName;
      return this;
    }

    public Builder setAvailabilityRestrictions(AvailabilityRestrictions availabilityRestrictions) {
      this.availabilityRestrictions = availabilityRestrictions;
      return this;
    }
  }
}
