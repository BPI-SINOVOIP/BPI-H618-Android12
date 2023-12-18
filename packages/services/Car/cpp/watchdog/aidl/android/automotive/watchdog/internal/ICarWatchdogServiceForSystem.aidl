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

package android.automotive.watchdog.internal;

import android.automotive.watchdog.internal.PackageInfo;
import android.automotive.watchdog.internal.PackageIoOveruseStats;
import android.automotive.watchdog.internal.TimeoutLength;

/**
 * ICarWatchdogServiceForSystem interface used by the watchdog server to communicate with the
 * watchdog service.
 *
 * @hide
 */
interface ICarWatchdogServiceForSystem {
  /**
   * Checks if the client is alive.
   *
   * Watchdog server calls this method, expecting the clients will respond within timeout.
   * The final timeout is decided by the server, considering the requested timeout on client
   * registration. If no response from the clients, watchdog server will dump process information
   * and kill them.
   *
   * @param sessionId                   Unique id to identify each health check session.
   * @param timeout                     Final timeout given by the server based on client request.
   */
  oneway void checkIfAlive(in int sessionId, in TimeoutLength timeout);

  /**
   * Notifies the client that it will be forcedly terminated in 1 second.
   */
  oneway void prepareProcessTermination();

  /**
   * Returns the package information for the given UIDs. Only UIDs with package information will be
   * returned.
   *
   * @param uids                        List of UIDs to resolve the package infos.
   * @param vendorPackagePrefixes       List of vendor package prefixes.
   */
  List<PackageInfo> getPackageInfosForUids(
            in int[] uids, in @utf8InCpp List<String> vendorPackagePrefixes);

  /**
   * Pushes the latest I/O overuse stats to the watchdog server.
   *
   * @param packageIoOveruseStats       Latest package I/O overuse stats, for all packages, from the
   *                                    recent collection.
   */
  oneway void latestIoOveruseStats(in List<PackageIoOveruseStats> packageIoOveruseStats);

  /**
   * Resets resource overuse stats on the watchdog server side.
   *
   * @param packageNames       Package names for which to reset the stats.
   */
  oneway void resetResourceOveruseStats(in @utf8InCpp List<String> packageNames);
}
