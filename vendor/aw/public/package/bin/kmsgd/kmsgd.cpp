#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/times.h>
#include <sys/types.h>
#include <utils/Timers.h>

#include <string>

#include <android-base/stringprintf.h>
#include <android-base/logging.h>

static int logRotateSizeKBytes = 1024;
static int maxRotatedLogs = 16;
static int maxRotationCountDigits = 2;
static int outByteCount = 0;
const char* outputFileName;
static int output_fd = 0;

static int format_message(char *msg) {
    int bytesWritten = 0;
    unsigned long long time_s, time_us;
    int facpri, subsystem, pos;
    char *p, *text;
    char buffer[8193];

    // add system read time
    struct tm tmBuf;
    struct tm* ptm;
    time_t now;
    unsigned long long nsec;
    size_t len;
    nsec = systemTime(SYSTEM_TIME_REALTIME);
    now = nsec / 1000000000L;
    nsec %= 1000000000L;
    ptm = localtime_r(&now, &tmBuf);
    strftime(buffer, sizeof(buffer), &"%Y-%m-%d %H:%M:%S"[3], ptm);
    len = strlen(buffer);
    len += snprintf(buffer + len, sizeof(buffer) - len, ".%06llu", nsec / 1000);
    bytesWritten += write(output_fd, buffer, strlen(buffer));

    if (sscanf(msg, "%i,%*u,%llu,%*[^;]; %n", &facpri, &time_us, &pos) != 2)
        return 0;

    time_s = time_us/1000000;
    time_us %= 1000000;

    // Drop extras after end of message text.
    if ((p = strchr(text = msg+pos, '\n'))) *p = 0;

    // Is there a subsystem? (The ": " is just a convention.)
    p = strstr(text, ": ");
    subsystem = p ? (p-text) : 0;

    // Format the time.
    sprintf(buffer, "[%5llu.%06llu] ", time_s, time_us);
    bytesWritten += write(output_fd, buffer, strlen(buffer));

    // Errors (or worse) are shown in red, subsystems are shown in yellow.
    if (subsystem) {
        sprintf(buffer, "%.*s", subsystem, text);
        bytesWritten += write(output_fd, buffer, strlen(buffer));
        text += subsystem;
    }
    sprintf(buffer, "%s\n", text);
    bytesWritten += write(output_fd, buffer, strlen(buffer));
    return bytesWritten;
}

static int openLogFile(const char* pathname) {
    int ret;
    if (access(pathname, F_OK)) {
        ret = creat(pathname, 0660);
        LOG(INFO) << "creat " << pathname << " " << ret << " " << errno;
        if (ret > 0) close(ret);
        ret = chmod(pathname, 0660);
        LOG(INFO) << "chmod " << pathname << " " << ret << " " << errno;
    }
    LOG(INFO) << "openLogFile append";
    return open(pathname, O_WRONLY | O_APPEND);
}

static void rotateLogs() {
    int err;

    // Can't rotate logs if we're not outputting to a file
    if (!outputFileName) return;

    for (int i = maxRotatedLogs; i > 0; i--) {
        std::string file1 = android::base::StringPrintf(
            "%s.%.*d", outputFileName, maxRotationCountDigits, i);

        std::string file0;
        if (!(i - 1)) {
            file0 = android::base::StringPrintf("%s", outputFileName);
        } else {
            file0 =
                android::base::StringPrintf("%s.%.*d", outputFileName,
                                            maxRotationCountDigits, i - 1);
        }

        if (!file0.length() || !file1.length()) {
            perror("while rotating log files");
            break;
        }

        err = rename(file0.c_str(), file1.c_str());

        if (err < 0 && errno != ENOENT) {
            perror("while rotating log files");
        }
    }

    output_fd = openLogFile(outputFileName);

    if (output_fd < 0) {
        return;
    }

    outByteCount = 0;
}

int main(int argc, char **argv) {
    char msg[8193]; // CONSOLE_EXT_LOG_MAX+1
    ssize_t len;
    int fd;
    bool clear = false;

    int opt;
    while ((opt = getopt(argc, argv, "f:n:r:c")) != -1) {
        switch (opt) {
            case 'c':
                clear = true;
                break;
            case 'f':
                outputFileName = optarg;
                break;
            case 'r':
                logRotateSizeKBytes = strtoul(optarg, NULL, 10);
                break;
            case 'n':
                maxRotatedLogs = strtoul(optarg, NULL, 10);
                break;
            case '?':
            default:
                return 1;
        }
    }

    if (clear) {
        LOG(WARNING) << "clear path " << outputFileName;
        snprintf(msg, sizeof(msg), "rm -rf %s", outputFileName);
        system(msg);
        return 0;
    }
    // Compute the maximum number of digits needed to count up to
    // maxRotatedLogs in decimal.  eg:
    // maxRotatedLogs == 30
    //   -> log10(30) == 1.477
    //   -> maxRotationCountDigits == 2
    LOG(WARNING) << "logRotateSizeKBytes is "<< logRotateSizeKBytes;
    LOG(WARNING) << "maxRotatedLogs is "<< maxRotatedLogs;
    LOG(WARNING) << "outputFileName is "<< outputFileName;
    maxRotationCountDigits =
        (maxRotatedLogs > 0)
            ? (int)(floor(log10(maxRotatedLogs) + 1))
            : 0;

    if (!outputFileName) {
        LOG(ERROR) << "no output file.";
        return -2;
    }
    output_fd = openLogFile(outputFileName);
    if (output_fd < 0) {
        LOG(ERROR) << "openLogFile failed.";
        return -3;
    }

    fd = open("/dev/kmsg", O_RDONLY);
    if (fd == -1) {
        LOG(ERROR) << "open /dev/kmsg failed.";
        return -4;
    }


    struct stat statbuf;
    if (fstat(output_fd, &statbuf) == -1) {
        LOG(ERROR) << "fstat output_fd failed.";
        return -5;
    }

    if ((size_t)statbuf.st_size > SIZE_MAX || statbuf.st_size < 0) {
        LOG(ERROR) << "invalid file size.";
        return -6;
    }

    outByteCount = statbuf.st_size;
    for (;;) {
      // why does /dev/kmesg return EPIPE instead of EAGAIN if oldest message
      // expires as we read it?
      if (-1==(len = read(fd, msg, sizeof(msg))) && errno==EPIPE) continue;
      // read() from kmsg always fails on a pre-3.5 kernel.
      if (len==-1 && errno==EINVAL) {
          LOG(ERROR) << "read kmsg failed.";
          return -7;
      }
      if (len<1) break;

      msg[len] = 0;
      outByteCount += format_message(msg);
      if (logRotateSizeKBytes > 0 && (outByteCount / 1024) >= logRotateSizeKBytes)
          rotateLogs();
    }
    close(output_fd);
    close(fd);

    return 0;
}
