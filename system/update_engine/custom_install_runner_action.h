//
// Copyright (C) 2018 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#ifndef UPDATE_ENGINE_CUSTOM_INSTALL_RUNNER_ACTION_H_
#define UPDATE_ENGINE_CUSTOM_INSTALL_RUNNER_ACTION_H_

#include <memory>
#include <string>
#include <vector>

#include <brillo/streams/stream.h>

#include "update_engine/common/action.h"
#include "update_engine/common/boot_control_interface.h"
#include "update_engine/payload_consumer/install_plan.h"
#include "update_engine/common/hardware_interface.h"
#include <gtest/gtest_prod.h>
#include <sys/stat.h>
#include <sys/types.h>

namespace chromeos_update_engine {

class CustomInstallRunnerAction : public InstallPlanAction {
 public:
  CustomInstallRunnerAction();

  ~CustomInstallRunnerAction() override;

  void PerformAction() override;

  void TerminateProcessing() override;

  static std::string StaticType() { return "CustomInstallRunnerAction"; }
  std::string Type() const override { return StaticType(); }

  void StartUpdateCustomPartitions(InstallPlan install_plan);
	void CompleteUpdateCustomPartitions();

 private:
  FRIEND_TEST(CustomInstallRunnerActionTest, SimpleTest);
  FRIEND_TEST(CustomInstallRunnerActionTest, DoubleActionTest);

  static bool updated_flags_;
  static bool is_running_;
  BootControlInterface* boot_control_;
  InstallPlan install_plan_;

  DISALLOW_COPY_AND_ASSIGN(CustomInstallRunnerAction);
};

}  // namespace chromeos_update_engine


#endif
