#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>

#include "gslX680.h"
#include "GSL_A63_PERF_8001280.h"
#include "gsl1680_7inch_863.h"
#include "gsl1680e_a86.h"
#include "gsl1680e_t1_v2.h"
#include "gslX680_3676.h"
#include "gslX680_m100.h"
#include "gslX680_m7300.h"
#include "gsl1680_7inch.h"
#include "gsl1680e_p1_1024x600.h"
#include "GSL3692_F1P0B_WJ_PG.h"
#include "gslX680_708.h"
#include "gslX680_m102gg.h"
#include "gslX680_m86hd.h"
#include "gsl1680e_86OGS.h"
#include "gsl1680e_p2.h"
#include "GSL3692_F1P0D_DZ_OGS.h"
#include "gslX680_inetd71.h"
#include "gslX680_m102.h"
#include "gslX680_mq88.h"
#include "gsl1680e_86pgpcd.h"
#include "gsl1680e_t1.h"
#include "gslX680_3676_1280x800.h"
#include "gslX680_jinghong.h"
#include "gslX680_m71.h"
#include "main_1680E_m320.h"
#include "gslX680_3676_8inch.h"
#include "gslX680_3676_10inch.h"
#include "gslX680_3676_xc131.h"

#define BigtoLittle32(B) ((((uint32_t)(B) & 0xff000000) >> 24) | \
                            (((uint32_t)(B) & 0x00ff0000) >> 8) | \
                            (((uint32_t)(B) & 0x0000ff00) << 8) | \
                            (((uint32_t)(B) & 0x000000ff) << 24))

#define ARRAY_SIZE(a)     (sizeof(a) / sizeof(a[0]))

struct gslX680_fw_array {
    const char *name;
    unsigned int size;
    const struct fw_data *fw;
} gslx680_fw_grp[] = {
    {"gslX680_inetd71", ARRAY_SIZE(GSLX680_FW_INETD71), GSLX680_FW_INETD71},
    {"gslX680_708", ARRAY_SIZE(GSLX680_FW_708), GSLX680_FW_708},
    {"gsl1680_7inch", ARRAY_SIZE(GSL1680_7INCH_FW), GSL1680_7INCH_FW},
    {"gsl1680e_p2", ARRAY_SIZE(GSL1680E_FW_P2), GSL1680E_FW_P2},
    {"gsl1680e_p1_1024x600", ARRAY_SIZE(GSL1680E_FW_P1_1024X600), GSL1680E_FW_P1_1024X600},
    {"gsl1680e_86OGS", ARRAY_SIZE(GSL1680E_86OGS_FW), GSL1680E_86OGS_FW},
    {"gsl_m86_hd", ARRAY_SIZE(GSLX680_FW_M86HD), GSLX680_FW_M86HD},
    {"gsl_m102", ARRAY_SIZE(GSLX680_FW_M102), GSLX680_FW_M102},
    {"gsl_m102gg", ARRAY_SIZE(GSLX680_FW_M102GG), GSLX680_FW_M102GG},
    {"gsl_m320", ARRAY_SIZE(GSLX680_FW_M320), GSLX680_FW_M320},
    {"gsl_m100", ARRAY_SIZE(GSLX680_FW_M100), GSLX680_FW_M100},
    {"gsl_mq88", ARRAY_SIZE(GSLX680_FW_MQ88), GSLX680_FW_MQ88},
    {"gsl_m71", ARRAY_SIZE(GSLX680_FW_M71), GSLX680_FW_M71},
    {"gsl_m7300", ARRAY_SIZE(GSLX680_FW_M7300), GSLX680_FW_M7300},
    {"gsl_jinghong", ARRAY_SIZE(GSLX680_FW_JINGHONG), GSLX680_FW_JINGHONG},
    {"gsl_t1", ARRAY_SIZE(GSL1680E_FW_T1), GSL1680E_FW_T1},
    {"gsl_t1_v2", ARRAY_SIZE(GSL1680E_FW_T1_V2), GSL1680E_FW_T1_V2},
    {"gsl_a86", ARRAY_SIZE(GSL1680E_FW_A86), GSL1680E_FW_A86},
    {"gslX680_a63_perf_8001280", ARRAY_SIZE(FW_GSL_A63_PERF_8001280), FW_GSL_A63_PERF_8001280},
    {"gslX680_a63_perf_25601600", ARRAY_SIZE(GSL3692_FW_F1P0B_WJ_PG), GSL3692_FW_F1P0B_WJ_PG},
    {"gslX680_a63_t1_25601600", ARRAY_SIZE(GSL3692_FW_F1P0B_ONDA_OGS), GSL3692_FW_F1P0B_ONDA_OGS},
    {"gsl1680e_86pgpcd", ARRAY_SIZE(GSL1680E_86PGPCD_FW), GSL1680E_86PGPCD_FW},
    {"gslX680_3676", ARRAY_SIZE(GSLX680_FW), GSLX680_FW},
    {"gslX680_3676_1280x800", ARRAY_SIZE(GSLX680_FW_3676_1280x800), GSLX680_FW_3676_1280x800},
    {"gsl1680_7inch_863", ARRAY_SIZE(GSL1680_7INCH_FW_863), GSL1680_7INCH_FW_863},
    {"gslX680_3676_8inch", ARRAY_SIZE(GSLX680_FW_3676_8INCH), GSLX680_FW_3676_8INCH},
    {"gslX680_3676_10inch", ARRAY_SIZE(GSLX680_FW_3676_10INCH), GSLX680_FW_3676_10INCH},
    {"gslX680_3676_xc131", ARRAY_SIZE(GSLX680_FW_3676_XC131), GSLX680_FW_3676_XC131},
};

struct cfg_data {
    unsigned int size;
    unsigned int *data;
} gslX680_config_data[] = {
    {ARRAY_SIZE(gsl_config_data_id_K71_OGS_1024600), gsl_config_data_id_K71_OGS_1024600},
    {ARRAY_SIZE(gsl_config_data_id_708), gsl_config_data_id_708},
    {ARRAY_SIZE(gsl_config_data_id_1680_7inch), gsl_config_data_id_1680_7inch},
    {ARRAY_SIZE(gsl_config_data_id_P2), gsl_config_data_id_P2},
    {ARRAY_SIZE(gsl_config_data_id_P1_1024X600), gsl_config_data_id_P1_1024X600},
    {ARRAY_SIZE(gsl_config_data_id_86OGS), gsl_config_data_id_86OGS},
    {ARRAY_SIZE(gsl_config_data_id_m86_1024600), gsl_config_data_id_m86_1024600},
    {ARRAY_SIZE(gsl_config_data_id_m102), gsl_config_data_id_m102},
    {ARRAY_SIZE(gsl_config_data_id_m102gg), gsl_config_data_id_m102gg},
    {ARRAY_SIZE(gsl_config_data_id_m320), gsl_config_data_id_m320},
    {ARRAY_SIZE(gsl_config_data_id_m100), gsl_config_data_id_m100},
    {ARRAY_SIZE(gsl_config_data_id_mq88), gsl_config_data_id_mq88},
    {ARRAY_SIZE(gsl_config_data_id_m71), gsl_config_data_id_m71},
    {ARRAY_SIZE(gsl_config_data_id_m7300), gsl_config_data_id_m7300},
    {ARRAY_SIZE(gsl_config_data_id_jinghong), gsl_config_data_id_jinghong},
    {ARRAY_SIZE(gsl_config_data_id_t1), gsl_config_data_id_t1},
    {ARRAY_SIZE(gsl_config_data_id_t1_v2), gsl_config_data_id_t1_v2},
    {ARRAY_SIZE(gsl_config_data_id_a86), gsl_config_data_id_a86},
    {ARRAY_SIZE(gsl_config_data_id_GSL_A63_PERF_8001280), gsl_config_data_id_GSL_A63_PERF_8001280},
    {ARRAY_SIZE(gsl_config_data_id_F1P0B_WJ_PG), gsl_config_data_id_F1P0B_WJ_PG},
    {ARRAY_SIZE(gsl_config_data_id_F1P0B_ONDA_OGS), gsl_config_data_id_F1P0B_ONDA_OGS},
    {ARRAY_SIZE(gsl_config_data_id_86PGPCD), gsl_config_data_id_86PGPCD},
    {ARRAY_SIZE(gsl_config_data_id_3676), gsl_config_data_id_3676},
    {ARRAY_SIZE(gsl_config_data_id_3676_1280x800), gsl_config_data_id_3676_1280x800},
    {ARRAY_SIZE(gsl_config_data_id_1680_7inch_863), gsl_config_data_id_1680_7inch_863},
    {ARRAY_SIZE(gsl_config_data_id_3676_8inch), gsl_config_data_id_3676_8inch},
    {ARRAY_SIZE(gsl_config_data_id_3676_10inch), gsl_config_data_id_3676_10inch},
    {ARRAY_SIZE(gsl_config_data_id_3676_xc131), gsl_config_data_id_3676_xc131},
};

void usage() {
    printf("gsltool [${fw_name}] ${out_dir}");
}

int isBigEndian() {
    int a = 1;
    char *p = (char*)&a;
    if(*p == 1)
    {
        return 0;
    }
    else
    {
        return 1;
    }
}

int createFirmwareFromBigEndian(int idx, char *dir) {
    int fd;
    int size;
    int i;
    unsigned int tmp;
    char fn[128];

    snprintf(fn, 128, "%s/%s.bin", dir, gslx680_fw_grp[idx].name);
    fd = open(fn, O_CREAT | O_WRONLY, 0644);
    if (fd < 0) {
        printf("can not create %s\n", fn);
        return -1;
    }
    size = gslx680_fw_grp[idx].size;
    tmp = BigtoLittle32(size);
    write(fd, &tmp, sizeof(size));
    for (i = 0; i < size; i++) {
        tmp = gslx680_fw_grp[idx].fw[i].offset;
        tmp = BigtoLittle32(tmp);
        write(fd, &tmp, sizeof(tmp));
        tmp = BigtoLittle32(gslx680_fw_grp[idx].fw[i].val);
        write(fd, &tmp, sizeof(int));
    }

    size = gslX680_config_data[idx].size;
    tmp = BigtoLittle32(size);
    write(fd, &tmp, sizeof(size));
    for (i = 0; i < size; i++) {
        tmp = BigtoLittle32(gslX680_config_data[idx].data[i]);
        write(fd, &tmp, sizeof(unsigned int));
    }
    size = strlen(gslx680_fw_grp[idx].name);
    write(fd, &size, sizeof(size));
    write(fd, gslx680_fw_grp[idx].name, size);
    close(fd);
    return 0;
}

int createFirmware(int idx, char *dir) {
    if (isBigEndian()) {
        return createFirmwareFromBigEndian(idx, dir);
    }

    int fd;
    int size;
    char fn[128];

    snprintf(fn, 128, "%s/%s.bin", dir, gslx680_fw_grp[idx].name);
    fd = open(fn, O_CREAT | O_WRONLY, 0644);
    if (fd < 0) {
        printf("can not create %s\n", fn);
        return -1;
    }
    size = gslx680_fw_grp[idx].size;
    write(fd, &size, sizeof(size));
    write(fd, gslx680_fw_grp[idx].fw, size * sizeof(struct fw_data));


    size = gslX680_config_data[idx].size;
    write(fd, &size, sizeof(size));
    write(fd, gslX680_config_data[idx].data, size * sizeof(unsigned int));
    size = strlen(gslx680_fw_grp[idx].name);
    write(fd, &size, sizeof(size));
    write(fd, gslx680_fw_grp[idx].name, size);
    close(fd);
    return 0;
}

static int find_fw_idx(const char *name)
{
    int i = 0;

    if (name != NULL) {
        for (i = 0; i < ARRAY_SIZE(gslx680_fw_grp); i++) {
            if (!strcmp(name, gslx680_fw_grp[i].name))
                return i;
        }
    }
    return -1;
}

int main(int argc, char** argv) {
    if (argc != 2 && argc != 3) {
        usage();
        return -1;
    }

    int i;
    char *out_dir;

    if (argc == 3) {
        char *fw_name = argv[1];
        int idx;
        out_dir = argv[2];
        idx = find_fw_idx(fw_name);
        if (idx < 0) {
            printf("gsl find fw for %s fail!\n", fw_name);
            return -1;
        }
        return createFirmware(idx, out_dir);
    }
    out_dir = argv[1];

    for (i = 0; i < ARRAY_SIZE(gslx680_fw_grp); i++) {
        if (createFirmware(i, out_dir))
            return -1;
    }
    return 0;
}
