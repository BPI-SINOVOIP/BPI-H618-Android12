/********************************************************************************************************
File:ATBM253Sample.c
Description:
	Example of ATBM253 SDK .
	

History:

*********************************************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include "ATBM253Api.h"
#include <unistd.h>

/***********Example Code*********************/
#if 1
int main(int argc,char *argv[])
{
/*
This is an example of using ATBM253.
*/
	ATBM253_ERROR_e ret = ATBM253_NO_ERROR;
	ATBM253InitConfig_t InitConfig;
	ATBM253CfgCMD_t Config;
	ATBM_U32 FreqKHz = 474000,BandWidthKHz = 8000;
	ATBM_S32 RSSI = 0;
	ATBM_U32 DevId = 0;
	int i, freqhz[5] = {474000};
	(void)argc;
	(void)argv;
	
	/*Get the default configuration from ATBM253 SDK. */
	if(ATBM253_NO_ERROR != ATBM253DefaultCfgGet(&InitConfig))
	{
		return -1;
	}
	/*Change some setting according to actual requirement.*/
	InitConfig.I2CParam.I2CSlaveAddr = 0xC0;
	InitConfig.I2CParam.Param = 0; /*user parameter, which may be used in ATBM253I2CRead/Write. */
	InitConfig.OSCCap.CalibValue = 0x08; /*OSC PPM calibration, from 0x00 to 0x0F.*/
	InitConfig.Mode = ATBM253_SIGNAL_MODE_DTMB;
	InitConfig.DtvIFOut.IFOutFreqHz = 5000000;/* 5MHz */
	InitConfig.DtvIFOut.IFOutLevel = ATBM253_IF_OUT_LEVEL2;	
	InitConfig.AtvIFOut.IFOutFreqHz = 5000000;/* 5MHz */
	InitConfig.AtvIFOut.IFOutLevel = ATBM253_IF_OUT_LEVEL1;
	/*Initialize with customer parameters.*/
	ret = ATBM253Init(DevId,&InitConfig);
	if(ATBM253_NO_ERROR != ret)
	{
		return -1;
	}

	for(i = 0; i < 1; i++) {
	/*Tune to frequency, e.g.  DTMB ,center frequency is 546000KHz, bandwidth is 8MHz, normal spectrum.*/
		FreqKHz = freqhz[i];
		BandWidthKHz = 8000;
		ret = ATBM253ChannelTune(DevId,ATBM253_SIGNAL_MODE_DTMB,FreqKHz,BandWidthKHz,ATBM253_SPECTRUM_INVERT);
		if(ATBM253_NO_ERROR != ret)
		{
			return -1;
		}
		while(1) {
			/*Get RSSI*/
			ret = ATBM253GetRSSI(DevId,&RSSI);
			if(ATBM253_NO_ERROR != ret)
				return -1;

			printf("RSSI:%d dBm\n",RSSI);
			usleep(3000000);
		}
	}

	/*Tune to another channel, e.g. DVBC, center frequency is 674000KHz, bandwidth is 8MHz, normal spectrum.*/
	FreqKHz = 674000;
	BandWidthKHz = 8000;
	//ret = ATBM253ChannelTune(DevId,ATBM253_SIGNAL_MODE_DVBC,FreqKHz,BandWidthKHz,ATBM253_SPECTRUM_NORMAL);
	if(ATBM253_NO_ERROR != ret)
	{
		return -1;
	}

	/*If you want to change some parameters, call 'ATBM253CfgSet' to do it. */
	//Config.CfgCmd = ATBM253_CFG_CMD_DTV_IF_OUT_SETTING;
	//Config.Cfg.IFOut.IFOutFreqHz = 5000000;
	//Config.Cfg.IFOut.IFOutLevel = ATBM253_IF_OUT_LEVEL3;

	//ret = ATBM253CfgSet(DevId,&Config);
	if(ATBM253_NO_ERROR != ret)
	{
		return -1;
	}

	while(1);
	return 0;
}
#endif

