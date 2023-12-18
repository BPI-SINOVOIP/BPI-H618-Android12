/*
* Copyright (C) 2020 The Android Open Source Project
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/
#ifndef _STATE_TRACKING_SUPPORT_H_
#define _STATE_TRACKING_SUPPORT_H_

#include "android/base/containers/HybridComponentManager.h"
#include "android/base/synchronization/AndroidLock.h"

#include <GLES2/gl2.h>

template <class IndexType, bool initialIsTrue>
class PredicateMap {
public:
    static const uint64_t kBitsPerEntry = 64;
    void add(IndexType objId) {
        static const uint64_t kNone = 0ULL;
        static const uint64_t kAll = ~kNone;
        IndexType index = objId / kBitsPerEntry;
        if (!mStorage.get_const(index)) {
            mStorage.add(index, initialIsTrue ? kAll : kNone);
        }
    }

    void remove(IndexType objId) {
        if (initialIsTrue) {
            set(objId, true);
        } else {
            set(objId, false);
        }
    }

    void set(IndexType objId, bool predicate) {
        IndexType index = objId / kBitsPerEntry;

        if (!mStorage.get_const(index)) return;

        uint64_t* current = mStorage.get(index);

        uint64_t flag = 1ULL << (objId % kBitsPerEntry);

        if (predicate) {
            *current = *current | flag;
        } else {
            *current = *current & (~flag);
        }
    }

    bool get(IndexType objId) const {
        IndexType index = objId / kBitsPerEntry;

        const uint64_t* current = mStorage.get_const(index);

        if (!current) return initialIsTrue;

        uint64_t flag = 1ULL << (objId % kBitsPerEntry);
        return (flag & (*current)) != 0;
    }

private:
    using Storage = android::base::HybridComponentManager<10000, IndexType, uint64_t>;
    Storage mStorage;
};

// Structures for fast validation of uniforms/attribs.

struct UniformLocationInfo {
    bool valid = false;
    uint32_t columns;
    uint32_t rows;
    bool isSampler;
    bool isInt;
    bool isArray;
    bool isUnsigned;
    bool isBool;
};

struct AttribIndexInfo {
    bool validInProgram = false;
};

using UniformValidationInfo = android::base::HybridComponentManager<1000, uint32_t, UniformLocationInfo>;
using AttribValidationInfo = android::base::HybridComponentManager<16, uint32_t, AttribIndexInfo>;

using LastQueryTargetInfo = android::base::HybridComponentManager<1000, uint32_t, uint32_t>;

using ExistenceMap = PredicateMap<uint32_t, false>;

struct RboProps {
    GLenum format;
    GLsizei multisamples;
    GLsizei width;
    GLsizei height;
    bool previouslyBound;
    uint32_t refcount;
    bool boundEGLImage;
};

struct SamplerProps {
    uint32_t refcount;
};

template <class T>
class ScopedLockedView {
public:
    ScopedLockedView(T* info) : mInfo(info) {
        mInfo->lock();
    }
    virtual ~ScopedLockedView() {
        mInfo->unlock();
    }
protected:
    T* mInfo;

    T* internalInfo() { return mInfo; }
    const T* internalInfo_const() const { return mInfo; }
};

struct RenderbufferInfo {
    android::base::guest::Lock infoLock;
    android::base::HybridComponentManager<1000, uint32_t, RboProps> component;

    void lock() { infoLock.lock(); }
    void unlock() { infoLock.unlock(); }

    class ScopedView : public ScopedLockedView<RenderbufferInfo> {
        public:
            ScopedView(RenderbufferInfo* info) : ScopedLockedView<RenderbufferInfo>(info) { }
            bool hasRbo(GLuint id) const {
                const RboProps* info = internalInfo_const()->component.get_const(id);
                if (!info) return false;
                return 0 != info->refcount;
            }
            virtual ~ScopedView() = default;
            RboProps* get(GLuint id) {
                return internalInfo()->component.get(id);
            }
            const RboProps* get_const(GLuint id) {
                return internalInfo_const()->component.get_const(id);
            }
            void addFresh(GLuint id) {
                RboProps props;
                props.format = GL_NONE;
                props.multisamples = 0;
                props.width = 0;
                props.height = 0;
                props.previouslyBound = false;
                props.refcount = 1;
                props.boundEGLImage = false;
                internalInfo()->component.add(id, props);
            }
            RboProps* bind(GLuint id) {
                if (!hasRbo(id)) addFresh(id);
                ref(id);
                RboProps* res = get(id);
                res->previouslyBound = true;
                return res;
            }
            void ref(GLuint id) {
                RboProps* props = get(id);
                if (!props) return;
                ++props->refcount;
            }
            bool unref(GLuint id) {
                RboProps* props = get(id);
                if (!props) return false;
                if (!props->refcount) return false;
                --props->refcount;
                bool gone = 0 == props->refcount;
                if (gone) {
                    props->format = 0;
                    props->multisamples = 0;
                    props->width = 0;
                    props->height = 0;
                    props->previouslyBound = false;
                    props->boundEGLImage = false;
                }
                return gone;
            }
    };
};

struct SamplerInfo {
    android::base::guest::Lock infoLock;
    android::base::HybridComponentManager<1000, uint32_t, SamplerProps> component;

    void lock() { infoLock.lock(); }
    void unlock() { infoLock.unlock(); }

    class ScopedView : public ScopedLockedView<SamplerInfo> {
        public:
            ScopedView(SamplerInfo* info) : ScopedLockedView<SamplerInfo>(info) { }
            bool samplerExists(GLuint id) const {
                const SamplerProps* info = internalInfo_const()->component.get_const(id);
                if (!info) return false;
                return 0 != info->refcount;
            }
            virtual ~ScopedView() = default;
            SamplerProps* get(GLuint id) {
                return internalInfo()->component.get(id);
            }
            const SamplerProps* get_const(GLuint id) {
                return internalInfo_const()->component.get_const(id);
            }
            void addFresh(GLuint id) {
                SamplerProps props;
                props.refcount = 1;
                internalInfo()->component.add(id, props);
            }
            SamplerProps* bind(GLuint id) {
                if (!samplerExists(id)) return 0;
                ref(id);
                SamplerProps* res = get(id);
                return res;
            }
            void ref(GLuint id) {
                SamplerProps* props = get(id);
                if (!props) return;
                ++props->refcount;
            }
            bool unref(GLuint id) {
                SamplerProps* props = get(id);
                if (!props) return false;
                if (!props->refcount) return false;
                --props->refcount;
                bool gone = 0 == props->refcount;
                return gone;
            }
    };
};


#endif
