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

#include "ServiceDescriptor.h"

#include <array>
#include <memory>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

ServiceDescriptor::ServiceDescriptor(std::string name, std::string cmd)
    : mName(name), mCommandLine(cmd) {}

std::optional<std::string> ServiceDescriptor::GetOutput(OutputConsumer* consumer) const {
    if (!IsAvailable()) return "service not available";

    const auto cmd = command();

    int commandExitStatus = 0;
    auto pipeStreamDeleter = [&commandExitStatus](std::FILE* fp) {
        commandExitStatus = pclose(fp);
    };
    std::unique_ptr<std::FILE, decltype(pipeStreamDeleter)> pipeStream(popen(cmd, "r"),
                                                                       pipeStreamDeleter);

    if (!pipeStream) {
        return std::string("Failed to execute ") + cmd + ", " + strerror(errno);
    }

    std::array<char, 65536> buffer;
    while (!std::feof(pipeStream.get())) {
        auto readLen = fread(buffer.data(), 1, buffer.size(), pipeStream.get());
        consumer->Write(buffer.data(), readLen);
    }

    pipeStream.reset();

    if (commandExitStatus == 0) {
        return std::nullopt;
    } else if (commandExitStatus < 0) {
        return std::string("Failed when pclose ") + cmd + ", " + strerror(errno);
    } else {
        return std::string("Error when executing ") + cmd +
               ", exit code: " + std::to_string(commandExitStatus);
    }
}
