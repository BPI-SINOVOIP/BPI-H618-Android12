/*
 * Copyright (C) 2017 The Android Open Source Project
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

#define LOG_TAG "audio_data_dump"
//#define LOG_NDEBUG 0

#include <dlfcn.h>
#include <cutils/log.h>
#include <cutils/properties.h>
#include <stdbool.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>

#include "audio_data_dump.h"

#define PROPERTY_AUDIO_DATA_DUMP_OUT "persist.vendor.audio.dump_data.out"
#define PROPERTY_AUDIO_DATA_DUMP_IN "persist.vendor.audio.dump_data.in"
#define AUDIO_DATA_DUMP_OUTFILE "/data/vendor/hardware/audio_d/out.pcm"
#define AUDIO_DATA_DUMP_INFILE "/data/vendor/hardware/audio_d/in.pcm"
/*
 * @{func}:audio dump data dynamicly
 * @{direction}: out when true and in when false
 */
void init_dump_flags(bool direction, struct audio_data_dump *con)
{
    if (direction) {
        con->enable_flags = property_get_bool(PROPERTY_AUDIO_DATA_DUMP_OUT, false);
        if (con->enable_flags && !con->file) {
            con->file = fopen(AUDIO_DATA_DUMP_OUTFILE, "w+");
            ALOGD("++++%d:init_dump_flags, con->file is %d", __LINE__, con->file);
            if(!con->file) {
                ALOGD("++++%d:init_dump_flags : open outfile(%s) err!!!", __LINE__, AUDIO_DATA_DUMP_OUTFILE);
                ALOGD("strerror(%s),errno is %d\n", strerror(errno),errno);
            }
        }
    } else {
        con->enable_flags = property_get_bool(PROPERTY_AUDIO_DATA_DUMP_IN, false);
        if (con->enable_flags && !con->file) {
            con->file = fopen(AUDIO_DATA_DUMP_INFILE, "w+");
            if(!con->file) {
                ALOGD("++++%d:init_dump_flags : open infile(%s) err!!!", __LINE__, AUDIO_DATA_DUMP_INFILE);
                ALOGD("strerror(%s),errno is %d\n", strerror(errno),errno);
            }
        }
    }
}

void close_dump_flags(struct audio_data_dump *con)
{
    if (con->file) {
        fclose(con->file);
        con->file = NULL;
    }
}

static size_t get_data_by_bytes(const void *srcbuffer, size_t bytes, const void *dstbuffer)
{
    size_t ret = 0;
    ret = fwrite(srcbuffer, 1, bytes, dstbuffer);
    return ret;
}

size_t debug_dump_data(const void *srcbuffer,size_t bytes, struct audio_data_dump *con)
{
    size_t ret = 0;
    if (con->file) {
        ret = get_data_by_bytes(srcbuffer, bytes, (void *)con->file);
    } else  {
        //ALOGD("++++%d:can't debug_dump_data due to file NULL err!!!", __LINE__);
        return -1;
    }
    return ret;
}
