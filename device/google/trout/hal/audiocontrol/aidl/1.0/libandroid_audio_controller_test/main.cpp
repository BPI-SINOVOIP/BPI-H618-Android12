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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sstream>
#include <string>

#include <android_audio_controller.h>

static void help(const char* argv0) {
    if (argv0 == nullptr || argv0[0] == 0) argv0 = "android_audio_controller_test";
    printf("Usage instructions:\n");
    printf("%s [-d] -f <file path> -s <server address> [-t]\n", argv0);
    printf("-d to switch between non-ducking (default) and ducking\n");
    printf("-e to switch between exclusive (default) and non-exclusive\n");
    printf("-f <file path>: path of the WAV file containing the sound sample to play\n");
    printf("-s <server address>: the address of the Android Audio Control HAL server\n");
    printf("-t to switch between non-transient (default) and transient\n");
    printf("%s -h to repeat this message\n", argv0);
}

static void error(const char* msg, int err) {
    fprintf(stderr, "error: %s", msg);
    if (err != 0) fprintf(stderr, " (%d %s)", err, strerror(err));
    fprintf(stderr, "\n");
    exit(1);
}

class AudioSession {
  public:
    ~AudioSession() {
        if (mSession != AAFC_SESSION_ID_INVALID) aafc_release_audio_focus(mSession);
    }

    aafc_session_id_t session() const { return mSession; }

    explicit operator bool() const { return mSession != AAFC_SESSION_ID_INVALID; }

    explicit AudioSession(const aafc_audio_focus_request_t& request) {
        mSession = aafc_acquire_audio_focus(request);
    }

  private:
    explicit AudioSession(aafc_session_id_t id);

    aafc_session_id_t mSession;
};

int main(int argc, char** argv) {
    int c;
    std::string file_path;
    std::string server_addr;

    // TODO(egranata): allow custom usage & zone
    aafc_audio_focus_request_t request = {.audio_usage = AAFC_AUDIO_USAGE_EMERGENCY,
                                          .zone_id = 0,
                                          .allow_duck = false,
                                          .is_transient = false,
                                          .is_exclusive = true};

    while ((c = getopt(argc, argv, "def:hs:t")) != -1) {
        switch (c) {
            case 'd':
                request.allow_duck = !request.allow_duck;
                break;
            case 'e':
                request.is_exclusive = !request.is_exclusive;
                break;
            case 'f':
                file_path = optarg;
                break;
            case 'h':
                help(argv[0]);
                exit(0);
                break;
            case 's':
                server_addr = optarg;
                break;
            case 't':
                request.is_transient = !request.is_transient;
                break;
        }
    }

    if (file_path.empty() || server_addr.empty()) {
        help(argv[0]);
        exit(0);
    }

    int ok = aafc_init_audio_focus_controller(server_addr.c_str());
    if (ok != 0) {
        error("server connection failed", ok);
    }

    AudioSession session(request);
    if (!session) {
        error("audio focus could not be acquired", 0);
    }

    // TODO(egranata): find a cleaner way to do this (e.g. tinyalsa APIs)
    std::stringstream ss;
    ss << "/usr/bin/aplay \"" << file_path << "\"";
    std::string cmd = ss.str();
    system(cmd.c_str());

    return 0;
}
