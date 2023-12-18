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

const int eColorEnhanceMask      = 0x01;
const int eColorEnhanceValueMask = 0x02;
const int eSmaretBacklightMask   = 0x04;

struct usropt {
    int mask;
    int colorEnhanceRequest;
    int colorEnhanceValue[3];
    int smartBacklightRequest;
};

static struct usropt requestopt;

static void usage(char *name)
{
    fprintf(stderr, "Usage: %s [-e mode] [-s on/off] [-h] [-c y u v]\n", name);
    fprintf(stderr, "   -e: color enhance\n");
    fprintf(stderr, "   -s: smart backlight\n");
    fprintf(stderr, "   -c: config color enhance value\n");
}

static void parseOpt(int argc, char **argv)
{
    memset(&requestopt, 0, sizeof(requestopt));

    while (1) {
        int c = getopt(argc, argv, "e:s:hc:");
        if (c == EOF)
            break;
        switch (c) {
            case 'e':
                requestopt.mask |= eColorEnhanceMask;
                requestopt.colorEnhanceRequest = strtoul(optarg, NULL, 0);
                break;
            case 's':
                requestopt.mask |= eSmaretBacklightMask;
                requestopt.smartBacklightRequest = strcmp(optarg, "on") == 0;
                break;
            case 'c':
                break;
            case 'h':
            default:
                usage(argv[0]);
                exit(1);
        }
    }

    if (optind != argc) {
        usage(argv[0]);
        exit(1);
    }
}

enum _enhance_component {
    kEnhanceMode        = 0,
    kEnhanceBright      = 1,
    kEnhanceContrast    = 2,
    kEnhanceDenoise     = 3,
    kEnhanceDetail      = 4,
    kEnhanceEdge        = 5,
    kEnhanceSaturation  = 6,
    kColorTemperature   = 7,
};

static const int kEnhanceSetting = 1;
static const int kSmartBackLight = 2;
static const int kTemperature    = 3;

int main(int argc, char **argv)
{
    parseOpt(argc, argv);

    android::sp<IDisplayConfig> service = IDisplayConfig::getService();
    if (service == nullptr) {
        fprintf(stderr, "can't get IDisplayConfig service\n");
        exit(1);
    }

    if (requestopt.mask & eColorEnhanceMask) {
        int ret = service->setDisplayArgs(
                0, kEnhanceSetting, kEnhanceMode, requestopt.colorEnhanceRequest);
        printf("kEnhanceSetting return %d\n", ret);
    }

    if (requestopt.mask & eSmaretBacklightMask) {
        int ret = service->setDisplayArgs(
                0, kSmartBackLight, 0, requestopt.smartBacklightRequest == 0 ? 0 : 2);
        printf("kEnhanceSetting return %d\n", ret);
    }
}

