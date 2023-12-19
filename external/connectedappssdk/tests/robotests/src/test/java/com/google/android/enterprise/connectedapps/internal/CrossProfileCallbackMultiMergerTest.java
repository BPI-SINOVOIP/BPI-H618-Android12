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
package com.google.android.enterprise.connectedapps.internal;

import static com.google.common.truth.Truth.assertThat;

import android.os.Build.VERSION_CODES;
import com.google.android.enterprise.connectedapps.Profile;
import com.google.android.enterprise.connectedapps.internal.CrossProfileCallbackMultiMerger.CrossProfileCallbackMultiMergerCompleteListener;
import java.util.Map;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.RobolectricTestRunner;
import org.robolectric.annotation.Config;

@RunWith(RobolectricTestRunner.class)
@Config(minSdk = VERSION_CODES.O)
public class CrossProfileCallbackMultiMergerTest {

  static class TestStringListener
      implements CrossProfileCallbackMultiMergerCompleteListener<String> {
    int timesResultsPosted = 0;
    Map<Profile, String> results;

    @Override
    public void onResult(Map<Profile, String> results) {
      timesResultsPosted++;
      this.results = results;
    }
  }

  private final Profile profile0 = Profile.fromInt(0);
  private final Profile profile1 = Profile.fromInt(1);
  private final Profile profile2 = Profile.fromInt(2);
  private static final String STRING = "String";

  private final TestStringListener stringListener = new TestStringListener();

  @Test
  public void onResult_expectedResultsNotReached_doesNotReportResult() {
    int expectedResults = 2;
    CrossProfileCallbackMultiMerger<String> merger =
        new CrossProfileCallbackMultiMerger<>(expectedResults, stringListener);

    merger.onResult(profile0, STRING);

    assertThat(stringListener.timesResultsPosted).isEqualTo(0);
  }

  @Test
  public void onResult_expectedResultsReached_doesReportResult() {
    int expectedResults = 2;
    CrossProfileCallbackMultiMerger<String> merger =
        new CrossProfileCallbackMultiMerger<>(expectedResults, stringListener);
    merger.onResult(profile0, STRING);

    merger.onResult(profile1, STRING);

    assertThat(stringListener.timesResultsPosted).isEqualTo(1);
  }

  @Test
  public void onResult_reportsCorrectResults() {
    int expectedResults = 2;
    CrossProfileCallbackMultiMerger<String> merger =
        new CrossProfileCallbackMultiMerger<>(expectedResults, stringListener);
    merger.onResult(profile0, STRING);

    merger.onResult(profile1, STRING);

    assertThat(stringListener.results.get(profile0)).isEqualTo(STRING);
    assertThat(stringListener.results.get(profile1)).isEqualTo(STRING);
  }

  @Test
  public void onResult_sameProfileReportsMultipleTimes_ignored() {
    int expectedResults = 2;
    CrossProfileCallbackMultiMerger<String> merger =
        new CrossProfileCallbackMultiMerger<>(expectedResults, stringListener);
    merger.onResult(profile0, STRING);

    merger.onResult(profile0, STRING);

    assertThat(stringListener.timesResultsPosted).isEqualTo(0);
  }

  @Test
  public void onResult_newResult_resultAlreadyReported_ignored() {
    int expectedResults = 2;
    CrossProfileCallbackMultiMerger<String> merger =
        new CrossProfileCallbackMultiMerger<>(expectedResults, stringListener);
    merger.onResult(profile0, STRING);
    merger.onResult(profile1, STRING);

    merger.onResult(profile2, STRING);

    assertThat(stringListener.timesResultsPosted).isEqualTo(1);
    assertThat(stringListener.results).doesNotContainKey(profile2);
  }

  @Test
  public void onResult_previousResultMissing_expectedResultsReached_doesReportResult() {
    int expectedResults = 2;
    CrossProfileCallbackMultiMerger<String> merger =
        new CrossProfileCallbackMultiMerger<>(expectedResults, stringListener);
    merger.missingResult(profile0);

    merger.onResult(profile1, STRING);

    assertThat(stringListener.timesResultsPosted).isEqualTo(1);
  }

  @Test
  public void missingResult_allResultsMissing_doesReportResult() {
    int expectedResults = 2;
    CrossProfileCallbackMultiMerger<String> merger =
        new CrossProfileCallbackMultiMerger<>(expectedResults, stringListener);
    merger.missingResult(profile0);

    merger.missingResult(profile1);

    assertThat(stringListener.timesResultsPosted).isEqualTo(1);
  }

  @Test
  public void missingResult_expectedResultsReached_doesReportResult() {
    int expectedResults = 2;
    CrossProfileCallbackMultiMerger<String> merger =
        new CrossProfileCallbackMultiMerger<>(expectedResults, stringListener);
    merger.onResult(profile1, STRING);

    merger.missingResult(profile0);

    assertThat(stringListener.timesResultsPosted).isEqualTo(1);
  }

  @Test
  public void missingResult_expectedResultsNotReached_doesNotReportResult() {
    int expectedResults = 2;
    CrossProfileCallbackMultiMerger<String> merger =
        new CrossProfileCallbackMultiMerger<>(expectedResults, stringListener);

    merger.missingResult(profile0);

    assertThat(stringListener.timesResultsPosted).isEqualTo(0);
  }

  @Test
  public void missingResult_resultAlreadyPosted_doesNotRecord() {
    int expectedResults = 2;
    CrossProfileCallbackMultiMerger<String> merger =
        new CrossProfileCallbackMultiMerger<>(expectedResults, stringListener);
    merger.onResult(profile0, STRING);

    merger.missingResult(profile0);
    merger.onResult(profile1, STRING);

    assertThat(stringListener.results.get(profile0)).isEqualTo(STRING);
  }

  @Test
  public void onResult_resultAlreadyPosted_doesNotRecord() {
    int expectedResults = 2;
    CrossProfileCallbackMultiMerger<String> merger =
        new CrossProfileCallbackMultiMerger<>(expectedResults, stringListener);
    merger.missingResult(profile0);

    merger.onResult(profile0, STRING);
    merger.onResult(profile1, STRING);

    assertThat(stringListener.results).doesNotContainKey(profile0);
  }

  @Test
  public void construct_noExpectedResults_reportsResultImmediately() {
    int expectedResults = 0;

    CrossProfileCallbackMultiMerger<String> ignoredMerger =
        new CrossProfileCallbackMultiMerger<>(expectedResults, stringListener);

    assertThat(stringListener.timesResultsPosted).isEqualTo(1);
    assertThat(stringListener.results).isEmpty();
  }
}
