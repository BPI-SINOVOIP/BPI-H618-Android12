/*
************************************************************************************************************************
*                                                         eGON
*                                         the Embedded GO-ON Bootloader System
*
*                             Copyright(C), 2006-2008, SoftWinners Microelectronic Co., Ltd.
*											       All Rights Reserved
*
* File Name   : boot0.h
*
* Author      : Gary.Wang
*
* Version     : 1.1.0
*
* Date        : 2009.05.21
*
* Description :
*
* Others      : None at present.
*
*
* History     :
*
*  <Author>        <time>       <version>      <description>
*
* Gary.Wang      2009.05.21       1.1.0        build the file
*
************************************************************************************************************************
*/
#ifndef  __boot0_v2_h
#define  __boot0_v2_h


#include "egon_def.h"
#include "egon_i.h"

#define UBOOT_BASE                      0x4a000000

#define BOOT0_START_BLK_NUM             0
#define BLKS_FOR_BOOT0                  2
#define BOOT0_LAST_BLK_NUM              ( BOOT0_START_BLK_NUM + BLKS_FOR_BOOT0 - 1 )

#define BOOT0_SPI_NOR_START_ADDR        0         // add for spi nor. by Gary,2009-12-8 11:47:17
#define SPI_NOR_SIZE_FOR_BOOT0          SZ_64K    // add for spi nor. by Gary,2009-12-8 11:47:17

#define BOOT0_MAGIC                     "eGON.BT0"

#define BOOT0_START_PAGE_NUM            0         // add for 1618
#define BOOT0_PAGE_ADVANCE              64        // add for 1618
#define BOOT0_PAGES_MAX_COUNT           ( BLKS_FOR_BOOT0 * 256 )      // add for 1618
#define BOOT0_PAGE_SIZE                 SZ_1K     // add for 1618
#define BOOT0_PAGE_SIZE_BIT_WIDTH       10        // add for 1618

#define BOOT0_23_START_PAGE_NUM         0         // add for 1623
#define BOOT0_23_PAGE_ADVANCE           64        // add for 1623
#define BOOT0_23_PAGES_MAX_COUNT        ( BOOT0_23_PAGE_ADVANCE * 8 )      // add for 1623
//#define BOOT0_23_PAGE_SIZE              SZ_1K     // add for 1623
#define BOOT0_23_PAGE_SIZE_BIT_WIDTH    10        // add for 1623

//以下是提供给SDMMC卡使用，固定不可改变
#define BOOT0_SDMMC_START_ADDR          16


typedef struct _boot_sdcard_info_t
{
	__s32               card_ctrl_num;                //总共的卡的个数
	__s32				boot_offset;                  //指定卡启动之后，逻辑和物理分区的管理
	__s32 				card_no[4];                   //当前启动的卡号, 16-31:GPIO编号，0-15:实际卡控制器编号
	__s32 				speed_mode[4];                //卡的速度模式，0：低速，其它：高速
	__s32				line_sel[4];                  //卡的线制，0: 1线，其它，4线
	__s32				line_count[4];                //卡使用线的个数
	__s32 				sdc_2xmode[4];
	__s32 				sdc_ddrmode[4];
	__s32 				sdc_f_max[4];
	__s32				sdc_ex_dly_used[4];			//used config.fex delay
	__s32				sdc_odly_25M[4];			//25MHz clk output delay
	__s32				sdc_sdly_25M[4];			//25MHz clk sample delay
	__s32				sdc_odly_50M[4];			//50MHz clk output delay
	__s32				sdc_sdly_50M[4];			//50MHz clk sample delay
}boot_sdcard_info_t;

typedef struct {
	u8	magic[8];
	__s32	readcmd;
	__s32	read_mode;
	__s32	write_mode;
	__s32	flash_size;
	__s32	addr4b_opcodes;
	__s32	erase_size;
	__s32	delay_cycle;/*When the frequency is greater than 60MHZ configured as 1;less than 24MHZ configured as 2;greater 24MHZ and less 60HZ as 3*/
	__s32	lock_flag;
	__s32	frequency;
	unsigned int sample_delay;
	unsigned int sample_mode;
} boot_spinor_info_t;

/******************************************************************************/
/*                              file head of Boot0                            */
/******************************************************************************/
typedef struct _boot0_private_head_t
{
	__u32                       prvt_head_size;
	__u8                       debug_mode;       // turn off print if realease
	__u8                        power_mode;      	 /*0:axp , 1: dummy pmu  */
	__u8                        reserve[2];
	unsigned int                dram_para[32];          // DRAM patameters for initialising dram. Original values is arbitrary,
	__s32						uart_port;              // UART控制器编号
	normal_gpio_cfg             uart_ctrl[2];           // UART控制器(调试打印口)数据信息
	__s32                       enable_jtag;            // 1 : enable,  0 : disable
    normal_gpio_cfg	            jtag_gpio[5];           // 保存JTAG的全部GPIO信息
    normal_gpio_cfg             storage_gpio[32];       // 存储设备 GPIO信息
    char                        storage_data[512 - sizeof(normal_gpio_cfg) * 32];      // 用户保留数据信息
    //boot_nand_connect_info_t    nand_connect_info;
}boot0_private_head_t;


#pragma pack(1)
typedef struct _boot_extend_head_t {
	__u8		version[8];		/*version:1.0*/
	__u8		magic[8];		/* ="DRAM.ext" */
	__u8            select_mode;		/*0:不进行自动识别 1:gpio识别模式 2:gpadc识别模式 3:1个IO+gpadc识别模式*/
	__u8            gpadc_channel;		/*select gpadc 通道*/
	__u8            reserve[2];
	normal_gpio_cfg dram_select_gpio[4];	/*select_mode=1|3 时设置的pin*/
	unsigned int    dram_para[15][32];	/*ext dram参数*/
	__u8			reserve1[12];
} boot_extend_head_t;
#pragma pack()

enum {
	SUNXI_PHY_I2C0 = 0,

	SUNXI_PHY_I2C1,

	SUNXI_PHY_I2C2,

	SUNXI_PHY_I2C3,

	SUNXI_PHY_I2C4,

	SUNXI_PHY_I2C5,

	SUNXI_PHY_R_I2C0,

	SUNXI_PHY_R_I2C1,

	/*The new i2c bus must be added before SUNXI_PHY_I2C_BUS_MAX*/
	SUNXI_PHY_I2C_BUS_MAX,

};

typedef struct _flash_map_config {
	__u32                  boot_param;
	__u32                  uboot_start_sector;
	__u32                  uboot_bak_start_sector;
	__u8                   reserved[4];
} flash_map_config;

typedef struct _boot0_file_head_t
{
	boot_file_head_t      boot_head;
	boot0_private_head_t  prvt_head;
	char hash[16];
	normal_gpio_cfg       i2c_gpio[2];
	/* write brack to spl to limit dram size*/
	__u32                 dram_size;
	/* record resources in flash location*/
	flash_map_config      flash_map;
	__u8                  reserved[20];
	boot_extend_head_t    extd_head;
}boot0_file_head_t;






#endif     //  ifndef __boot0_h

/* end of boot0.h */
