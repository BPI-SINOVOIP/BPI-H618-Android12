/**
*     @file       main.c
*
*	@author 	Alex Li (alexli@trident.com.cn)
*
*	@date    	2011/5/6
*
* @if copyright_display
*		Copyright (c) 2011 Trident Microsystems, Inc.
*		All rights reserved
*
*	 	The content of this file or document is CONFIDENTIAL and PROPRIETARY
*		to Trident Microsystems, Inc.  It is subject to the terms of a
*		License Agreement between Licensee and Trident Microsystems, Inc.
*		restricting among other things, the use, reproduction, distribution
*		and transfer.  Each of the embodiments, including this information and
*		any derivative work shall retain this copyright notice.
* @endif
*
*		This tuner test case can cover all avaible demod/tuner cases for atsc and dvb.
*
* @if modification_History
*
*		<b>Modification History:\n</b>
*		Date				Name			Comment \n\n
*
*
* @endif
*/
#include "TunerAPI.h"
#if defined(TEST_ATV_LAYER)
#include <linux/fb.h>
#include <fcntl.h>
#include "thal_display_source.h"
#include "thal_display_misc.h"
#include "thal_display_wce.h"
#endif
TRID_DEBUG_CATEGORY(atsc_debug);
TRID_DEBUG_CATEGORY(dvb_debug);

// define local
TRID_DEBUG_CATEGORY(main_debug);
#define TRID_CAT_DEFAULT (main_debug)

static void help(char* ProgramName) {
    printf("%s <ATSC> [AIR|STD|HRC|IRC|ATV] [ChanNo]\n", ProgramName);
    printf("%s <DVBT> [DTV|ATV] [FreqKHz]\n", ProgramName);
    printf("%s <DVBC> [DTV|ATV] [FreqKHz]\n", ProgramName);
    printf("%s <ISDBT> [DTV|ATV] [ChanNo]\n", ProgramName);

    return;
}

static int showAtvLayer(void) {
#if defined(TEST_ATV_LAYER)
    int fb = open("/dev/fb0", O_RDWR);
    if(fb < 0)
    {
        printf("Failed to open fb\n");
        return SYS_FAILED;
    }
    THal_Vp_Init();
    THal_Vp_SetSource(kHalSourceID_ATV);
    ioctl(fb, FBIOBLANK, FB_BLANK_POWERDOWN);
    sleep(3);
    THal_Vp_SetSource(kHalSourceID_VideoDec);
    ioctl(fb, FBIOBLANK, FB_BLANK_UNBLANK);
    THal_Vp_Deinit();
    close(fb);
#endif
    return SYS_NOERROR;
}

int main(int argc, char** argv) {
    trid_bool bLock = trid_false;
    RETURN_TYPE ret = SYS_NOERROR;
    char ProgramName[20];
    char Stack[10];
    char TunerType[10];
    int Val;
    int CheckCount = 5;
    TunerSignalQualityInfo_t SignalQualityInfo;
    TunerSignalStrengthInfo_t SignalStrength;    // register map
    TriHidtvRegMap();

    // debug info
    trid_debug_init("TUNERAPP");  // TRID_DEBUG_TUNERAPP in shell
    TRID_DEBUG_CATEGORY_INIT(main_debug, "MAIN", 0, "main entry");
    TRID_DEBUG_CATEGORY_INIT(atsc_debug, "ATSC", 0, "atsc tuner");
    TRID_DEBUG_CATEGORY_INIT(dvb_debug, "DVB", 0, "dvb tuner");
    printf("New tuner Test\n");
    // init parameter
    if (argc < 4) {
        help(argv[0]);
        return 1;
    }
    strcpy(ProgramName, argv[0]);
    strcpy(Stack, argv[1]);
    strcpy(TunerType, argv[2]);
    Val = atoi(argv[3]);

    if (argc == 5) CheckCount = atoi(argv[4]);  // check signal times
    // action
    if (!strncmp(Stack, "ATSC", 4)) {
        int ChanNo;

        if (ATSC_TunerInit() != SYS_NOERROR) {
            return 2;
        }

        ChanNo = Val;

        // Tune channel
        if (!strncmp(TunerType, "AIR", 3)) {
            ret = ATSC_TuneDTVChan(_AIR_STD_, ChanNo, _8VSB_);
            if (ret != SYS_NOERROR) {
                TRID_ERROR("Failed to tune ATSC AIR DTV ChanNo:%d\n", ChanNo);
                return 4;
            }
            bLock = ATSC_TunerIsLocked(_AIR_DIGITAL_TUNER_);
            if (bLock) {
                printf("\n===>>>AIR DTV [%d] is \33[1m\033[40;31mlocked\033[0m.\n", ChanNo);
                while (CheckCount--) {
                    ATSC_TunerSignalCheck(_AIR_DIGITAL_TUNER_, &SignalQualityInfo, &SignalStrength);
                    sleep(1);
                }
                return SYS_NOERROR;
            } else {
                printf("\n===>>>\33[1m\033[40;31mFailed\033[0m to lock AIR DTV [%d] .\n", ChanNo);
                return SYS_FAILED;
            }
        } else if (!strncmp(TunerType, "STD", 3) || !strncmp(TunerType, "HRC", 3) || !strncmp(TunerType, "IRC", 3)) {
            TunerStdType_e TunerStdType;

            // select cable standard
            if (!strncmp(TunerType, "STD", 3))
                TunerStdType = _CABLE_STD_;
            else if (!strncmp(TunerType, "HRC", 3))
                TunerStdType = _CABLE_HRC_;
            else if (!strncmp(TunerType, "IRC", 3))
                TunerStdType = _CABLE_IRC_;

            // tune channel
            ret = ATSC_TuneDTVChan(TunerStdType, ChanNo, _QAM256_);  // Try QAM256 first
            if (ret != SYS_NOERROR) {
                TRID_ERROR("Failed to tune ATSC Cable DTV ChanNo:%d\n", ChanNo);
                return 4;
            }
            bLock = ATSC_TunerIsLocked(_CABLE_DIGITAL_TUNER_);
            if (bLock) {
                printf("\n===>>> Cable DTV [%d]/QAM256 is \33[1m\033[40;31mlocked\033[0m.\n", ChanNo);
                while (CheckCount--) {
                    ATSC_TunerSignalCheck(_CABLE_DIGITAL_TUNER_, &SignalQualityInfo, &SignalStrength);
                    sleep(1);
                }
                return SYS_NOERROR;
            } else {
                // try QAM64
                ret = ATSC_TuneDTVChan(TunerStdType, ChanNo, _QAM64_);
                if (ret != SYS_NOERROR) {
                    TRID_ERROR("Failed to tune ATSC Cable DTV ChanNo:%d\n", ChanNo);
                    return 4;
                }
                bLock = ATSC_TunerIsLocked(_CABLE_DIGITAL_TUNER_);
                if (bLock) {
                    printf("\n===>>> Cable DTV [%d]/QAM64 is \33[1m\033[40;31mlocked\033[0m.\n", ChanNo);
                    while (CheckCount--) {
                        ATSC_TunerSignalCheck(_CABLE_DIGITAL_TUNER_, &SignalQualityInfo, &SignalStrength);
                        sleep(1);
                    }
                    return SYS_NOERROR;
                }
                printf("\n===>>>\33[1m\033[40;31mFailed\033[0m to lock cable DTV [%d] .\n", ChanNo);
                return SYS_FAILED;
            }
        } else if (!strncmp(TunerType, "ATV", 3)) {
            // only test Air ATV here.
            ATSC_TuneATVChan(_AIR_STD_, ChanNo, _8VSB_);
            bLock = ATSC_TunerIsLocked(_AIR_ANALOG_TUNER_);
            if (bLock) {
                printf("\n===>>>AIR ATV [%d] is \33[1m\033[40;31mlocked\033[0m.\n", ChanNo);
                return SYS_NOERROR;
            } else {
                printf("\n===>>>\33[1m\033[40;31mFailed\033[0m to lock AIR ATV [%d] .\n", ChanNo);
                return SYS_FAILED;
            }

        } else {
            help(ProgramName);
            return SYS_FAILED;
        }

    } else if (!strncmp(Stack, "DVBT", 4)) {
        if (!strncmp(TunerType, "DTV", 3)) {
            if (DVB_TunerInit(DVB_DTV) != SYS_NOERROR) {
                TRID_ERROR("Init failed.\n");
                return 2;
            }

            DVBScanParam_t Param;

            Param.Freq = Val;
            Param.ModulationMode = _DEFAULT_MODULATION_MODE_;
            Param.BandWidth = _SCAN_BANDWIDTH_8M_;
            Param.priority = 1;
            ret = DVB_TuneDTVChan(_AIR_DIGITAL_TUNER_, &Param);
            if (ret != SYS_NOERROR) {
                TRID_ERROR("Failed to tune DVBT DTV channel[%d KHz]\n", Param.Freq);
                return 4;
            }
            bLock = DVB_TunerIsLocked(_AIR_DIGITAL_TUNER_);
            if (bLock) {
                printf("\n===>>>DVBT DTV [%d KHz] is \33[1m\033[40;31mlocked\033[0m.\n", Param.Freq);
                while (CheckCount--) {
                    DVB_TunerSignalCheck(_AIR_DIGITAL_TUNER_, &SignalQualityInfo, &SignalStrength);
                    sleep(1);
                }
            } else
                printf("\n===>>>DVBT DTV [%d KHz] is \33[1m\033[40;31mNOT locked\033[0m.\n", Param.Freq);
            return SYS_NOERROR;

        } else if (!strncmp(TunerType, "ATV", 3)) {
            if (DVB_TunerInit(DVB_ATV) != SYS_NOERROR) {
                TRID_ERROR("Init failed.\n");
                return 2;
            }

            int Freq = Val;
            ret = DVB_TuneATVChan(SOUND_STANDARD_BG, Val);
            if (ret != SYS_NOERROR) {
                TRID_ERROR("Failed to tune DVBT ATV channel[%d KHz]\n", Freq);
                return 4;
            }
            bLock = DVB_TunerIsLocked(_AIR_ANALOG_TUNER_);
            if (bLock)
                printf("\n===>>>DVBT ATV [%d KHz] is \33[1m\033[40;31mlocked\033[0m.\n", Freq);
            else
                printf("\n===>>>DVBT ATV [%d KHz] is \33[1m\033[40;31mNOT locked\033[0m.\n", Freq);
            return SYS_NOERROR;
        } else {
            help(ProgramName);
            return SYS_FAILED;
        }

    } else if (!strncmp(Stack, "DVBC", 4)) {
        if (!strncmp(TunerType, "DTV", 3)) {
            if (DVB_TunerInit(DVB_DTV) != SYS_NOERROR) {
                TRID_ERROR("Init failed.\n");
                return 2;
            }

            DVBScanParam_t Param;

            Param.Freq = Val;
            Param.SymbolRate = 6900;
            Param.ModulationMode = _QAM64_;  // try QAM 64 first
            Param.BandWidth = _SCAN_BANDWIDTH_8M_;
            Param.priority = 1;

            // auto symbol rate
            DVB_SetConfig(trid_true);

            // tune
            ret = DVB_TuneDTVChan(_CABLE_DIGITAL_TUNER_, &Param);
            if (ret != SYS_NOERROR) {
                TRID_ERROR("Failed to tune DVBC DTV channel[%d KHz]\n", Param.Freq);
                return 4;
            }

            bLock = DVB_TunerIsLocked(_CABLE_DIGITAL_TUNER_);
            if (bLock) {
                printf("\n===>>>DVBC DTV [%d KHz] is \33[1m\033[40;31mlocked\033[0m at QAM64.\n", Param.Freq);
                TRID_INFO("Symbol rate = %d\n", Param.SymbolRate);
                while (CheckCount--) {
                    DVB_TunerSignalCheck(_CABLE_DIGITAL_TUNER_, &SignalQualityInfo, &SignalStrength);
                    sleep(1);
                }
            } else {
                // try QAM128
                Param.ModulationMode = _QAM128_;
                ret = DVB_TuneDTVChan(_CABLE_DIGITAL_TUNER_, &Param);
                if (ret != SYS_NOERROR) {
                    TRID_ERROR("Failed to tune DVBC DTV channel[%d KHz]\n", Param.Freq);
                    return 4;
                }

                bLock = DVB_TunerIsLocked(_CABLE_DIGITAL_TUNER_);
                if (bLock) {
                    printf("\n===>>>DVBC DTV [%d KHz] is \33[1m\033[40;31mlocked\033[0m at QAM128.\n", Param.Freq);
                    TRID_INFO("Symbol rate = %d\n", Param.SymbolRate);
                    while (CheckCount--) {
                        DVB_TunerSignalCheck(_CABLE_DIGITAL_TUNER_, &SignalQualityInfo, &SignalStrength);
                        sleep(1);
                    }
                } else {
                    // try QAM256
                    Param.ModulationMode = _QAM256_;
                    ret = DVB_TuneDTVChan(_CABLE_DIGITAL_TUNER_, &Param);
                    if (ret != SYS_NOERROR) {
                        TRID_ERROR("Failed to tune DVBC DTV channel[%d KHz]\n", Param.Freq);
                        return 4;
                    }

                    bLock = DVB_TunerIsLocked(_CABLE_DIGITAL_TUNER_);
                    if (bLock) {
                        printf("\n===>>>DVBC DTV [%d KHz] is \33[1m\033[40;31mlocked\033[0m. at QAM256\n", Param.Freq);
                        TRID_INFO("Symbol rate = %d\n", Param.SymbolRate);
                        while (CheckCount--) {
                            DVB_TunerSignalCheck(_CABLE_DIGITAL_TUNER_, &SignalQualityInfo, &SignalStrength);
                            sleep(1);
                        }
                    } else {
                        // try QAM32
                        Param.ModulationMode = _QAM32_;
                        ret = DVB_TuneDTVChan(_CABLE_DIGITAL_TUNER_, &Param);
                        if (ret != SYS_NOERROR) {
                            TRID_ERROR("Failed to tune DVBC DTV channel[%d KHz]\n", Param.Freq);
                            return 4;
                        }

                        bLock = DVB_TunerIsLocked(_CABLE_DIGITAL_TUNER_);
                        if (bLock) {
                            printf("\n===>>>DVBC DTV [%d KHz] is \33[1m\033[40;31mlocked\033[0m at QAM32.\n", Param.Freq);
                            TRID_INFO("Symbol rate = %d\n", Param.SymbolRate);
                            while (CheckCount--) {
                                DVB_TunerSignalCheck(_CABLE_DIGITAL_TUNER_, &SignalQualityInfo, &SignalStrength);
                                sleep(1);
                            }

                        } else {
                            printf("\n===>>>DVBC DTV [%d KHz] is \33[1m\033[40;31mNOT locked\033[0m  with QAM64/QAM128/QAM256/QAM32.\n",
                                   Param.Freq);
                        }
                    }
                }
            }

            return SYS_NOERROR;
        } else if (!strncmp(TunerType, "ATV", 3)) {
            if (DVB_TunerInit(DVB_ATV) != SYS_NOERROR) {
                TRID_ERROR("Init failed.\n");
                return SYS_FAILED;
            }

            int Freq = Val;
            ret = DVB_TuneATVChan(SOUND_STANDARD_DK, Val);
            if (ret != SYS_NOERROR) {
                TRID_ERROR("Failed to tune DVBT ATV channel[%d KHz]\n", Freq);
                return SYS_FAILED;
            }
            bLock = DVB_TunerIsLocked(_CABLE_ANALOG_TUNER_);
            if (bLock) {
                printf("\n===>>>DVBC ATV [%d KHz] is \33[1m\033[40;31mlocked\033[0m.\n", Freq);
                return showAtvLayer();
            } else {
                printf("\n===>>>DVBC ATV [%d KHz] is \33[1m\033[40;31mNOT locked\033[0m.\n", Freq);
                return SYS_FAILED;
            }
        } else {
            help(ProgramName);
            return SYS_FAILED;
        }

    } else if (!strncmp(Stack, "ISDBT", 5)) {
        if (!strncmp(TunerType, "DTV", 3)) {
            if (DVB_TunerInit(DVB_DTV) != SYS_NOERROR) {
                TRID_ERROR("Init failed.\n");
                return 2;
            }
        }
        // system("cd /mnt/WorkDir/Program");
        system("regtest w w 06142044 4000000");
        ret = ISDBT_SetTunerByChanNo(_AIR_DIGITAL_TUNER_, Val);
        if (ret != SYS_NOERROR) {
            TRID_ERROR("Failed to tune ISDBT DTV channel[%d]\n", Val);
            return 4;
        }

        bLock = ISDBT_TunerIsLocked(_AIR_DIGITAL_TUNER_);
        if (bLock) {
            printf("\n===>>>ISDBT DTV [%d] is \33[1m\033[40;31mlocked\033[0m.\n", Val);

        } else
            printf("\n===>>>ISDBT DTV [%d] is \33[1m\033[40;31mNOT locked\033[0m.\n", Val);

        return SYS_NOERROR;

    } else {
        help(ProgramName);
    }

    return 0;
}
