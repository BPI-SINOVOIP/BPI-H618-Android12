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

#ifndef SYS_SENDFILE_H
#define SYS_SENDFILE_H

#include <sys/cdefs.h>
#include <sys/types.h>

__BEGIN_DECLS

/*
 * sendfile() here is a relatively straightforward implementation, but
 * obviously not as efficient as the typical syscall implementation which
 * does not require buffers be transferred to userspace and can take
 * advantage of DMA vectored I/O.
 */
ssize_t sendfile(int out_fd, int in_fd, off_t* offset, size_t count);

__END_DECLS

#endif  // SYS_SENDFILE_H
