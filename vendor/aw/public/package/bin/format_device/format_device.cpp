#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/syscall.h>
#include <sys/mount.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include <android-base/file.h>
#include <android-base/logging.h>
#include <android-base/macros.h>
#include <android-base/stringprintf.h>
#include <android-base/strings.h>

#include <cutils/fs.h>
#include <cutils/properties.h>

#define BUF_LENGTH                      (512)

int format_device(const char* devicePath, const char* label)
{
    char sector[BUF_LENGTH];
    int fd;
    pid_t child;
    int status;

    fd = open(devicePath, O_RDONLY);
    if (fd <= 0) {
        LOG(ERROR) << "open device error: " << strerror(errno);
        return 1;
    }
    memset(sector, 0, BUF_LENGTH);
    read(fd, sector, BUF_LENGTH);
    close(fd);
    if ((sector[510] == 0x55) && (sector[511] == 0xaa)) {
        LOG(INFO) << "Don't need to format " << devicePath;
        property_set("sys.format_device", label);
        return 0;
    } else {
        LOG(INFO) << "Start format " << devicePath;
        child = fork();
        if (child == 0) {
            LOG(INFO) << "fork to format " << devicePath;
            //execl("/system/bin/newfs_msdos", "/system/bin/newfs_msdos",  devicePath, NULL);
            execl("/system/bin/newfs_msdos","/system/bin/newfs_msdos","-F","16","-c","4", devicePath, NULL);
            exit(0);
        } else {
            LOG(DEBUG) << "wait for format " << devicePath;
            while (waitpid(-1, &status, 0) != child);
            LOG(INFO) << "format " << devicePath << " ok";
        }
    }
    property_set("sys.format_device", label);
    return 0;
}

int main(int nargs, char **args) {
    format_device(args[1], args[2]);
    return 0;
}
