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
#include <aidl/android/frameworks/stats/IStats.h>

#include <android/binder_manager.h>

#include <statslog.h>

#include <getopt.h>
#include <iostream>

using aidl::android::frameworks::stats::IStats;
using aidl::android::frameworks::stats::VendorAtom;
using aidl::android::frameworks::stats::VendorAtomValue;

void expect_message(int32_t action) {
    std::cout << "expect the following log in logcat:\n";
    std::cout << "statsd.*(" << action << ")0x10000->\n";
}

void show_help() {
    std::cout << "AIDL Stats HAL client\n";
    std::cout << " arguments:\n";
    std::cout << " -h or --help - shows help information\n";
    std::cout << " -v or --VendorAtom - tests report reportVendorAtom API\n";
    std::cout << "Please enable statsd logging using 'cmd stats print-logs'";
    std::cout << "\n\n you can use multiple arguments to trigger multiple events.\n";
}

int main(int argc, char* argv[]) {
    // get instance of the aidl version
    const std::string instance = std::string() + IStats::descriptor + "/default";
    std::shared_ptr<IStats> service =
        IStats::fromBinder(ndk::SpAIBinder(AServiceManager_getService(instance.c_str())));
    if (!service) {
        std::cerr << "No Stats aidl HAL";
        return 1;
    }

    std::cout << "Service instance obtained : " << instance << std::endl;

    static struct option opts[] = {
        {"VendorAtom", no_argument, 0, 'v'},
        {"help", no_argument, 0, 'h'},
    };

    int c;
    int hal_calls = 0;
    int failed_calls = 0;
    while ((c = getopt_long(argc, argv, "vh", opts, nullptr)) != -1) {
        switch (c) {
            case 'h': {
                show_help();
                break;
            }
            case 'v': {
                std::vector<VendorAtomValue> values;
                VendorAtomValue tmp;
                tmp.set<VendorAtomValue::longValue>(70000);
                values.push_back(tmp);
                tmp.set<VendorAtomValue::intValue>(7);
                values.push_back(tmp);
                tmp.set<VendorAtomValue::floatValue>(8.5);
                values.push_back(tmp);
                tmp.set<VendorAtomValue::stringValue>("test");
                values.push_back(tmp);
                tmp.set<VendorAtomValue::intValue>(3);
                values.push_back(tmp);
                VendorAtom atom = {
                    .reverseDomainName = "com.google.pixel", .atomId = 100001, .values = values};
                const ndk::ScopedAStatus ret = service->reportVendorAtom(atom);
                if (!ret.isOk()) {
                    std::cout << "reportVendorAtom failed: " << ret.getServiceSpecificError()
                              << ". Message: " << ret.getMessage() << std::endl;
                    ++failed_calls;
                }
                ++hal_calls;
                break;
            }
            default: {
                show_help();
                return 1;
            }
        }
    }

    if (hal_calls > 0) {
        std::cout << hal_calls << " HAL methods called.\n";
        std::cout << "try: logcat | grep \"statsd.*0x1000\"\n";
    }

    return failed_calls;
}
