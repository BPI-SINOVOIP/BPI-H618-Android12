#ifndef DC_DEMOD_COMMON_H
#define DC_DEMOD_COMMON_H
#include <stdint.h>

typedef enum{
    /** later extend **/
    DVB_STD_UNKNOW = 0,
    DVB_STD_DVB_C,
    DVB_STD_DTMB,
    DVB_STD_DVB_T,
    DVB_STD_ATSC,
    DVB_STD_END
}DvbStandardType;

typedef enum{
    DVB_DEFAULT_MODULATION_MODE_ = 0, /**<default modulation mode*/
    DVB_8VSB_MODE,                        /**<8VSB*/
    DVB_QAM16_MODE,                       /**<QAM16*/
    DVB_QAM32_MODE,                       /**<QAM32*/
    DVB_QAM64_MODE,                       /**<QAM64*/
    DVB_QAM128_MODE,                      /**<QAM128*/
    DVB_QAM256_MODE,                      /**<QAM256*/
} DvbModulationMode;

typedef struct{
    unsigned int symbol_rate;
    unsigned int fec_inner;
    DvbModulationMode modulation_mode; /**<modulation mode*/
}SatParam;

typedef struct{
    unsigned int symbol_rate;
    DvbModulationMode modulation_mode; /**<modulation mode*/
}CabParam;

typedef struct{
    unsigned int bandwidth;
    unsigned int hierarchy_information;
    unsigned int code_rate_HP;
    unsigned int code_rate_LP;
    unsigned int guard_interval;
    unsigned int transmission_mode;
}TerParam;

typedef struct {
    unsigned int uiFreq;
    union {
        TerParam ter; /** DVB-T, ISDB-T, DVB-T2, DTMB etc. */
        SatParam sat;
        CabParam cab;
    } connectParam;
}TunerConnectParam;

#endif

