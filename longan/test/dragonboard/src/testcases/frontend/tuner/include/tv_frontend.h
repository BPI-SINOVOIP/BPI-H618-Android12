#ifndef TV_FRONTEND_H
#define TV_FRONTEND_H

#ifdef __cpluscplus
extern "C"
{
#endif

#include "common_define.h"
#include "fe_log.h"

#define IFOUT_FREQ_5MHZ  (5000000)

typedef enum SUPPORT_TUNER {
    ATBM253 = 1,
    R842,
    SI2151,
} SUPPORT_TUNER_E;

typedef enum SUPPORT_STANDARD {
    /* DTV - Terrestrial & mobile TV */
    MODE_DTMB = 1,
    MODE_DVBT,
    MODE_DVBT2,
    MODE_ATSC,
    MODE_ISDBT,

    /* DTV - Calbe TV */
    MODE_DVBC,
    MODE_DVBC2,
    MODE_ISDBC,
    MODE_J83B,//US Cable

    /* DTV - Satellite TV */
    MODE_DVBS,
    MODE_DVBS2,
    MODE_DVBS2X,

    /* ATV - Analog TV */
    MODE_SECAM_L,
    MODE_SECAM_L1,
    MODE_PAL_I,
    MODE_PAL_M,
    MODE_PAL_B,
    MODE_PAL_DK,
    MODE_PAL_GH,
    MODE_NTSC_MN,

    MODE_MAX
} SUPPORT_STANDARD_E;

typedef enum SUPPORT_BANDWIDTH {
    TV_MODE_BW_6MHZ = 1,
    TV_MODE_BW_7MHZ,
    TV_MODE_BW_8MHZ,
} SUPPORT_BANDWIDTH_E;

typedef enum SUPPORT_SPECTRUM {
    NORMAL_SPECTRUM = 1,
    INVERTED_SPECTRUM
} SUPPORT_SPECTRUM_E;

typedef enum SUPPORT_WORKSTATUS {
    DEVICE_DETECT = 1,
    IFFREQ_GET,
    LOCKSTATUS_GET,
    RSSI_GET,
} SUPPORT_WORKSTATUS_E;

#define TV_BANDWIDTH(x)                 \
    ((x == TV_MODE_BW_6MHZ)  ? "6 MHz"  \
    : (x == TV_MODE_BW_7MHZ) ? "7 MHz"  \
    : (x == TV_MODE_BW_8MHZ) ? "8 MHz"  \
    : "(Invalid)")

#define TV_STANDARD(x)                 \
    ((x == MODE_DTMB)      ? "DTMB"  \
    : (x == MODE_DVBT)     ? "DVBT"  \
    : (x == MODE_DVBT2)    ? "DVBT2"  \
    : (x == MODE_ATSC)     ? "ATSC"  \
    : (x == MODE_ISDBT)    ? "ISDBT"  \
    : (x == MODE_DVBC)     ? "DVBC"  \
    : (x == MODE_DVBC2)    ? "DVBC2"  \
    : (x == MODE_ISDBC)    ? "ISDBC"  \
    : (x == MODE_J83B)     ? "J83B"  \
    : (x == MODE_DVBS)     ? "DVBS"  \
    : (x == MODE_DVBS2)    ? "DVBS2"  \
    : (x == MODE_DVBS2X)   ? "DVBS2X"  \
    : (x == MODE_SECAM_L)  ? "SECAM_L"  \
    : (x == MODE_SECAM_L1) ? "SECAM_L1"  \
    : (x == MODE_PAL_I)    ? "PAL_I"  \
    : (x == MODE_PAL_M)    ? "PAL_M"  \
    : (x == MODE_PAL_B)    ? "PAL_B"  \
    : (x == MODE_PAL_DK)   ? "PAL_DK"  \
    : (x == MODE_PAL_GH)   ? "PAL_GH"  \
    : (x == MODE_NTSC_MN)  ? "NTSC_MN"  \
    : "(Invalid)")

typedef struct FrontendOpsS {
    int (*init)(void);
    int (*set_parament)(void *tuner_config);
    int (*set_frequency)(void);
    int (*get_workstatus)(SUPPORT_WORKSTATUS_E index, void *argv);
    int (*reset)(void);
    int (*standby)(void);
    int (*wakeup)(void);
} FrontendOpsS;

typedef struct TV_FrontendS {
    struct FrontendOpsS *ops;
    SUPPORT_WORKSTATUS_E index;
} TV_FrontendS;

/*
 * only one device, we need singleton instance
 * and shold only has one owner
 */
TV_FrontendS *Frontend_GetInstance(enum SUPPORT_TUNER type);

static inline int Frontend_Init(TV_FrontendS *tuner)
{
    return tuner->ops->init();
}

static inline int Frontend_SetParament(TV_FrontendS *tuner, void *tuner_config)
{
    return tuner->ops->set_parament(tuner_config);
}

static inline int Frontend_SetFreqHz(TV_FrontendS *tuner)
{
    return tuner->ops->set_frequency();
}

static inline int Frontend_GetWorkstatus(TV_FrontendS *tuner, void *argv)
{
    return tuner->ops->get_workstatus(tuner->index, argv);
}

static inline int Frontend_Reset(TV_FrontendS *tuner)
{
    return tuner->ops->reset();
}

static inline int Frontend_standby(TV_FrontendS *tuner)
{
    return tuner->ops->standby();
}

static inline int Frontend_wakeup(TV_FrontendS *tuner)
{
    return tuner->ops->wakeup();
}

#ifdef __cpluscplus
}
#endif

#endif