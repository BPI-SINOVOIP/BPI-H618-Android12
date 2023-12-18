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

#ifndef SYS_PRCTL_H
#define SYS_PRCTL_H

#include <errno.h>
#include <sys/cdefs.h>

__BEGIN_DECLS

inline int prctl(int option,
                 unsigned long arg2,
                 unsigned long arg3,
                 unsigned long arg4,
                 unsigned long arg5) {
  errno = EINVAL;
  return -1;
}

__END_DECLS

#endif  // SYS_PRCTL_H
