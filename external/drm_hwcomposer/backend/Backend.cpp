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

#include "Backend.h"

#include "BackendManager.h"
#include "bufferinfo/BufferInfoGetter.h"

namespace android {

HWC2::Error Backend::ValidateDisplay(DrmHwcTwo::HwcDisplay *display,
                                     uint32_t *num_types,
                                     uint32_t *num_requests) {
  *num_types = 0;
  *num_requests = 0;
  size_t avail_planes = display->primary_planes().size() +
                        display->overlay_planes().size();

  /*
   * If more layers then planes, save one plane
   * for client composited layers
   */
  if (avail_planes < display->layers().size())
    avail_planes--;

  std::map<uint32_t, DrmHwcTwo::HwcLayer *> z_map, z_map_tmp;
  uint32_t z_index = 0;
  // First create a map of layers and z_order values
  for (std::pair<const hwc2_layer_t, DrmHwcTwo::HwcLayer> &l :
       display->layers())
    z_map_tmp.emplace(std::make_pair(l.second.z_order(), &l.second));
  // normalise the map so that the lowest z_order layer has key 0
  for (std::pair<const uint32_t, DrmHwcTwo::HwcLayer *> &l : z_map_tmp)
    z_map.emplace(std::make_pair(z_index++, l.second));

  uint32_t total_pixops = display->CalcPixOps(z_map, 0, z_map.size());
  uint32_t gpu_pixops = 0;

  int client_start = -1, client_size = 0;

  if (display->compositor().ShouldFlattenOnClient()) {
    client_start = 0;
    client_size = z_map.size();
    display->MarkValidated(z_map, client_start, client_size);
  } else {
    std::tie(client_start, client_size) = GetClientLayers(display, z_map);

    int extra_client = (z_map.size() - client_size) - avail_planes;
    if (extra_client > 0) {
      int start = 0, steps;
      if (client_size != 0) {
        int prepend = std::min(client_start, extra_client);
        int append = std::min(int(z_map.size() - (client_start + client_size)),
                              extra_client);
        start = client_start - prepend;
        client_size += extra_client;
        steps = 1 + std::min(std::min(append, prepend),
                             int(z_map.size()) - (start + client_size));
      } else {
        client_size = extra_client;
        steps = 1 + z_map.size() - extra_client;
      }

      gpu_pixops = INT_MAX;
      for (int i = 0; i < steps; i++) {
        uint32_t po = display->CalcPixOps(z_map, start + i, client_size);
        if (po < gpu_pixops) {
          gpu_pixops = po;
          client_start = start + i;
        }
      }
    }

    display->MarkValidated(z_map, client_start, client_size);

    bool testing_needed = !(client_start == 0 && client_size == z_map.size());

    if (testing_needed &&
        display->CreateComposition(true) != HWC2::Error::None) {
      ++display->total_stats().failed_kms_validate_;
      gpu_pixops = total_pixops;
      client_size = z_map.size();
      display->MarkValidated(z_map, 0, client_size);
    }
  }

  *num_types = client_size;

  display->total_stats().frames_flattened_ = display->compositor()
                                                 .GetFlattenedFramesCount();
  display->total_stats().gpu_pixops_ += gpu_pixops;
  display->total_stats().total_pixops_ += total_pixops;

  return *num_types ? HWC2::Error::HasChanges : HWC2::Error::None;
}

std::tuple<int, int> Backend::GetClientLayers(
    DrmHwcTwo::HwcDisplay *display,
    const std::map<uint32_t, DrmHwcTwo::HwcLayer *> &z_map) {
  int client_start = -1, client_size = 0;

  for (auto & [ z_order, layer ] : z_map) {
    if (IsClientLayer(display, layer)) {
      if (client_start < 0)
        client_start = z_order;
      client_size = (z_order - client_start) + 1;
    }
  }

  return std::make_tuple(client_start, client_size);
}

bool Backend::IsClientLayer(DrmHwcTwo::HwcDisplay *display,
                            DrmHwcTwo::HwcLayer *layer) {
  return !display->HardwareSupportsLayerType(layer->sf_type()) ||
         !BufferInfoGetter::GetInstance()->IsHandleUsable(layer->buffer()) ||
         display->color_transform_hint() != HAL_COLOR_TRANSFORM_IDENTITY ||
         (layer->RequireScalingOrPhasing() &&
          display->resource_manager()->ForcedScalingWithGpu());
}

REGISTER_BACKEND("generic", Backend);

}  // namespace android
