#ifndef SI2151_AW_HAL_H
#define SI2151_AW_HAL_H

#include "tv_frontend.h"

#define SI2151_I2C_ADDRESS     (0xC0)
typedef struct Si2151_config {
    U8 tuner_i2c_addr;
    SUPPORT_BANDWIDTH_E bandwidth;
    SUPPORT_STANDARD_E tv_standard;
    SUPPORT_SPECTRUM_E spectrum;
    unsigned long FreqHz;//RF Freq(Hz)
    unsigned int  IFFreqHz;//Center Freq(Hz)
} Si2151_config;

struct FrontendOpsS *get_Si2151Ops(void);

#endif