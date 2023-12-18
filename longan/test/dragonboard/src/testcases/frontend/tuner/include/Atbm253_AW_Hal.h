#ifndef ATBM253_AW_HAL_H
#define ATBM253_AW_HAL_H

#include "tv_frontend.h"

#define ATBM253_I2C_ADDRESS     (0xC0)
typedef struct Atbm253_config {
    U8 tuner_i2c_addr;
    SUPPORT_BANDWIDTH_E bandwidth;
    SUPPORT_STANDARD_E tv_standard;
    SUPPORT_SPECTRUM_E spectrum;
    unsigned long FreqHz;
    unsigned int  IfFreq_dtv;
    unsigned int  IfFreq_atv;
} Atbm253_config;

struct FrontendOpsS *get_Atbm253Ops(void);

#endif