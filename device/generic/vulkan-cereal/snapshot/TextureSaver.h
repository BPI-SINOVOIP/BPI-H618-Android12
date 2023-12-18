// Copyright 2017 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#pragma once

#include "base/SmallVector.h"
#include "base/export.h"
#include "base/StdioStream.h"
#include "base/System.h"
#include "snapshot/common.h"

#include <functional>
#include <vector>

namespace android {
namespace snapshot {

class ITextureSaver {
    DISALLOW_COPY_AND_ASSIGN(ITextureSaver);

protected:
    ~ITextureSaver() = default;

public:
    ITextureSaver() = default;

    using Buffer = android::base::SmallVector<unsigned char>;
    using saver_t = std::function<void(android::base::Stream*, Buffer*)>;

    // Save texture to a stream as well as update the index
    virtual void saveTexture(uint32_t texId, const saver_t& saver) = 0;
    virtual bool hasError() const = 0;
    virtual uint64_t diskSize() const = 0;
    virtual bool compressed() const = 0;
    virtual bool getDuration(uint64_t* duration) = 0;
};

class TextureSaver final : public ITextureSaver {
    DISALLOW_COPY_AND_ASSIGN(TextureSaver);

public:
    AEMU_EXPORT TextureSaver(android::base::StdioStream&& stream);
    AEMU_EXPORT ~TextureSaver();
    AEMU_EXPORT void saveTexture(uint32_t texId, const saver_t& saver) override;
    AEMU_EXPORT void done();

    AEMU_EXPORT bool hasError() const override { return mHasError; }
    AEMU_EXPORT uint64_t diskSize() const override { return mDiskSize; }
    AEMU_EXPORT bool compressed() const override { return mIndex.version > 1; }

    // getDuration():
    // Returns true if there was save with measurable time
    // (and writes it to |duration| if |duration| is not null),
    // otherwise returns false.
    AEMU_EXPORT bool getDuration(uint64_t* duration) override {
        if (mEndTime < mStartTime) {
            return false;
        }

        if (duration) {
            *duration = mEndTime - mStartTime;
        }
        return true;
    }

private:
    struct FileIndex {
        struct Texture {
            uint32_t texId;
            int64_t filePos;
        };

        int64_t startPosInFile;
        int32_t version = 2;
        std::vector<Texture> textures;
    };

    void writeIndex();

    android::base::StdioStream mStream;
    // A buffer for fetching data from GPU memory to RAM.
    android::base::SmallFixedVector<unsigned char, 128> mBuffer;

    FileIndex mIndex;
    uint64_t mDiskSize = 0;
    bool mFinished = false;
    bool mHasError = false;

    uint64_t mStartTime = 0;
    uint64_t mEndTime = 0;
};

}  // namespace snapshot
}  // namespace android
