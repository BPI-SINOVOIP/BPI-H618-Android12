#include <dc_types.h>
#include <dc_log.h>
#include <dc_demod.h>
#include <unistd.h>
#include <stdio.h>

unsigned int freq_all[48] = {
    52500000, 60500000, 68500000, 80000000,
    171000000, 179000000, 187000000, 195000000,
    203000000, 211000000, 219000000, 474000000,
    482000000,490000000, 498000000, 506000000,
    514000000, 522000000, 530000000, 538000000,
    546000000, 554000000, 562000000, 610000000,
    618000000, 626000000, 634000000, 642000000,
    650000000, 658000000, 666000000, 674000000,
    682000000, 690000000, 698000000, 706000000,
    714000000, 722000000, 730000000, 738000000,
    746000000, 754000000, 762000000, 770000000,
    778000000, 786000000, 794000000, 802000000,
};

int TC_Demod_searchall()
{
    int ret = 0, i, j = 0;
    DC_DemodT *demod = NULL;
    uint32_t freq;

    printf("-------------TC-Demod begin---------------");
    TunerInitParam mTunerInitParam;
    mTunerInitParam.stdType = DVB_STD_DTMB;

    demod = DC_DemodCreate(mTunerInitParam);
    DC_ASSERT(demod != NULL);

    ret = DC_DemodInit(demod);
    DC_ASSERT(ret == 0);

    for (j = 0; j < 48; j++) {
        freq = freq_all[j];
        ret = DC_DemodSetFrequency(demod, freq);
        if (ret < 0) {
            printf("DC_DemodSetFrequency failed.");
            continue;
        } else {
            DemodWorkStatusT work_status;
            for (i = 0; i < 3; i++) {
                ret = DC_DemodGetWorkStatus(demod, &work_status);
                printf("ret(%d), state(%d) lock(%d), strength(%d), quality(%d), freq(%d)\n",
                    ret, work_status.state, work_status.is_lock, work_status.strength,
                    work_status.quality, work_status.freq);
                usleep(3000000);
            }
        }
    }
    printf("-------------TC-Demod end---------------");
    return 0;
}

int TC_Demod(uint32_t freq)
{
    int ret = 0;
    DC_DemodT *demod = NULL;

    printf("-------------TC-Demod begin---------------");
    TunerInitParam mTunerInitParam;
    mTunerInitParam.stdType = DVB_STD_DTMB;

    demod = DC_DemodCreate(mTunerInitParam);
    DC_ASSERT(demod != NULL);

    ret = DC_DemodInit(demod);
    DC_ASSERT(ret == 0);

    ret = DC_DemodSetFrequency(demod, freq);
    if (ret < 0) {
        printf("DC_DemodSetFrequency failed.");
        return -1;
    }

    DemodWorkStatusT work_status;

    while(1) {
        ret = DC_DemodGetWorkStatus(demod, &work_status);
        printf(" ret(%d), state(%d) lock(%d), strength(%d), quality(%d), freq(%d)\n",
            ret, work_status.state, work_status.is_lock, work_status.strength,
            work_status.quality, work_status.freq);
        usleep(3000000);
    }

    printf("-------------TC-Demod end---------------");
    return 0;
}
