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

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

#include "Debug.h"
#include "draminfo.h"

namespace sunxi {

static bool readFromFile(const char *path, char *buf, size_t size)
{
    int fd = open(path, O_RDONLY, 0);
    if (fd == -1) {
        DLOGE("Could not open '%s', %s(%d)\n", path, strerror(errno), errno);
        return -errno;
    }
    size_t count = read(fd, buf, size);
    close(fd);
    return count == size;
}

#define DRAM_CLK_DTS_PATH "/sys/firmware/devicetree/base/dram/dram_clk"
#define DRAM_BUS_WIDTH_DTS_PATH "/sys/firmware/devicetree/base/dram/dram_mr1"
#define DRAM_TYPE_DTS_PATH "/sys/firmware/devicetree/base/dram/dram_type"

draminfo::draminfo()
  : mFrequency(0),
    mDataBusWidth(8),
    mType(0),
    mEffectiveBandwidth(0)
{
    getDramInfoFromDeviceTree();
    mEffectiveBandwidth = computeEffectiveBandwidth();
}

void draminfo::getDramInfoFromDeviceTree()
{
    uint8_t buf[4] = {0};
    if (!readFromFile(DRAM_CLK_DTS_PATH, (char*)buf, 4))
        DLOGE("get dram clock error!");
    else
        mFrequency =
            (buf[0] << 24) |
            (buf[1] << 16) |
            (buf[2] << 8)  |
            (buf[3]);

    if (!readFromFile(DRAM_BUS_WIDTH_DTS_PATH, (char*)buf, 4))
        DLOGE("get dram bus width error!");
    else
        mDataBusWidth = buf[3] & 0x01 ? 16 : 32;

    if (!readFromFile(DRAM_TYPE_DTS_PATH, (char*)buf, 4))
        DLOGE("get dram type error!");
    else
        mType =
            (buf[0] << 24) |
            (buf[1] << 16) |
            (buf[2] << 8)  |
            (buf[3]);

    DLOGI("draminfo: freq %d MHz, bus width %d bits, type %d",
            mFrequency, mDataBusWidth, mType);
}

/*
 * get effective bandwidth by level,
 * param level should be 0 or 1.
 * level == 0 : for ui resolution under 1080p
 * level == 1 : for ui resolution equal or exceed 1080p
 */
int draminfo::computeEffectiveBandwidth()
{
    /* 3:DDR3, 4:DDR4, 7:LPDDR3, 8:LPDDR4 */
    static const int DRAM_TYPE_DDR3   = 3;
    static const int DRAM_TYPE_DDR4   = 4;
    static const int DRAM_TYPE_LPDDR3 = 7;
    static const int DRAM_TYPE_LPDDR4 = 8;

    struct effectiveFactor {
        int type;
        float effective;
    };
    static const effectiveFactor factors_[4] = {
        {DRAM_TYPE_DDR3,   0.65},
        {DRAM_TYPE_DDR4,   0.65},
        {DRAM_TYPE_LPDDR3, 0.65},
        {DRAM_TYPE_LPDDR4, 0.45},
    };

    // default dram bandwidth effective;
    float effective = 0.5;

    for (int i = 0; i < 4; i++) {
        if (mType == factors_[i].type) {
            effective = factors_[i].effective;
            break;
        }
    }

    float factor = mDataBusWidth == 32 ? 1.0 : 0.5;
    int bandwidth = mFrequency * 8 * effective * factor;

    DLOGI("dram effective bandwidth: %d MBytes", bandwidth);
    return bandwidth;
}

int draminfo::frequency() const
{
    return mFrequency;
}

int draminfo::effectiveBandwidth() const
{
    return mEffectiveBandwidth;
}

draminfo* draminfo::getInstance()
{
    static draminfo instance;
    return &instance;
}

} // namespace sunxi
