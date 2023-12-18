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

package android.automotive.watchdog.internal;

/**
 * Describes the action taken on resource overuse.
 */
@Backing(type="int")
enum ResourceOveruseActionType {
  /**
   * The package is not killed because either it is not killable, internal error occurred, or
   * already disabled.
   */
  NOT_KILLED,

  /**
   * The package is not killed as the user opted-out the package from killing on resource overuse.
   */
  NOT_KILLED_USER_OPTED,

  /**
   * The package is killed on resource overuse.
   */
  KILLED,

  /**
   * The package is killed as it has recurring resource overuse pattern.
   */
  KILLED_RECURRING_OVERUSE,
}
