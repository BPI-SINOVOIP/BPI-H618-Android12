// Copyright (C) 2021 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "profiler.h"

#include <android-base/properties.h>
#include <sys/prctl.h>

#include "perfetto.h"

namespace {

class GpuCounterDataSource : public perfetto::DataSource<GpuCounterDataSource> {
 public:
  void OnSetup(const SetupArgs& args) override {
    PERFETTO_ILOG("GpuCounterDataSource OnSetup, name: %s", args.config->name().c_str());
    const std::string& config_raw = args.config->gpu_counter_config_raw();
    perfetto::protos::pbzero::GpuCounterConfig::Decoder config(config_raw);
    for(auto it = config.counter_ids(); it; ++it) {
      counter_ids.push_back(it->as_uint32());
    }
    first = true;
  }

  void OnStart(const StartArgs&) override {
    PERFETTO_ILOG("GpuCounterDataSource OnStart called");
  }

  void OnStop(const StopArgs&) override {
    PERFETTO_ILOG("GpuCounterDataSource OnStop called");
  }

  bool first = true;
  uint64_t count = 0;
  std::vector<uint32_t> counter_ids;
};

class GpuRenderStageDataSource: public perfetto::DataSource<GpuRenderStageDataSource> {
 public:
  void OnSetup(const SetupArgs& args) override {
    PERFETTO_ILOG("GpuRenderStageDataSource OnSetup called, name: %s",
      args.config->name().c_str());
    first = true;
  }

  void OnStart(const StartArgs&) override {
    PERFETTO_ILOG("GpuRenderStageDataSource OnStart called");
  }

  void OnStop(const StopArgs&) override {
    PERFETTO_ILOG("GpuRenderStageDataSource OnStop called");
  }

  bool first = true;
  uint64_t count = 0;
};

}

void try_register_goldfish_perfetto() {
  std::string enableString = android::base::GetProperty("debug.graphics.gpu.profiler.perfetto", "");
  if (enableString != "1" && enableString != "true") {
    return;
  }
  if (!prctl(PR_GET_DUMPABLE, 0, 0, 0, 0)) {
    return;
  }
  perfetto::TracingInitArgs args;
  args.backends = perfetto::kSystemBackend;
  perfetto::Tracing::Initialize(args);
  {
    perfetto::DataSourceDescriptor dsd;
    dsd.set_name("gpu.counters");
    GpuCounterDataSource::Register(dsd);
  }

  {
    perfetto::DataSourceDescriptor dsd;
    dsd.set_name("gpu.renderstages");
    GpuRenderStageDataSource::Register(dsd);
  }
}
