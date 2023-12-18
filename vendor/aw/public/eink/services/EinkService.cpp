/*
 * Copyright 2016 The Android Open Source Project
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

#include <android-base/logging.h>
#include <android-base/stringprintf.h>
#include <binder/BinderService.h>
#include <binder/IServiceManager.h>
#include <binder/Status.h>

#include "com/softwinner/BnEinkService.h"
#include "DisplayConfigHal.h"
#include "Handwriting.h"
#include "Bitmap.h"

using namespace android;
using ::com::softwinner::IEinkMode;
using android::base::StringAppendF;

namespace vendor {
namespace display {

class EinkService : public com::softwinner::BnEinkService {

public:
    static char const* getServiceName() {
        return "eink";
    }

    EinkService() {
        LOG(INFO) << "EinkService created.";
        mRefreshMode = IEinkMode::INIT;
    }

    ::android::binder::Status init(const ::android::view::Surface& surface, const ::std::vector<int32_t>& location) {
        LOG(INFO) << "EinkService init";
        mHandwriting = new Handwriting(new Surface(surface.graphicBufferProducer), location[0], location[1]);
        return binder::Status::ok();
    }

    ::android::binder::Status start() {
        LOG(INFO) << "EinkService start";
        mHandwriting->start();
        return binder::Status::ok();
    }

    ::android::binder::Status stop() {
        LOG(INFO) << "EinkService stop";
        mHandwriting->stop();
        mHandwriting = nullptr;
        return binder::Status::ok();
    }

    ::android::binder::Status setWindowFocus(bool winFocus) {
        LOG(INFO) << "EinkService winFocus";
        mHandwriting->setWindowFocus(winFocus);
        return binder::Status::ok();
    }

    ::android::binder::Status clear() {
        LOG(INFO) << "EinkService clear";
        mHandwriting->clear();
        return binder::Status::ok();
    }

    ::android::binder::Status refresh() {
        LOG(INFO) << "EinkService refresh";
        mHandwriting->refresh();
        return binder::Status::ok();
    }

    ::android::binder::Status setBackground(const ::android::graphics::Bitmap& bitmap) {
        LOG(INFO) << "EinkService setBackground";
        SkBitmap* sb = nullptr;
        if (bitmap.nativeBitmap != nullptr) {
            sb = new SkBitmap();
            bitmap.nativeBitmap->getSkBitmap(sb);
        } else {
            LOG(ERROR) << "EinkService setBackground nullptr";
        }
        mHandwriting->setBackground(sb);
        return binder::Status::ok();
    }

    ::android::binder::Status save(const ::std::string& path, bool withBackground,  bool* _aidl_return) {
        LOG(INFO) << "EinkService save to " << path << " withBackground=" << withBackground;
        *_aidl_return = mHandwriting->save(path, withBackground);
        return binder::Status::ok();
    }

    ::android::binder::Status getRefreshMode(int32_t* _aidl_return) {
        LOG(INFO) << "EinkService getRefreshMode";
        *_aidl_return = mRefreshMode;
        return binder::Status::ok();
    }

    ::android::binder::Status setRefreshMode(int mode) {
        LOG(INFO) << "EinkService setRefreshMode " << mode;
        mRefreshMode = mode;
        DisplayConfigHal::getInstance()->setRefreshMode(mode);
        return binder::Status::ok();
    }

    ::android::binder::Status forceGlobalRefresh(bool rightNow, int32_t* _aidl_return) {
        LOG(INFO) << "EinkService forceRefresh rightNow=" << rightNow;
        *_aidl_return = DisplayConfigHal::getInstance()->forceGlobalRefresh(rightNow);
        return binder::Status::ok();
    }

    ::android::binder::Status autoTest(bool start, int interval, int pressureMin, int pressureMax, int32_t* _aidl_return) {
        LOG(INFO) << "EinkService autoTest start=" << start;
        *_aidl_return = mHandwriting->autoTest(start, interval, pressureMin, pressureMax);
        return binder::Status::ok();
    }


    android::status_t dump(int fd, const Vector<String16>& args) {
        UNUSED(args);
        LOG(INFO) << "EinkService dump";

        std::string result;
        StringAppendF(&result, "EinkService(dumpsys eink):\n\n");

        if (args.size() == 0) {
            android::base::StringAppendF(&result, "option is null\n\n");
        } else {
            for(int i=0; i < args.size(); i++) {
                String16 c = args[i];
                //LOG(DEBUG) << "dump " << c;

                if (c == String16("--randomtest")) {
                    if (i+8 < args.size()) {
                        if (args[i+1] == String16("--mode")
                            && args[i+3] == String16("--step")
                            && args[i+5] == String16("--stepinterval")
                            && args[i+7] == String16("--interval")) {
                            LOG(DEBUG) << "random test mode:" << args[i+2] << " step:" << args[i+4]
                                    << " stepinterval:" << args[i+6]<< " interval:" << args[i+8];
                            if (mHandwriting != nullptr) {
                                int mode = atoi(String8(args[i+2]).c_str());
                                int step= atoi(String8(args[i+4]).c_str());
                                int stepinterval= atoi(String8(args[i+6]).c_str());
                                int interval = atoi(String8(args[i+8]).c_str());
                                if (mode == 0) {
                                    mHandwriting->autoTest(false, 0, 0, 0, 0, 0);
                                } else {
                                    mHandwriting->autoTest(true, interval, (PRESSURE_MIN + PRESSURE_MAX)/2,
                                        (PRESSURE_MIN + PRESSURE_MAX)/2, step, stepinterval);
                                }
                            }
                         }
                         i += 8;
                    } else {
                        StringAppendF(&result, "start random line auto test\n");
                        StringAppendF(&result, "stepinterval for n millisecond to draw\n");
                        StringAppendF(&result, "interval for n second to clear\n");
                        StringAppendF(&result, "--randomtest --mode 1 --step 50 --stepinterval 100 --interval 120\n");
                        StringAppendF(&result, "stop random line auto test\n");
                        StringAppendF(&result, "--randomtest --mode 0 --step 50 --stepinterval 100 --interval 120\n\n");
                    }
                    break;
                }

                if (c == String16("--fixtest")) {
                    if (i+8 < args.size()) {
                        if (args[i+1] == String16("--mode")
                            && args[i+3] == String16("--xstep")
                            && args[i+5] == String16("--ystep")
                            && args[i+7] == String16("--interval")) {
                            LOG(DEBUG) << "fix test mode:" << args[i+2] << " xstep:" << args[i+4]<< " ystep:" << args[i+6]<< " interval:" << args[i+8];
                            if (mHandwriting != nullptr) {
                                int mode = atoi(String8(args[i+2]).c_str());
                                int xstep= atoi(String8(args[i+4]).c_str());
                                int ystep= atoi(String8(args[i+6]).c_str());
                                int interval= atoi(String8(args[i+8]).c_str());
                                mHandwriting->autoFixLineTest(mode, xstep, ystep, interval);
                            }
                        }
                        i += 8;
                    } else {
                        StringAppendF(&result, "start fix line auto test\n");
                        StringAppendF(&result, "interval for n millisecond to one point\n");
                        StringAppendF(&result, "--fixtest --mode 1 --xstep 50 --ystep 50 --interval 100\n");
                        StringAppendF(&result, "stop fix line auto test\n");
                        StringAppendF(&result, "--fixtest --mode 0 --xstep 50 --ystep 50 --interval 100\n\n");
                    }
                    break;
                }
            }
        }

        android::base::StringAppendF(&result, "mRefreshMode=0x%x\n", mRefreshMode);
        write(fd, result.c_str(), result.size());
        return NO_ERROR;
    }

private:
    ~EinkService() {
        LOG(INFO) << "EinkService destroyed";
    }
    sp<Handwriting> mHandwriting;
    int mRefreshMode;
};

}  // namespace display
}  // namespace vendor

int main(const int /* argc */, char *argv[]) {
    setenv("ANDROID_LOG_TAGS", "*:v", 1);
    base::InitLogging(argv);
    LOG(INFO) << "main EinkService is running.";
    //IPCThreadState::self()->disableBackgroundScheduling(true);
    status_t ret = BinderService<::vendor::display::EinkService>::publish();
    if (ret != android::OK) {
        return ret;
    }

    sp<ProcessState> ps(ProcessState::self());
    ps->startThreadPool();
    ps->giveThreadPoolName();
    IPCThreadState::self()->joinThreadPool();

    LOG(INFO) << "EinkService shutting down";
    return 0;
}
