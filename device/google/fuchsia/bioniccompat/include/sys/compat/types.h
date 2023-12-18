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

#ifndef SYS_COMPAT_TYPES_H
#define SYS_COMPAT_TYPES_H

#include <sys/cdefs.h>
#include <threads.h>
#include <zircon/process.h>

__BEGIN_DECLS

/*
 * Zircon does not define an identifier for thread id. Instead
 * we rely on the handle. This is slightly risky as there is a
 * chance that other code may replace this handle with a different
 * one, so we will need to come up with a better solution in the
 * future. In particular, we should revisit when the Zircon team
 * determines whether or not to implement actual thread IDs.
 */
inline pid_t gettid() {
  return zx_thread_self();
}

__END_DECLS

#endif  // SYS_COMPAT_TYPES_H
