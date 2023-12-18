#ifndef __IR_I_H__
#define __IR_I_H__

#include "include.h"
#define IR_USED  1
#if  IR_USED
/* NOTE: there is a conflit between IR and 32K clock(AC100 supply) on A83 evb,
 * so, we have to use 24M clock by config IR_24M_USED 1, the 32K clk will be
 * used for next evb version by config IR_24M_USED 0
 */
#define IR_24M_USED             (0)

/* ir register list */
#define IR_REG_BASE             (R_CIR_REG_BASE)
#define IR_CTRL_REG             (IR_REG_BASE + 0x00)    /* IR Control */
#define IR_RXCFG_REG            (IR_REG_BASE + 0x10)    /* Rx Config */
#define IR_RXDAT_REG            (IR_REG_BASE + 0x20)    /* Rx Data fifo */
#define IR_RXINTE_REG           (IR_REG_BASE + 0x2C)    /* Rx Interrupt Enable */
#define IR_RXINTS_REG           (IR_REG_BASE + 0x30)    /* Rx Interrupt Status */
#define IR_CFG_REG              (IR_REG_BASE + 0x34)    /* IR Configure */

/* Bit Definition of IR_RXINTS_REG Register */
#define IR_RXINTS_RXOF          (0x1<<0)                /* Rx FIFO Overflow */
#define IR_RXINTS_RXPE          (0x1<<1)                /* Rx Packet End */
#define IR_RXINTS_RXDA          (0x1<<4)                /* Rx FIFO Data Available */

/* Frequency of Sample Clock = 32768Hz, Cycle is 30.5us
 * Pulse of NEC Remote >560us
 */
#define ATHC_UNIT               (0)                     /* 0:sample clock,1:128 sample clock */
#define ATHC_THRE               (0)

#define IR_FIFO_SIZE            (64)
#define IR_FIFO_TRIGER          (48)
#ifdef FPGA_PLATFORM
#define IR_L1_MIN               (160)                   /* 80*42.7 = ~3.4ms, Lead1(4.5ms) > IR_L1_MIN */
#define IR_L0_MIN               (80)                    /* 40*42.7 = ~1.7ms, Lead0(4.5ms) Lead0R(2.25ms)> IR_L0_MIN */
#define IR_PMAX                 (52)                    /* 26*42.7 = ~1109us ~= 561*2, Pluse < IR_PMAX */
#define IR_DMID                 (52)                    /* 26*42.7 = ~1109us ~= 561*2, D1 > IR_DMID, D0 =< IR_DMID */
#define IR_DMAX                 (106)                   /* 53*42.7 = ~2263us ~= 561*4, D < IR_DMAX */
#else
#if IR_24M_USED
#define IR_SAMPLE_128           (0x1<<0)                /* 3MHz/128 =23437.5Hz (42.7us) */
#define IR_RXFILT_VAL           (((8)&0x3f)<<2)         /* Filter Threshold = 8*42.7 = ~341us	< 500us */
#define IR_RXIDLE_VAL           (((2)&0xff)<<8)         /* Idle Threshold = (2+1)*128*42.7 = ~16.4ms > 9ms */
#define IR_ACTIVE_T             ((0&0xff)<<16)          /* Active Threshold */
#define IR_ACTIVE_T_C           ((1&0xff)<<23)          /* Active Threshold */
#define IR_L1_MIN               (70)                    /* 80*42.7 = ~3.4ms, Lead1(4.5ms) > IR_L1_MIN */
#define IR_L0_MIN               (40)                    /* 40*42.7 = ~1.7ms, Lead0(4.5ms) Lead0R(2.25ms)> IR_L0_MIN */
#define IR_PMAX                 (26)                    /* 26*42.7 = ~1109us ~= 561*2, Pluse < IR_PMAX */
#define IR_DMID                 (26)                    /* 26*42.7 = ~1109us ~= 561*2, D1 > IR_DMID, D0 =< IR_DMID */
#define IR_DMAX                 (53)                    /* 53*42.7 = ~2263us ~= 561*4, D < IR_DMAX */
#else
#define IR_RXFILT_VAL           (11)                    /* Filter Threshold = 11*30.5 = ~335.5us < 500us */
#define IR_RXIDLE_VAL           (3)                     /* Idle Threshold = (3+1)*128*30.5 = ~16.4ms > 9ms */
#define IR_L1_MIN               (112)                   /* 112*30.5 = ~3.4ms, Lead1(4.5ms) > IR_L1_MIN */
#define IR_L0_MIN               (56)                    /* 56*30.5 = ~1.7ms, Lead0(4.5ms) Lead0R(2.25ms)> IR_L0_MIN */
#define IR_PMAX                 (36)                    /* 36*30.5 = ~1098us ~= 561*2, Pluse < IR_PMAX */
#define IR_DMID                 (30)                    /* 30*30.5 = ~915us, D1 > IR_DMID, D0 =< IR_DMID */
#define IR_DMAX                 (74)                    /* 74*30.5 = ~2263us ~= 561*4, D < IR_DMAX */
#endif
#endif

#define IR_ERROR_CODE           (0xffffffff)
#define IR_REPEAT_CODE          (0x00000000)

//#define IR_CHECK_ADDR_CODE
#define IR_ADDR_CODE            (0xff00)

/* ir modes */
#define IR_MODE_CIR             0x03
#define IR_MODE_LSMIR           0x00
#define IR_MODE_HSMIR           0x01
#define IR_MODE_FIR             0x02

#define IR_RAW_BUFFER_SIZE      (256)
typedef struct ir_raw_buffer
{
	u32 count;
	u8  data[IR_RAW_BUFFER_SIZE];
} ir_raw_buffer_t;


s32 ir_set_paras(ir_code_t *ir_code, int index);
static int ir_sysconfig_cfg(void);

#endif // IR_USED
#endif  //__IR_I_H__
