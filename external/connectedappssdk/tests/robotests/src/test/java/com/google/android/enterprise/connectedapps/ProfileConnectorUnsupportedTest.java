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

import static com.google.common.truth.Truth.assertThat;
import static org.junit.Assert.assertThrows;

import android.app.Application;
import android.os.Build.VERSION_CODES;
import androidx.test.core.app.ApplicationProvider;
import com.google.android.enterprise.connectedapps.annotations.CustomProfileConnector;
import com.google.android.enterprise.connectedapps.exceptions.UnavailableProfileException;
import com.google.android.enterprise.connectedapps.testapp.connector.TestProfileConnector;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.RobolectricTestRunner;
import org.robolectric.annotation.Config;

/** Tests for the {@link CustomProfileConnector} class running on unsupported Android versions. */
@RunWith(RobolectricTestRunner.class)
@Config(maxSdk = VERSION_CODES.N_MR1)
public class ProfileConnectorUnsupportedTest {
  private final Application context = ApplicationProvider.getApplicationContext();
  private final TestProfileConnector testProfileConnector = TestProfileConnector.create(context);

  @Test
  public void startConnecting_doesNotCrash() {
    testProfileConnector.startConnecting();
  }

  @Test
  public void connect_throwsUnavailableProfileException() {
    assertThrows(UnavailableProfileException.class, testProfileConnector::connect);
  }

  @Test
  public void isAvailable_returnsFalse() {
    assertThat(testProfileConnector.isAvailable()).isFalse();
  }

  @Test
  public void isConnected_returnsFalse() {
    assertThat(testProfileConnector.isConnected()).isFalse();
  }

  @Test
  public void stopManualConnectionManagement_doesNotCrash() {
    testProfileConnector.stopManualConnectionManagement();
  }

  @Test
  public void crossProfileSender_returnsNull() {
    assertThat(testProfileConnector.crossProfileSender()).isNull();
  }

  @Test
  public void registerConnectionListener_doesNotCrash() {
    testProfileConnector.registerConnectionListener(() -> {});
  }

  @Test
  public void unregisterConnectionListener_doesNotCrash() {
    testProfileConnector.unregisterConnectionListener(() -> {});
  }

  @Test
  public void registerAvailabilityListener_doesNotCrash() {
    testProfileConnector.registerAvailabilityListener(() -> {});
  }

  @Test
  public void unregisterAvailabilityListener_doesNotCrash() {
    testProfileConnector.unregisterAvailabilityListener(() -> {});
  }

  @Test
  public void utils_returnsInstance() {
    assertThat(testProfileConnector.utils()).isNotNull();
  }
}
