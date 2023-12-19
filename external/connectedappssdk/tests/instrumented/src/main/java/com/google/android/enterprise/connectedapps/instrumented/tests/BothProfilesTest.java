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
package com.google.android.enterprise.connectedapps.instrumented.tests;

import static com.google.common.truth.Truth.assertThat;

import android.app.Application;
import androidx.test.core.app.ApplicationProvider;
import com.google.android.enterprise.connectedapps.Profile;
import com.google.android.enterprise.connectedapps.instrumented.utils.BlockingStringCallbackListenerMulti;
import com.google.android.enterprise.connectedapps.instrumented.utils.InstrumentedTestUtilities;
import com.google.android.enterprise.connectedapps.testapp.connector.TestProfileConnector;
import com.google.android.enterprise.connectedapps.testapp.types.ProfileTestCrossProfileTypeWhichNeedsContext;
import java.util.Map;
import java.util.concurrent.ExecutionException;

import org.junit.After;
import org.junit.AfterClass;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

/** Tests regarding calling a method on both profiles. */
@RunWith(JUnit4.class)
public class BothProfilesTest {
  private static final int FIVE_SECONDS = 5000;
  private static final String STRING = "String";

  private static final Application context = ApplicationProvider.getApplicationContext();

  private static final TestProfileConnector connector = TestProfileConnector.create(context);
  private static final InstrumentedTestUtilities utilities =
      new InstrumentedTestUtilities(context, connector);

  private final ProfileTestCrossProfileTypeWhichNeedsContext type =
      ProfileTestCrossProfileTypeWhichNeedsContext.create(connector);

  @Before
  public void setup() {
    utilities.ensureReadyForCrossProfileCalls();
  }

  @AfterClass
  public static void teardown() {
    utilities.ensureNoWorkProfile();
  }

  /** This test could not be covered by Robolectric. */
  @Test
  public void both_synchronous_timesOutOnWorkProfile_timeoutNotEnforcedOnSynchronousCalls() {
    utilities.manuallyConnectAndWait();

    Map<Profile, String> result =
        type.both()
            .timeout(FIVE_SECONDS)
            .identityStringMethodWhichDelays10SecondsOnWorkProfile(STRING);

    assertThat(result).containsKey(connector.utils().getPersonalProfile());
    assertThat(result).containsKey(connector.utils().getWorkProfile());
  }

  /** This test could not be covered by Robolectric. */
  @Test
  public void both_async_timesOutOnWorkProfile_onlyIncludesPersonalProfile()
      throws InterruptedException {

    BlockingStringCallbackListenerMulti callbackListener =
        new BlockingStringCallbackListenerMulti();

    type.both()
        .timeout(FIVE_SECONDS)
        .asyncIdentityStringMethodWhichDelays10SecondsOnWorkProfile(STRING, callbackListener);
    Map<Profile, String> result = callbackListener.await();

    assertThat(result).containsKey(connector.utils().getPersonalProfile());
    assertThat(result).doesNotContainKey(connector.utils().getWorkProfile());
  }

  /** This test could not be covered by Robolectric. */
  @Test
  public void both_future_timesOutOnWorkProfile_onlyIncludesPersonalProfile()
      throws InterruptedException, ExecutionException {
    Map<Profile, String> result =
        type.both()
            .timeout(FIVE_SECONDS)
            .futureIdentityStringMethodWhichDelays10SecondsOnWorkProfile(STRING)
            .get();

    assertThat(result).containsKey(connector.utils().getPersonalProfile());
    assertThat(result).doesNotContainKey(connector.utils().getWorkProfile());
  }
}
