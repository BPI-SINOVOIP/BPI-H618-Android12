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

#define LOG_TAG "audio_dump_data"
#define LOG_NDEBUG 0

#include <cutils/log.h>
#include "audio_plugin.h"
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>

#define UNUSED(x) (void)(x)

#define DEFAULT_REC_PATH "/data/rec.pcm"
#define DEFAULT_PLAY_PATH "/data/play.pcm"

struct file_fd {
    int fd_rec;
    int fd_play;
};

struct file_fd fd;

int open_file()
{
    FILE *fp;

    ALOGD("fopen.....");
    fp = fopen(DEFAULT_REC_PATH, "w+");
    ALOGD("fp:%p", fp);
    if (fp)
        fclose(fp);

    fd.fd_rec = open(DEFAULT_REC_PATH, O_WRONLY | O_TRUNC);
    fd.fd_play = open(DEFAULT_PLAY_PATH, O_WRONLY | O_TRUNC);

    return 0;
}

int close_file()
{
    close(fd.fd_rec);
    close(fd.fd_play);

    return 0;
}

int get_input_data(struct pcm_config config, const void *buffer, size_t bytes)
{
    UNUSED(config);

    if (fd.fd_rec > 0) {
        write(fd.fd_rec, buffer, bytes);
    } else {
        ALOGE("can't open:%s", DEFAULT_REC_PATH);
        return -1;
    }

    return 0;
}

int get_output_data(struct pcm_config config, const void *buffer, size_t bytes)
{
    UNUSED(config);

    if (fd.fd_play > 0) {
        write(fd.fd_play, buffer, bytes);
    } else {
        ALOGE("can't open:%s", DEFAULT_PLAY_PATH);
        return 0;
    }

    return 0;
}

struct audio_plugin dump_data = {
    .name = "dump_data",
    .handle = &fd,
    .on_adev_open = open_file,
    .on_adev_close = close_file,
    .on_out_write = get_output_data,
    .on_in_read = get_input_data,
};
