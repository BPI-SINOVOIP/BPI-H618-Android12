/*
 * Copyright (C) 2016 The Android Open Source Project
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

#ifndef SUNXI_HWC_NORMAL_WRITE_BACK_DISP2_H_
#define SUNXI_HWC_NORMAL_WRITE_BACK_DISP2_H_

#include "UtilsDisp2.h"
#include "WriteBackBufferPool.h"

namespace sunxi {

    class NormalWriteBackDisp2: public WriteBackBase {
    public:
        ~NormalWriteBackDisp2();
        NormalWriteBackDisp2(int id);
        virtual int writebackOneFrame(int fence);
        virtual std::shared_ptr<Layer> acquireLayer();
        virtual int releaseLayer(std::shared_ptr<Layer>& lyr, int fence);
        virtual void setScreenSize(int width, int height);
        virtual void setFrameSize(int width, int height);
        virtual void setMargin(int hpersent, int vpersent);
        virtual int performFrameCommit(int hwid, unsigned int syncnum,
                disp_layer_config2* conf, int lyrNum);

    private:
        int setupByDriver(std::shared_ptr<Layer>& lyr);
        int setupWbInfo(struct disp_capture_info2* info, std::shared_ptr<Layer>& lyr);
        int mHwId;
        int mCurW;
        int mCurH;
        int mWidth;
        int mHeight;
        int mVarWidth;
        int mVarHeight;
        int mHpercent;
        int mVpercent;
        disp_pixel_format mFormat;
        unsigned int mSyncCnt;
        unsigned int mSigCnt;
        WriteBackBufferPool* mBufPool;
        UtilsDisp2* mUtils;
        struct disp_capture_info2* mWbInfo;
        bool mCnfChanged;
    };

} // namespace sunxi

#endif // NORMAL_WRITE_BACK_DISP2_H_

