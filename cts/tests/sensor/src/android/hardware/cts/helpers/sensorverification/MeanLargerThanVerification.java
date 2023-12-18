/*
 * Copyright (C) 2021 The Android Open Source Project
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

package android.hardware.cts.helpers.sensorverification;

import junit.framework.Assert;

import android.hardware.Sensor;
import android.hardware.cts.helpers.SensorCtsHelper;
import android.hardware.cts.helpers.SensorStats;
import android.hardware.cts.helpers.TestSensorEnvironment;

import java.util.HashMap;
import java.util.Map;

/**
 * A {@link ISensorVerification} which verifies that the any absolute means larger than the expected
 * measurement.
 */
public class MeanLargerThanVerification extends AbstractMeanVerification {
  public static final String PASSED_KEY = "mean_larger_than_passed";

  // sensorType: {expected, threshold}
  private static final Map<Integer, ExpectedValuesAndThresholds> DEFAULTS =
      new HashMap<Integer, ExpectedValuesAndThresholds>(1);

  static {
    // Use a method so that the @deprecation warning can be set for that method only
    setDefaults();
  }

  private final float[] mExpected;
  private final float[] mThresholds;

  /**
   * Construct a {@link MeanLargerThanVerification}
   *
   * @param expected the expected values
   * @param thresholds the thresholds
   */
  public MeanLargerThanVerification(float[] expected, float[] thresholds) {
    mExpected = expected;
    mThresholds = thresholds;
  }

  /**
   * Get the default {@link MeanLargerThanVerification} for a sensor.
   *
   * @param environment the test environment
   * @return the verification or null if the verification does not apply to the sensor.
   */
  public static MeanLargerThanVerification getDefault(TestSensorEnvironment environment) {

    Map<Integer, ExpectedValuesAndThresholds> currentDefaults =
        new HashMap<Integer, ExpectedValuesAndThresholds>(DEFAULTS);

    int sensorType = environment.getSensor().getType();
    if (!currentDefaults.containsKey(sensorType)) {
      return null;
    }
    float[] expected = currentDefaults.get(sensorType).mExpectedValues;
    float[] thresholds = currentDefaults.get(sensorType).mThresholds;
    return new MeanLargerThanVerification(expected, thresholds);
  }

  /**
   * Verify that the any absolute mean is larget than the expected value. Add {@value #PASSED_KEY}
   * and {@value SensorStats#MEAN_KEY} keys to {@link SensorStats}.
   *
   * @throws AssertionError if the verification failed.
   */
  @Override
  public void verify(TestSensorEnvironment environment, SensorStats stats) {
    verify(stats);
  }

  /** Visible for unit tests only. */
  void verify(SensorStats stats) {
    if (getCount() < 1) {
      stats.addValue(PASSED_KEY, true);
      return;
    }

    float[] means = getMeans();
    int meanSize = mExpected.length < means.length ? mExpected.length : means.length;
    boolean failed = true;
    for (int i = 0; i < meanSize; i++) {
      if ((Math.abs(means[i]) >= mExpected[i] + mThresholds[i])) {
        failed = false;
      }
    }

    stats.addValue(PASSED_KEY, !failed);
    stats.addValue(SensorStats.MEAN_KEY, means);

    if (failed) {
      Assert.fail(
          String.format(
              "Mean out of range: mean=%s (expected larger than %s +/- %s)",
              SensorCtsHelper.formatFloatArray(means),
              SensorCtsHelper.formatFloatArray(mExpected),
              SensorCtsHelper.formatFloatArray(mThresholds)));
    }
  }

  @Override
  public MeanLargerThanVerification clone() {
    return new MeanLargerThanVerification(mExpected, mThresholds);
  }

  @SuppressWarnings("deprecation")
  private static void setDefaults() {
    // Uncalibrated gyroscope X,Y,Z should be 0.0005 for a static device but allow a bigger
    // threshold.
    DEFAULTS.put(
        Sensor.TYPE_GYROSCOPE_UNCALIBRATED,
        new ExpectedValuesAndThresholds(
            new float[] {0.0f, 0.0f, 0.0f},
            new float[] {
                0.0005f, /* rad / s */ 0.0005f, /* rad / s */ 0.0005f /* rad / s */
            }));
  }

  private static final class ExpectedValuesAndThresholds {
    private float[] mExpectedValues;
    private float[] mThresholds;

    private ExpectedValuesAndThresholds(float[] expectedValues, float[] thresholds) {
      mExpectedValues = expectedValues;
      mThresholds = thresholds;
    }
  }
}
