/*
 * Copyright (C) 2020 The Android Open Source Project
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

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <cstdlib>

int main(int argc, char* argv[]) {
    if (argc != 3) {
        fprintf(stderr, "syntax: %s <path> <command>\n", argv[0]);
        return 1;
    }

    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) {
        fprintf(stderr, "socket creation failed: %d\n", errno);
        return 1;
    }

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, argv[1], sizeof(addr.sun_path));

    int ok = connect(fd, (struct sockaddr*)&addr, sizeof(addr));
    if (ok < 0) {
        fprintf(stderr, "connection could not be established: %d\n", errno);
        return 1;
    }

    ok = write(fd, argv[2], strlen(argv[2]));
    if (ok < 0) {
        fprintf(stderr, "write failed: %d\n", errno);
        return 1;
    }

    return 0;
}
