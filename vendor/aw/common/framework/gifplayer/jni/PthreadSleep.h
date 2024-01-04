//
// Created by huangweibin on 2021/4/26.
//

#ifndef GIFTEST_PTHREADSLEEP_H
#define GIFTEST_PTHREADSLEEP_H

#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>

class PthreadSleep {
public:
    void msleep(unsigned int ms);

    void reset();

    void interrupt();

private:
    pthread_mutex_t sleep_mutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_cond_t sleep_cond = PTHREAD_COND_INITIALIZER;
    bool is_interrupt = false;
};

#endif //GIFTEST_PTHREADSLEEP_H
