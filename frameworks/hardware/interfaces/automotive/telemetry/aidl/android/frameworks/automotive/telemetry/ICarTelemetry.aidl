/*
 * Copyright (c) 2021, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package android.frameworks.automotive.telemetry;

import android.frameworks.automotive.telemetry.CarData;

/**
 * This service collects data from varios other services, buffers them, and delivers to the
 * listening scripts running in CarTelemetryService.
 */
@VintfStability
interface ICarTelemetry {
  /**
   * Sends a list of CarData to CarTelemetry.
   *
   * <p>Total `dataList` size is limited to 10KB, if it's larger than that, all the data is dropped.
   * Note that Binder has 1MB data limit from all the clients combined, please keep that in
   * mind when writing frequent data.
   *
   * @throws {@code Parcel#EX_ILLEGAL_ARGUMENT} if {@code dataList} is too large.
   */
  void write(in CarData[] dataList);
}
