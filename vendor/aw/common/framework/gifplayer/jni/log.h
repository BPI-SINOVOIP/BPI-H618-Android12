//
// Created by huangweibin on 2021/4/26.
//

#ifndef GIFTEST_LOG_H
#define GIFTEST_LOG_H

#define LOG_DEBUG true

#define TAG "GifPlayer"

#include <android/log.h>

#ifdef LOG_DEBUG
#define LOGI(...) \
        __android_log_print(ANDROID_LOG_INFO,TAG,__VA_ARGS__)

#define LOGD(...) \
        __android_log_print(ANDROID_LOG_DEBUG,TAG,__VA_ARGS__)

#define LOGW(...) \
        __android_log_print(ANDROID_LOG_WARN,TAG,__VA_ARGS__)

#define LOGE(...) \
        __android_log_print(ANDROID_LOG_ERROR,TAG,__VA_ARGS__)
#else
#define LOGI(...)
#define LOGD(...)
#define LOGW(...)
#define LOGE(...)
#endif

#endif //GIFTEST_LOG_H
