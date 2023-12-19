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

// BEGIN-INTERNAL
package org.robolectric.shadows;

import static com.google.common.truth.Truth.assertThat;

import static android.os.Build.VERSION_CODES.S;
import static com.google.common.truth.Truth.assertThat;

import android.icu.util.TimeZone;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import java.util.List;
import java.util.stream.Collectors;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.annotation.Config;
import org.robolectric.util.ReflectionHelpers;
import org.robolectric.util.ReflectionHelpers.ClassParameter;

/** Unit tests for {@link ShadowTimeZoneFinderS}. */
@RunWith(AndroidJUnit4.class)
public class ShadowTimeZoneFinderSTest {

  @Test
  @Config(minSdk = S)
  public void lookupTimeZonesByCountry_shouldReturnExpectedTimeZones() throws Exception {
    Class<?> cls = Class.forName("com.android.i18n.timezone.TimeZoneFinder");
    Object timeZoneFinder = ReflectionHelpers.callStaticMethod(cls, "getInstance");
    List<TimeZone> timezones =
            ReflectionHelpers.callInstanceMethod(
                    cls,
                    timeZoneFinder,
                    "lookupTimeZonesByCountry",
                    ClassParameter.from(String.class, "us"));

    assertThat(timezones.stream().map(TimeZone::getID).collect(Collectors.toList()))
            .containsAtLeast("America/Los_Angeles", "America/New_York", "Pacific/Honolulu");
  }
}
// END-INTERNAL
