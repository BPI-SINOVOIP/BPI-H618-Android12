/**
 ******************************************************************************
 *
 * aic_bsp_8800d.c
 *
 * Copyright (C) RivieraWaves 2014-2019
 *
 ******************************************************************************
 */

#include <linux/list.h>
#include <linux/version.h>
#include <linux/firmware.h>
#include "aicsdio_txrxif.h"
#include "aicsdio.h"
#include "aic_bsp_driver.h"

#define RAM_FMAC_FW_ADDR            0x00120000
#define FW_RAM_ADID_BASE_ADDR       0x00161928
#define FW_RAM_ADID_BASE_ADDR_U03   0x00161928
#define FW_RAM_PATCH_BASE_ADDR      0x00100000

static u32 patch_tbl[][2] = {
};

static u32 syscfg_tbl_masked[][3] = {
	{0x40506024, 0x000000FF, 0x000000DF}, // for clk gate lp_level
};

static u32 rf_tbl_masked[][3] = {
	{0x40344058, 0x00800000, 0x00000000},// pll trx
};

static u32 aicbsp_syscfg_tbl[][2] = {
	{0x40500014, 0x00000101}, // 1)
	{0x40500018, 0x00000109}, // 2)
	{0x40500004, 0x00000010}, // 3) the order should not be changed

	// def CONFIG_PMIC_SETTING
	// U02 bootrom only
	{0x40040000, 0x00001AC8}, // 1) fix panic
	{0x40040084, 0x00011580},
	{0x40040080, 0x00000001},
	{0x40100058, 0x00000000},

	{0x50000000, 0x03220204}, // 2) pmic interface init
	{0x50019150, 0x00000002}, // 3) for 26m xtal, set div1
	{0x50017008, 0x00000000}, // 4) stop wdg
};

static const struct aicbsp_firmware fw_u02[] = {
	[AICBSP_CPMODE_WORK] = {
		.desc          = "normal work mode(sdio u02)",
		.bt_adid       = "fw_adid.bin",
		.bt_patch      = "fw_patch.bin",
		.bt_table      = "fw_patch_table.bin",
		.wl_fw         = "fmacfw.bin"
	},

	[AICBSP_CPMODE_TEST] = {
		.desc          = "rf test mode(sdio u02)",
		.bt_adid       = "fw_adid.bin",
		.bt_patch      = "fw_patch.bin",
		.bt_table      = "fw_patch_table.bin",
		.wl_fw         = "fmacfw_rf.bin"
	},
};

static const struct aicbsp_firmware fw_u03[] = {
	[AICBSP_CPMODE_WORK] = {
		.desc          = "normal work mode(sdio u03/u04)",
		.bt_adid       = "fw_adid_u03.bin",
		.bt_patch      = "fw_patch_u03.bin",
		.bt_table      = "fw_patch_table_u03.bin",
		.wl_fw         = "fmacfw.bin"
	},

	[AICBSP_CPMODE_TEST] = {
		.desc          = "rf test mode(sdio u03/u04)",
		.bt_adid       = "fw_adid_u03.bin",
		.bt_patch      = "fw_patch_u03.bin",
		.bt_table      = "fw_patch_table_u03.bin",
		.wl_fw         = "fmacfw_rf.bin"
	},
};

static int aicbt_init(struct aic_sdio_dev *sdiodev)
{
	struct aicbt_info_t btcfg = {
		.btmode        = AICBT_BTMODE_DEFAULT,
		.btport        = AICBT_BTPORT_DEFAULT,
		.uart_baud     = AICBT_UART_BAUD_DEFAULT,
		.uart_flowctrl = AICBT_UART_FC_DEFAULT,
		.lpm_enable    = AICBT_LPM_ENABLE_DEFAULT,
		.txpwr_lvl     = AICBT_TXPWR_LVL_DEFAULT,
	};

	struct aicbt_patch_table *head = aicbt_patch_table_alloc(aicbsp_firmware_list[aicbsp_info.cpmode].bt_table);
	int ret = aicbt_patch_info_unpack(head, &btcfg);

	if (head == NULL)
		return -1;

	if (ret) {
		btcfg.addr_adid  = FW_RAM_ADID_BASE_ADDR;
		btcfg.addr_patch = FW_RAM_PATCH_BASE_ADDR;
		if (aicbsp_info.chipinfo->rev != CHIP_REV_U02)
			btcfg.addr_adid = FW_RAM_ADID_BASE_ADDR_U03;
	}

	ret = rwnx_plat_bin_fw_upload_android(sdiodev, btcfg.addr_adid, aicbsp_firmware_list[aicbsp_info.cpmode].bt_adid);
	if (ret)
		goto err;

	ret = rwnx_plat_bin_fw_upload_android(sdiodev, btcfg.addr_patch, aicbsp_firmware_list[aicbsp_info.cpmode].bt_patch);
	if (ret)
		goto err;

	ret = aicbt_patch_table_load(sdiodev, &btcfg, head);
	if (ret)
		printk("aicbt_patch_table_load fail\n");

err:
	aicbt_patch_table_free(&head);
	return ret;
}

static int aicwifi_start_from_bootrom(struct aic_sdio_dev *sdiodev)
{
	int ret = 0;

	/* memory access */
	const u32 fw_addr = RAM_FMAC_FW_ADDR;
	struct dbg_start_app_cfm start_app_cfm;

	/* fw start */
	ret = rwnx_send_dbg_start_app_req(sdiodev, fw_addr, HOST_START_APP_AUTO, &start_app_cfm);
	if (ret) {
		return -1;
	}
	aicbsp_info.hwinfo_r = start_app_cfm.bootstatus & 0xFF;

	return 0;
}

static int aicwifi_sys_config(struct aic_sdio_dev *sdiodev)
{
	int ret, cnt;
	int syscfg_num = sizeof(syscfg_tbl_masked) / sizeof(u32) / 3;
	for (cnt = 0; cnt < syscfg_num; cnt++) {
		ret = rwnx_send_dbg_mem_mask_write_req(sdiodev,
			syscfg_tbl_masked[cnt][0], syscfg_tbl_masked[cnt][1], syscfg_tbl_masked[cnt][2]);
		if (ret) {
			printk("%x mask write fail: %d\n", syscfg_tbl_masked[cnt][0], ret);
			return ret;
		}
	}

	ret = rwnx_send_dbg_mem_mask_write_req(sdiodev,
				rf_tbl_masked[0][0], rf_tbl_masked[0][1], rf_tbl_masked[0][2]);
	if (ret) {
		printk("rf config %x write fail: %d\n", rf_tbl_masked[0][0], ret);
		return ret;
	}

	return 0;
}

static int aicwifi_patch_config(struct aic_sdio_dev *sdiodev)
{
	const u32 rd_patch_addr = RAM_FMAC_FW_ADDR + 0x0180;
	u32 config_base;
	uint32_t start_addr = 0x1e6000;
	u32 patch_addr = start_addr;
	u32 patch_num = sizeof(patch_tbl)/4;
	struct dbg_mem_read_cfm rd_patch_addr_cfm;
	int ret = 0;
	u16 cnt = 0;
	u32 patch_addr_reg = 0x1e5318;
	u32 patch_num_reg = 0x1e531c;

	if (aicbsp_info.cpmode == AICBSP_CPMODE_TEST) {
		patch_addr_reg = 0x1e5304;
		patch_num_reg = 0x1e5308;
	}

	ret = rwnx_send_dbg_mem_read_req(sdiodev, rd_patch_addr, &rd_patch_addr_cfm);
	if (ret) {
		printk("patch rd fail\n");
		return ret;
	}

	config_base = rd_patch_addr_cfm.memdata;

	ret = rwnx_send_dbg_mem_write_req(sdiodev, patch_addr_reg, patch_addr);
	if (ret) {
		printk("0x%x write fail\n", patch_addr_reg);
		return ret;
	}

	ret = rwnx_send_dbg_mem_write_req(sdiodev, patch_num_reg, patch_num);
	if (ret) {
		printk("0x%x write fail\n", patch_num_reg);
		return ret;
	}

	for (cnt = 0; cnt < patch_num/2; cnt += 1) {
		ret = rwnx_send_dbg_mem_write_req(sdiodev, start_addr+8*cnt, patch_tbl[cnt][0]+config_base);
		if (ret) {
			printk("%x write fail\n", start_addr+8*cnt);
			return ret;
		}

		ret = rwnx_send_dbg_mem_write_req(sdiodev, start_addr+8*cnt+4, patch_tbl[cnt][1]);
		if (ret) {
			printk("%x write fail\n", start_addr+8*cnt+4);
			return ret;
		}
	}

	return 0;
}

static int aicwifi_init(struct aic_sdio_dev *sdiodev)
{
	if (rwnx_plat_bin_fw_upload_android(sdiodev, RAM_FMAC_FW_ADDR, aicbsp_firmware_list[aicbsp_info.cpmode].wl_fw)) {
		printk("download wifi fw fail\n");
		return -1;
	}

	if (aicwifi_patch_config(sdiodev)) {
		printk("aicwifi_patch_config fail\n");
		return -1;
	}

	if (aicwifi_sys_config(sdiodev)) {
		printk("aicwifi_sys_config fail\n");
		return -1;
	}

	if (aicwifi_start_from_bootrom(sdiodev)) {
		printk("wifi start fail\n");
		return -1;
	}

	return 0;
}

static int aicbsp_system_config(struct aic_sdio_dev *sdiodev)
{
	int syscfg_num = sizeof(aicbsp_syscfg_tbl) / sizeof(u32) / 2;
	int ret, cnt;
	for (cnt = 0; cnt < syscfg_num; cnt++) {
		ret = rwnx_send_dbg_mem_write_req(sdiodev, aicbsp_syscfg_tbl[cnt][0], aicbsp_syscfg_tbl[cnt][1]);
		if (ret) {
			sdio_err("%x write fail: %d\n", aicbsp_syscfg_tbl[cnt][0], ret);
			return ret;
		}
	}
	return 0;
}

int aicbsp_8800d_fw_init(struct aic_sdio_dev *sdiodev)
{
	const u32 mem_addr = 0x40500000;
	struct dbg_mem_read_cfm rd_mem_addr_cfm;

	uint8_t binding_status;
	uint8_t dout[16];

	need_binding_verify = false;
	aicbsp_firmware_list = fw_u02;

	if (rwnx_send_dbg_mem_read_req(sdiodev, mem_addr, &rd_mem_addr_cfm))
		return -1;

	aicbsp_info.chipinfo->rev = (u8)(rd_mem_addr_cfm.memdata >> 16);
	if (aicbsp_info.chipinfo->rev != CHIP_REV_U02 &&
		aicbsp_info.chipinfo->rev != CHIP_REV_U03 &&
		aicbsp_info.chipinfo->rev != CHIP_REV_U04) {
		pr_err("aicbsp: %s, unsupport chip rev: %d\n", __func__, aicbsp_info.chipinfo->rev);
		return -1;
	}

	printk("aicbsp: %s, chip rev: %d\n", __func__, aicbsp_info.chipinfo->rev);

	if (aicbsp_info.chipinfo->rev != CHIP_REV_U02)
		aicbsp_firmware_list = fw_u03;

	if (aicbsp_system_config(sdiodev))
		return -1;

	if (aicbt_init(sdiodev))
		return -1;

	if (aicwifi_init(sdiodev))
		return -1;

	if (need_binding_verify) {
		printk("aicbsp: crypto data %02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X\n",
						binding_enc_data[0],  binding_enc_data[1],  binding_enc_data[2],  binding_enc_data[3],
						binding_enc_data[4],  binding_enc_data[5],  binding_enc_data[6],  binding_enc_data[7],
						binding_enc_data[8],  binding_enc_data[9],  binding_enc_data[10], binding_enc_data[11],
						binding_enc_data[12], binding_enc_data[13], binding_enc_data[14], binding_enc_data[15]);

		/* calculate verify data from crypto data */
		if (wcn_bind_verify_calculate_verify_data(binding_enc_data, dout)) {
			pr_err("aicbsp: %s, binding encrypt data incorrect\n", __func__);
			return -1;
		}

		printk("aicbsp: verify data %02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X\n",
						dout[0],  dout[1],  dout[2],  dout[3],
						dout[4],  dout[5],  dout[6],  dout[7],
						dout[8],  dout[9],  dout[10], dout[11],
						dout[12], dout[13], dout[14], dout[15]);

		if (rwnx_send_dbg_binding_req(sdiodev, dout, &binding_status)) {
			pr_err("aicbsp: %s, send binding request failn", __func__);
			return -1;
		}

		if (binding_status) {
			pr_err("aicbsp: %s, binding verify fail\n", __func__);
			return -1;
		}
	}

	if (aicwf_sdio_writeb(sdiodev->func[0], SDIOWIFI_WAKEUP_REG, 4)) {
		sdio_err("reg:%d write failed!\n", SDIOWIFI_WAKEUP_REG);
		return -1;
	}

	return 0;
}

