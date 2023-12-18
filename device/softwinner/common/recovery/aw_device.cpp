/*
 * Copyright (C) 2015 The Android Open Source Project
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

#include "recovery_ui/device.h"
#include "recovery_ui/screen_ui.h"
#include <unistd.h>

class AwDevice : public Device {

  public:
    AwDevice(RecoveryUI* ui) :
        Device(ui) {
    }

};

class AwRecoveryUI : public ScreenRecoveryUI {
  public:
    AwRecoveryUI() : ScreenRecoveryUI(true) {
      touch_screen_allowed_ = true;
    }

    void SetProgressType(ProgressType type) override {
      ScreenRecoveryUI::SetProgressType(type);
      if (type == RecoveryUI::EMPTY) {
        show_text = true;
        show_text_ever = true;
      }
    }
};

Device* make_device() {
  return new AwDevice(new AwRecoveryUI);
}
