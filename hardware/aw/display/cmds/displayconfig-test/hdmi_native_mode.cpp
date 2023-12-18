/*
 * hdmi_native_mode/hdmi_native_mode.cpp
 *
 * Copyright (c) 2007-2020 Allwinnertech Co., Ltd.
 * Author: zhengxiaobin <zhengxiaobin@allwinnertech.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#include <stdio.h>
#include <iostream>
#include <vendor/display/config/1.0/IDisplayConfig.h>
#include <vendor/display/config/1.0/types.h>

using ::vendor::display::config::V1_0::IDisplayConfig;
using ::vendor::display::config::V1_0::SNRInfo;
using ::vendor::display::config::V1_0::SNRFeatureMode;
using android::hardware::hidl_string;
using android::hardware::hidl_vec;

android::sp<IDisplayConfig> mDisplayConfig = IDisplayConfig::getService();;

static void usage(char *name)
{
    fprintf(stderr, "Usage: %s [-u] [-g] [-s hdmi_mode] [-h]\n", name);
    fprintf(stderr, "   -h: command usage\n");
    fprintf(stderr, "   -u: get user hdmi setting\n");
    fprintf(stderr, "   -g: get hdmi native mode\n");
    fprintf(stderr, "   -s hdmi_mode: 0:hdmi auto mode, 1:manually mode\n");
}

static void parseOpt(int argc, char **argv)
{
    int c;

    while (1) {
        c = getopt(argc, argv, "hugs:");
        if (c == EOF)
            break;
        switch (c) {
        case 'u':
            // get user setting
            std::cout << "display0 Hdmi user setting is "
                      << mDisplayConfig->getHdmiUserSetting(0) << std::endl;
            std::cout << "display1 Hdmi user setting is "
                      << mDisplayConfig->getHdmiUserSetting(1) << std::endl;
            break;
        case 'g':
            std::cout << "display0 Hdmi native_mode is "
                      << mDisplayConfig->getHDMINativeMode(0) << std::endl;
            std::cout << "display1 Hdmi native_mode is "
                      << mDisplayConfig->getHDMINativeMode(1) << std::endl;
            break;
        case 's':
            std::cout << "set display0 mode  " << strtoul(optarg, NULL, 0)
                      << std::endl;
            mDisplayConfig->setActiveMode(0, strtoul(optarg, NULL, 0));
            break;
        case 'h':
            usage(argv[0]);
            exit(0);
            break;
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

int main(int argc, char **argv)
{
    parseOpt(argc, argv);
    return 0;
}
