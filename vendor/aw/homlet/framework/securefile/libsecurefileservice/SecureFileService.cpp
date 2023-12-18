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

// Proxy for secure file implementations

#undef NDEBUG
#define LOG_TAG "SecureFileService"
#include <utils/Log.h>

//#include <sys/types.h>
//#include <sys/stat.h>
//#include <dirent.h>
//#include <unistd.h>

#include <string.h>

#include <binder/IServiceManager.h>
//#include <cutils/atomic.h>
//#include <cutils/properties.h> // for property_get
//
//#include <utils/misc.h>
//
//#include <android_runtime/ActivityManager.h>
//
//#include <binder/IPCThreadState.h>

#include <utils/Errors.h>  // for status_t
//#include <utils/String8.h>

#include "SecureFileService.h"

#include "LocalArray.h"
#include "ScopedFd.h"
#include "ScopedLocalRef.h"
#include "ScopedPrimitiveArray.h"
#include "ScopedUtfChars.h"

#include <stdio.h>
#include <malloc.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/vfs.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <utime.h>

#define    DEBUG true
namespace android {

void SecureFileService::instantiate() {
    defaultServiceManager()->addService(
            String16("softwinner.securefile"), new SecureFileService());
}

SecureFileService::SecureFileService()
{
    ALOGD("SecureFileService created");
}

SecureFileService::~SecureFileService()
{
    ALOGD("SecureFileService destroyed");
}

bool SecureFileService::deleteFile(const char *path) {
    if(DEBUG)
        ALOGD("SecureFileService::Delete: path = %s", path);

    if (path == NULL) {
        return false;
    }
    return (remove(path) == 0);
}

static bool doStat(const char *path, struct stat& sb) {
    if (path == NULL) {
        return false;
    }
    return (stat(path, &sb) == 0);
}

long long SecureFileService::length(const char *path) {
    if(DEBUG)
        ALOGD("SecureFileService::length(): path = %s", path);

    struct stat sb;
    if (!doStat(path, sb)) {
        // We must return 0 for files that don't exist.
        // TODO: shouldn't we throw an IOException for ELOOP or EACCES?
        return 0;
    }

    /*
     * This android-changed code explicitly treats non-regular files (e.g.,
     * sockets and block-special devices) as having size zero. Some synthetic
     * "regular" files may report an arbitrary non-zero size, but
     * in these cases they generally report a block count of zero.
     * So, use a zero block count to trump any other concept of
     * size.
     *
     * TODO: why do we do this?
     */
    if (!S_ISREG(sb.st_mode) || sb.st_blocks == 0) {
        return 0;
    }
    return sb.st_size;
}

unsigned long SecureFileService::lastModified(const char *path) {
    if(DEBUG)
        ALOGD("SecureFileService::lastModified(): path = %s", path);

    struct stat sb;
    if (!doStat(path, sb)) {
        return 0;
    }
    unsigned long l = sb.st_mtime;
    ALOGD("SecureFileService::lastModified(): length = %ld", l);
    return l;
}

bool SecureFileService::isDirectory(const char *path)
{
    if(DEBUG)
        ALOGD("SecureFileService::isDirectory(): path = %s", path);

    struct stat sb;
    return (doStat(path, sb) && S_ISDIR(sb.st_mode));
}

bool SecureFileService::isFile(const char *path) {
    if(DEBUG)
        ALOGD("SecureFileService::isFile(): path = %s", path);

    struct stat sb;
    return (doStat(path, sb) && S_ISREG(sb.st_mode));
}

static bool doAccess(const char *path, int mode) {

    if (path == NULL) {
        return false;
    }
    return (access(path, mode) == 0);
}

bool SecureFileService::exists(const char *path) {
    return doAccess(path, F_OK);
}

bool SecureFileService::setLastModified(const char *path, unsigned long ms) {
    if(DEBUG)
        ALOGD("SecureFileService::setLastModified(): path = %s time = %ld", path, ms);

    if (path == NULL) {
        return false;
    }

    // We want to preserve the access time.
    struct stat sb;
    if (stat(path, &sb) == -1) {
        return false;
    }

    // TODO: we could get microsecond resolution with utimes(3), "legacy" though it is.
    utimbuf times;
    times.actime = sb.st_atime;
    times.modtime = static_cast<time_t>(ms);
    return (utime(path, &times) == 0);
}

static bool doStatFs(const char *path, struct statfs& sb) {
    if (path == NULL) {
        return false;
    }

    int rc = statfs(path, &sb);
    ALOGD("rc = %d", rc);
    return (rc != -1);
}

long long SecureFileService::getFreeSpace(const char *path) {
    if(DEBUG)
        ALOGD("SecureFileService::getFreeSpace(): path = %s", path);

    struct statfs sb;
    if (!doStatFs(path, sb)) {
        return 0;
    }
    return sb.f_bfree * sb.f_bsize; // free block count * block size in bytes.
}

long long SecureFileService::getTotalSpace(const char *path) {
    if(DEBUG)
        ALOGD("SecureFileService::getTotalSpace(): path = %s", path);

    struct statfs sb;
    if (!doStatFs(path, sb)) {
        return 0;
    }
    return sb.f_blocks * sb.f_bsize; // total block count * block size in bytes.
}

long long SecureFileService::getUsableSpace(const char *path) {
    if(DEBUG)
        ALOGD("SecureFileService::getUsableSpace(): path = %s", path);

    struct statfs sb;
    if (!doStatFs(path, sb)) {
        return 0;
    }
    return sb.f_bavail * sb.f_bsize; // non-root free block count * block size in bytes.
}

// Iterates over the filenames in the given directory.
class ScopedReaddir {
public:
    ScopedReaddir(const char* path) {
        mDirStream = opendir(path);
        mIsBad = (mDirStream == NULL);
    }

    ~ScopedReaddir() {
        if (mDirStream != NULL) {
            closedir(mDirStream);
        }
    }

    // Returns the next filename, or NULL.
    const char* next() {
        dirent* result = NULL;
        int rc = readdir_r(mDirStream, &mEntry, &result);
        if (rc != 0) {
            mIsBad = true;
            return NULL;
        }
        return (result != NULL) ? result->d_name : NULL;
    }

    // Has an error occurred on this stream?
    bool isBad() const {
        return mIsBad;
    }

private:
    DIR* mDirStream;
    dirent mEntry;
    bool mIsBad;

    // Disallow copy and assignment.
    ScopedReaddir(const ScopedReaddir&);
    void operator=(const ScopedReaddir&);
};

// DirEntry and DirEntries is a minimal equivalent of std::forward_list
// for the filenames.
struct DirEntry {
    DirEntry(const char* filename) : name(strlen(filename)) {
        strcpy(&name[0], filename);
        next = NULL;
    }
    // On Linux, the ext family all limit the length of a directory entry to
    // less than 256 characters.
    LocalArray<256> name;
    DirEntry* next;
};

class DirEntries {
public:
    DirEntries() : mSize(0), mHead(NULL) {
    }

    ~DirEntries() {
        while (mHead) {
            pop_front();
        }
    }

    bool push_front(const char* name) {
        DirEntry* oldHead = mHead;
        mHead = new DirEntry(name);
        if (mHead == NULL) {
            return false;
        }
        mHead->next = oldHead;
        ++mSize;
        return true;
    }

    const char* front() const {
        return &mHead->name[0];
    }

    void pop_front() {
        DirEntry* popped = mHead;
        if (popped != NULL) {
            mHead = popped->next;
            --mSize;
            delete popped;
        }
    }

    size_t size() const {
        return mSize;
    }

private:
    size_t mSize;
    DirEntry* mHead;

    // Disallow copy and assignment.
    DirEntries(const DirEntries&);
    void operator=(const DirEntries&);
};

// Reads the directory referred to by 'pathBytes', adding each directory entry
// to 'entries'.
static bool readDirectory(const char *path, DirEntries& entries) {
    if(DEBUG)
        ALOGD("SecureFileService::readDirectory(): path = %s", path);

    if (path == NULL) {
        return false;
    }

    ScopedReaddir dir(path);
    if (dir.isBad()) {
        return false;
    }
    const char* filename;
    while ((filename = dir.next()) != NULL) {
        if (strcmp(filename, ".") != 0 && strcmp(filename, "..") != 0) {
            if (!entries.push_front(filename)) {
                return false;
            }
        }
    }
    return true;
}

int SecureFileService::list(const char *path, SecureItemName *itemArray, int count) {
    if(DEBUG)
        ALOGD("SecureFileService::list(): path = %s", path);

    // Read the directory entries into an intermediate form.
    DirEntries files;
    if (!readDirectory(path, files)) {
        return -1;
    }

    int size = count < files.size() ? count : files.size();
    SecureItemName *p = itemArray;
    for (int i = 0; i < size; files.pop_front(), ++i, p++) {
        strncpy(*p, files.front(), SECUREFILE_FILE_NAME_LEN_MAX - 1);
        (*p)[SECUREFILE_FILE_NAME_LEN_MAX-1] = '\0';
    }
    return size;
}

int SecureFileService::getCount(const char *path) {
    if(DEBUG)
        ALOGD("SecureFileService::getCount(): path = %s", path);

    // Read the directory entries into an intermediate form.
    DirEntries files;
    if (!readDirectory(path, files)) {
        return -1;
    }
    return files.size();
}

bool SecureFileService::mkDir(const char *path) {
    if(DEBUG)
        ALOGD("SecureFileService::mkDir(): path = %s", path);

    if (path == NULL) {
        return false;
    }

    // On Android, we don't want default permissions to allow global access.
    return (mkdir(path, S_IRWXU) == 0);
}

bool SecureFileService::createFile(const char *path) {
    if(DEBUG)
        ALOGD("SecureFileService::createNewFile(): path = %s", path);

    if (path == NULL) {
        return false;
    }

    // On Android, we don't want default permissions to allow global access.
    ScopedFd fd(open(path, O_CREAT | O_EXCL, 0600));
    if (fd.get() != -1) {
        // We created a new file. Success!
        return true;
    }
    if (errno == EEXIST) {
        // The file already exists.
        return false;
    }
    return false; // Ignored by Java; keeps the C++ compiler happy.
}

bool SecureFileService::renameTo(const char * oldPath, const char * newPath) {
    if(DEBUG)
        ALOGD("SecureFileService::renameTo(): oldPath = %s newPath = %s", oldPath, newPath);

    return (rename(oldPath, newPath) == 0);
}

bool SecureFileService::writeToFile(const char * srcPath,const char * desPath,bool append){
    if(DEBUG)
        ALOGD("SecureFileService::writeToFile(): srcPath = %s desPath = %s", srcPath, desPath);

    if(!doAccess(srcPath,F_OK | R_OK) || !doAccess(desPath,F_OK | W_OK)){
        ALOGE("file access error");
        return false;
    }

    int MAX_NUM = 2048;
    FILE *srcFp;
    if((srcFp = fopen(srcPath, "r")) == NULL){
        ALOGE("cannot open file %s to read",srcPath);
        return false;
    }
    FILE *desFp = NULL;
    if(append){
        desFp = fopen(desPath, "a");
    }else{
        desFp = fopen(desPath, "w");
    }
    if(desFp == NULL){
        ALOGE("cannot open file %s to write",desPath);
        fclose(srcFp);
        return false;
    }

    while(1){
        int8_t *data = (int8_t *)malloc(MAX_NUM * sizeof(int8_t));
        int num;
        num = fread(data, sizeof(int8_t), MAX_NUM, srcFp);
        if(num <= 0){
            free(data);
            break;
        }
        fwrite(data, sizeof(int8_t), num, desFp);
        free(data);
    }
    fclose(srcFp);
    fflush(desFp);
    fsync(fileno(desFp));
    fclose(desFp);
    return true;
}

bool SecureFileService::writeInData(int8_t * data,int count,const char * desPath,bool append){
    if(DEBUG)
        ALOGD("SecureFileService::writeInData(): path = %s", desPath);

    if(!doAccess(desPath,F_OK | W_OK)){
        ALOGE("file access error");
        return false;
    }

    int MAX_NUM = 2048;
    FILE *desFp;
    if(append){
        desFp = fopen(desPath, "a");
    }else{
        desFp = fopen(desPath, "w");
    }
    if(desFp == NULL){
        ALOGE("cannot open file %s to write",desPath);
        return false;
    }
    int remainder = count;
    int num = 1;
    int8_t *p = data;
    if(count <= 0){
        fclose(desFp);
        return false;
    }
    while(remainder > 0 && num > 0){
        if(remainder >= MAX_NUM){
            num = fwrite(p, sizeof(int8_t), MAX_NUM, desFp);
            remainder = remainder - num;
            p = p + num;
        }
        else{
            num = fwrite(p, sizeof(int8_t), remainder, desFp);
            remainder = remainder - num;
            p = p + num;
        }
    }
    fflush(desFp);
  fsync(fileno(desFp));
  fclose(desFp);
    return true;
}

bool SecureFileService::read(int8_t * data,int count,const char * srcPath)
{
    if(DEBUG)
        ALOGD("SecureFileService::read(): srcPath = %s  count = %d ", srcPath,count);

    if(!doAccess(srcPath,F_OK | R_OK)){
        ALOGE("file access error");
        return false;
    }

    int MAX_NUM = 2048;
    FILE *srcFp;
    if((srcFp = fopen(srcPath, "r")) == NULL){
        ALOGE("cannot open file %s to read",srcPath);
        return false;
    }

    int remainder = count;
    int num = 1;
    int8_t *p = data;
    if(count <= 0){
        fclose(srcFp);
        return false;
    }
    while(remainder > 0 && num > 0){
        if(remainder >= MAX_NUM){
            num = fread(p, sizeof(int8_t), MAX_NUM, srcFp);
            remainder = remainder - num;
            p = p + num;
        }else{
            num = fread(p, sizeof(int8_t), remainder, srcFp);
            remainder = remainder - num;
            p = p + num;
        }
    }
    fclose(srcFp);
    return true;
}

} // namespace android
