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

import static android.os.Build.VERSION_CODES.S;
import static java.nio.charset.StandardCharsets.UTF_8;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import com.android.i18n.timezone.TimeZoneFinder;
import org.robolectric.annotation.Implementation;
import org.robolectric.annotation.Implements;

/** Shadow for TimeZoneFinder on S or above. */
@Implements(
    value = TimeZoneFinder.class,
    minSdk = S,
    isInAndroidSdk = false,
    looseSignatures = true)
public class ShadowTimeZoneFinderS {

  private static final String TZLOOKUP_PATH = "/usr/share/zoneinfo/tzlookup.xml";

  @Implementation
  protected static Object getInstance() {
    return TimeZoneFinder.createInstanceForTests(readTzlookup());
  }

  /**
   * Reads tzlookup.xml from the files bundled inside android-all JARs. We need to read the file
   * instead of passing in the path because the real implementation uses {@link java.nio.file.Paths}
   * which doesn't support reading from JARs.
   */
  private static String readTzlookup() {
    StringBuilder stringBuilder = new StringBuilder();
    InputStream is = null;
    try {
      try {
        is = ShadowTimeZoneFinder.class.getResourceAsStream(TZLOOKUP_PATH);
        BufferedReader reader = new BufferedReader(new InputStreamReader(is, UTF_8));
        for (String line; (line = reader.readLine()) != null; ) {
          stringBuilder.append(line);
        }
      } finally {
        if (is != null) {
          is.close();
        }
      }
    } catch (IOException e) {
      // ignore
    }

    return stringBuilder.toString();
  }
}
// END-INTERNAL
