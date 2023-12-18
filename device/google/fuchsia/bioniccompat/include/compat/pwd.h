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

#ifndef COMPAT_PWD_H
#define COMPAT_PWD_H

#include <pwd.h>

#include <sys/cdefs.h>

__BEGIN_DECLS

static struct passwd PASSWORD = {
  .pw_name = (char *)"android",
  .pw_uid = 1,
  .pw_gid = 1,
  .pw_passwd = (char *)"",
  .pw_dir = (char *)"/",
  .pw_gecos = NULL,
  .pw_shell = (char *)"/bin/bash",
};

int getpwnam_r(const char *name __UNUSED, struct passwd *pwd __UNUSED,
               char *buf __UNUSED, size_t buflen __UNUSED, struct passwd **result) {
  *result = &PASSWORD;
  return 0;
}

int getpwuid_r(uid_t uid __UNUSED, passwd* pwd __UNUSED, char *buf __UNUSED,
               size_t byte_count __UNUSED, struct passwd** result) {
  *result = &PASSWORD;
  return 0;
}

__END_DECLS

#endif  // COMPAT_PWD_H
