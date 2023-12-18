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

#include "sys/sendfile.h"

#include <errno.h>
#include <unistd.h>

namespace {
// This value is arbitrarily chosen.
constexpr size_t kBufSize = 4096;
} // namespace

// TODO(ZX-429): This is a painfully simple sendfile implementation that does
// not attempt to optimize for anything and is only provided as something that "works".
ssize_t sendfile(int out_fd, int in_fd, off_t *offset, size_t count) {
  // We don't support seek() for now.
  if (offset != nullptr) {
    errno = ESPIPE;
    return -1;
  }

  size_t total = 0;
  unsigned char buffer[kBufSize];
  while(count > 0) {
    int bytes = read(in_fd, buffer, sizeof(buffer));
    if (bytes == 0) {
      break;
    }

    if (bytes < 0) {
      return bytes;
    }

    count -= bytes;

    unsigned char *data = buffer;
    while(bytes > 0) {
      int written = write(out_fd, data, bytes);
      if (written < 0) {
        return written;
      }


      bytes -= written;
      data += written;
      total += written;
    }
  }

  return total;
}
