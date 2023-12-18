#include <iostream>
#include <fstream>
#include <string.h>
#include <string>
#include <iostream>
#include <sstream>
#include <errno.h>
#include "gptcl.h"
#include <fcntl.h>
#include <unistd.h>

static int fuzz_gpt(char* partition_file) {
    BasicMBRData mbrData;
    GPTData gptData;
    GPTPart partData;
    int numParts = 0;
    stringstream res;

    gptData.JustLooking();
    gptData.LoadPartitions((string) partition_file);
    gptData.LoadMainTable();
    gptData.GetDiskGUID();
    numParts = gptData.GetNumParts();

    //Extracted from the android_dump function in sgdisk.cc, hits more code
    for (int i = 0; i < numParts; i++) {
        partData = gptData[i];
        if (partData.GetFirstLBA() > 0) {
            partData.GetType();
            partData.GetUniqueGUID();
            partData.GetDescription();;
        }
    }
    return 0;
}

#ifdef GPTFDISK_FUZZER_DEVICE
#define TMPFILE_TEMPLATE "/data/local/tmp/gptfuzzXXXXXXXX\x00"
#else
#define TMPFILE_TEMPLATE "/dev/shm/gptfuzzXXXXXXXX\x00"
#endif

size_t TMPFILE_LEN = sizeof(TMPFILE_TEMPLATE);

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    char partition_tmp_file[TMPFILE_LEN];
    int tmpfd;

    memcpy(partition_tmp_file, TMPFILE_TEMPLATE, TMPFILE_LEN);
    tmpfd = mkstemp(partition_tmp_file);
    if(tmpfd < 0)
        return -1;
    write(tmpfd, data, size);
    close(tmpfd);
    fuzz_gpt(partition_tmp_file);
    remove(partition_tmp_file);
    return 0;
}
