/*
 * Copyright (C) 2018 The Android Open Source Project
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

#include <getopt.h>
#include <stdio.h>
#include <string.h>
#include <android-base/macros.h>
#include <vendor/display/config/1.0/IDisplayConfig.h>

using ::vendor::display::config::V1_0::IDisplayConfig;

struct logCategory {
    const char *tag;
    const char *description;
    uint32_t mask;
};

static const struct logCategory kLogCategories[] = {
    { "layer",  "Layer planner debug output",           (1UL << 1) },
    { "vsync",  "Vsync timestamp print",                (1UL << 4) },
    { "scale",  "Hardware scaler detect",               (1UL << 5) },
    { "fps",    "Show primary display refresh rate",    (1UL << 6) },
    { "rotate", "Hardware rotator relate information ", (1UL << 7) },
    { "perf",   "Hardware rotator relate information ", (1UL << 8) },
};

static void listSupportedCategories()
{
    for (size_t i = 0; i < arraysize(kLogCategories); i++) {
        printf("  %10s - %s \n", kLogCategories[i].tag, kLogCategories[i].description);
    }
}

static int getLogTagMask(const char* tag)
{
    for (size_t i = 0; i < arraysize(kLogCategories); i++) {
        if (strcmp(tag, kLogCategories[i].tag) == 0) {
            return kLogCategories[i].mask;
        }
    }
    fprintf(stderr, "unknow log tag '%s'\n", tag);
    return 0;
}

// Print the command usage help to stderr.
static void showHelp(const char *cmd)
{
    fprintf(stderr, "usage: %s [options] [categories...]\n", cmd);
    fprintf(stderr, "options include:\n"
                    "  -o      enable hwc log output which indicate by the\n"
                    "          following categories\n"
                    "  -x      disbale all hwc log output\n"
                    "  --list  list the available logging categories\n"
                    "  --dump <type> <count> [z]\n"
                    "          request dump buffer raw data into file\n"
                    "          type: 0 - input  buffer\n"
                    "                1 - output buffer\n"
                    "                2 - hardware rotated buffer\n"
                    "          count: total request count\n"
                    "          z : which z order of input buffer will be dump\n"
                    "  --freeze <enable> -s <display>\n"
                    "          freezes the output content of the specified logic display for debugging\n"
                    "  --help  show this usage\n"
            );
}

/* Command line options */
static uint32_t gLogCategories = 0;
static bool gRequestLogTagEnable = false;
static bool gRequestLogTagClear  = false;
static bool gRequestDump = false;
static int dumpArgs[3] = {0};

static bool gRequestFreezeDisplayOutput = false;
static int gFreezeData = 0;
static int gTargetDisplay = 0;

void parseDumpBufferParam(const char *arg)
{
    static int argIndex = 0;
    if (argIndex >= 3)
        return;

    dumpArgs[argIndex] = strtoul(arg, NULL, 0);
    argIndex++;
}

int main(int argc, char** argv)
{
    if (argc == 2 && 0 == strcmp(argv[1], "--help")) {
        showHelp(argv[0]);
        exit(0);
    }

    for (;;) {
        int ret;
        int option_index = 0;
        static struct option long_options[] = {
            {"help",   no_argument, nullptr, 'h' },
            {"list",   no_argument, nullptr, 'l' },
            {"dump",   no_argument, nullptr, 'd' },
            {"freeze", required_argument, nullptr, 'f' },
            {nullptr,            0, nullptr,  0  }
        };

        ret = getopt_long(argc, argv, "o:s:x", long_options, &option_index);

        if (ret < 0) {
            for (int i = optind; i < argc; i++) {
                parseDumpBufferParam(argv[i]);
            }
            break;
        }

        switch (ret) {
            case 'o':
                gRequestLogTagEnable = true;
                if (optarg) {
                    gLogCategories |= getLogTagMask(optarg);
                }
                break;
            case 'x':
                gRequestLogTagClear = true;
                break;
            case 'd':
                gRequestDump = true;
                break;
            case 's':
                gTargetDisplay = strtoul(optarg, NULL, 0);
                break;
            case 'f':
                gRequestFreezeDisplayOutput = true;
                gFreezeData = strtoul(optarg, NULL, 0);
                break;
            case 'l':
                listSupportedCategories();
                exit(0);
            case 'h':
            default:
                showHelp(argv[0]);
                exit(0);
        }
    }

    android::sp<IDisplayConfig> service = IDisplayConfig::getService();
    if (service == nullptr) {
        fprintf(stderr, "can't get IDisplayConfig service\n");
        exit(1);
    }

    if (gRequestLogTagEnable) {
        service->setDisplayArgs(0, 0x0000DEAD, 0, gLogCategories);
    } else if (gRequestLogTagClear) {
        service->setDisplayArgs(0, 0x0000DEAD, 0, 0);
    } else if (gRequestDump) {
        printf("dump buffer request: type=%d count=%d layer's zorder=%d\n",
                dumpArgs[0], dumpArgs[1], dumpArgs[2]);
        service->setDisplayArgs(dumpArgs[0], 0x0010DEAD, dumpArgs[1], (1 << dumpArgs[2]));
    } else if (gRequestFreezeDisplayOutput) {
        printf("freeze dispaly output: dispaly=%d enable=%d\n", gTargetDisplay, gFreezeData);
        service->setDisplayArgs(gTargetDisplay, 0x0020DEAD, 0, gFreezeData);
    }
    return 0;
}

