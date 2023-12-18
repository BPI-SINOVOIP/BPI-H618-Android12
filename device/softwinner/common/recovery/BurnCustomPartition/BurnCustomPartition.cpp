#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <android-base/stringprintf.h>
#include <android-base/logging.h>

#include "../Utils.h"
#include "BurnCustomPartition.h"

#define DEVNODE_PATH_PARTITION   "/dev/block/by-name/"

int burnPartition(BufferExtractCookie *cookie, std::uint32_t targetSlot, std::string partition) {
  std::string partitionTmp;
  int fd;
  switch (targetSlot) {
    case 0: partitionTmp = DEVNODE_PATH_PARTITION + partition + "_a";break;
    case 1: partitionTmp = DEVNODE_PATH_PARTITION + partition + "_b";break;
    default: partitionTmp = DEVNODE_PATH_PARTITION + partition + "_other";break;
  }

  fd = open((char*)partitionTmp.c_str() , O_RDWR);
  LOG(INFO) << "partitionTmp: " << partitionTmp;
	if (fd == -1) {
		LOG(ERROR) << "burnPartition: open device node failed ! errno is " << errno << " : " << strerror(errno);
    // if can not find AB partition, try Non AB update
    partitionTmp = DEVNODE_PATH_PARTITION + partition;
    LOG(ERROR) << "partitionTmp: " << partitionTmp;
    fd = open((char*)partitionTmp.c_str(), O_RDWR);
    if (fd == -1) {
      LOG(ERROR) << "burnPartition: open device node failed ! errno is " << errno << " : " << strerror(errno);
      return -1;
    }
	}

  int ret = write(fd, cookie->buffer, cookie->len);
  fsync(fd);

	if (ret > 0) {
		LOG(INFO) << "burnPartition: burnPartition succeed!";
	} else {
		LOG(ERROR) << "burnPartition: open device node failed ! errno is " << errno << " : " << strerror(errno);
	}

	close(fd);
	return ret;
}
