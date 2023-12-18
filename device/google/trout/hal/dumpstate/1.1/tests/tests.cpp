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

#include "gtest/gtest.h"

#include "DumpstateServer.h"
#include "ServiceDescriptor.h"
#include "ServiceSupplier.h"
#include "config/XmlServiceSupplier.h"
#include "config/dumpstate_hal_configuration_V1_0.h"

#include <sstream>
#include <string>

static ServiceDescriptor MakePrinterService(const std::string& msg) {
    return ServiceDescriptor{msg, "/bin/echo -n \"" + msg + "\""};
}

class AccumulatorConsumer : public ServiceDescriptor::OutputConsumer {
  public:
    void Write(char* ptr, size_t len) override { ss.write(ptr, len); }

    std::string data() { return ss.str(); }

  private:
    std::stringstream ss;
};

TEST(DumpstateServer, RunCommand) {
    auto svc = MakePrinterService("hello world");
    AccumulatorConsumer ac;
    auto ok = svc.GetOutput(&ac);
    ASSERT_FALSE(ok.has_value());
    ASSERT_EQ("hello world", ac.data());
}

TEST(Configuration, FromXmlBuffer) {
    std::string buf = R"foo(
<dumpstateHalConfiguration version="1.0">
    <services>
        <service name="svc1" command="cmd1"/>
        <service name="svc2" command="cmd2 arg1"/>
    </services>
    <systemLogs>
        <service name="log" command="logcat"/>
    </systemLogs>
</dumpstateHalConfiguration>
  )foo";

    auto supplier = XmlServiceSupplier::fromBuffer(buf);
    ASSERT_TRUE(supplier.has_value());

    ASSERT_TRUE(supplier->GetSystemLogsService().has_value());
    ASSERT_EQ(2, supplier->GetServices().size());

    ASSERT_STREQ("log", supplier->GetSystemLogsService()->name());
    ASSERT_STREQ("logcat", supplier->GetSystemLogsService()->command());

    ASSERT_STREQ("svc1", supplier->GetServices().at(0).name());
    ASSERT_STREQ("svc2", supplier->GetServices().at(1).name());

    ASSERT_STREQ("cmd1", supplier->GetServices().at(0).command());
    ASSERT_STREQ("cmd2 arg1", supplier->GetServices().at(1).command());
}
