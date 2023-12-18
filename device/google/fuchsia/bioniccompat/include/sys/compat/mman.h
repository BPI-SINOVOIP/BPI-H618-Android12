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

#ifndef SYS_COMPAT_MMAN_H
#define SYS_COMPAT_MMAN_H

#include "mman.h"
#include "sys/cdefs.h"

__BEGIN_DECLS

// Fuchsia holds all pages in memory, see http://b/119503290 for more.
inline int mincore(void *addr __UNUSED, size_t length, unsigned char *vec) {
  for(size_t i = 0; i < length; i++) {
    vec[i] = 1;
  }
  return 0;
}

__END_DECLS

#endif  // SYS_COMPAT_MMAN_H
