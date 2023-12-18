#include <stdlib.h>
#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include "include/arisc.h"
#include "include/script.h"

#define  MAX_PATH             (260)
#define HEADER_OFFSET     (0x4000)
#define ARISC_INF      /* printf */

struct bitmap_name_mapping pwr_dm_bitmap_name_mapping[] = {
	{VDD_CPUA_BIT,    "vdd-cpua"    },
	{VDD_CPUB_BIT,    "vdd-cpub"    },
	{VCC_DRAM_BIT,    "vcc-dram"    },
	{VDD_GPU_BIT,     "vdd-gpu"     },
	{VDD_SYS_BIT,     "vdd-sys"     },
	{VDD_VPU_BIT,     "vdd-vpu"     },
	{VDD_CPUS_BIT,    "vdd-cpus"    },
	{VDD_DRAMPLL_BIT, "vdd-drampll" },
	{VCC_ADC_BIT,     "vcc-adc"     },
	{VCC_PL_BIT,      "vcc-pl"      },
	{VCC_PM_BIT,      "vcc-pm"      },
	{VCC_IO_BIT,      "vcc-io"      },
	{VCC_CPVDD_BIT,   "vcc-cpvdd"   },
	{VCC_LDOIN_BIT,   "vcc-ldoin"   },
	{VCC_PLL_BIT,     "vcc-pll"     },
	{VCC_LPDDR_BIT,   "vcc-lpddr"   },
	{VDD_TEST_BIT,    "vdd-test"    },
	{VDD_RES1_BIT,    "vdd-res1-bit"},
	{VDD_RES2_BIT,    "vdd-res2-bit"},
	{VDD_RES3_BIT,    "vdd-res3-bit"},
};

static void usage(void)
{
	printf(" update_scp scp.bin dtb.bin\n");
	printf(" update arisc para from dtb\n");

	return ;
}

int IsFullName(const char *FilePath)
{
	if (isalpha(FilePath[0]) && ':' == FilePath[1])
		return 1;
	else
		return 0;
}

void GetFullPath(char *dName, const char *sName)
{
    char Buffer[256];

	if (IsFullName(sName)) {
	    strcpy(dName, sName);
		return ;
	}

   /* Get the current working directory: */
	if (getcwd(Buffer, 256) == NULL) {
		perror("getcwd error");
		 return ;
	}
	sprintf(dName, "%s/%s", Buffer, sName);
}

char *probe_file_data(char *file_name, int *file_len)
{
	FILE *pfile;
	int   len;
	char *buffer;

	pfile = fopen(file_name, "rb");
	if (pfile == NULL) {
		printf("file %s cant be opened\n",file_name);

		return NULL;
	}
	fseek(pfile, 0, SEEK_END);
	len = ftell(pfile);

	buffer = malloc(len);
	if (buffer == NULL) {
		printf("buffer cant be malloc\n");
		fclose(pfile);

		return NULL;
	}

	memset(buffer, 0, len);

	fseek(pfile, 0, SEEK_SET);
	fread(buffer, len, 1, pfile);
	fclose(pfile);

	*file_len = len;

	return buffer;
}

int parse_arisc_space_node(const void*working_fdt,arisc_dts_para_t *arisc_para)
{
	int nodeoffset;
	uint value[4] = {0, 0, 0};

	nodeoffset = fdt_path_offset(working_fdt,"/soc/arisc_space");
	if (nodeoffset < 0)
		return -1;

	fdt_getprop_u32(working_fdt, nodeoffset, "space4", value);
	arisc_para->basic_para.message_pool_phys = value[0];
	arisc_para->basic_para.message_pool_size = value[2];

	return 0;
}


int parse_standby_space_node(const void*working_fdt,arisc_dts_para_t *arisc_para)
{
	int nodeoffset;
	uint value[4] = {0, 0, 0};

	nodeoffset = fdt_path_offset(working_fdt,"/soc/standby_space");
	if (nodeoffset < 0)
		return -1;

	fdt_getprop_u32(working_fdt, nodeoffset, "space1", value);
	arisc_para->basic_para.standby_base = value[0];
	arisc_para->basic_para.standby_size = value[2];

	return 0;
}

int parse_s_uart_node(const void*working_fdt,arisc_dts_para_t *arisc_para)
{
	int nodeoffset;

	nodeoffset = fdt_path_offset(working_fdt, "/soc/s_uart");
	if (nodeoffset < 0)
		return -1;

	arisc_para->basic_para.suart_status = fdtdec_get_is_enabled(working_fdt, nodeoffset);

	return 0;
}

int parse_pmu0_node(const void*working_fdt,arisc_dts_para_t *arisc_para)
{
	int nodeoffset;
	nodeoffset = fdt_path_offset(working_fdt, "/soc/pmu/charger");
	if (nodeoffset < 0) {
		nodeoffset = fdt_path_offset(working_fdt, "/soc/pmu1/charger");
		if (nodeoffset < 0)
			return -1;
	}

	fdt_getprop_u32(working_fdt, nodeoffset, "pmu_bat_shutdown_ltf", &arisc_para->basic_para.pmu_bat_shutdown_ltf);
	fdt_getprop_u32(working_fdt, nodeoffset, "pmu_bat_shutdown_htf", &arisc_para->basic_para.pmu_bat_shutdown_htf);
	fdt_getprop_u32(working_fdt, nodeoffset, "pmu_pwroff_vol", &arisc_para->basic_para.pmu_pwroff_vol);
	fdt_getprop_u32(working_fdt, nodeoffset, "power_start", &arisc_para->basic_para.power_start);

	return 0;
}

int parse_standby_supply_node(const void *working_fdt, arisc_dts_para_t *arisc_para)
{
	int nodeoffset;
	char dts_node_path[40];
	int i;
	int len;
	const unsigned char *regulator_name;

	/* parse power tree */
	for (i = 0; i < REGULATOR_NUM; i++) {
		sprintf(dts_node_path, "regulator0/regulator%d", (i + 1));

		nodeoffset = fdt_path_offset(working_fdt, dts_node_path);
		if (nodeoffset < 0) {
			continue;
		}

		regulator_name = fdt_getprop(working_fdt, nodeoffset, "regulator-name", &len);

		if (strstr(regulator_name, pwr_dm_bitmap_name_mapping[VDD_CPUA_BIT].id_name) != NULL)
			arisc_para->vdd_cpua |= (1 << i);
		if (strstr(regulator_name, pwr_dm_bitmap_name_mapping[VDD_CPUB_BIT].id_name) != NULL)
			arisc_para->vdd_cpub |= (1 << i);
		if (strstr(regulator_name, pwr_dm_bitmap_name_mapping[VDD_SYS_BIT].id_name) != NULL)
			arisc_para->vdd_sys |= (1 << i);
		if (strstr(regulator_name, pwr_dm_bitmap_name_mapping[VCC_PLL_BIT].id_name) != NULL)
			arisc_para->vcc_pll |= (1 << i);
	}

	printf("vdd_cpua:0x%8.8x\n", arisc_para->vdd_cpua);
	printf("vdd_cpub:0x%8.8x\n", arisc_para->vdd_cpub);
	printf("vdd_sys:0x%8.8x\n", arisc_para->vdd_sys);
	printf("vcc_pll:0x%8.8x\n", arisc_para->vcc_pll);

	return 0;
}

int parse_arisc_node(const void*working_fdt,arisc_dts_para_t *arisc_para)
{
	int nodeoffset;

	nodeoffset = fdt_path_offset(working_fdt, "/soc/arisc");
	if (nodeoffset < 0)
		return -1;

	fdt_getprop_u32(working_fdt, nodeoffset, "powchk_used", &arisc_para->basic_para.powchk_used);
	fdt_getprop_u32(working_fdt, nodeoffset, "power_reg", &arisc_para->basic_para.power_reg );
	fdt_getprop_u32(working_fdt, nodeoffset, "system_power", &arisc_para->basic_para.system_power);

	return 0;
}

int parse_s_cir_node(const void*working_fdt,arisc_dts_para_t *arisc_para)
{
	int nodeoffset;
	int i;
	int rc5_used = 0;
	char subkey[64];

	nodeoffset = fdt_path_offset(working_fdt, "/soc/s_cir");
	if (nodeoffset < 0)
		return -1;
/*
 *default to support NEC without configuring the node of "ir_protocol_used".
 */
	if (fdt_getprop_u32(working_fdt, nodeoffset, "ir_protocol_used", &rc5_used) < 0)
		rc5_used = 0;

	printf("rc5_used = %d\n",rc5_used);
	if (fdt_getprop_u32(working_fdt, nodeoffset, "ir_power_key_code", &arisc_para->ir_key.ir_code_depot[0].key_code) > 0) {
		if (fdt_getprop_u32(working_fdt, nodeoffset, "ir_addr_code", &arisc_para->ir_key.ir_code_depot[0].addr_code) > 0) {
			arisc_para->ir_key.num = 1;
			goto print_ir_paras;
		}
	}

	arisc_para->ir_key.num = 0;

	if (rc5_used == 1) {
		for (i = 0; i < IR_NUM_KEY_SUP; i++) {
			sprintf(subkey, "%s%d", "rc5_ir_power_key_code", i);
			if (fdt_getprop_u32(working_fdt, nodeoffset, subkey, &arisc_para->ir_key.ir_code_depot[i].key_code) > 0) {
				sprintf(subkey, "%s%d", "rc5_ir_addr_code", i);
				if (fdt_getprop_u32(working_fdt, nodeoffset, subkey, &arisc_para->ir_key.ir_code_depot[i].addr_code) > 0) {
					arisc_para->ir_key.num++;
				}
		}	}
	} else {
		for (i = 0; i < IR_NUM_KEY_SUP; i++) {
			sprintf(subkey, "%s%d", "ir_power_key_code", i);
			if (fdt_getprop_u32(working_fdt, nodeoffset, subkey, &arisc_para->ir_key.ir_code_depot[i].key_code) > 0) {
				sprintf(subkey, "%s%d", "ir_addr_code", i);
				if (fdt_getprop_u32(working_fdt, nodeoffset, subkey, &arisc_para->ir_key.ir_code_depot[i].addr_code) > 0) {
					arisc_para->ir_key.num++;
					printf("addr[%d] = 0x%x\n",i,arisc_para->ir_key.ir_code_depot[i].addr_code);
				}
			}
		}
	}

print_ir_paras:
	for (i = 0; i < arisc_para->ir_key.num; i++) {
		ARISC_INF(" ir_code[%u].key_code:0x%x, ir_code[%u].addr_code:0x%x\n",
			i, arisc_para->ir_key.ir_code_depot[i].key_code, i,  arisc_para->ir_key.ir_code_depot[i].addr_code);
	}

	if (rc5_used == 1)
		arisc_para->ir_key.num |= 0x10000000;

	return 0;
}

int parse_dram24_node(const void *working_fdt, arisc_dts_para_t *arisc_para)
{
	int nodeoffset;

	nodeoffset = fdt_path_offset(working_fdt, "/dram");
	if (nodeoffset < 0)
		return -1;

	fdt_getprop_u32(working_fdt, nodeoffset, "dram_clk",
			&arisc_para->dram_para_24.dram_clk);
	fdt_getprop_u32(working_fdt, nodeoffset, "dram_type",
			&arisc_para->dram_para_24.dram_type);
	fdt_getprop_u32(working_fdt, nodeoffset, "dram_zq",
			&arisc_para->dram_para_24.dram_zq);
	fdt_getprop_u32(working_fdt, nodeoffset, "dram_odt_en",
			&arisc_para->dram_para_24.dram_odt_en);
	fdt_getprop_u32(working_fdt, nodeoffset, "dram_para1",
			&arisc_para->dram_para_24.dram_para1);
	fdt_getprop_u32(working_fdt, nodeoffset, "dram_para2",
			&arisc_para->dram_para_24.dram_para2);
	fdt_getprop_u32(working_fdt, nodeoffset, "dram_mr0",
			&arisc_para->dram_para_24.dram_mr0);
	fdt_getprop_u32(working_fdt, nodeoffset, "dram_mr1",
			&arisc_para->dram_para_24.dram_mr1);
	fdt_getprop_u32(working_fdt, nodeoffset, "dram_mr2",
			&arisc_para->dram_para_24.dram_mr2);
	fdt_getprop_u32(working_fdt, nodeoffset, "dram_mr3",
			&arisc_para->dram_para_24.dram_mr3);
	fdt_getprop_u32(working_fdt, nodeoffset, "dram_tpr0",
			&arisc_para->dram_para_24.dram_tpr0);
	fdt_getprop_u32(working_fdt, nodeoffset, "dram_tpr1",
			&arisc_para->dram_para_24.dram_tpr1);
	fdt_getprop_u32(working_fdt, nodeoffset, "dram_tpr2",
			&arisc_para->dram_para_24.dram_tpr2);
	fdt_getprop_u32(working_fdt, nodeoffset, "dram_tpr3",
			&arisc_para->dram_para_24.dram_tpr3);
	fdt_getprop_u32(working_fdt, nodeoffset, "dram_tpr4",
			&arisc_para->dram_para_24.dram_tpr4);
	fdt_getprop_u32(working_fdt, nodeoffset, "dram_tpr5",
			&arisc_para->dram_para_24.dram_tpr5);
	fdt_getprop_u32(working_fdt, nodeoffset, "dram_tpr6",
			&arisc_para->dram_para_24.dram_tpr6);
	fdt_getprop_u32(working_fdt, nodeoffset, "dram_tpr7",
			&arisc_para->dram_para_24.dram_tpr7);
	fdt_getprop_u32(working_fdt, nodeoffset, "dram_tpr8",
			&arisc_para->dram_para_24.dram_tpr8);
	fdt_getprop_u32(working_fdt, nodeoffset, "dram_tpr9",
			&arisc_para->dram_para_24.dram_tpr9);
	fdt_getprop_u32(working_fdt, nodeoffset, "dram_tpr10",
			&arisc_para->dram_para_24.dram_tpr10);
	fdt_getprop_u32(working_fdt, nodeoffset, "dram_tpr11",
			&arisc_para->dram_para_24.dram_tpr11);
	fdt_getprop_u32(working_fdt, nodeoffset, "dram_tpr12",
			&arisc_para->dram_para_24.dram_tpr12);
	fdt_getprop_u32(working_fdt, nodeoffset, "dram_tpr13",
			&arisc_para->dram_para_24.dram_tpr13);

	return 0;
}
int parse_dram_node(const void*working_fdt,arisc_dts_para_t *arisc_para)
{
	int nodeoffset;

	nodeoffset = fdt_path_offset(working_fdt, "/dram");
	if (nodeoffset < 0)
		return -1;
#if 0
	fdt_getprop_u32(working_fdt, nodeoffset, "dram_clk",   		&arisc_para->dram_para->dram_clk);
	fdt_getprop_u32(working_fdt, nodeoffset,  "dram_type",  	&arisc_para->dram_para->dram_type);
	fdt_getprop_u32(working_fdt, nodeoffset,  "dram_zq",   		&arisc_para->dram_para->dram_zq);
	fdt_getprop_u32(working_fdt, nodeoffset,  "dram_odt_en", 	&arisc_para->dram_para->dram_odt_en);
	fdt_getprop_u32(working_fdt, nodeoffset,  "dram_para1",	&arisc_para->dram_para->dram_para1);
	fdt_getprop_u32(working_fdt, nodeoffset,  "dram_para2",	&arisc_para->dram_para->dram_para2);
	fdt_getprop_u32(working_fdt, nodeoffset,  "dram_mr0",  	&arisc_para->dram_para->dram_mr0);
	fdt_getprop_u32(working_fdt, nodeoffset,  "dram_mr1",  	&arisc_para->dram_para->dram_mr1);
	fdt_getprop_u32(working_fdt, nodeoffset,  "dram_mr2",  	&arisc_para->dram_para->dram_mr2);
	fdt_getprop_u32(working_fdt, nodeoffset,  "dram_mr3",  	&arisc_para->dram_para->dram_mr3);
	fdt_getprop_u32(working_fdt, nodeoffset,  "dram_tpr0",    	&arisc_para->dram_para->dram_tpr0);
	fdt_getprop_u32(working_fdt, nodeoffset,  "dram_tpr1",    	&arisc_para->dram_para->dram_tpr1);
	fdt_getprop_u32(working_fdt, nodeoffset,  "dram_tpr2",    	&arisc_para->dram_para->dram_tpr2);
	fdt_getprop_u32(working_fdt, nodeoffset,  "dram_tpr3",    	&arisc_para->dram_para->dram_tpr3);
	fdt_getprop_u32(working_fdt, nodeoffset,  "dram_tpr4",    	&arisc_para->dram_para->dram_tpr4);
	fdt_getprop_u32(working_fdt, nodeoffset,  "dram_tpr5",   	&arisc_para->dram_para->dram_tpr5);
	fdt_getprop_u32(working_fdt, nodeoffset,  "dram_tpr6",    	&arisc_para->dram_para->dram_tpr6);
	fdt_getprop_u32(working_fdt, nodeoffset,  "dram_tpr7",    	&arisc_para->dram_para->dram_tpr7);
	fdt_getprop_u32(working_fdt, nodeoffset,  "dram_tpr8",    	&arisc_para->dram_para->dram_tpr8);
	fdt_getprop_u32(working_fdt, nodeoffset,  "dram_tpr9",    	&arisc_para->dram_para->dram_tpr9);
	fdt_getprop_u32(working_fdt, nodeoffset,  "dram_tpr10",   	&arisc_para->dram_para->dram_tpr10);
	fdt_getprop_u32(working_fdt, nodeoffset,  "dram_tpr11",   	&arisc_para->dram_para->dram_tpr11);
	fdt_getprop_u32(working_fdt, nodeoffset,  "dram_tpr12",   	&arisc_para->dram_para->dram_tpr12);
	fdt_getprop_u32(working_fdt, nodeoffset,  "dram_tpr13",   	&arisc_para->dram_para->dram_tpr13);
#endif
	return 0;
}

int parse_dvfs_table_node(const void*working_fdt,arisc_dts_para_t *arisc_para)
{
	int 	nodeoffset;
	int 	i;
	uint val;
	uint vf_table_count = 0;
	uint vf_table_type = 0;
	char vf_table_main_key[64];
	char vf_table_sub_key[64];
	uint  vf_table_size = 0;

	nodeoffset = fdt_path_offset(working_fdt, "/dvfs_table");
	if (nodeoffset < 0)
		return -1;

	if (fdt_getprop_u32(working_fdt, nodeoffset, "vf_table_count", &vf_table_count) < 0) {
		ARISC_INF("%s: support only one vf_table\n", __func__);
		sprintf(vf_table_main_key, "%s", "/dvfs_table");
	}
	else
		sprintf(vf_table_main_key, "%s%d", "/vf_table", vf_table_type);

	ARISC_INF("%s: vf table type [%d=%s]\n", __func__, vf_table_type, vf_table_main_key);

	nodeoffset = fdt_path_offset(working_fdt, vf_table_main_key);

	/* parse system config v-f table information */
	fdt_getprop_u32(working_fdt, nodeoffset, "lv_count", &vf_table_size);

	for (i = 0; i < vf_table_size; i++) {
		sprintf(vf_table_sub_key, "lv%d_freq", i + 1);
		if (fdt_getprop_u32(working_fdt, nodeoffset, vf_table_sub_key, &val) > 0)
			arisc_para->vf[i].freq = val;

		ARISC_INF("%s: freq [%s-%d=%d]\n", __func__, vf_table_sub_key, i, val);
		sprintf(vf_table_sub_key, "lv%d_volt", i + 1);
		if (fdt_getprop_u32(working_fdt, nodeoffset, vf_table_sub_key, &val))
			arisc_para->vf[i].voltage = val;

		ARISC_INF("%s: volt [%s-%d=%d]\n", __func__, vf_table_sub_key, i, val);
	}

	return 0;
}

static int parse_gpio_power_key_node(arisc_dts_para_t *arisc_para)
{
	script_gpio_set_t gp_key_set;
	int gp_key_used = 0, key_index = 0, key_group = 0, trigger_mode = 0;
	char  gp_key_node_str[32];

	memset(gp_key_node_str, 0, 32);
	strcpy(gp_key_node_str, "gpio_power_key");

	if (!script_parser_fetch(gp_key_node_str, "gpio_power_key_used", (int *)&gp_key_used)) {
		if (gp_key_used == 1) {
			memset(&gp_key_set, 0, sizeof(script_gpio_set_t));
			if (!script_parser_fetch(gp_key_node_str, "key_io", (int *)&gp_key_set)) {
				if (gp_key_set.port == 12) {
					key_group  = 0;
				} else if (gp_key_set.port == 13) {
					key_group  = 1;
				}
				key_index = gp_key_set.port_num;
				if (script_parser_fetch(gp_key_node_str, "trigger_mode", (int *)&trigger_mode))
					trigger_mode = 1;
			}
			arisc_para->start_os.pmukey_used |= key_index << 11 | key_group << 10  | trigger_mode << 8;
			printf("parse_gpio_power_key is okay\n");
		} else {
			arisc_para->start_os.pmukey_used |= 0xfe; /*cpus will judge:if 0xfe, will not support gpio_power_key */
		}
	} else {
		arisc_para->start_os.pmukey_used |= 0xfe; /*cpus will judge:if 0xfe, will not support gpio_power_key */
	}
	printf("key_group = %d\tkey_index = %d\ttrigger_mode = %d\tpmukey_used = %u\n", \
		key_group, key_index, trigger_mode, arisc_para->start_os.pmukey_used);
}

static int parse_bt_wakeup_pin(arisc_dts_para_t *arisc_para)
{
	script_gpio_set_t	bt_gpio_set;
	char  bt_node_str[32];
	int bt_hostwake_enable = 0, bt_gpio_port = 0, bt_gpio_num = 0;

	memset(bt_node_str, 0, 32);
	strcpy(bt_node_str, "btlpm");
	if (script_parser_fetch(bt_node_str, "bt_hostwake_enable", (int *)&bt_hostwake_enable)) {
		/*some platform didn't have bt_hostwake_enable, default to support it*/
		bt_hostwake_enable = 1;
	}

	if (bt_hostwake_enable == 1) {
		memset(&bt_gpio_set, 0, sizeof(script_gpio_set_t));
		if (!script_parser_fetch(bt_node_str, "bt_hostwake", (int *)&bt_gpio_set)) {
			bt_gpio_port = bt_gpio_set.port - 11;
			bt_gpio_num  = bt_gpio_set.port_num;
			arisc_para->start_os.pmukey_used |= \
				((bt_gpio_port & 0xff) << 24) | ((bt_gpio_num & 0xff) << 16);
		} else {
			arisc_para->start_os.pmukey_used &= 0xffff;
			arisc_para->start_os.pmukey_used |= 0xfefe0000;
		}
	} else {
		arisc_para->start_os.pmukey_used &= 0xffff;
		arisc_para->start_os.pmukey_used |= 0xfefe0000;
	}
	printf("bt_hostwake_enable = %d\tbt_gpio_port = %d\tbt_gpio_num = %d, \
			pmukey_used = 0x%x\n", bt_hostwake_enable, bt_gpio_port, \
			bt_gpio_num, arisc_para->start_os.pmukey_used);

	return 0;
}

int parse_box_start_os0_node(const void*working_fdt,arisc_dts_para_t *arisc_para)
{
	int 	nodeoffset;

	nodeoffset = fdt_path_offset(working_fdt, "/soc/box_start_os0");
	if (nodeoffset < 0)
		return -1;

	arisc_para->start_os.used = fdtdec_get_is_enabled(working_fdt, nodeoffset);
	fdt_getprop_u32(working_fdt, nodeoffset, "start_type", &arisc_para->start_os.start_type);
	fdt_getprop_u32(working_fdt, nodeoffset, "irkey_used", &arisc_para->start_os.irkey_used);
	fdt_getprop_u32(working_fdt, nodeoffset, "pmukey_used", &arisc_para->start_os.pmukey_used);

	return 0;
}

int parse_power_mode(const void*working_fdt,arisc_dts_para_t *arisc_para)
{
	int 	nodeoffset;

	nodeoffset = fdt_path_offset(working_fdt, "/soc/target");
	if (nodeoffset < 0)
		return -1;

	fdt_getprop_u32(working_fdt, nodeoffset, "power_mode", &arisc_para->basic_para.power_mode);
}

arisc_dts_para_t *arisc_set_paras(struct arisc_dts_para *dts_, void *arisc_para, int type)
{
	struct arisc_para_old *old;
	struct arisc_para_new *new;
	struct arisc_para_coco *coco;
	struct arisc_dts_para *dts;

	dts = dts_;
	if (type == 1) {
		new = (struct arisc_para_new *)arisc_para;
		new->message_pool_phys		= dts->basic_para.message_pool_phys;
		new->message_pool_size		= dts->basic_para.message_pool_size;
		new->standby_base		= dts->basic_para.standby_base;
		new->standby_size		= dts->basic_para.standby_size;
		new->suart_status		= dts->basic_para.suart_status;
		new->pmu_bat_shutdown_ltf	= dts->basic_para.pmu_bat_shutdown_ltf;
		new->pmu_bat_shutdown_htf	= dts->basic_para.pmu_bat_shutdown_htf;
		new->pmu_pwroff_vol		= dts->basic_para.pmu_pwroff_vol;
		new->power_mode			= dts->basic_para.power_mode;
		new->power_start		= dts->basic_para.power_start;
		new->powchk_used		= dts->basic_para.powchk_used;
		new->power_reg			= dts->basic_para.power_reg;
		new->system_power		= dts->basic_para.system_power;

		memcpy(&new->dram_para, &dts->dram_para_32, sizeof(struct dram_para_32));
		memcpy(new->power_regu_tree, dts->power_regu_tree, sizeof(u32) * DM_MAX);
		memcpy(&new->start_os, &dts->start_os, sizeof(struct box_start_os_cfg));
		memcpy(&new->ir_key, &dts->ir_key, sizeof(struct ir_key));
		memcpy(new->vf, dts->vf, sizeof(struct freq_voltage) * DVFS_VF_TABLE_MAX);
	} else if (type == 0) {
		old = (struct arisc_para_old *)arisc_para;
		old->message_pool_phys		= dts->basic_para.message_pool_phys;
		old->message_pool_size		= dts->basic_para.message_pool_size;
		old->standby_base		= dts->basic_para.standby_base;
		old->standby_size		= dts->basic_para.standby_size;
		old->suart_status		= dts->basic_para.suart_status;
		old->pmu_bat_shutdown_ltf	= dts->basic_para.pmu_bat_shutdown_ltf;
		old->pmu_bat_shutdown_htf	= dts->basic_para.pmu_bat_shutdown_htf;
		old->pmu_pwroff_vol		= dts->basic_para.pmu_pwroff_vol;
		old->power_mode			= dts->basic_para.power_mode;
		old->power_start		= dts->basic_para.power_start;
		old->powchk_used		= dts->basic_para.powchk_used;
		old->power_reg			= dts->basic_para.power_reg;
		old->system_power		= dts->basic_para.system_power;

		memcpy(&old->dram_para, &dts->dram_para_24, sizeof(struct dram_para_24));
		memcpy(old->power_regu_tree, dts->power_regu_tree, sizeof(u32) * DM_MAX);
		memcpy(&old->start_os, &dts->start_os, sizeof(struct box_start_os_cfg));
		memcpy(&old->ir_key, &dts->ir_key, sizeof(struct ir_key));
		memcpy(old->vf, dts->vf, sizeof(struct freq_voltage) * DVFS_VF_TABLE_MAX);

	} else if (type == 2) {
		coco = (struct arisc_para_coco *)arisc_para;
		coco->message_pool_phys		= dts->basic_para.message_pool_phys;
		coco->message_pool_size		= dts->basic_para.message_pool_size;
		coco->standby_base		= dts->basic_para.standby_base;
		coco->standby_size		= dts->basic_para.standby_size;
		coco->suart_status		= dts->basic_para.suart_status;
		coco->pmu_bat_shutdown_ltf	= dts->basic_para.pmu_bat_shutdown_ltf;
		coco->pmu_bat_shutdown_htf	= dts->basic_para.pmu_bat_shutdown_htf;
		coco->pmu_pwroff_vol		= dts->basic_para.pmu_pwroff_vol;
		coco->power_mode		= dts->basic_para.power_mode;
		coco->power_start		= dts->basic_para.power_start;
		coco->powchk_used		= dts->basic_para.powchk_used;
		coco->power_reg			= dts->basic_para.power_reg;
		coco->system_power		= dts->basic_para.system_power;
		coco->vdd_cpua			= dts->vdd_cpua;
		coco->vdd_cpub			= dts->vdd_cpub;
		coco->vdd_sys			= dts->vdd_sys ;
		coco->vcc_pll			= dts->vcc_pll;

		memcpy(&coco->dram_para, &dts->dram_para_24, sizeof(struct dram_para_24));
		memcpy(&coco->start_os, &dts->start_os, sizeof(struct box_start_os_cfg));
		memcpy(&coco->ir_key, &dts->ir_key, sizeof(struct ir_key));

	}
	return dts;
	return dts;
}


int magic_invoke(arisc_para_new_t *arisc_para)
{
	u32 magic = arisc_para->para_magic;
	u32 info = arisc_para->para_info;
	u32 len = 0;
	char  magic_char[4] ;
	u32 result = 0;

	magic_char[0] = magic & 0xff;
	magic_char[1] = (magic >> 8) & 0xff;
	magic_char[2] = (magic >> 16) & 0xff;
	magic_char[3] = (magic >> 24) & 0xff;

	printf("arisc_para  version : %x\n", info);
	printf("arisc_para  magic : %s\n", magic_char);

	result = memcmp(magic_char, "CPUs", 4);

	if (result == 0) {
		if ((info & 0xf) == 2) {
			len = sizeof(struct arisc_para_new);
			arisc_para->para_info |= (len << 16);
			printf("arisc_para size : %u\n", len);
			return 0;
		} else if ((info & 0xf) == 3) {
			printf("no need to copy paras\n");
			return 1;
		} else if ((info & 0xf) == 4) {
			len = sizeof(struct arisc_para_new);
			arisc_para->para_info |= (len << 16);
			printf("arisc_para size : %d\n", len);
			return 2;
		}
	}
	else {
		printf("old arisc para type\n");
		return -1;
	}

}


int main(int argc, char *argv[])
{
	char dtbpath[MAX_PATH]="";
	char scppath[MAX_PATH]="";
	FILE *scp_file;
	char *working_fdt;
	char *working_scp;
	int  dtb_len,scp_len,i;
	char *script_buf = NULL;
	char script_file_name[MAX_PATH];
	arisc_para_new_t *arisc_para_new;
	arisc_para_old_t *arisc_para_old;
	arisc_para_coco_t *arisc_para_coco;
	arisc_dts_para_t *arisc_dts_para;
	u32 ret;

	arisc_dts_para = malloc(sizeof(struct arisc_dts_para));

	if (argc != 3) {
		printf("parameters invalid\n");
		usage();
		free(arisc_dts_para);
		return -1;
	}

	GetFullPath(dtbpath, argv[2]);
	GetFullPath(scppath, argv[1]);
	printf("dtbpath=%s\n", dtbpath);
	printf("scppath=%s\n", scppath);

	working_fdt = probe_file_data(dtbpath, &dtb_len);
	working_scp = probe_file_data(scppath, &scp_len);

	/*add_begin: to get gpio_para by script*/
	GetFullPath(script_file_name, "sys_config.bin");
	script_buf = (char *)script_file_decode(script_file_name);
	if (!script_buf) {
		printf("update_scp err: unable to get script data\n");
		free(script_buf);
		free(arisc_dts_para);
		return -1;
	}
	script_parser_init(script_buf);
	/*add_end*/

	if ((working_fdt == NULL) || (working_scp == NULL)) {
		printf("file invalid\n");
		free(script_buf);
		free(arisc_dts_para);
		return -1;
	}

	scp_file = fopen(scppath, "wb");
	if (scp_file == NULL) {
		printf("file %s cant be opened\n",scppath);
		free(script_buf);
		free(arisc_dts_para);
		return -1;
	}

	parse_arisc_space_node(working_fdt,arisc_dts_para);
	parse_standby_space_node(working_fdt,arisc_dts_para);
	parse_s_uart_node(working_fdt,arisc_dts_para);
	parse_pmu0_node(working_fdt,arisc_dts_para);

	parse_standby_supply_node(working_fdt, arisc_dts_para);

	parse_arisc_node(working_fdt,arisc_dts_para);
	parse_s_cir_node(working_fdt,arisc_dts_para);
	parse_dram_node(working_fdt,arisc_dts_para);
	parse_dvfs_table_node(working_fdt,arisc_dts_para);
	parse_box_start_os0_node(working_fdt,arisc_dts_para);
	parse_power_mode(working_fdt,arisc_dts_para);
	parse_gpio_power_key_node(arisc_dts_para);
	parse_bt_wakeup_pin(arisc_dts_para);

	arisc_para_new = (arisc_para_new_t *)(working_scp+HEADER_OFFSET);

	ret = magic_invoke(arisc_para_new);
	if (ret == 0) {
		arisc_set_paras(arisc_dts_para, (void *)arisc_para_new, 1);
	} else if (ret == -1) {
		arisc_para_old = (arisc_para_old_t *)(working_scp+HEADER_OFFSET);
		parse_dram24_node(working_fdt, arisc_dts_para);
		arisc_set_paras(arisc_dts_para, (void *)arisc_para_old, 0);
	} else if (ret == 1)
		;
	else if (ret == 2) {
		arisc_para_coco = (arisc_para_coco_t *)(working_scp+HEADER_OFFSET);
		arisc_set_paras(arisc_dts_para, (void *)arisc_para_coco, 2);
	}

	free(arisc_dts_para);

	fwrite(working_scp, scp_len, 1, scp_file);
	fclose(scp_file);

	if (working_fdt)
		free(working_fdt);
	if (working_scp)
		free(working_scp);
	if (script_buf)
		free(script_buf);
	printf("update scp finish!\n");

	return 0;
}

