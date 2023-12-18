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

//#define LOG_NDEBUG 0
#define LOG_TAG "GCH_HidlProfiler"

#include "hidl_profiler.h"

#include <log/log.h>

#include <memory>
#include <mutex>
#include <utility>

#include "profiler.h"

namespace android {
namespace hardware {
namespace camera {
namespace implementation {
namespace {

using ::google::camera_common::Profiler;

// setprop key for profiling open/close camera
constexpr char kPropKeyProfileOpenClose[] =
    "persist.vendor.camera.profiler.open_close";
// setprop key for profiling camera fps
constexpr char kPropKeyProfileFps[] = "persist.vendor.camera.profiler.fps";

constexpr char kFirstFrame[] = "First frame";
constexpr char kHalTotal[] = "HAL Total";
constexpr char kIdleString[] = "<-- IDLE -->";
constexpr char kOverall[] = "Overall";

class HidlProfilerImpl : public HidlProfiler {
 public:
  HidlProfilerImpl(uint32_t camera_id, int32_t latency_flag, int32_t fps_flag)
      : camera_id_string_("Cam" + std::to_string(camera_id)),
        camera_id_(camera_id),
        latency_flag_(latency_flag),
        fps_flag_(fps_flag) {
  }

  std::unique_ptr<HidlScopedProfiler> MakeScopedProfiler(
      ScopedType type) override {
    std::lock_guard lock(api_mutex_);

    if (type == ScopedType::kConfigureStream && fps_profiler_ == nullptr) {
      fps_profiler_ = CreateFpsProfiler();
    }

    if (latency_profiler_ == nullptr) {
      latency_profiler_ = CreateLatencyProfiler();
      if (latency_profiler_ != nullptr) {
        has_camera_open_ = false;
        config_count_ = 0;
        flush_count_ = 0;
        idle_count_ = 0;
      } else {
        return nullptr;
      }
    }

    IdleEndLocked();

    const char* name = nullptr;
    int32_t id = 0;
    switch (type) {
      case ScopedType::kOpen:
        name = "Open";
        has_camera_open_ = true;
        latency_profiler_->SetUseCase(camera_id_string_ + "-Open");
        break;
      case ScopedType::kConfigureStream:
        name = "ConfigureStream";
        if (!has_camera_open_) {
          latency_profiler_->SetUseCase(camera_id_string_ + "-Reconfiguration");
        }
        id = config_count_++;
        break;
      case ScopedType::kFlush:
        name = "Flush";
        latency_profiler_->SetUseCase(camera_id_string_ + "-Flush");
        id = flush_count_++;
        break;
      case ScopedType::kClose:
        name = "Close";
        latency_profiler_->SetUseCase(camera_id_string_ + "-Close");
        break;
      default:
        ALOGE("%s: Unknown type %d", __FUNCTION__, type);
        return nullptr;
    }
    return std::make_unique<HidlScopedProfiler>(
        latency_profiler_, name, id, [this, type]() {
          std::lock_guard lock(api_mutex_);
          if (type == ScopedType::kClose) {
            DeleteProfilerLocked();
          } else {
            IdleStartLocked();
          }
        });
  }

  void FirstFrameStart() override {
    std::lock_guard lock(api_mutex_);
    IdleEndLocked();
    if (latency_profiler_ != nullptr) {
      latency_profiler_->Start(kFirstFrame, Profiler::kInvalidRequestId);
      latency_profiler_->Start(kHalTotal, Profiler::kInvalidRequestId);
    }
  }

  void FirstFrameEnd() override {
    std::lock_guard lock(api_mutex_);
    if (latency_profiler_ != nullptr) {
      latency_profiler_->End(kFirstFrame, Profiler::kInvalidRequestId);
      latency_profiler_->End(kHalTotal, Profiler::kInvalidRequestId);
      DeleteProfilerLocked();
    }
  }

  void ProfileFrameRate(const std::string& name) override {
    std::lock_guard lock(api_mutex_);
    if (fps_profiler_ != nullptr) {
      fps_profiler_->ProfileFrameRate(name);
    }
  }

  void SetLatencyProfiler(std::unique_ptr<Profiler> profiler) override {
    if (profiler == nullptr || latency_profiler_ == nullptr) {
      return;
    }
    latency_profiler_ = std::move(profiler);
    if (latency_profiler_ != nullptr) {
      latency_profiler_->SetDumpFilePrefix(
          "/data/vendor/camera/profiler/hidl_open_close_");
      latency_profiler_->Start(kOverall, Profiler::kInvalidRequestId);
      has_camera_open_ = false;
      config_count_ = 0;
      flush_count_ = 0;
      idle_count_ = 0;
    }
  }

  void SetFpsProfiler(std::unique_ptr<Profiler> profiler) override {
    if (profiler == nullptr || fps_profiler_ == nullptr) {
      return;
    }
    fps_profiler_ = std::move(profiler);
    if (fps_profiler_ != nullptr) {
      fps_profiler_->SetDumpFilePrefix(
          "/data/vendor/camera/profiler/hidl_fps_");
    }
  }

 private:
  std::shared_ptr<Profiler> CreateLatencyProfiler() {
    if (latency_flag_ == Profiler::SetPropFlag::kDisable) {
      return nullptr;
    }
    std::shared_ptr<Profiler> profiler = Profiler::Create(latency_flag_);
    if (profiler == nullptr) {
      ALOGE("%s: Failed to create profiler", __FUNCTION__);
      return nullptr;
    }
    profiler->SetDumpFilePrefix(
        "/data/vendor/camera/profiler/hidl_open_close_");
    profiler->Start(kOverall, Profiler::kInvalidRequestId);
    return profiler;
  }

  std::shared_ptr<Profiler> CreateFpsProfiler() {
    if (fps_flag_ == Profiler::SetPropFlag::kDisable) {
      return nullptr;
    }
    std::shared_ptr<Profiler> profiler = Profiler::Create(fps_flag_);
    if (profiler == nullptr) {
      ALOGE("%s: Failed to create profiler", __FUNCTION__);
      return nullptr;
    }
    profiler->SetDumpFilePrefix("/data/vendor/camera/profiler/hidl_fps_");
    return profiler;
  }

  void DeleteProfilerLocked() {
    if (latency_profiler_ != nullptr) {
      latency_profiler_->End(kOverall, Profiler::kInvalidRequestId);
      latency_profiler_ = nullptr;
    }
  }

  void IdleStartLocked() {
    if (latency_profiler_ != nullptr) {
      latency_profiler_->Start(kIdleString, idle_count_++);
    }
  }

  void IdleEndLocked() {
    if (latency_profiler_ != nullptr && idle_count_ > 0) {
      latency_profiler_->End(kIdleString, idle_count_ - 1);
    }
  }

  uint32_t GetCameraId() const {
    return camera_id_;
  }
  int32_t GetLatencyFlag() const {
    return latency_flag_;
  }
  int32_t GetFpsFlag() const {
    return fps_flag_;
  }

  const std::string camera_id_string_;
  const uint32_t camera_id_;
  const int32_t latency_flag_;
  const int32_t fps_flag_;

  // Protect all API functions mutually exclusive, all member variables should
  // also be protected by this mutex.
  std::mutex api_mutex_;
  std::shared_ptr<Profiler> latency_profiler_;
  std::shared_ptr<Profiler> fps_profiler_;
  bool has_camera_open_;
  uint8_t config_count_;
  uint8_t flush_count_;
  uint8_t idle_count_;
};

class HidlProfilerMock : public HidlProfiler {
  std::unique_ptr<HidlScopedProfiler> MakeScopedProfiler(ScopedType) override {
    return nullptr;
  }

  void FirstFrameStart() override {
  }

  void FirstFrameEnd() override {
  }

  void ProfileFrameRate(const std::string&) override {
  }

  void SetLatencyProfiler(
      std::unique_ptr<google::camera_common::Profiler> /* profiler */) override {
  }

  void SetFpsProfiler(
      std::unique_ptr<google::camera_common::Profiler> /* profiler */) override {
  }

  uint32_t GetCameraId() const override {
    return 0;
  }
  int32_t GetLatencyFlag() const override {
    return 0;
  }
  int32_t GetFpsFlag() const override {
    return 0;
  }
};

}  // anonymous namespace

std::shared_ptr<HidlProfiler> HidlProfiler::Create(uint32_t camera_id) {
  int32_t latency_flag = property_get_int32(
      kPropKeyProfileOpenClose, Profiler::SetPropFlag::kCustomProfiler);
  int32_t fps_flag = property_get_int32(kPropKeyProfileFps,
                                        Profiler::SetPropFlag::kCustomProfiler);
  if (latency_flag == Profiler::SetPropFlag::kDisable &&
      fps_flag == Profiler::SetPropFlag::kDisable) {
    return std::make_shared<HidlProfilerMock>();
  }
  // Use stopwatch flag to print result.
  if ((latency_flag & Profiler::SetPropFlag::kPrintBit) != 0) {
    latency_flag |= Profiler::SetPropFlag::kStopWatch;
  }
  // Use interval flag to print fps instead of print on end.
  if ((fps_flag & Profiler::SetPropFlag::kPrintBit) != 0) {
    fps_flag |= Profiler::SetPropFlag::kPrintFpsPerIntervalBit;
    fps_flag &= ~Profiler::SetPropFlag::kPrintBit;
  }
  return std::make_shared<HidlProfilerImpl>(camera_id, latency_flag, fps_flag);
}

HidlScopedProfiler::HidlScopedProfiler(std::shared_ptr<Profiler> profiler,
                                       const std::string name, int id,
                                       std::function<void()> end_callback)
    : profiler_(profiler),
      name_(std::move(name)),
      id_(id),
      end_callback_(end_callback) {
  profiler_->Start(name_, id_);
  profiler_->Start(kHalTotal, Profiler::kInvalidRequestId);
}

HidlScopedProfiler::~HidlScopedProfiler() {
  profiler_->End(kHalTotal, Profiler::kInvalidRequestId);
  profiler_->End(name_, id_);
  if (end_callback_) {
    end_callback_();
  }
}

}  // namespace implementation
}  // namespace camera
}  // namespace hardware
}  // namespace android
