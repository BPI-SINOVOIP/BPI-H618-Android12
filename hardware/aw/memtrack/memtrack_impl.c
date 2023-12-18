/*
 * Copyright (C) 2013 The Android Open Source Project
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

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <utils/Log.h>

#include <hardware/memtrack.h>

#include "memtrack.h"

#define MEMTRACK_LOG 0
#define ARRAY_SIZE(x) (sizeof(x)/sizeof(x[0]))
#define min(x, y) ((x) < (y) ? (x) : (y))

struct memtrack_record record_templates[] = {
//    {
//        .flags = MEMTRACK_FLAG_SMAPS_ACCOUNTED |
//                 MEMTRACK_FLAG_PRIVATE |
//                 MEMTRACK_FLAG_NONSECURE,
//    },
    {
        .flags = MEMTRACK_FLAG_SMAPS_UNACCOUNTED |
                 MEMTRACK_FLAG_PRIVATE |
                 MEMTRACK_FLAG_NONSECURE,
    },
};

int memtrack_get_memory_impl(pid_t pid, enum memtrack_type type,
                             struct memtrack_record *records,size_t *num_records)
{
    size_t allocated_records = min(*num_records, ARRAY_SIZE(record_templates));
    FILE *fp;
    char line[1024];
    size_t unaccounted_size = 0;
   
    ALOGV("pid = %d type = %d", pid , type ); 

    *num_records = ARRAY_SIZE(record_templates);

    /* fastpath to return the necessary number of records */
    if (allocated_records == 0) {
        return 0;
    }

    //snprintf(tmp, sizeof(tmp), "/proc/%d/cmdline", pid);
    fp = fopen("/sys/kernel/debug/ion/heaps/cma","r");

    if (fp == NULL) {
#if MEMTRACK_LOG
	    ALOGW("memtrack open tmp error");
#endif
	    return -errno;
    }

    memcpy(records, record_templates, sizeof(struct memtrack_record) * allocated_records);

    while(1)
    {
	    unsigned long mapsize;
	    char line_type[50];
	    int ret;

	    if(fgets(line, sizeof(line), fp) == NULL)
	    {
#if MEMTRACK_LOG
		    ALOGW("memtrack###############reading finish");
#endif
		    break;
	    }
#if MEMTRACK_LOG
	    ALOGW("memtrack###############memtrack read line string : %s", line);
#endif

	    ret = sscanf(line, "%16s %16lu\n",line_type,&mapsize);
	    if (ret != 2)
	    {
		    continue;
	    }
#if MEMTRACK_LOG
	    ALOGW("memtrack###############memtrack line_type: %s", line_type);
	    ALOGW("memtrack###############memtrack accounted_size: %zu", accounted_size);
#endif

	    if (strcmp(line_type, "total") == 0)
            {
#if MEMTRACK_LOG
		    ALOGW("memtrack###############reading total");
#endif
		    unaccounted_size += mapsize/1024; 
		    break;
	    }
    }

#if MEMTRACK_LOG
    ALOGW("###########memtrack accounted_size :%d\n", accounted_size);
#endif
    if (allocated_records > 0) {
        records[0].size_in_bytes = unaccounted_size;
    }

    fclose(fp);

    return 0;
}
