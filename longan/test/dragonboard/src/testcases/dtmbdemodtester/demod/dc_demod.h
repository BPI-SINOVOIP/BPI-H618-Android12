#ifndef DC_DEMOD_H
#define DC_DEMOD_H
#include <stdint.h>
#include <dc_log.h>
#include <pthread.h>

#include "dc_demod_common.h"

typedef int TunerHandle;

typedef void (*TunerCallback)(uint32_t event,uint32_t param1,uint32_t param2);

typedef struct DemodWorkStatusS DemodWorkStatusT;
typedef struct DC_DemodS DC_DemodT;

enum DemodCtlCmdE
{
    DEMOD_CMD_UNKNOW = -1,
};

enum DC_DvbStandardE
{
    DC_UNKNOW = 0,
    DC_DTMB,
};

enum DemodStateE
{
    DEMOD_STATE_NOTREADY, /* create object but not set work paramter */
    DEMOD_STATE_IDEAL,      /* when disable success */
    DEMOD_STATE_WORKING,  /* when enable success */
    DEMOD_STATE_ERROR,      /* something error */
};

enum TunerProType {
    TUNER_TYPE_MTV808,
    TUNER_TYPE_RDATV,
    TUNER_TYPE_END
};

enum DemodType {
    DEMOD_TYPE_HDIC2312A,
    DEMOD_TYPE_HDIC2311,
    DEMOD_TYPE_END,
};

//Tuner event type
#define TUNER_LOCK_FAIL 0x00
#define TUNER_LOCK_OK 0x01

//Tuner error index
#define TUNER_ERROR_INIT_FAIL 0x00
#define TUNER_ERROR_FEQ_FAIL 0x01

typedef struct {
    unsigned int freq;
    unsigned int quanlity;
    unsigned int strength;
    /** fixme,other info of tuner need to be included **/
}TunerInfo;

typedef struct {
    /** tuner类型，目前只有DTMB **/
    DvbStandardType stdType;
    /** tuner索引，只有一个tuner情况下为0 **/
    int index;
    /** tuner的厂家型号**/
    enum TunerProType proType;
    /** demod的厂家型号**/
    enum DemodType dmType;
}TunerInitParam;

struct DemodWorkStatusS {
    enum DemodStateE state;
    int32_t is_lock;
    uint32_t freq;
    uint8_t quality;
    uint8_t strength;

    /* more detail */
    double snrmc;
    double snrsc;
    double snr;
    double ldpc_ber;
    double ber;
    uint32_t field_strength;
};

struct DC_DemodS {
    TunerHandle handle;
    DvbStandardType type;
    TunerConnectParam *connectParam;
    pthread_t mThreadId;
    TunerCallback callback;
    uint8_t quality;
    uint8_t strength;
    struct DemodOpsS *ops;
};

struct DemodOpsS {
    int32_t (*init)(DC_DemodT *);
    int32_t (*set_frequency)(DC_DemodT *, uint32_t);
    int32_t (*get_workstatus)(DC_DemodT *, DemodWorkStatusT *);
    int32_t (*reset)(DC_DemodT *);
    void (*stop)(DC_DemodT *);
};

#ifdef __cplusplus
extern "C"
{
#endif

DC_DemodT *DC_DemodCreate(TunerInitParam mTunerInitParam);

#ifdef __cplusplus
}
#endif

static inline int32_t DC_DemodInit(DC_DemodT *demod)
{
    DC_CHECK_INTERFACE(demod, init);
    return demod->ops->init(demod);
}

static inline int32_t DC_DemodSetFrequency(DC_DemodT *demod, uint32_t freq)
{
    DC_CHECK_INTERFACE(demod, set_frequency);
    return demod->ops->set_frequency(demod, freq);
}

static inline int32_t DC_DemodGetWorkStatus(DC_DemodT *demod, DemodWorkStatusT *status)
{
    DC_CHECK_INTERFACE(demod, get_workstatus);
    return demod->ops->get_workstatus(demod, status);
}

static inline int32_t DC_DemodReset(DC_DemodT *demod)
{
    DC_CHECK_INTERFACE(demod, reset);
    return demod->ops->reset(demod);
}

static inline void DC_DemodStop(DC_DemodT *demod)
{
    DC_CHECK_INTERFACE(demod, stop);
    demod->ops->stop(demod);
    return ;
}

#endif

