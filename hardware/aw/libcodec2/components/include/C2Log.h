/*
 * Copyright (C) 2021 by Allwinnertech Co. Ltd.
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

#ifndef ANDROID_COMPONENTS_INCLUDE_C2LOG_H_
#define ANDROID_COMPONENTS_INCLUDE_C2LOG_H_

#include <log/log.h>

using namespace android;
#define c2_logv(fmt, arg...) \
  ALOGV("[%s] <%s:%u>: " fmt, COMPONENT_NAME, __FUNCTION__, __LINE__, ##arg)

#define c2_logd(fmt, arg...) \
  ALOGD("[%s] <%s:%u>: " fmt, COMPONENT_NAME, __FUNCTION__, __LINE__, ##arg)

#define c2_logi(fmt, arg...) \
  ALOGI("[%s] <%s:%u>: " fmt, COMPONENT_NAME, __FUNCTION__, __LINE__, ##arg)

#define c2_logw(fmt, arg...) \
  ALOGW("[%s] <%s:%u>: " fmt, COMPONENT_NAME, __FUNCTION__, __LINE__, ##arg)

#define c2_loge(fmt, arg...) \
  ALOGE("[%s] <%s:%u>: " fmt, COMPONENT_NAME, __FUNCTION__, __LINE__, ##arg)

#define C2_TRACE() \
  do {             \
    c2_logd("");   \
  } while (0)

#define C2_TRACE_VERBOSE() \
  do {             \
    c2_logv("");   \
  } while (0)

//#define FILE_DUMP_ENABLE
#ifdef FILE_DUMP_ENABLE

#define INPUT_VDEC_DUMP_PATH "/data/camera/decode_input"
#define OUTPUT_VDEC_DUMP_PATH "/data/camera/decode_output"
#define INPUT_VENC_DUMP_PATH "/data/camera/encode_input"
#define OUTPUT_VENC_DUMP_PATH "/data/camera/encode_output"
#define DAT_DUMP_EXT "dat"
#define YUV_DUMP_EXT "yuv"

#define GENERATE_VDEC_FILE_NAMES()                                             \
  {                                                                            \
    GETTIME(&mTimeStart, NULL);                                                \
    strcpy(mInFile, "");                                                       \
    sprintf(mInFile, "%s_%ld.%ld.%s", INPUT_VDEC_DUMP_PATH, mTimeStart.tv_sec, \
            mTimeStart.tv_usec, DAT_DUMP_EXT);                                 \
    strcpy(mOutFile, "");                                                      \
    sprintf(mOutFile, "%s_%ld.%ld.%s", OUTPUT_VDEC_DUMP_PATH,                  \
            mTimeStart.tv_sec, mTimeStart.tv_usec, YUV_DUMP_EXT);              \
  }

#define GENERATE_VENC_FILE_NAMES()                                             \
  {                                                                            \
    GETTIME(&mTimeStart, NULL);                                                \
    strcpy(mInFile, "");                                                       \
    sprintf(mInFile, "%s_%ld.%ld.%s", INPUT_VENC_DUMP_PATH, mTimeStart.tv_sec, \
            mTimeStart.tv_usec, YUV_DUMP_EXT);                                 \
    strcpy(mOutFile, "");                                                      \
    sprintf(mOutFile, "%s_%ld.%ld.%s", OUTPUT_VENC_DUMP_PATH,                  \
            mTimeStart.tv_sec, mTimeStart.tv_usec, DAT_DUMP_EXT);              \
  }

#define CREATE_DUMP_FILE(m_filename)                 \
  {                                                  \
    FILE *fp = fopen(m_filename, "wb");              \
    if (fp != NULL) {                                \
      c2_logd("Opened file %s", m_filename);         \
      fclose(fp);                                    \
    } else {                                         \
      c2_logd("Could not open file %s", m_filename); \
    }                                                \
  }

#define DUMP_TO_FILE(m_filename, m_buf, m_size)          \
  {                                                      \
    FILE *fp = fopen(m_filename, "ab");                  \
    if (fp != NULL && m_buf != NULL) {                   \
      int i;                                             \
      i = fwrite(m_buf, 1, m_size, fp);                  \
      c2_logd("fwrite ret %d to write %d", i, m_size);   \
      if (i != static_cast<int>(m_size)) {               \
        c2_loge("Error in fwrite, returned %d", i);      \
        perror("Error in write to file");                \
      }                                                  \
      fclose(fp);                                        \
    } else {                                             \
      c2_loge("Could not write to file %s", m_filename); \
      if (fp != NULL) fclose(fp);                        \
    }                                                    \
  }

#else /* FILE_DUMP_ENABLE */
#define INPUT_DUMP_PATH
#define INPUT_DUMP_EXT
#define OUTPUT_DUMP_PATH
#define OUTPUT_DUMP_EXT
#define GENERATE_VDEC_FILE_NAMES()
#define GENERATE_VENC_FILE_NAMES()
#define CREATE_DUMP_FILE(m_filename)
#define DUMP_TO_FILE(m_filename, m_buf, m_size)
#endif /* FILE_DUMP_ENABLE */

#endif
