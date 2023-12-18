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
#ifndef COMPAT_FCNTL_H
#define COMPAT_FCNTL_H

#include <fcntl.h>

#include <errno.h>
#include <sys/cdefs.h>

/*
 * zircon does not support this operation.
 *
 * This is super hacky: supply a function that takes a long * and explicitly
 * cast to loff_t, which on Fuchsia is long long. This is safe on Fuchsia but
 * obviously not portable, yet it allows us to leave libcore code alone.
 */
ssize_t splice(int fd_in, long *off_in, int fd_out,
               long *off_out, size_t len, unsigned int flags) {
  return splice(fd_in, (loff_t *)off_in, fd_out, (loff_t *)off_out, len, flags);
}

ssize_t splice(int fd_in __UNUSED, loff_t *off_in __UNUSED, int fd_out __UNUSED,
               loff_t *off_out __UNUSED, size_t len __UNUSED, unsigned int flags __UNUSED) {
  errno = EINVAL;
  return -1;
}

#endif  // COMPAT_FCNTL_H
