
#ifndef AWESOME_PGSEARCH_H
#define AWESOME_PGSEARCH_H
#include <ts_types.h>
typedef struct PGSctxS PGShandlerT;

enum PGS_ErrnoE
{
    PGS_ERR_OK = 0,
    PGS_ERR_TIMEOUT,
    PGS_ERR_FAULT,
};

struct PGS_ListenerS
{
    int (*onFinish)(void * /*cookie*/, int /* err */, struct ProgramInfoS * /* program_info */, int /* program_num */);
};

#ifdef __cplusplus
extern "C"
{
#endif

PGShandlerT *PGS_Instance(struct PGS_ListenerS *listener, void *cookie, uint32_t freq, DvbStandardType type);

/* disposable OBJ, should only USE one time. */
int PGS_Start(PGShandlerT *pgs);

int PGS_Stop(PGShandlerT *pgs);

int PGS_Destroy(PGShandlerT *pgs);

#ifdef __cplusplus
}
#endif

#endif
