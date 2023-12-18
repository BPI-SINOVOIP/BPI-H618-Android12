/*
 * Copyright (C) 2021 The Android Open Source Project
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

#pragma once

#include <linux/bpf.h>
#include <linux/unistd.h>

#ifdef BPF_FD_JUST_USE_INT
  #define BPF_FD_TYPE int
  #define BPF_FD_TO_U32(x) static_cast<__u32>(x)
#else
  #include <android-base/unique_fd.h>
  #define BPF_FD_TYPE base::unique_fd&
  #define BPF_FD_TO_U32(x) static_cast<__u32>((x).get())
#endif

#define ptr_to_u64(x) ((uint64_t)(uintptr_t)(x))

namespace android {
namespace bpf {

/* Note: bpf_attr is a union which might have a much larger size then the anonymous struct portion
 * of it that we are using.  The kernel's bpf() system call will perform a strict check to ensure
 * all unused portions are zero.  It will fail with E2BIG if we don't fully zero bpf_attr.
 */

inline int bpf(int cmd, const bpf_attr& attr) {
    return syscall(__NR_bpf, cmd, &attr, sizeof(attr));
}

inline int createMap(bpf_map_type map_type, uint32_t key_size, uint32_t value_size,
                     uint32_t max_entries, uint32_t map_flags) {
    return bpf(BPF_MAP_CREATE, {
                                       .map_type = map_type,
                                       .key_size = key_size,
                                       .value_size = value_size,
                                       .max_entries = max_entries,
                                       .map_flags = map_flags,
                               });
}

inline int writeToMapEntry(const BPF_FD_TYPE map_fd, const void* key, const void* value,
                           uint64_t flags) {
    return bpf(BPF_MAP_UPDATE_ELEM, {
                                            .map_fd = BPF_FD_TO_U32(map_fd),
                                            .key = ptr_to_u64(key),
                                            .value = ptr_to_u64(value),
                                            .flags = flags,
                                    });
}

inline int findMapEntry(const BPF_FD_TYPE map_fd, const void* key, void* value) {
    return bpf(BPF_MAP_LOOKUP_ELEM, {
                                            .map_fd = BPF_FD_TO_U32(map_fd),
                                            .key = ptr_to_u64(key),
                                            .value = ptr_to_u64(value),
                                    });
}

inline int deleteMapEntry(const BPF_FD_TYPE map_fd, const void* key) {
    return bpf(BPF_MAP_DELETE_ELEM, {
                                            .map_fd = BPF_FD_TO_U32(map_fd),
                                            .key = ptr_to_u64(key),
                                    });
}

inline int getNextMapKey(const BPF_FD_TYPE map_fd, const void* key, void* next_key) {
    return bpf(BPF_MAP_GET_NEXT_KEY, {
                                             .map_fd = BPF_FD_TO_U32(map_fd),
                                             .key = ptr_to_u64(key),
                                             .next_key = ptr_to_u64(next_key),
                                     });
}

inline int getFirstMapKey(const BPF_FD_TYPE map_fd, void* firstKey) {
    return getNextMapKey(map_fd, NULL, firstKey);
}

inline int bpfFdPin(const BPF_FD_TYPE map_fd, const char* pathname) {
    return bpf(BPF_OBJ_PIN, {
                                    .pathname = ptr_to_u64(pathname),
                                    .bpf_fd = BPF_FD_TO_U32(map_fd),
                            });
}

inline int bpfFdGet(const char* pathname, uint32_t flag) {
    return bpf(BPF_OBJ_GET, {
                                    .pathname = ptr_to_u64(pathname),
                                    .file_flags = flag,
                            });
}

inline int mapRetrieve(const char* pathname, uint32_t flag) {
    return bpfFdGet(pathname, flag);
}

inline int mapRetrieveRW(const char* pathname) {
    return mapRetrieve(pathname, 0);
}

inline int mapRetrieveRO(const char* pathname) {
    return mapRetrieve(pathname, BPF_F_RDONLY);
}

inline int mapRetrieveWO(const char* pathname) {
    return mapRetrieve(pathname, BPF_F_WRONLY);
}

inline int retrieveProgram(const char* pathname) {
    return bpfFdGet(pathname, BPF_F_RDONLY);
}

inline int attachProgram(bpf_attach_type type, const BPF_FD_TYPE prog_fd,
                         const BPF_FD_TYPE cg_fd) {
    return bpf(BPF_PROG_ATTACH, {
                                        .target_fd = BPF_FD_TO_U32(cg_fd),
                                        .attach_bpf_fd = BPF_FD_TO_U32(prog_fd),
                                        .attach_type = type,
                                });
}

inline int detachProgram(bpf_attach_type type, const BPF_FD_TYPE cg_fd) {
    return bpf(BPF_PROG_DETACH, {
                                        .target_fd = BPF_FD_TO_U32(cg_fd),
                                        .attach_type = type,
                                });
}

}  // namespace bpf
}  // namespace android

#undef ptr_to_u64
#undef BPF_FD_TO_U32
#undef BPF_FD_TYPE
#undef BPF_FD_JUST_USE_INT
