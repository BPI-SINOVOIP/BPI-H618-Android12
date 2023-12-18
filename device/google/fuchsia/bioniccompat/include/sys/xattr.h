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

#ifndef SYS_XATTR_H
#define SYS_XATTR_H

#include <errno.h>
#include <sys/cdefs.h>

__BEGIN_DECLS

#ifndef ENOATTR
#define ENOATTR ENODATA
#endif

/*
 * Fuchsia does not support extended attirbutes on inodes (yet?), so
 * any getxattr implementation will not make much sense. Simply report
 * that the requested attribute does not exist and return.
 */
inline ssize_t getxattr(const char* path __UNUSED, const char* name __UNUSED, void* value __UNUSED, size_t size __UNUSED) {
  errno = ENOTSUP;
  return -1;
}

inline ssize_t listxattr(const char *path __UNUSED, char *list __UNUSED, size_t size __UNUSED) {
  errno = ENOTSUP;
  return -1;
}

inline int setxattr(const char *path __UNUSED, const char *name __UNUSED, const void *value __UNUSED, size_t size __UNUSED, int flags __UNUSED) {
  errno = ENOTSUP;
  return -1;
}

inline int removexattr(const char *path __UNUSED, const char *name __UNUSED) {
  errno = ENOTSUP;
  return -1;
}

__END_DECLS

#endif  // SYS_XATTR_H
