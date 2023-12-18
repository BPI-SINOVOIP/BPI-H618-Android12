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

#ifndef __AUDIO_DATA_DUMP_H_
#define __AUDIO_DATA_DUMP_H_

/*
 * the audio_data_dump struct
 *
 */
struct audio_data_dump {
	FILE *file;
	bool enable_flags;
};

/*
 * interface for user
 *
 */
void init_dump_flags(bool direction, struct audio_data_dump *con);

void close_dump_flags(struct audio_data_dump *con);

static size_t get_data_by_bytes(const void *srcbuffer, size_t bytes, const void *dstbuffer);

size_t debug_dump_data(const void *srcbuffer,size_t bytes, struct audio_data_dump *con);


#endif
