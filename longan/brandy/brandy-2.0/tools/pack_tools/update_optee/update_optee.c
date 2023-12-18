#include <stdlib.h>
#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include "include/optee.h"
#include "include/script.h"

#include "../update_scp/libfdt/libfdt_env.h"
#include "../update_scp/libfdt/fdt.h"
#include "../update_scp/libfdt/libfdt.h"
#include "../update_scp/libfdt/libfdt_internal.h"
#include "../include/type_def.h"

#define  MAX_PATH             (260)

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
	printf(" update_optee optee.bin dtb.bin\n");
	printf(" update optee header para from dtb\n");

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
		printf("file %s cant be opened\n", file_name);

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

int parse_pmu_dts_node(const void *working_fdt, pmu_dts_para_t *pmu_dts_para)
{
	int nodeoffset, parent_nodeoffset;
	char dts_node_path[40];
	int i;
	int len;
	const unsigned char *regulator_name, *parent_device_type;

	/* parse port and rsb/twi type */
	nodeoffset = fdt_path_offset(working_fdt, "pmu0");
	if (nodeoffset < 0) {
		return -1;
	}

	parent_nodeoffset = fdt_parent_offset(working_fdt, nodeoffset);
	if (parent_nodeoffset < 0) {
		return -1;
	}

	parent_device_type = fdt_getprop(working_fdt, parent_nodeoffset, "device_type", &len);
	if (!parent_device_type)
		return -1;
	if (strstr(parent_device_type, "twi") != NULL) {
		pmu_dts_para->pmu_para |= 0x1;			/* bit0 = 1, twi type */
		pmu_dts_para->pmu_port =
			parent_device_type[len-2] - '0';	/* twi port number */
	} else {
		pmu_dts_para->pmu_para &= (~0x1);		/* bit0 = 0, rsb type */
		pmu_dts_para->pmu_port = 0x0;			/* rsb type do not care port number */
	}
	printf("port = 0x%8.8x\npara = 0x%8.8x\n",
		pmu_dts_para->pmu_port, pmu_dts_para->pmu_para);

	/* parse pmu count, id, address */
	for (i = 0; i < PMU_NUM; i++) {
		sprintf(dts_node_path, "pmu%d", i);

		nodeoffset = fdt_path_offset(working_fdt, dts_node_path);
		if (nodeoffset < 0) {
			break;
		}

		/* parse pmu id */
		fdt_getprop_u32(working_fdt, nodeoffset, "pmu_id", &pmu_dts_para->pmu_id[i]);

		/* parse pmu address */
		fdt_getprop_u32(working_fdt, nodeoffset, "reg", &pmu_dts_para->pmu_addr[i]);

		/* parse pmu count */
		pmu_dts_para->pmu_count++;

		printf("pmu%d:\nid = 0x%8.8x\naddr = 0x%8.8x\n",
			i, pmu_dts_para->pmu_id[i], pmu_dts_para->pmu_addr[i]);
	}
	printf("pmu_count:0x%8.8x\n", pmu_dts_para->pmu_count);

	/* parse power tree */
	for (i = 0; i < REGULATOR_NUM; i++) {
		sprintf(dts_node_path, "regulator0/regulator%d", (i + 1));

		nodeoffset = fdt_path_offset(working_fdt, dts_node_path);
		if (nodeoffset < 0) {
			continue;
		}

		regulator_name = fdt_getprop(working_fdt, nodeoffset, "regulator-name", &len);

		if (strstr(regulator_name, pwr_dm_bitmap_name_mapping[VDD_CPUA_BIT].id_name) != NULL)
			pmu_dts_para->vdd_cpua |= (1 << i);
		if (strstr(regulator_name, pwr_dm_bitmap_name_mapping[VDD_CPUB_BIT].id_name) != NULL)
			pmu_dts_para->vdd_cpub |= (1 << i);
		if (strstr(regulator_name, pwr_dm_bitmap_name_mapping[VDD_SYS_BIT].id_name) != NULL)
			pmu_dts_para->vdd_sys |= (1 << i);
		if (strstr(regulator_name, pwr_dm_bitmap_name_mapping[VCC_IO_BIT].id_name) != NULL)
			pmu_dts_para->vcc_io |= (1 << i);
		if (strstr(regulator_name, pwr_dm_bitmap_name_mapping[VCC_PLL_BIT].id_name) != NULL)
			pmu_dts_para->vcc_pll |= (1 << i);
	}

	printf("vdd_cpua:0x%8.8x\n", pmu_dts_para->vdd_cpua);
	printf("vdd_cpub:0x%8.8x\n", pmu_dts_para->vdd_cpub);
	printf("vdd_sys:0x%8.8x\n", pmu_dts_para->vdd_sys);
	printf("vcc_io:0x%8.8x\n", pmu_dts_para->vcc_io);
	printf("vcc_pll:0x%8.8x\n", pmu_dts_para->vcc_pll);

	return 0;
}

pmu_dts_para_t *optee_set_paras(pmu_dts_para_t *pmu_dts_para, void *optee_para)
{
	struct spare_boot_ctrl_head *header;
	int i;

	header = (spare_boot_ctrl_head_t *)optee_para;

	header->vdd_cpua = pmu_dts_para->vdd_cpua;
	header->vdd_cpub = pmu_dts_para->vdd_cpub;
	header->vdd_sys = pmu_dts_para->vdd_sys;
	header->vcc_io = pmu_dts_para->vcc_io;
	header->vcc_pll = pmu_dts_para->vcc_pll;

	header->pmu_count = pmu_dts_para->pmu_count;
	header->pmu_port = pmu_dts_para->pmu_port;
	header->pmu_para = pmu_dts_para->pmu_para;

	for (i = 0; i < PMU_NUM; i++) {
		header->pmu_id[i] = pmu_dts_para->pmu_id[i];
		header->pmu_addr[i] = pmu_dts_para->pmu_addr[i];
	}

	return pmu_dts_para;
}

int magic_invoke(void *optee_para)
{
	struct spare_boot_ctrl_head *header;
	unsigned char *expand_magic;
	unsigned char *expand_version;
	int result = 0;
	int ret = 0;

	header = (spare_boot_ctrl_head_t *)optee_para;

	expand_magic = header->expand_magic;
	expand_version = header->expand_version;

	result = strcmp(expand_magic, "expand");

	if (result == 0) {
		if (!strcmp(expand_version, "1.0")) {
			printf("optee header expand version %s\n", expand_version);
			ret = 0;
		} else {
			printf("expand version %s mismatch\n", expand_version);
			ret = -1;
		}
	} else {
		printf("no expand optee header type\n");
		ret = -1;
	}

	return ret;
}

int main(int argc, char *argv[])
{
	char dtbpath[MAX_PATH] = "";
	char opteepath[MAX_PATH] = "";
	FILE *optee_file;
	char *working_fdt;
	char *working_optee;
	int  dtb_len, optee_len, i;
	char *script_buf = NULL;
	char script_file_name[MAX_PATH];
	pmu_dts_para_t *pmu_dts_para;

	u32 ret;

	pmu_dts_para = malloc(sizeof(struct pmu_dts_para));

	if (argc != 3) {
		printf("parameters invalid\n");
		usage();
		free(pmu_dts_para);
		return -1;
	}

	GetFullPath(dtbpath, argv[2]);
	GetFullPath(opteepath, argv[1]);
	printf("dtbpath=%s\n", dtbpath);
	printf("opteepath=%s\n", opteepath);

	working_fdt = probe_file_data(dtbpath, &dtb_len);
	working_optee = probe_file_data(opteepath, &optee_len);

	/*add_begin: to get gpio_para by script*/
	GetFullPath(script_file_name, "sys_config.bin");
	script_buf = (char *)script_file_decode(script_file_name);
	if (!script_buf) {
		printf("update_optee err: unable to get script data\n");
		free(script_buf);
		free(pmu_dts_para);
		return -1;
	}
	script_parser_init(script_buf);
	/*add_end*/

	if ((working_fdt == NULL) || (working_optee == NULL)) {
		printf("file invalid\n");
		free(script_buf);
		free(pmu_dts_para);
		return -1;
	}

	optee_file = fopen(opteepath, "wb");
	if (optee_file == NULL) {
		printf("file %s cant be opened\n", opteepath);
		free(script_buf);
		free(pmu_dts_para);
		return -1;
	}

	ret = magic_invoke((void *)working_optee);
	if (ret == 0) {
		parse_pmu_dts_node(working_fdt, pmu_dts_para);
		optee_set_paras(pmu_dts_para, (void *)working_optee);
	} else if (ret == -1) {
		;
	} else if (ret == 1) {
		;
	} else if (ret == 2) {
		;
	}

	free(pmu_dts_para);

	fwrite(working_optee, optee_len, 1, optee_file);
	fclose(optee_file);

	if (working_fdt)
		free(working_fdt);
	if (working_optee)
		free(working_optee);
	if (script_buf)
		free(script_buf);
	printf("update optee finish!\n");

	return 0;
}

