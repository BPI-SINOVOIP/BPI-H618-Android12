/*
 * Copyright (C) 2016 The Android Open Source Project
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

#include "chre/platform/shared/nanoapp_support_lib_dso.h"

#include <chre.h>

#include "chre/platform/shared/debug_dump.h"
#include "chre/util/macros.h"
#include "chre/util/system/napp_permissions.h"
#ifdef CHRE_NANOAPP_USES_WIFI
#include "chre/util/system/wifi_util.h"
#endif

/**
 * @file
 * The Nanoapp Support Library (NSL) that gets built with nanoapps to act as an
 * intermediary to the reference CHRE implementation. It provides hooks so the
 * app can be registered with the system, and also provides a layer where we can
 * implement cross-version compatibility features as needed.
 */

namespace {

constexpr uint32_t kNanoappPermissions = 0
// DO NOT USE this macro outside of specific CHQTS nanoapps. This is only used
// to allow testing of invalid permission declarations.
#ifdef CHRE_TEST_NANOAPP_PERMS
                                         | CHRE_TEST_NANOAPP_PERMS
#else
#ifdef CHRE_NANOAPP_USES_AUDIO
                                         | static_cast<uint32_t>(
                                               chre::NanoappPermissions::
                                                   CHRE_PERMS_AUDIO)
#endif
#ifdef CHRE_NANOAPP_USES_GNSS
                                         | static_cast<uint32_t>(
                                               chre::NanoappPermissions::
                                                   CHRE_PERMS_GNSS)
#endif
#ifdef CHRE_NANOAPP_USES_WIFI
                                         | static_cast<uint32_t>(
                                               chre::NanoappPermissions::
                                                   CHRE_PERMS_WIFI)
#endif
#ifdef CHRE_NANOAPP_USES_WWAN
                                         | static_cast<uint32_t>(
                                               chre::NanoappPermissions::
                                                   CHRE_PERMS_WWAN)
#endif
#endif  // CHRE_TEST_NANOAPP_PERMS
    ;

#if defined(CHRE_SLPI_UIMG_ENABLED) || defined(CHRE_TCM_ENABLED)
constexpr int kIsTcmNanoapp = 1;
#else
constexpr int kIsTcmNanoapp = 0;
#endif  // CHRE_SLPI_UIMG_ENABLED

#if !defined(CHRE_NANOAPP_DISABLE_BACKCOMPAT) && defined(CHRE_NANOAPP_USES_GNSS)
// Return a v1.3+ GnssLocationEvent for the nanoapp when running on a v1.2-
// platform.
chreGnssLocationEvent translateLegacyGnssLocation(
    const chreGnssLocationEvent &legacyEvent) {
  // Copy v1.2- fields over to a v1.3+ event.
  chreGnssLocationEvent newEvent = {};
  newEvent.timestamp = legacyEvent.timestamp;
  newEvent.latitude_deg_e7 = legacyEvent.latitude_deg_e7;
  newEvent.longitude_deg_e7 = legacyEvent.longitude_deg_e7;
  newEvent.altitude = legacyEvent.altitude;
  newEvent.speed = legacyEvent.speed;
  newEvent.bearing = legacyEvent.bearing;
  newEvent.accuracy = legacyEvent.accuracy;
  newEvent.flags = legacyEvent.flags;

  // Unset flags that are defined in v1.3+ but not in v1.2-.
  newEvent.flags &= ~(CHRE_GPS_LOCATION_HAS_ALTITUDE_ACCURACY |
                      CHRE_GPS_LOCATION_HAS_SPEED_ACCURACY |
                      CHRE_GPS_LOCATION_HAS_BEARING_ACCURACY);
  return newEvent;
}

void nanoappHandleEventCompat(uint32_t senderInstanceId, uint16_t eventType,
                              const void *eventData) {
  if (eventType == CHRE_EVENT_GNSS_LOCATION &&
      chreGetApiVersion() < CHRE_API_VERSION_1_3) {
    chreGnssLocationEvent event = translateLegacyGnssLocation(
        *static_cast<const chreGnssLocationEvent *>(eventData));
    nanoappHandleEvent(senderInstanceId, eventType, &event);
  } else {
    nanoappHandleEvent(senderInstanceId, eventType, eventData);
  }
}
#endif

}  // anonymous namespace

//! Used to determine the given unstable ID that was provided when building this
//! nanoapp, if any. The symbol is placed in its own section so it can be
//! stripped to determine if the nanoapp changed compared to a previous version.
//! We also align the variable to match the minimum alignment of the surrounding
//! sections, since for compilers with a default size-1 alignment, there might
//! be a spill-over from the previous segment if not zero-padded, when we
//! attempt to read the string.
DLL_EXPORT extern "C" const char _chreNanoappUnstableId[]
    __attribute__((section(".unstable_id"))) __attribute__((aligned(8))) =
        NANOAPP_UNSTABLE_ID;

DLL_EXPORT extern "C" const struct chreNslNanoappInfo _chreNslDsoNanoappInfo = {
    /* magic */ CHRE_NSL_NANOAPP_INFO_MAGIC,
    /* structMinorVersion */ CHRE_NSL_NANOAPP_INFO_STRUCT_MINOR_VERSION,
    /* isSystemNanoapp */ NANOAPP_IS_SYSTEM_NANOAPP,
    /* isTcmNanoapp */ kIsTcmNanoapp,
    /* reservedFlags */ 0,
    /* reserved */ 0,
    /* targetApiVersion */ CHRE_API_VERSION,

    // These values are supplied by the build environment.
    /* vendor */ NANOAPP_VENDOR_STRING,
    /* name */ NANOAPP_NAME_STRING,
    /* appId */ NANOAPP_ID,
    /* appVersion */ NANOAPP_VERSION,
    /* entryPoints */
    {
        /* start */ nanoappStart,
#if !defined(CHRE_NANOAPP_DISABLE_BACKCOMPAT) && defined(CHRE_NANOAPP_USES_GNSS)
        /* handleEvent */ nanoappHandleEventCompat,
#else
        /* handleEvent */ nanoappHandleEvent,
#endif
        /* end */ nanoappEnd,
    },
    /* appVersionString */ _chreNanoappUnstableId,
    /* appPermissions */ kNanoappPermissions,
};

// The code section below provides default implementations for new symbols
// introduced in CHRE API v1.2+ to provide binary compatibility with previous
// CHRE implementations. Note that we don't presently include symbols for v1.1,
// as the current known set of CHRE platforms that use this NSL implementation
// are all v1.1+.
// If a nanoapp knows that it is only targeting the latest platform version, it
// can define the CHRE_NANOAPP_DISABLE_BACKCOMPAT flag, so this indirection will
// be avoided at the expense of a nanoapp not being able to load at all on prior
// implementations.

#ifndef CHRE_NANOAPP_DISABLE_BACKCOMPAT

#include <dlfcn.h>

/**
 * Lazily calls dlsym to find the function pointer for a given function
 * (provided without quotes) in another library (i.e. the CHRE platform DSO),
 * caching and returning the result.
 */
#define CHRE_NSL_LAZY_LOOKUP(functionName)            \
  ({                                                  \
    static bool lookupPerformed = false;              \
    static decltype(functionName) *fptr = nullptr;    \
    if (!lookupPerformed) {                           \
      fptr = reinterpret_cast<decltype(fptr)>(        \
          dlsym(RTLD_NEXT, STRINGIFY(functionName))); \
      lookupPerformed = true;                         \
    }                                                 \
    fptr;                                             \
  })

#ifdef CHRE_NANOAPP_USES_AUDIO

WEAK_SYMBOL
bool chreAudioGetSource(uint32_t handle, struct chreAudioSource *audioSource) {
  auto *fptr = CHRE_NSL_LAZY_LOOKUP(chreAudioGetSource);
  return (fptr != nullptr) ? fptr(handle, audioSource) : false;
}

WEAK_SYMBOL
bool chreAudioConfigureSource(uint32_t handle, bool enable,
                              uint64_t bufferDuration,
                              uint64_t deliveryInterval) {
  auto *fptr = CHRE_NSL_LAZY_LOOKUP(chreAudioConfigureSource);
  return (fptr != nullptr)
             ? fptr(handle, enable, bufferDuration, deliveryInterval)
             : false;
}

WEAK_SYMBOL
bool chreAudioGetStatus(uint32_t handle, struct chreAudioSourceStatus *status) {
  auto *fptr = CHRE_NSL_LAZY_LOOKUP(chreAudioGetStatus);
  return (fptr != nullptr) ? fptr(handle, status) : false;
}

#endif /* CHRE_NANOAPP_USES_AUDIO */

WEAK_SYMBOL
void chreConfigureHostSleepStateEvents(bool enable) {
  auto *fptr = CHRE_NSL_LAZY_LOOKUP(chreConfigureHostSleepStateEvents);
  if (fptr != nullptr) {
    fptr(enable);
  }
}

WEAK_SYMBOL
bool chreIsHostAwake(void) {
  auto *fptr = CHRE_NSL_LAZY_LOOKUP(chreIsHostAwake);
  return (fptr != nullptr) ? fptr() : false;
}

#ifdef CHRE_NANOAPP_USES_GNSS

WEAK_SYMBOL
bool chreGnssConfigurePassiveLocationListener(bool enable) {
  auto *fptr = CHRE_NSL_LAZY_LOOKUP(chreGnssConfigurePassiveLocationListener);
  return (fptr != nullptr) ? fptr(enable) : false;
}

#endif /* CHRE_NANOAPP_USES_GNSS */

#ifdef CHRE_NANOAPP_USES_WIFI

WEAK_SYMBOL
bool chreWifiRequestScanAsync(const struct chreWifiScanParams *params,
                              const void *cookie) {
  auto *fptr = CHRE_NSL_LAZY_LOOKUP(chreWifiRequestScanAsync);

  if (fptr == nullptr) {
    // Should never happen
    return false;
  } else if (chreGetApiVersion() < CHRE_API_VERSION_1_5) {
    const struct chreWifiScanParams legacyParams =
        chre::translateToLegacyWifiScanParams(params);
    return fptr(&legacyParams, cookie);
  } else {
    return fptr(params, cookie);
  }
}

WEAK_SYMBOL
bool chreWifiRequestRangingAsync(const struct chreWifiRangingParams *params,
                                 const void *cookie) {
  auto *fptr = CHRE_NSL_LAZY_LOOKUP(chreWifiRequestRangingAsync);
  return (fptr != nullptr) ? fptr(params, cookie) : false;
}

#endif /* CHRE_NANOAPP_USES_WIFI */

WEAK_SYMBOL
bool chreSensorFind(uint8_t sensorType, uint8_t sensorIndex, uint32_t *handle) {
  auto *fptr = CHRE_NSL_LAZY_LOOKUP(chreSensorFind);
  if (fptr != nullptr) {
    return fptr(sensorType, sensorIndex, handle);
  } else if (sensorIndex == 0) {
    return chreSensorFindDefault(sensorType, handle);
  } else {
    return false;
  }
}

WEAK_SYMBOL
bool chreSensorConfigureBiasEvents(uint32_t sensorHandle, bool enable) {
  auto *fptr = CHRE_NSL_LAZY_LOOKUP(chreSensorConfigureBiasEvents);
  return (fptr != nullptr) ? fptr(sensorHandle, enable) : false;
}

WEAK_SYMBOL
bool chreSensorGetThreeAxisBias(uint32_t sensorHandle,
                                struct chreSensorThreeAxisData *bias) {
  auto *fptr = CHRE_NSL_LAZY_LOOKUP(chreSensorGetThreeAxisBias);
  return (fptr != nullptr) ? fptr(sensorHandle, bias) : false;
}

WEAK_SYMBOL
bool chreSensorFlushAsync(uint32_t sensorHandle, const void *cookie) {
  auto *fptr = CHRE_NSL_LAZY_LOOKUP(chreSensorFlushAsync);
  return (fptr != nullptr) ? fptr(sensorHandle, cookie) : false;
}

WEAK_SYMBOL
void chreConfigureDebugDumpEvent(bool enable) {
  auto *fptr = CHRE_NSL_LAZY_LOOKUP(chreConfigureDebugDumpEvent);
  if (fptr != nullptr) {
    fptr(enable);
  }
}

WEAK_SYMBOL
void chreDebugDumpLog(const char *formatStr, ...) {
  auto *fptr = CHRE_NSL_LAZY_LOOKUP(platform_chreDebugDumpVaLog);
  if (fptr != nullptr) {
    va_list args;
    va_start(args, formatStr);
    fptr(formatStr, args);
    va_end(args);
  }
}

WEAK_SYMBOL
bool chreSendMessageWithPermissions(void *message, size_t messageSize,
                                    uint32_t messageType, uint16_t hostEndpoint,
                                    uint32_t messagePermissions,
                                    chreMessageFreeFunction *freeCallback) {
  auto *fptr = CHRE_NSL_LAZY_LOOKUP(chreSendMessageWithPermissions);
  if (fptr != nullptr) {
    return fptr(message, messageSize, messageType, hostEndpoint,
                messagePermissions, freeCallback);
  } else {
    return chreSendMessageToHostEndpoint(message, messageSize, messageType,
                                         hostEndpoint, freeCallback);
  }
}

WEAK_SYMBOL
int8_t chreUserSettingGetState(uint8_t setting) {
  int8_t settingState = CHRE_USER_SETTING_STATE_UNKNOWN;
  auto *fptr = CHRE_NSL_LAZY_LOOKUP(chreUserSettingGetState);
  if (fptr != nullptr) {
    settingState = fptr(setting);
  }
  return settingState;
}

WEAK_SYMBOL
void chreUserSettingConfigureEvents(uint8_t setting, bool enable) {
  auto *fptr = CHRE_NSL_LAZY_LOOKUP(chreUserSettingConfigureEvents);
  if (fptr != nullptr) {
    fptr(setting, enable);
  }
}

#endif  // CHRE_NANOAPP_DISABLE_BACKCOMPAT
