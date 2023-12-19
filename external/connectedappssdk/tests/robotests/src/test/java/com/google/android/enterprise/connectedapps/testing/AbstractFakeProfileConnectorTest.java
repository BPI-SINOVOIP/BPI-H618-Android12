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

import static com.google.common.truth.Truth.assertThat;
import static org.junit.Assert.assertThrows;

import android.content.Context;
import android.os.Build.VERSION_CODES;
import androidx.test.core.app.ApplicationProvider;
import com.google.android.enterprise.connectedapps.ConnectedAppsUtils;
import com.google.android.enterprise.connectedapps.TestAvailabilityListener;
import com.google.android.enterprise.connectedapps.TestConnectionListener;
import com.google.android.enterprise.connectedapps.annotations.CustomProfileConnector.ProfileType;
import com.google.android.enterprise.connectedapps.exceptions.UnavailableProfileException;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.RobolectricTestRunner;
import org.robolectric.annotation.Config;

@RunWith(RobolectricTestRunner.class)
@Config(minSdk = VERSION_CODES.O)
public class AbstractFakeProfileConnectorTest {
  static class FakeProfileConnector extends AbstractFakeProfileConnector {
    FakeProfileConnector(Context context, ProfileType primaryProfileType) {
      super(context, primaryProfileType);
    }
  }

  private final Context context = ApplicationProvider.getApplicationContext();
  private final FakeProfileConnector fakeProfileConnector =
      new FakeProfileConnector(context, /* primaryProfileType= */ ProfileType.NONE);
  private final TestAvailabilityListener availabilityListener = new TestAvailabilityListener();
  private final TestConnectionListener connectionListener = new TestConnectionListener();

  @Test
  public void startConnecting_connectionIsAvailable_isConnected() {
    fakeProfileConnector.turnOnWorkProfile();

    fakeProfileConnector.startConnecting();

    assertThat(fakeProfileConnector.isConnected()).isTrue();
  }

  @Test
  public void startConnecting_connectionIsAvailable_notifiesConnectionChanged() {
    fakeProfileConnector.turnOnWorkProfile();
    fakeProfileConnector.registerConnectionListener(connectionListener);

    fakeProfileConnector.startConnecting();

    assertThat(connectionListener.connectionChangedCount()).isEqualTo(1);
  }

  @Test
  public void startConnecting_unregisteredConnectionListener_doesNotNotifyConnectionChanged() {
    fakeProfileConnector.turnOnWorkProfile();
    fakeProfileConnector.registerConnectionListener(connectionListener);
    fakeProfileConnector.unregisterConnectionListener(connectionListener);

    fakeProfileConnector.startConnecting();

    assertThat(connectionListener.connectionChangedCount()).isEqualTo(0);
  }

  @Test
  public void startConnecting_connectionIsNotAvailable_doesNotNotifyOfConnectionChanged() {
    fakeProfileConnector.removeWorkProfile();
    fakeProfileConnector.registerConnectionListener(connectionListener);

    fakeProfileConnector.startConnecting();

    assertThat(connectionListener.connectionChangedCount()).isEqualTo(0);
  }

  @Test
  public void connect_connectionIsAvailable_isConnected() throws Exception {
    fakeProfileConnector.turnOnWorkProfile();

    fakeProfileConnector.connect();

    assertThat(fakeProfileConnector.isConnected()).isTrue();
  }

  @Test
  public void connect_connectionIsAvailable_notifiesConnectionChanged() throws Exception {
    fakeProfileConnector.turnOnWorkProfile();
    fakeProfileConnector.registerConnectionListener(connectionListener);

    fakeProfileConnector.connect();

    assertThat(connectionListener.connectionChangedCount()).isEqualTo(1);
  }

  @Test
  public void connect_unregisteredConnectionListener_doesNotNotifyConnectionChanged()
      throws Exception {
    fakeProfileConnector.turnOnWorkProfile();
    fakeProfileConnector.registerConnectionListener(connectionListener);
    fakeProfileConnector.unregisterConnectionListener(connectionListener);

    fakeProfileConnector.connect();

    assertThat(connectionListener.connectionChangedCount()).isEqualTo(0);
  }

  @Test
  public void connect_connectionIsNotAvailable_throwsUnavailableProfileException() {
    fakeProfileConnector.removeWorkProfile();
    fakeProfileConnector.registerConnectionListener(connectionListener);

    assertThrows(UnavailableProfileException.class, fakeProfileConnector::connect);
  }

  @Test
  public void turnOnWorkProfile_workProfileWasOff_notifiesAvailabilityChange() {
    fakeProfileConnector.registerAvailabilityListener(availabilityListener);

    fakeProfileConnector.turnOnWorkProfile();

    assertThat(availabilityListener.availabilityChangedCount()).isEqualTo(1);
  }

  @Test
  public void turnOnWorkProfile_workProfileWasOn_doesNotNotifyAvailabilityChange() {
    fakeProfileConnector.turnOnWorkProfile();
    fakeProfileConnector.registerAvailabilityListener(availabilityListener);

    fakeProfileConnector.turnOnWorkProfile();

    assertThat(availabilityListener.availabilityChangedCount()).isEqualTo(0);
  }

  @Test
  public void turnOffWorkProfile_workProfileWasOn_notifiesAvailabilityChange() {
    fakeProfileConnector.turnOnWorkProfile();
    fakeProfileConnector.registerAvailabilityListener(availabilityListener);

    fakeProfileConnector.turnOffWorkProfile();

    assertThat(availabilityListener.availabilityChangedCount()).isEqualTo(1);
  }

  @Test
  public void turnOffWorkProfile_workProfileWasOff_doesNotNotifyAvailabilityChange() {
    fakeProfileConnector.turnOffWorkProfile();
    fakeProfileConnector.registerAvailabilityListener(availabilityListener);

    fakeProfileConnector.turnOffWorkProfile();

    assertThat(availabilityListener.availabilityChangedCount()).isEqualTo(0);
  }

  @Test
  public void turnOffWorkProfile_wasConnected_notifiesConnectionChange() throws Exception {
    fakeProfileConnector.turnOnWorkProfile();
    fakeProfileConnector.connect();
    fakeProfileConnector.registerConnectionListener(connectionListener);

    fakeProfileConnector.turnOffWorkProfile();

    assertThat(connectionListener.connectionChangedCount()).isEqualTo(1);
  }

  @Test
  public void setRunningOnProfile_setsRunningOnProfile() {
    fakeProfileConnector.setRunningOnProfile(ProfileType.WORK);

    assertThat(fakeProfileConnector.runningOnProfile()).isEqualTo(ProfileType.WORK);
  }

  @Test
  public void setRunningOnWorkProfile_startsWorkProfile() {
    fakeProfileConnector.setRunningOnProfile(ProfileType.WORK);
    fakeProfileConnector.setRunningOnProfile(ProfileType.PERSONAL);

    assertThat(fakeProfileConnector.isAvailable()).isTrue();
  }

  @Test
  public void removeWorkProfile_workProfileBecomesUnavailable() {
    fakeProfileConnector.turnOnWorkProfile();
    fakeProfileConnector.removeWorkProfile();

    assertThat(fakeProfileConnector.isAvailable()).isFalse();
  }

  @Test
  public void isConnected_isConnected_returnsTrue() throws Exception {
    fakeProfileConnector.turnOnWorkProfile();

    fakeProfileConnector.connect();

    assertThat(fakeProfileConnector.isConnected()).isTrue();
  }

  @Test
  public void isConnected_isNotConnected_returnsFalse() {
    fakeProfileConnector.turnOnWorkProfile();

    fakeProfileConnector.disconnect();

    assertThat(fakeProfileConnector.isConnected()).isFalse();
  }

  @Test
  public void getCurrentProfile_getOtherProfile_areDifferent() {
    ConnectedAppsUtils utils = fakeProfileConnector.utils();
    assertThat(utils.getCurrentProfile()).isNotEqualTo(utils.getOtherProfile());
  }

  @Test
  public void getWorkProfile_runningOnWorkProfile_returnsCurrent() {
    ConnectedAppsUtils utils = fakeProfileConnector.utils();
    fakeProfileConnector.setRunningOnProfile(ProfileType.WORK);

    assertThat(utils.getWorkProfile()).isEqualTo(utils.getCurrentProfile());
  }

  @Test
  public void getWorkProfile_runningOnPersonalProfile_returnsOther() {
    ConnectedAppsUtils utils = fakeProfileConnector.utils();
    fakeProfileConnector.setRunningOnProfile(ProfileType.PERSONAL);

    assertThat(utils.getWorkProfile()).isEqualTo(utils.getOtherProfile());
  }

  @Test
  public void getPersonalProfile_runningOnPersonalProfile_returnsCurrent() {
    ConnectedAppsUtils utils = fakeProfileConnector.utils();
    fakeProfileConnector.setRunningOnProfile(ProfileType.PERSONAL);

    assertThat(utils.getPersonalProfile()).isEqualTo(utils.getCurrentProfile());
  }

  @Test
  public void getPersonalProfile_runningOnWorkProfile_returnsOther() {
    ConnectedAppsUtils utils = fakeProfileConnector.utils();
    fakeProfileConnector.setRunningOnProfile(ProfileType.WORK);

    assertThat(utils.getPersonalProfile()).isEqualTo(utils.getOtherProfile());
  }

  @Test
  public void getPrimaryProfile_noPrimaryProfileSet_throwsIllegalStateException() {
    FakeProfileConnector fakeProfileConnector =
        new FakeProfileConnector(context, /* primaryProfileType= */ ProfileType.NONE);
    ConnectedAppsUtils utils = fakeProfileConnector.utils();

    assertThrows(IllegalStateException.class, () -> utils.getPrimaryProfile());
  }

  @Test
  public void getSecondaryProfile_noPrimaryProfileSet_throwsIllegalStateException() {
    FakeProfileConnector fakeProfileConnector =
        new FakeProfileConnector(context, /* primaryProfileType= */ ProfileType.NONE);
    ConnectedAppsUtils utils = fakeProfileConnector.utils();

    assertThrows(IllegalStateException.class, () -> utils.getSecondaryProfile());
  }

  @Test
  public void getPrimaryProfile_primaryProfileSetToWork_returnsWork() {
    FakeProfileConnector fakeProfileConnector =
        new FakeProfileConnector(context, /* primaryProfileType= */ ProfileType.WORK);
    ConnectedAppsUtils utils = fakeProfileConnector.utils();

    assertThat(utils.getPrimaryProfile()).isEqualTo(utils.getWorkProfile());
  }

  @Test
  public void getPrimaryProfile_primaryProfileSetToPersonal_returnsPersonal() {
    FakeProfileConnector fakeProfileConnector =
        new FakeProfileConnector(context, /* primaryProfileType= */ ProfileType.PERSONAL);
    ConnectedAppsUtils utils = fakeProfileConnector.utils();

    assertThat(utils.getPrimaryProfile()).isEqualTo(utils.getPersonalProfile());
  }

  @Test
  public void getSecondaryProfile_primaryProfileSetToWork_returnsPersonal() {
    FakeProfileConnector fakeProfileConnector =
        new FakeProfileConnector(context, /* primaryProfileType= */ ProfileType.WORK);
    ConnectedAppsUtils utils = fakeProfileConnector.utils();

    assertThat(utils.getSecondaryProfile()).isEqualTo(utils.getPersonalProfile());
  }

  @Test
  public void getSecondaryProfile_primaryProfileSetToPersonal_returnsWork() {
    FakeProfileConnector fakeProfileConnector =
        new FakeProfileConnector(context, /* primaryProfileType= */ ProfileType.PERSONAL);
    ConnectedAppsUtils utils = fakeProfileConnector.utils();

    assertThat(utils.getSecondaryProfile()).isEqualTo(utils.getWorkProfile());
  }

  @Test
  public void runningOnWork_runningOnWork_returnsTrue() {
    fakeProfileConnector.setRunningOnProfile(ProfileType.WORK);

    assertThat(fakeProfileConnector.utils().runningOnWork()).isTrue();
  }

  @Test
  public void runningOnWork_runningOnPersonal_returnsFalse() {
    fakeProfileConnector.setRunningOnProfile(ProfileType.PERSONAL);

    assertThat(fakeProfileConnector.utils().runningOnWork()).isFalse();
  }

  @Test
  public void runningOnPersonal_runningOnPersonal_returnsTrue() {
    fakeProfileConnector.setRunningOnProfile(ProfileType.PERSONAL);

    assertThat(fakeProfileConnector.utils().runningOnPersonal()).isTrue();
  }

  @Test
  public void runningOnPersonal_runningOnWork_returnsFalse() {
    fakeProfileConnector.setRunningOnProfile(ProfileType.WORK);

    assertThat(fakeProfileConnector.utils().runningOnPersonal()).isFalse();
  }

  @Test
  public void canMakeCrossProfileCalls_defaultsToTrue() {
    assertThat(fakeProfileConnector.permissions().canMakeCrossProfileCalls()).isTrue();
  }

  @Test
  public void canMakeCrossProfileCalls_setToFalse_returnsFalse() {
    fakeProfileConnector.setHasPermissionToMakeCrossProfileCalls(false);

    assertThat(fakeProfileConnector.permissions().canMakeCrossProfileCalls()).isFalse();
  }

  @Test
  public void canMakeCrossProfileCalls_setToTrue_returnsTrue() {
    fakeProfileConnector.setHasPermissionToMakeCrossProfileCalls(false);
    fakeProfileConnector.setHasPermissionToMakeCrossProfileCalls(true);

    assertThat(fakeProfileConnector.permissions().canMakeCrossProfileCalls()).isTrue();
  }

  @Test
  public void isManuallyManagingConnection_returnsFalse() {
    assertThat(fakeProfileConnector.isManuallyManagingConnection()).isFalse();
  }

  @Test
  public void isManuallyManagingConnection_hasStartedManuallyConnecting_returnsTrue() {
    fakeProfileConnector.startConnecting();

    assertThat(fakeProfileConnector.isManuallyManagingConnection()).isTrue();
  }

  @Test
  public void isManuallyManagingConnection_hasManuallyConnected_returnsTrue() throws Exception {
    fakeProfileConnector.turnOnWorkProfile();
    fakeProfileConnector.connect();

    assertThat(fakeProfileConnector.isManuallyManagingConnection()).isTrue();
  }

  @Test
  public void isManuallyManagingConnection_hasCalledStopManualConnectionManagement_returnsFalse() {
    fakeProfileConnector.startConnecting();

    fakeProfileConnector.stopManualConnectionManagement();

    assertThat(fakeProfileConnector.isManuallyManagingConnection()).isFalse();
  }

  @Test
  public void timeoutConnection_isManuallyManagingConnection_doesNotDisconnect() throws Exception {
    fakeProfileConnector.turnOnWorkProfile();
    fakeProfileConnector.connect();

    fakeProfileConnector.timeoutConnection();

    assertThat(fakeProfileConnector.isConnected()).isTrue();
  }

  @Test
  public void timeoutConnection_isNotManuallyManagingConnection_disconnects() {
    fakeProfileConnector.turnOnWorkProfile();
    fakeProfileConnector.stopManualConnectionManagement();

    fakeProfileConnector.timeoutConnection();

    assertThat(fakeProfileConnector.isConnected()).isFalse();
  }
}
