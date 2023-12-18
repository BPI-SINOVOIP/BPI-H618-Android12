// Copyright 2020 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

/* Keys of the properties found in avd/name.ini and config.ini files.
 *
 * These keys must match their counterpart defined in
 * sdk/sdkmanager/libs/sdklib/src/com/android/sdklib/internal/avd/AvdManager.java
 */


/* -- Keys used in avd/name.ini -- */

/* Absolute path of the AVD content directory.
 */
#define  ROOT_ABS_PATH_KEY    "path"

/* Relative path of the AVD content directory.
 * Path is relative to the bufprint_config_path().
 */
#define  ROOT_REL_PATH_KEY    "path.rel"


/* -- Keys used in config.ini -- */

/* AVD/config.ini key name representing the abi type of the specific avd
 */
#define ABI_TYPE "abi.type"

/* AVD/config.ini key name representing the CPU architecture of the specific avd
 */
#define CPU_ARCH "hw.cpu.arch"
/* the prefix of config.ini keys that will be used for search directories
 * of system images.
 */
#define  SEARCH_PREFIX   "image.sysdir."

/* the maximum number of search path keys we're going to read from the
 * config.ini file
 */
#define  MAX_SEARCH_PATHS  2

/* the config.ini key that will be used to indicate the full relative
 * path to the skin directory (including the skin name).
 */
#define  SKIN_PATH       "skin.path"

/* the config.ini key that will be used to indicate the default skin's name.
 * this is ignored if there is a valid SKIN_PATH entry in the file.
 */
#define  SKIN_NAME       "skin.name"

/* default skin name */
#define  SKIN_DEFAULT    "HVGA"

/* the config.ini key that is used to indicate the absolute path
 * to the SD Card image file, if you don't want to place it in
 * the content directory.
 */
#define  SDCARD_PATH     "sdcard.path"

/* The config.ini key name representing the second path where the emulator looks
 * for system images. Typically this is the path to the platform system image.
 */
#define IMAGES_2 "image.sysdir.2"

/* AVD/config.ini key name representing the presence of the snapshots file.
 */
#define SNAPSHOT_PRESENT "snapshot.present"

/* AVD/config.ini key name representing the size of the SD card.
 */
#define SDCARD_SIZE "sdcard.size"

/* AVD/config.ini key name representing the tag id of the specific avd
 */
#define TAG_ID "tag.id"

/* AVD/config.ini value for tag id of Chrome OS.
 */
#define TAG_ID_CHROMEOS "chromeos"

/* AVD/config.ini key name representing the tag display of the specific avd
 */
#define TAG_DISPLAY "tag.display"
