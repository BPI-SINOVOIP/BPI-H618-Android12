#ifndef __FUSE_PTHREAD_H
#define __FUSE_PTHREAD_H

#define SIG_CANCEL_SIGNAL SIGUSR1
#define PTHREAD_CANCEL_ENABLE 1
#define PTHREAD_CANCEL_DISABLE 0

typedef long pthread_t;

int pthread_setcancelstate(int state, int *oldstate);

int pthread_cancel(pthread_t thread);

#endif
