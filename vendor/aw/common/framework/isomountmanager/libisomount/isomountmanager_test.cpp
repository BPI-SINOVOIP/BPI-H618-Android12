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

// System headers required for setgroups, etc.
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>

#include <string.h>

#include <binder/IServiceManager.h>
#include "ISOMountManagerService.h"

using namespace android;

int main(int argc, char** argv)
{

    sp<IServiceManager> sm = defaultServiceManager();
    sp<IISOMountManagerService> isoManager;
    int count=0;
    sp<IBinder> binder;
    do {
        binder = sm->getService(String16("softwinner.isomountmanager"));
        if (binder != 0) {
            isoManager =interface_cast<IISOMountManagerService>(binder);
            break;
        }
        printf("softwinner isomountmanager service not published, waiting...\n");
        sleep(1);
        count++;
    } while(count<3);
    if (count>2) {
        printf("softwinner isomountmanager service not ready, abort\n");
        return 0;
    }

    if (argc==1 || (argc==2 && !(strncmp(argv[1],"--help",6)
        && strncmp(argv[1],"-h",2)))) {
        printf("useage :\n");
        printf("\n");
        printf("print all iso mount: \n");
        printf("    isomountmanager_test mount \n");
        printf("\n");
        printf("umount all iso : \n");
        printf("    isomountmanager_test umount \n");
        printf("\n");
        printf("mount iso : \n");
        printf("    isomountmanager_test mount [iso] [mountpoint]\n");
    } else if (argc==2 && !strncmp(argv[1],"umount",6)) {
        int count=0;
        ISOMountManager_MountInfo *mountInfo = new ISOMountManager_MountInfo[5];
        if((count=isoManager->getMountInfoList(mountInfo,5))>0) {
            int ret=isoManager->umountAll();
            if (ret) {
                printf("isomountmanager unmount all iso fail : %d\n",ret);
            }
            else {
                printf("isomountmanager unmount all iso success \n");
            }
        } else {
            printf("no iso need to be unmounted \n");
        }
    } else if (argc==2 && !strncmp(argv[1],"mount",5)) {
        ISOMountManager_MountInfo *mountInfo = new ISOMountManager_MountInfo[5];
        int count=0;
        if((count=isoManager->getMountInfoList(mountInfo,5))>0) {
            for(int i=0;i<count;i++)
                printf("isopath:%s,mountpath:%s\n",mountInfo[i].mMountPath,
                    mountInfo[i].mISOPath);
        }
    } else if (argc==4 && !strncmp(argv[1],"mount",5)) {
        printf(" isomountmanager mount %s on %s\n",argv[2],argv[3]);
        int ret=isoManager->isoMount(argv[3],argv[2]);
        if (ret)
            printf("isomountmanager mount fail : %d\n",ret);
        else
            printf("isomountmanager mount success \n");
    }
    return 0;
}
