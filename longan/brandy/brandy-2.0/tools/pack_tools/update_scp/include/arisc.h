#ifndef _ARISC_H
#define _ARISC_H

#define DVFS_VF_TABLE_MAX               (16)
#define IR_NUM_KEY_SUP                  (64)            /* the number of IR code support */

#define REGULATOR_NUM                   (20)            /* the number of regulator */
#define PMU_NUM                         (4)             /* the number of pmu */

typedef unsigned int u32;
typedef unsigned short u16;

typedef enum power_dm {
	DM_CPUA = 0,		/* 0  */
	DM_CPUB,		/* 1  */
	DM_DRAM,		/* 2  */
	DM_GPU,			/* 3  */
	DM_SYS,			/* 4  */
	DM_VPU,			/* 5  */
	DM_CPUS,		/* 6  */
	DM_DRAMPLL,		/* 7  */
	DM_ADC,			/* 8  */
	DM_PL,			/* 9  */
	DM_PM,			/* 10 */
	DM_IO,			/* 11 */
	DM_CPVDD,		/* 12 */
	DM_LDOIN,		/* 13 */
	DM_PLL,			/* 14 */
	DM_LPDDR,		/* 15 */
	DM_TEST,		/* 16 */
	DM_RES1,		/* 17 */
	DM_RES2,		/* 18 */
	DM_RES3,		/* 19 */
	DM_MAX,			/* 20 */
} power_dm_e;

typedef enum VDD_BIT {
	VDD_CPUA_BIT = 0,
	VDD_CPUB_BIT,
	VCC_DRAM_BIT,
	VDD_GPU_BIT,
	VDD_SYS_BIT,
	VDD_VPU_BIT,
	VDD_CPUS_BIT,
	VDD_DRAMPLL_BIT,
	VCC_ADC_BIT,
	VCC_PL_BIT,
	VCC_PM_BIT,
	VCC_IO_BIT,
	VCC_CPVDD_BIT,
	VCC_LDOIN_BIT,
	VCC_PLL_BIT,
	VCC_LPDDR_BIT,
	VDD_TEST_BIT,
	VDD_RES1_BIT,
	VDD_RES2_BIT,
	VDD_RES3_BIT,
	VCC_MAX_INDEX,
} VDD_BIT_e;

typedef struct bitmap_name_mapping {
	unsigned int mask_bit;
	char id_name[20];
} bitmap_name_mapping_t;

typedef struct ir_code {
	u32 key_code;
	u32 addr_code;
} ir_code_t;

typedef struct ir_key {
	u32 num;
	ir_code_t ir_code_depot[IR_NUM_KEY_SUP];
} ir_key_t;

typedef struct dram_para_24 {
	/*normal configuration*/
	unsigned int        dram_clk;
	unsigned int        dram_type;      /*dram_type         DDR2: 2             DDR3: 3     LPDDR2: 6   LPDDR3: 7   DDR3L: 31*/
	/*unsigned int        lpddr2_type;  //LPDDR2 type       S4:0    S2:1    NVM:2*/
	unsigned int        dram_zq;        /*do not need*/
	unsigned int        dram_odt_en;

	/*control configuration*/
	unsigned int        dram_para1;
	unsigned int        dram_para2;

	/*timing configuration*/
	unsigned int        dram_mr0;
	unsigned int        dram_mr1;
	unsigned int        dram_mr2;
	unsigned int        dram_mr3;
	unsigned int        dram_tpr0;  /*DRAMTMG0*/
	unsigned int        dram_tpr1;  /*DRAMTMG1*/
	unsigned int        dram_tpr2;  /*DRAMTMG2*/
	unsigned int        dram_tpr3;  /*DRAMTMG3*/
	unsigned int        dram_tpr4;  /*DRAMTMG4*/
	unsigned int        dram_tpr5;  /*DRAMTMG5*/
	unsigned int        dram_tpr6;  /*DRAMTMG8*/

	/*reserved for future use*/
	unsigned int        dram_tpr7;
	unsigned int        dram_tpr8;
	unsigned int        dram_tpr9;
	unsigned int        dram_tpr10;
	unsigned int        dram_tpr11;
	unsigned int        dram_tpr12;
	unsigned int        dram_tpr13;
} dram_para_24_t;


typedef struct dram_para_32 {
	/*normal configuration*/
	unsigned int        dram_clk;
	unsigned int        dram_type;      /*dram_type         DDR2: 2             DDR3: 3     LPDDR2: 6   LPDDR3: 7   DDR3L: 31*/
	/*unsigned int        lpddr2_type;*/  /*LPDDR2 type       S4:0    S2:1    NVM:2*/
	unsigned int        dram_zq;        /*do not need*/
	unsigned int        dram_odt_en;

	/*control configuration*/
	unsigned int        dram_para1;
	unsigned int        dram_para2;

	/*timing configuration*/
	unsigned int        dram_mr0;
	unsigned int        dram_mr1;
	unsigned int        dram_mr2;
	unsigned int        dram_mr3;
	unsigned int        dram_mr4;
	unsigned int        dram_mr5;
	unsigned int        dram_mr6;
	unsigned int        dram_tpr0;  /*DRAMTMG0*/
	unsigned int        dram_tpr1;  /*DRAMTMG1*/
	unsigned int        dram_tpr2;  /*DRAMTMG2*/
	unsigned int        dram_tpr3;  /*DRAMTMG3*/
	unsigned int        dram_tpr4;  /*DRAMTMG4*/
	unsigned int        dram_tpr5;  /*DRAMTMG5*/
	unsigned int        dram_tpr6;  /*DRAMTMG8*/

	/*reserved for future use*/
	unsigned int        dram_tpr7;
	unsigned int        dram_tpr8;
	unsigned int        dram_tpr9;
	unsigned int        dram_tpr10;
	unsigned int        dram_tpr11;
	unsigned int        dram_tpr12;
	unsigned int        dram_tpr13;
	unsigned int        dram_tpr14;
	unsigned int        dram_tpr15;
	unsigned int        dram_tpr16;
	unsigned int        dram_tpr17;
	unsigned int        dram_tpr18;
} dram_para_32_t;

typedef struct freq_voltage {
	u32 freq;       /*cpu frequency*/
	u32 voltage;    /*voltage for the frequency*/
	u32 axi_div;    /*the divide ratio of AXI bus*/
} freq_voltage_t;

typedef struct box_start_os_cfg {
	u32 used;
	u32 start_type;
	u32 irkey_used;
	u32 pmukey_used;
	u32 pmukey_num;
	u32 led_power;
	u32 led_state;
} box_start_os_cfg_t;


typedef struct arisc_para_old {
	u32 message_pool_phys;
	u32 message_pool_size;
	u32 standby_base;
	u32 standby_size;
	u32 suart_status;
	u32 pmu_bat_shutdown_ltf;
	u32 pmu_bat_shutdown_htf;
	u32 pmu_pwroff_vol;
	u32 power_mode;
	u32 power_start;
	u32 powchk_used;
	u32 power_reg;
	u32 system_power;
	struct ir_key ir_key;
	struct dram_para_24 dram_para;
	struct freq_voltage vf[DVFS_VF_TABLE_MAX];
	u32 power_regu_tree[DM_MAX];
	struct box_start_os_cfg start_os;
} arisc_para_old_t;

typedef struct arisc_para_new {
	u32 para_info;
	u32 para_magic;
	struct dram_para_32 dram_para;
	u32 message_pool_phys;
	u32 message_pool_size;
	u32 standby_base;
	u32 standby_size;
	u32 suart_status;
	u32 pmu_bat_shutdown_ltf;
	u32 pmu_bat_shutdown_htf;
	u32 pmu_pwroff_vol;
	u32 power_mode;
	u32 power_start;
	u32 powchk_used;
	u32 power_reg;
	u32 system_power;
	struct freq_voltage vf[DVFS_VF_TABLE_MAX];
	u32 power_regu_tree[DM_MAX];
	struct box_start_os_cfg start_os;
	struct ir_key ir_key;
} arisc_para_new_t;

typedef struct arisc_para_coco {
	u32 para_info;
	u32 para_magic;
	struct dram_para_32 dram_para;
	u32 message_pool_phys;
	u32 message_pool_size;
	u32 standby_base;
	u32 standby_size;
	u32 suart_status;
	u32 pmu_bat_shutdown_ltf;
	u32 pmu_bat_shutdown_htf;
	u32 pmu_pwroff_vol;
	u32 power_mode;
	u32 power_start;
	u32 powchk_used;
	u32 power_reg;
	u32 system_power;
	u32 vdd_cpua;
	u32 vdd_cpub;
	u32 vdd_sys;
	u32 vcc_pll;
	struct box_start_os_cfg start_os;
	struct ir_key ir_key;
	u32 resvert[65];
} arisc_para_coco_t;

typedef struct basic_para {
	u32 message_pool_phys;
	u32 message_pool_size;
	u32 standby_base;
	u32 standby_size;
	u32 suart_status;
	u32 pmu_bat_shutdown_ltf;
	u32 pmu_bat_shutdown_htf;
	u32 pmu_pwroff_vol;
	u32 power_mode;
	u32 power_start;
	u32 powchk_used;
	u32 power_reg;
	u32 system_power;
} basic_para_t;

typedef struct arisc_dts_para {
	struct basic_para basic_para;
	struct freq_voltage vf[DVFS_VF_TABLE_MAX];
	struct box_start_os_cfg start_os;
	struct ir_key ir_key;
	struct dram_para_32 dram_para_32;
	struct dram_para_24 dram_para_24;
	u32 power_regu_tree[DM_MAX];
	u32 vdd_cpua;
	u32 vdd_cpub;
	u32 vdd_sys;
	u32 vcc_pll;
} arisc_dts_para_t;


#endif /* _ARISC_H */
