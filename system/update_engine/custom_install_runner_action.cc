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

#include "update_engine/custom_install_runner_action.h"

#include <errno.h>
#include <sys/stat.h>
#include <base/bind.h>
#include <base/logging.h>
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/mount.h>
#include <sys/types.h>
#include <unistd.h>

#include <cmath>
#include <algorithm>
#include <cstdlib>
#include <string>

#include <base/files/file_path.h>
#include <base/files/file_util.h>
#include <base/logging.h>
#include <base/strings/string_split.h>
#include <base/strings/string_util.h>

#include "update_engine/common/action_processor.h"
#include "update_engine/common/boot_control_interface.h"
#include "update_engine/common/platform_constants.h"
#include "update_engine/common/subprocess.h"
#include "update_engine/common/utils.h"
#include "update_engine/common/boot_control.h"
#include "update_engine/payload_consumer/install_plan.h"

#include <base/bind.h>
#include <brillo/data_encoding.h>

#include "UpdateCustom.h"

#include <base/format_macros.h>
#include <base/strings/string_number_conversions.h>
#include <base/strings/stringprintf.h>
#include "update_engine/payload_consumer/payload_constants.h"

using std::string;

namespace chromeos_update_engine {

bool CustomInstallRunnerAction::updated_flags_ = false;
bool CustomInstallRunnerAction::is_running_ = false;
bool update_uboot_flags_ = true;
bool update_env_flags_ = true;

CustomInstallRunnerAction::CustomInstallRunnerAction() {}

CustomInstallRunnerAction::~CustomInstallRunnerAction() {}

void CustomInstallRunnerAction::PerformAction() {
	CHECK(HasInputObject());

  install_plan_ = GetInputObject();

  if (is_running_) {
    LOG(INFO) << "Custom update is running, nothing to do.";
    processor_->ActionComplete(this, ErrorCode::kSuccess);
    return;
  }
  if (updated_flags_) {
    LOG(INFO) << "Already updated Custom content. Skipping.";
    processor_->ActionComplete(this, ErrorCode::kSuccess);
    return;
  }

  // start update custom conten.
  is_running_ = true;
  StartUpdateCustomPartitions(install_plan_);
}

void CustomInstallRunnerAction::TerminateProcessing() {
  is_running_ = false;
}

void CustomInstallRunnerAction::StartUpdateCustomPartitions(InstallPlan install_plan_) {
#ifdef __ANDROID_RECOVERY__
  char updateContent[] = "/tmp/custom.zip";
#else
  char updateContent[] = "/data/ota_package/custom.zip";
#endif
  updated_flags_ = UpdateExec(updateContent, install_plan_.target_slot);
  CompleteUpdateCustomPartitions();
}

void CustomInstallRunnerAction::CompleteUpdateCustomPartitions() {

  if (!is_running_) {
    LOG(INFO) << "CustomInstallRunnerAction is no longer running.";
    updated_flags_= true;
  }

  is_running_ = false;

  if (HasOutputPipe()) {
    SetOutputObject(install_plan_);
  }
  if (!updated_flags_) {
    LOG(INFO) << "CustomInstallRunnerAction is kError.";
    processor_->ActionComplete(this, ErrorCode::kError);
  } else {
    LOG(INFO) << "CustomInstallRunnerAction is kSuccess.";
    processor_->ActionComplete(this, ErrorCode::kSuccess);
  }
}


}  // namespace chromeos_update_engine
