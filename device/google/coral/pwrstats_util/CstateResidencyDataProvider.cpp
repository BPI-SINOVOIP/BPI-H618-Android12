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
#define LOG_TAG "pwrstats_util"

#include "CstateResidencyDataProvider.h"
#include <dataproviders/DataProviderHelper.h>

#include <regex>
#include <string>
#include <fstream>
#include <iostream>
#include <string>

#include <android-base/parsedouble.h>
#include <android-base/logging.h>

int CstateResidencyDataProvider::getImpl(PowerStatistic* stat) const {
    std::ifstream file("/sys/kernel/debug/lpm_stats/stats");

    std::smatch matches;
    const std::regex searchExpr("\\[(.*?)\\] (.*?):");
    std::string line;
    const std::string searchStr = "total success time:";

    auto residencies = stat->mutable_c_state_residency();
    while (std::getline(file, line)) {
        if (std::regex_search(line, matches, searchExpr)) {
            auto residency = residencies->add_residency();
            residency->set_entity_name(matches[1]);
            residency->set_state_name(matches[2]);

            while (std::getline(file, line)) {
                size_t pos = line.find(searchStr);
                if (pos != std::string::npos) {
                    float val;
                    if (android::base::ParseFloat(line.substr(pos + searchStr.size()), &val)) {
                        residency->set_time_ms(static_cast<uint64_t>(val * 1000));
                    } else {
                        LOG(ERROR) << __func__ << ": failed to parse c-state data";
                    }
                    break;
                }
            }
        }
    }

    // Sort entries first by entity_name, then by state_name.
    // Sorting is needed to make interval processing efficient.
    std::sort(residencies->mutable_residency()->begin(),
        residencies->mutable_residency()->end(),
        [](const auto& a, const auto& b) {
            // First sort by entity_name, then by state_name
            if (a.entity_name() != b.entity_name()) {
                return a.entity_name() < b.entity_name();
            }

            return a.state_name() < b.state_name();
        });
    return 0;
}

int CstateResidencyDataProvider::getImpl(const PowerStatistic& start, PowerStatistic* interval) const {
    auto startResidency = start.c_state_residency().residency();
    auto intervalResidency = interval->mutable_c_state_residency()->mutable_residency();

    if (0 != StateResidencyInterval(startResidency, intervalResidency)) {
        interval->clear_c_state_residency();
        return 1;
    }

    return 0;
}

void CstateResidencyDataProvider::dumpImpl(const PowerStatistic& stat,
                                            std::ostream* output) const {
    *output << "C-State Residencies:" << std::endl;
    StateResidencyDump(stat.c_state_residency().residency(), output);
}

PowerStatCase CstateResidencyDataProvider::typeOf() const {
    return PowerStatCase::kCStateResidency;
}
