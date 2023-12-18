
/*
 * Copyright (C) 2019 The Android Open Source Project
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

#ifndef SYS_RESOURCE_H
#define SYS_RESOURCE_H

#include <sys/cdefs.h>

__BEGIN_DECLS

typedef unsigned long rlim_t;

#define RLIM_INFINITY ((rlim_t) -1)

struct rlimit {
  rlim_t rlim_cur;
  rlim_t rlim_max;
};

/*
 * Fuchsia has no concept of resource limits yet. Tell the system that
 * usage is unlimited (no soft or hard cap).
 */
inline int getrlimit(int resource, struct rlimit *rlim) {
  if (!rlim) {
    errno = EINVAL;
    return -1;
  }
  rlim->rlim_cur = RLIM_INFINITY;
  rlim->rlim_max = RLIM_INFINITY;
  return 0;
}

__END_DECLS

#endif  // SYS_RESOURCE_H
