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

#ifndef SUNXI_ROTATOR_MANAGER_H_
#define SUNXI_ROTATOR_MANAGER_H_

#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <unordered_map>
#include <hardware/hwcomposer_defs.h>

namespace sunxi {

class BufferPool;
class BufferPoolCache;
class Layer;
class RotateTask;

// OutputBuffer_t describe all the necessary information of the rotator output,
// which declare in HardwareRotator.h.
typedef struct OutputBuffer OutputBuffer_t;

// Each active rotate layer will attach to a RotateSession object.
class RotateSession {
public:
    RotateSession();
   ~RotateSession();

    // create rotate task for the given input layer,
    // return 0 on success, and the layer should be set as device composition,
    // otherwise return -1, and the layer should be set as client composition.
    int createAsyncRotateTask(const std::shared_ptr<Layer>& layer);
    int getRotatedBuffer(const OutputBuffer_t** buffer, int* acquireFence);
    void postRotateTask();
    int putRotatedBuffer(int releaseFence);
    bool active() const { return mRotateTask!= nullptr; }
    void setBufferPoolCache(BufferPoolCache* cache) { mBufferPoolCache = cache; }
    void onRotateFailed() { mRotateFailedCount++; }

private:
    const int BUFFER_POOL_DEPTH = 4;
    std::unique_ptr<BufferPool> mBufferPool;
    std::shared_ptr<RotateTask> mRotateTask;
    BufferPoolCache* mBufferPoolCache;

    int mAllocatedBufferWidth;
    int mAllocatedBufferHeight;
    int mAllocatedBufferPixelFormat;

    int mRotateFailedCount;
};

class RotatorManager {
public:
    RotatorManager();
   ~RotatorManager();

    // try using hardware rotator to handle the given layer,
    // return 0 on success, otherwise return -1.
    int tryUsingHardwareRotator(const std::shared_ptr<Layer>& layer);
    int getRotatedBuffer(const std::shared_ptr<Layer>& layer,
            const OutputBuffer_t** buffer, int *acquireFence);
    void removeRotateSession(hwc2_layer_t id);
    void postRotateTask(hwc2_layer_t id);
    void postCommit(int releaseFenceFd);
    void dump(std::string& out);

private:
    RotateSession* getSessionById(hwc2_layer_t id);
    class LayerDestroyedListener;

    std::shared_ptr<LayerDestroyedListener> mLayerDestroyedListener;
    std::mutex mSessionLock;
    std::map<hwc2_layer_t, std::unique_ptr<RotateSession>> mRotateSession;
    std::unique_ptr<BufferPoolCache> mBufferPoolCache;
};

} // namespace sunxi

#endif
