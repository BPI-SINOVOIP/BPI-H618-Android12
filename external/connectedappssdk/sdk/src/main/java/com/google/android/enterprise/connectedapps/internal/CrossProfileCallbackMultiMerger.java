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

import com.google.android.enterprise.connectedapps.Profile;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.Set;

/** Receives a number of async results, merge them, and relays the merged results. */
public class CrossProfileCallbackMultiMerger<R> {

  // TODO: This should time-out if it doesn't receive the expected number

  /**
   * A listener for results from the {@link CrossProfileCallbackMultiMerger}.
   *
   * <p>This will be called when all results are received.
   */
  public interface CrossProfileCallbackMultiMergerCompleteListener<R> {
    void onResult(Map<Profile, R> results);
  }

  private boolean hasCompleted = false;
  private final int expectedResults;
  private final Map<Profile, R> results = new HashMap<>();
  private final Set<Profile> missedResults = new HashSet<>();
  private final CrossProfileCallbackMultiMergerCompleteListener<R> listener;

  public CrossProfileCallbackMultiMerger(
      int expectedResults, CrossProfileCallbackMultiMergerCompleteListener<R> listener) {
    if (listener == null) {
      throw new NullPointerException();
    }

    this.expectedResults = expectedResults;
    this.listener = listener;

    checkIfCompleted();
  }

  /**
   * Indicate that a result is missing, so results can be posted with fewer than expected.
   *
   * <p>This should be called for every missing result. For example, if a remote call fails.
   */
  public void missingResult(Profile profileId) {
    if (hasCompleted) {
      // Once a result has been posted we don't check any more
      return;
    }

    if (results.containsKey(profileId) || missedResults.contains(profileId)) {
      // Only one result per profile is accepted
      return;
    }
    missedResults.add(profileId);

    checkIfCompleted();
  }

  public void onResult(Profile profileId, R value) {
    if (hasCompleted) {
      // Once a result has been posted we don't check any more
      return;
    }
    if (results.containsKey(profileId) || missedResults.contains(profileId)) {
      // Only one result per profile is accepted
      return;
    }

    results.put(profileId, value);

    checkIfCompleted();
  }

  private void checkIfCompleted() {
    if (results.size() + missedResults.size() >= expectedResults) {
      hasCompleted = true;
      listener.onResult(results);
    }
  }
}
