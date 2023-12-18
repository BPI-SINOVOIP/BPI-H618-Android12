#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <dc_demod.h>
#include <dc_types.h>
#include "DataType.h"
#include "dtmbip_sunxi.h"
#include <linux/types.h>
#include <awesome_pgsearch.h>

extern int TC_Demod(uint32_t freq);
extern int TC_Demod_searchall();

static int pgs_start = 1;
static PGShandlerT *pgs;
static DC_DemodT *demod = NULL;
int PGS_Result;

int utOnFinish(void *cookie, int err, struct ProgramInfoS *program_info, int program_num)
{
    (void)cookie;

    printf("111utOnFinish., program '%d' err=%d\n", program_num, err);

    for (int i = 0; i<program_num; i++)
    {
        printf("[%d]pmt_pid:'%d', name:'%s'\n", i, program_info[i].pmt_pid, program_info[i].name);
    }
    PGS_Result = err;
    pgs_start = 1;

    //if (demod != NULL)
    printf("111\n");
    DC_DemodStop(demod);

    return err;
}

struct PGS_ListenerS ut_pgs_listener =
{
    .onFinish = utOnFinish
};

static int TC_PGS(uint32_t freq, char *tv_standard)
{
    int ret;
    TunerInitParam mTunerInitParam;

    pgs_start = 0;

    if (!demod)
    {
        if (strcmp(tv_standard, "DTMB") == 0)
            mTunerInitParam.stdType = DVB_STD_DTMB;
        else if (strcmp(tv_standard, "DVBC") == 0)
        {
            mTunerInitParam.stdType = DVB_STD_DVB_C;
        }
        else
        {
            printf("check TV standard param !!!\n");
            return -1;
        }

        demod = DC_DemodCreate(mTunerInitParam);
        DC_ASSERT(demod != NULL);

        ret = DC_DemodInit(demod);
        DC_ASSERT(ret == 0);

        ret = DC_DemodSetFrequency(demod, freq);
        if (ret)
        {
            printf("DC_DemodSetFrequency fail ret=%d\n");
            return -1;
        }
    }

    pgs = PGS_Instance(&ut_pgs_listener, NULL, freq, mTunerInitParam.stdType);
    PGS_Start(pgs);

    return 0;
}

int main(int argc, char* argv[])
{
    uint32_t freq = 0;
    int ret;

    if(argc > 4) {
        printf("check param !!!\n");
        return -1;
    }

    printf(" TC begin, freq: '%u'\n", freq);

    /* dvbserver start we should make sure it power off */

    if (strcmp(argv[1], "demod") == 0)
    {
        if (argc == 3 && argv[2] != 0) {
            freq = atoi(argv[2]);
            freq = freq * 1000000;
        }
        TC_Demod(freq);
    }
    else if (strcmp(argv[1], "pgs") == 0)
    {
        if (argc == 4 && argv[3] != 0) {
            freq = atoi(argv[3]);
            freq = freq * 1000000;
        }

        if (pgs_start)
        {
            ret = TC_PGS(freq, argv[2]);
            if (ret)
                return -1;
        }

        while (!pgs_start)
            usleep(1000000);


        printf("ut_demod pgs end\n");
        return PGS_Result;
    }
    else if (strcmp(argv[1], "demod_searchall") == 0)
    {
        if (argc == 3 && argv[2] != 0) {
            freq = atoi(argv[2]);
            freq = freq * 1000000;
        }
        TC_Demod_searchall();
    }
    else
    {
        printf("invalid cmd '%s'\n", argv[1]);
        return -1;
    }

    return 0;
}

