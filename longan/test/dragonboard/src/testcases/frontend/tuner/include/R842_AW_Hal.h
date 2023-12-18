#ifndef _R842_AW_HAL_H_
#define _R842_AW_HAL_H_

#include "tv_frontend.h"

#define R842_I2C_ADDRESS       (0xF6)//bit0 is R/W bit
typedef struct R842_config {
    U8 tuner_i2c_addr;
    SUPPORT_BANDWIDTH_E bandwidth;
    SUPPORT_STANDARD_E tv_standard;
    SUPPORT_SPECTRUM_E spectrum;           //no use
    unsigned long FreqHz;  //RF Freq(Hz)
    unsigned int  IFFreqHz;//Center Freq(Hz)
} R842_config;

struct FrontendOpsS *get_R842Ops(void);

#endif