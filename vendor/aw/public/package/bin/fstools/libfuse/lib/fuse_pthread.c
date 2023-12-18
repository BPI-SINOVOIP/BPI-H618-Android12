#include "fuse_pthread.h"
#include <signal.h>
#include <pthread.h>

int __attribute__((weak)) pthread_setcancelstate(int state, int *oldstate) {
    sigset_t   new, old;
    int ret;
    sigemptyset (&new);
    sigaddset (&new, SIG_CANCEL_SIGNAL);

    ret = pthread_sigmask(state == PTHREAD_CANCEL_ENABLE ? SIG_BLOCK : SIG_UNBLOCK, &new , &old);
    if(oldstate != NULL)
    {
            *oldstate =sigismember(&old,SIG_CANCEL_SIGNAL) == 0 ? PTHREAD_CANCEL_DISABLE : PTHREAD_CANCEL_ENABLE;
        }
    return ret;
}

int __attribute__((weak)) pthread_cancel(pthread_t thread) {
    return pthread_kill(thread, SIG_CANCEL_SIGNAL);
}
