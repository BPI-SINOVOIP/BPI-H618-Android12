/*
**
** Copyright 2008, The Android Open Source Project
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

// Proxy for ISOMountManager implementations

#undef NDEBUG
#define LOG_TAG "ISOMountManagerService"
#include <utils/Log.h>

#include <string.h>

#include <binder/IServiceManager.h>

#include <utils/Errors.h>  // for status_t
#include <utils/String8.h>

#include "ISOMountManagerService.h"

#include <stdio.h>
#include <malloc.h>
#include <linux/loop.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/vfs.h>
#include <sys/types.h>
#include <sys/sysmacros.h>
#include <sys/mount.h>
#include <time.h>
#include <unistd.h>
#include <utime.h>

#define    DEBUG true

#define MountPath "/mnt/bluray"

#define MOUNT_POINT_MAX 32

namespace android {

    void ISOMountManagerService::instantiate() {
        defaultServiceManager()->addService(
                String16("softwinner.isomountmanager"), new ISOMountManagerService());
    }

    ISOMountManagerService::ISOMountManagerService()
    {
//        ALOGD("ISOMountManagerService created");
        mCursor = 0;
        mInfoArray = new ISOMountManager_MountInfo *[MOUNT_POINT_MAX];
        memset(mInfoArray, 0, MOUNT_POINT_MAX * sizeof(ISOMountManager_MountInfo *));
        if(access(MountPath,0) != 0)
            mkdir(MountPath,0744);
    }

    ISOMountManagerService::~ISOMountManagerService()
    {
//        ALOGD("ISOMountManagerService destroyed");
        if(mInfoArray != NULL){
            delete [] mInfoArray;
            mInfoArray = NULL;
        }
    }

    bool ISOMountManagerService::isMountPointUsed(const char *mountPath){
        for(int i = 0; i < mCursor; i++){
            if(strcmp(mInfoArray[i]->mMountPath, mountPath) == 0){
                return true;
            }
        }
        return false;
    }

    void ISOMountManagerService::addMountInfo(const char *mountPath, const char *isoPath,
            const char *devPath){
        mInfoArray[mCursor] = new ISOMountManager_MountInfo();
        if(mInfoArray[mCursor] == NULL){
            ALOGE("ISOMountManagerService::addMountInfo()..ERROR!");
            return;
        }
        strncpy(mInfoArray[mCursor]->mMountPath, mountPath, PATH_SIZE_MAX);
        strncpy(mInfoArray[mCursor]->mISOPath, isoPath, PATH_SIZE_MAX);
        strncpy(mInfoArray[mCursor]->mDevPath, devPath, PATH_SIZE_MAX);
        mCursor++;
    }

    void ISOMountManagerService::deleteMountInfo(const char *mountPath){
        for(int i = 0; i < mCursor; i++){
            if(strcmp(mInfoArray[i]->mMountPath, mountPath) == 0){
                delete mInfoArray[i];
                for(int j = i; j < mCursor; j++){
                    mInfoArray[j] = mInfoArray[j + 1];
                }
                mCursor--;
                return;
            }
        }
        ALOGE("ISOMountManagerService::deleteMountInfo()...ERROR!");
    }

    int ISOMountManagerService::set_loop(char **device, const char *file,
            unsigned long long offset){
        char dev[PATH_SIZE_MAX];
        char *tryDev;
        loop_info64 loopinfo;
        struct stat statbuf;
        int i, dfd, ffd, mode, rc = -1;

        /* Open the file.  Barf if this doesn't work.  */
        mode = O_RDWR;
        ffd = open(file, mode);
        if (ffd < 0) {
            mode = O_RDONLY;
            ffd = open(file, mode);
            if (ffd < 0)
                return -errno;
        }

        /* Find a loop device.  */
        tryDev = *device ? *device : dev;
        /* 1048575 is a max possible minor number in Linux circa 2010 */
        for (i = 0; rc && i < 15; i++) {
            sprintf(dev, "/dev/block/loop%d", i);
            errno = 0;
            if (stat(tryDev, &statbuf) != 0 || !S_ISBLK(statbuf.st_mode)) {
                if (errno == ENOENT && tryDev == dev ) {
                    /* Node doesn't exist, try to create it.  */
                    if (mknod(dev, S_IFBLK|0644, makedev(7, i)) == 0)
                        goto try_to_open;
                }
                /* Ran out of block devices, return failure.  */
                rc = -ENOENT;
                break;
            }
try_to_open:
            /* Open the sucker and check its loopiness.  */
            dfd = open(tryDev, mode);
            if (dfd < 0 && errno == EROFS) {
                mode = O_RDONLY;
                dfd = open(tryDev, mode);
            }
            if (dfd < 0)
                goto try_again;

            memset(&loopinfo, 0, sizeof(loopinfo));
            rc = ioctl(dfd, LOOP_GET_STATUS64, &loopinfo);

            /* If device is free, claim it.  */
            if (rc && errno == ENXIO) {
                memset(&loopinfo, 0, sizeof(loopinfo));
                strncpy((char *)loopinfo.lo_file_name, file, LO_NAME_SIZE);
                loopinfo.lo_offset = offset;
                /* Associate free loop device with file.  */
                if (ioctl(dfd, LOOP_SET_FD, ffd) == 0) {
                    if (ioctl(dfd, LOOP_GET_STATUS64, &loopinfo) == 0)
                        rc = 0;
                    else {
                        ioctl(dfd, LOOP_CLR_FD, 0);
                    }
                } else {
                }

            /* If this block device already set up right, re-use it.
               (Yes this is racy, but associating two loop devices with the same
               file isn't pretty either.  In general, mounting the same file twice
               without using losetup manually is problematic.)
             */
            } else if (strcmp(file, (char *)loopinfo.lo_file_name) != 0
             || offset != loopinfo.lo_offset) {
                rc = -1;
            }
            close(dfd);
try_again:
            if (*device) break;
        }
        close(ffd);
        if (rc == 0) {
            if (!*device)
                *device = strdup(dev);
            return (mode == O_RDONLY); /* 1:ro, 0:rw */
        }
        return rc;
    }

    const char *ISOMountManagerService::getDevPath(const char *mountPath){
        for(int i = 0; i < mCursor; i++){
            if(strcmp(mInfoArray[i]->mMountPath, mountPath) == 0){
                return mInfoArray[i]->mDevPath;
            }
        }
        return NULL;
    }

    int ISOMountManagerService::isoMount(const char *mountPath, const char *isoPath) {
//        if(DEBUG)
            ALOGD("ISOMountManagerService::isoMount(): mountPath = %s, isoPath = %s",
                mountPath, isoPath);

        if(isMountPointUsed(mountPath)){
            ALOGD("iso mount isMountPointUsed = false");
            return -1;
        }

        char *dev = NULL;
        set_loop(&dev, isoPath, 0);
        int ret = mount(dev, mountPath, "iso9660", MS_MANDLOCK|MS_RDONLY, NULL);
        ALOGD("start mount dev = %s mountPath = %s ret = %d",dev,mountPath,ret);
        if(ret == 0){
            ALOGD("start mount dev = %s mountPath = %s type=iso9660 success",dev,mountPath);
            addMountInfo(mountPath, isoPath, dev);
        }
        else{
            ALOGD("mount as iso9660 failed errno = %s ",strerror(errno));
            ret = mount(dev, mountPath, "udf", MS_MANDLOCK|MS_RDONLY, NULL);
            if(ret == 0){
                ALOGD("start mount dev = %s mountPath = %s type=udf success",dev,mountPath);
                addMountInfo(mountPath, isoPath, dev);
            }
            else{
                ALOGD("mount as udf failed errno = %s ",strerror(errno));
                ret = errno;
            }
        }
        return ret;
    }

    int ISOMountManagerService::isoUmount(const char *mountPath) {
//        if(DEBUG)
            ALOGD("ISOMountManagerService::isoUmount(): mountPath = %s", mountPath);

        int ret = umount(mountPath);
        if(ret == 0){

            const char *devPath = getDevPath(mountPath);
            if(devPath != NULL){
                int fd;
                fd = open(devPath, O_RDONLY);
                if (fd < 0)
                    return -1;
                if(ioctl(fd, LOOP_CLR_FD, 0) == 0){
                    deleteMountInfo(mountPath);
                }
                close(fd);
            }
        }
        else{
            ret = errno;
        }
        return ret;
    }

    bool ISOMountManagerService::umountAll(){
//        if(DEBUG)
            ALOGD("ISOMountManagerService::umountAll()..");

        bool ret = true;
        for(int i = 0; i < mCursor; i++){
            if(isoUmount(mInfoArray[i]->mMountPath) != 0){
                ret = false;
            }
        }
        return ret;
    }

    int    ISOMountManagerService::getMountInfoList(ISOMountManager_MountInfo *plist, int length){
//        if(DEBUG)
            ALOGD("ISOMountManagerService::getMountInfoList()..length = %d", length);

        if(length < mCursor){
            return -1;
        }
        for(int i = 0; i < mCursor; i++){
            strncpy(plist[i].mMountPath, mInfoArray[i]->mMountPath, PATH_SIZE_MAX);
            strncpy(plist[i].mISOPath, mInfoArray[i]->mISOPath, PATH_SIZE_MAX);
        }
        return mCursor;
    }

    bool ISOMountManagerService::getISOPath(const char *mountPath, char *isoPath){
//        if(DEBUG)
            ALOGD("ISOMountManagerService::getIsoPath()..mountPath = %s", mountPath);

        for(int i = 0; i < mCursor; i++){
            if(strcmp(mInfoArray[i]->mMountPath, mountPath) == 0){
                strncpy(isoPath, mInfoArray[i]->mISOPath, PATH_SIZE_MAX);
                return true;
            }
        }
        return false;
    }

    int ISOMountManagerService::getMountPoints(const char *isoPath, Path *mountPoints, int length){
//        if(DEBUG)
            ALOGD("ISOMountManagerService::getMountPoints()..isoPath = %s, length = %d",
                isoPath, length);

        int ret = 0;
        for(int i = 0; i < mCursor; i++){
            if(strcmp(mInfoArray[i]->mISOPath, isoPath) == 0){
                strncpy(mountPoints[ret++], mInfoArray[i]->mMountPath, PATH_SIZE_MAX);
            }
        }
        return ret;
    }

    int    ISOMountManagerService::getTotalMountCount(){
        return mCursor;
    }

    int ISOMountManagerService::getISOMountCount(const char *isoPath){
        int ret = 0;
        for(int i = 0; i < mCursor; i++){
            if(strcmp(mInfoArray[i]->mISOPath, isoPath) == 0){
                ret++;
            }
        }
        return ret;
    }

} // namespace android
