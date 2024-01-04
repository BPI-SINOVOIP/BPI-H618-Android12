#ifndef __LIBBOOT_INFO_H
#define __LIBBOOT_INFO_H
#define LIBBOOT_API_VERSION 1.0
typedef int (*DeviceBurn)(const char* bootFile);
struct libboot_info{
    int flashType;
    int secureMode;
    DeviceBurn burnBoot0;
    DeviceBurn burnUboot;
    DeviceBurn burnConfig;
};

int getLibbootVersion();
int getLibbootInfo(struct libboot_info * info);
#endif
