/*
 * Copyright 2013 The Android Open Source Project
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
#ifndef __AWKEYMASTER_MODULEAPI_INCLUDE_H_
#define __AWKEYMASTER_MODULEAPI_INCLUDE_H_

#include <utils/Log.h>

//#define AW_DEBUG
#ifdef AW_DEBUG
#define AW_LOG(format, ...)  \
  ALOGD("*AW-DBG*----:(%s-%u):----" format, __FUNCTION__, __LINE__, ##__VA_ARGS__)

#define AW_ERR(format, ...)  \
  ALOGE("******AW-ERR******:(%s-%u):----" format, __FUNCTION__, __LINE__, ##__VA_ARGS__)

#define AW_WARN(format, ...)  \
  ALOGW("***AW-WARNING***:(%s-%u):----" format, __FUNCTION__, __LINE__, ##__VA_ARGS__)

#else
#define AW_LOG(format, ...)
#define AW_ERR(format, ...)

#endif

static const uint8_t AW_KEY_MAGIC[] = { 'A', 'W', 'K', 'M' ,'S','C','H','W','0','1'};

#define SCHW_DEVICE_INTF "/dev/scdev"
#define SCHW_IOCTL_BASE 'C'
#define SCHW_IOCTL_OPRA 'S'
#define SCHW_HARDWARE 0xfaaf
#define SCHW_RECOVREY 0Xf55f

#define  SCHW_GET_SLOT        _IOR(SCHW_IOCTL_BASE, 1, int)
#define  SCHW_SET_SLOT          _IOR(SCHW_IOCTL_BASE, 2, unsigned char)

#define  SCHW_SECURE_STORE       _IOW(SCHW_IOCTL_OPRA, 1, unsigned char)
#define  SCHW_SECURE_PULL       _IOW(SCHW_IOCTL_OPRA, 2, int)
#define  SCHW_SECURE_LEN       _IOW(SCHW_IOCTL_OPRA, 3, int)
#define  SCHW_SECURE_CLEAR       _IOW(SCHW_IOCTL_OPRA, 4, unsigned char)


#define HIDDEN_EXPORT __attribute__((visibility("hidden")))
#define SHARE_EXPORT __attribute__((visibility("default")))

#define AW_KEYMASTER_API_VERSION 0
#define AW_KEYMASTER_CRYPTO_VERSION 1

#define AW_KEY_SIZE_MAX  (512)           /* 4096 bits */
#define AW_MAGIC_SIZE    (10)
#define AW_HASH_SIZE     (128)

#define AW_EC_NAMED_CURVE  0x001

#define AW_KEYMASTER_TEE_HAL    "aw_sunxi_keymaster_tee_device"

#if 0
struct aw_key_blob_format {
  unsigned char magic[AW_MAGIC_SIZE];
  int map_id;
  int key_type;
  int is_hardware;
  int public_key_len;
  int private_key_len;
  int key_blob_len;
  unsigned char  hash[AW_HASH_SIZE];
};
typedef struct aw_key_blob_format aw_key_blob_format_t;
#endif

typedef struct {
    keymaster_digest_algorithm_t digest_type;
} keymaster_private_sign_params_t;

#define AW_KEYMASTER_BLOB_SIZE    (32)

#endif /* AWKEYMASTER_INCLUDE_H */
