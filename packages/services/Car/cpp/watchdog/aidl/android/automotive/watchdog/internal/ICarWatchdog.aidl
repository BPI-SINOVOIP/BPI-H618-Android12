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

import android.automotive.watchdog.internal.ComponentType;
import android.automotive.watchdog.internal.ICarWatchdogMonitor;
import android.automotive.watchdog.internal.ICarWatchdogServiceForSystem;
import android.automotive.watchdog.internal.PackageResourceOveruseAction;
import android.automotive.watchdog.internal.ResourceOveruseConfiguration;
import android.automotive.watchdog.internal.StateType;

/**
 * ICarWatchdog is an interface implemented by watchdog server. This interface is used only by the
 * internal services to communicate with the watchdog server.
 * Watchdog service is the counter part of the watchdog server to help communicate with
 * the car service and Java side services.
 * For health check, 3 components are involved: watchdog server, watchdog service, watchdog monitor.
 *   - watchdog server:   1. Checks clients' health status by pinging and waiting for the response.
 *                        2. Monitors I/O overuse for system, OEM and third-party applications.
 *   - watchdog service: is a watchdog client by reporting its health status to the server, and
 *                        at the same time plays a role of watchdog server by checking its clients'
 *                        health status.
 *   - watchdog monitor:  captures and reports the process state of watchdog clients.
 */
interface ICarWatchdog {
  /**
   * Register the CarWatchdogService to the watchdog server.
   * The caller should have system UID. Otherwise, returns security exception binder error.
   *
   * @param service             CarWatchdogService to register.
   */
  void registerCarWatchdogService(in ICarWatchdogServiceForSystem service);

  /**
   * Unregister the CarWatchdogService from the watchdog server.
   * The caller should have system UID. Otherwise, returns security exception binder error.
   *
   * @param service             CarWatchdogService to unregister.
   */
  void unregisterCarWatchdogService(in ICarWatchdogServiceForSystem service);

  /**
   * Register the monitor to the watchdog server.
   * The caller should have system UID. Otherwise, returns security exception binder error.
   *
   * @param monitor             Watchdog monitor to register.
   */
  void registerMonitor(in ICarWatchdogMonitor monitor);

  /**
   * Unregister the monitor from the watchdog server.
   * The caller should have system UID. Otherwise, returns security exception binder error.
   *
   * @param monitor             Watchdog monitor to unregister.
   */
  void unregisterMonitor(in ICarWatchdogMonitor monitor);

  /**
   * Tell watchdog server that the CarWatchdogService is alive together with the status of clients
   * under the CarWatchdogService.
   * The caller should have system UID. Otherwise, returns security exception binder error.
   *
   * @param service              Watchdog service that is responding.
   * @param clientsNotResponding Array of process id of clients which haven't responded to the
   *                             mediator.
   * @param sessionId            Session id given by watchdog server.
   */
  void tellCarWatchdogServiceAlive(
          in ICarWatchdogServiceForSystem service, in int[] clientsNotResponding, in int sessionId);

  /**
   * Tell watchdog server that the monitor has finished dumping process information.
   * The caller should have system UID. Otherwise, returns security exception binder error.
   *
   * @param monitor              Watchdog monitor that is registered to watchdog server.
   * @param pid                  Process id that has been dumped.
   */
  void tellDumpFinished(in ICarWatchdogMonitor monitor, in int pid);

  /**
   * Notify watchdog server about the system state change.
   * The caller should have system UID. Otherwise, returns security exception binder error.
   *
   * @param type                 One of the change types defined in the StateType enum.
   * @param arg1                 First state change information for the specified type.
   * @param arg2                 Second state change information for the specified type.
   *
   * When type is POWER_CYCLE, arg1 should contain the current power cycle of the device.
   * When type is USER_STATE, arg1 and arg2 should contain the user ID and the current user state.
   * When type is BOOT_PHASE, arg1 should contain the current boot phase.
   */
  void notifySystemStateChange(in StateType type, in int arg1, in int arg2);

  /**
   * Update the given resource overuse configurations.
   * The caller should have system UID. Otherwise, returns security exception binder error.
   *
   * @param configs              List of resource overuse configurations.
   */
  void updateResourceOveruseConfigurations(in List<ResourceOveruseConfiguration> configs);

  /**
   * Return the latest list of resource overuse configuration per component.
   * The caller should have system UID. Otherwise, returns security exception binder error.
   *
   * @return configs             List of resource overuse configurations.
   */
  List<ResourceOveruseConfiguration> getResourceOveruseConfigurations();

  /**
   * CarWatchdogService notifies the native service with the actions taken on the resource overusing
   * applications.
   *
   * @param actions              List of actions take on resource overusing packages.
   */
   void actionTakenOnResourceOveruse(in List<PackageResourceOveruseAction> actions);
}
