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

#ifndef SYS_CAPABILITY_H
#define SYS_CAPABILITY_H

#include <errno.h>
#include <sys/cdefs.h>

__BEGIN_DECLS

typedef struct __user_cap_header_struct {
  uint32_t version;
  int pid;
} *cap_user_header_t;

typedef struct __user_cap_data_struct {
  uint32_t effective;
  uint32_t permitted;
  uint32_t inheritable;
} *cap_user_data_t;

inline int capget(cap_user_header_t hdrp, cap_user_data_t datap) {
  datap->effective = datap->permitted = datap->inheritable = 0;
  return 0;
}

inline int capset(cap_user_header_t hdrp, const cap_user_data_t datap) {
  errno = EPERM;
  return -1;
}

__END_DECLS

#endif  // SYS_CAPABILITY_H
