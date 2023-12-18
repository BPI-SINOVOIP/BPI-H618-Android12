#include <dc_demod.h>
#include <dc_wrapper_Fusion.h> //DVB-C
#include <dc_wrapper_dtmbip.h> //DTMB
#include <stdio.h>

DC_DemodT *DC_DemodCreate(TunerInitParam mTunerInitParam)
{
    DC_DemodT *demod = NULL;
    DvbStandardType stdType = mTunerInitParam.stdType;
    printf(" %s  demod type %d !\n",__FUNCTION__, stdType);

    if(stdType == DVB_STD_DTMB)
    {
        demod = DTMBIP_SingletonGetInstance();//DTMB
    }
    else if(stdType == DVB_STD_DVB_C)
    {
        demod = Fusion_SingletonGetInstance();//DVB-C
    }
    else
    {
        printf("%s %d not support type %d demod !\n",__FUNCTION__, __LINE__, stdType);
    }

    return demod;
}

