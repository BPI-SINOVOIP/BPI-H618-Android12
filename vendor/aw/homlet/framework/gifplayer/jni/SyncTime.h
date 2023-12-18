//
// Created by huangweibin on 2021/4/26.
//

#ifndef GIFTEST_SYNCTIME_H
#define GIFTEST_SYNCTIME_H

#include <time.h>
#include "log.h"

class SyncTime{

public:

    void set_clock();

    unsigned int synchronize_time(int m_time);

private:
    timespec current_ts;
    timespec last_ts;
};

#endif //GIFTEST_SYNCTIME_H
