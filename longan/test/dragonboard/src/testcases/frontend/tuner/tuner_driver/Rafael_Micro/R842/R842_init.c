
#include <fcntl.h>
#include <getopt.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <linux/ioctl.h>
#include <sys/ioctl.h>

#include "R842.h"
#include "os_adapter.h"

static int getinput()
{
    int ret;
    do {
        ret = getchar();
    } while (ret == '\n');

    int c;
    do {
        c = getchar();
    } while (c != EOF && c != '\n');

    printf("getinput: %c\n", c);
    return ret;
}

void tuner_init(void)
{
    BYTE c;
    UINT32 err = 0; //Error_NO_ERROR;
    R842_Set_Info R842_Info;

    UINT32 dwFreq_KHz ; //= R842_Info.RF_KHz;
    UINT8 u8chipInited = 0;
    char freq[15];

#define ATV
#ifdef ATV
    //设置 标准
    R842_Info.R842_Standard = R842_PAL_BGH_8M;
    //设置频率
    R842_Info.RF_KHz = 48250;    //KHz
#else
    //Set Standard
    R842_Info.R842_Standard = R842_DVB_C_8M_IF_5M;
    //Set Frequency
    R842_Info.RF_KHz = 858000;    //KHz
#endif
    //R842_Info.R842_LT = LT_OFF;   //LT Off
    //R842_Info.R842_ClkOutMode = CLK_OUT_OFF; //No tuner clock output
    R842_Info.R842_IfAgc_Select = R842_IF_AGC1;     //Select IF AGC 1
    dwFreq_KHz = R842_Info.RF_KHz;
    //UINT32 dwFreq_KHz = R842_Info.RF_KHz;
    //UINT8 u8chipInited = 0;

    printf("\n************************************************\n");
    //printf("China Dragon Tech Common Tuner Debug Tools v2.00\n");
    //printf("Writer: ylzhu Date: 2015-11-20\n");
    //printf("CopyRight(c):  China Dragon Tech 2014-2015\n");
    printf("Type:9NT372-RF01  Chip:R842\n");
#ifdef ATV
    printf("Standard:PAL B/G  BW: 8MHz\n");
    printf("//---> Standard : %d \n", R842_Info.R842_Standard);
    printf("//---> Frequency: %ld \n", R842_Info.RF_KHz);
#else
    printf("Standard:DVB-C  BW: 8MHz\n");
    printf("//---> Standard : %d \n", R842_Info.R842_Standard);
    printf("//---> Frequency: %d \n", R842_Info.RF_KHz);
#endif
    printf("system test\n");
    printf("modify time: 2020.10.11 \n");
    printf("************************************************\n");

    while(1) {

        printf("================================================\n");
        printf("\ninput any key to continue\n");
        printf("================================================\n");
        printf("i : initialize\n");
        printf("m : mode config\n");
        printf("s : frequency config\n");
        printf("x : exit tuner config\n");

        c = getinput();
        switch(c)
        {
            case 'i':
            case 'I':
                {
                    UINT8 u8Ret = 0;
                    UINT8 u8i = 0;
                    I2C_LEN_TYPE dataI2C;
                    memset(&dataI2C, 0, sizeof(I2C_LEN_TYPE));
                    dataI2C.RegAddr = 0x00;
                    dataI2C.Len = 1;

                    i2c_connection_check("/dev/i2c-2", R828_ADDRESS);

                    if (I2C_Read_Len_R842(&dataI2C) == 0) {
                        printf("IIC connect failed!\n");
                        break;
                    }

                    printf("Read data: 0x%02x\n", dataI2C.Data[0]);

                    u8Ret = R842_Init();
                    if(u8Ret == 1&&dataI2C.Data[0]!=0)
                    {
                        printf("--->R842 init success!\n");
                        u8chipInited= 1;
                    }
                    else
                    {
                        printf("--->R842 init falied!\n");
                        u8chipInited= 0;
                    }
                    // R842_SetXtalIntCap(XTAL_CAP_SMALL);
                    break;
                }

            case 's':
            case 'S':
                {
                    if(u8chipInited == 1)
                    {
                        printf("\t\ninput frequency(KHz):");
                        printf("\t:");
                        scanf ("%ld", &dwFreq_KHz);

                        //R842 Frequency Setting
                        R842_Info.RF_KHz = dwFreq_KHz;
                        //Loop Through
                        //R842_Info.R842_LT = LT_OFF; // Loop through OFF
                        //Clk output
                        //R842_Info.R842_ClkOutMode = CLK_OUT_OFF; // No tuner clock output for other IC
                        //IF AGC select
                        R842_Info.R842_IfAgc_Select = R842_IF_AGC1; // Select IF AGC1

                        printf("--->input freq: %ld \n", R842_Info.RF_KHz);
                        R842_SetPllData(R842_Info);
                        if(R842_PLL_Lock() ==1)
                        {
                            printf("--->frequency locked!\n");
                        }
                    }
                    else
                    {
                        printf("--->do init first!\n");
                    }
                    break;
                }

            case 'm':
            case 'M':
                {
                    printf("\nSelect Standard\n");
                    printf("A: ATV\n");
                    printf("D: DTV\n");
                    c = getinput();

                    if (c=='a' || c=='A') {
                        printf("ATV Standard\n");
                        printf("\tB: PAL B/G\n");
                        printf("\tD: PAL D/K\n");
                        printf("\tI: PAL I\n");
                        printf("\tn: NTSC M/N\n");
                        printf("\tS: SECAM L\n");//add by madianqi
                        c = getinput();
                        switch(c)
                        {
                            case 'n':
                            case 'N':
                            case 'm':
                            case 'M':
                                {
                                    R842_Info.R842_Standard = R842_MN_5800;
                                    break;
                                }
                            case 'i':
                            case 'I':
                                {
                                    R842_Info.R842_Standard = R842_PAL_I;
                                    break;
                                }
                            case 'd':
                            case 'D':
                                {
                                    R842_Info.R842_Standard = R842_PAL_DK;
                                    break;
                                }
                            case 'b':
                            case 'B':
                                {
                                    R842_Info.R842_Standard = R842_PAL_BGH_8M;
                                    break;
                                }
                                /***********************add by MDQ***************************/
                            case 's':
                            case 'S':
                                {
                                    R842_Info.R842_Standard = R842_SECAM_L;//add by madianqi
                                    break;
                                }
                                /************************************************************/
                            default:
                                {
                                    R842_Info.R842_Standard = R842_PAL_DK;
                                    break;
                                }
                        }
                    }
                    else if(c=='d' || c=='D')
                    {
                        printf("DTV Standard\n");
                        printf("\t1: DVB-T  Bandwidth:7MHz\n");
                        printf("\t2: DVB-T  Bandwidth:8MHz\n");
                        printf("\t3: DVB-T2 Bandwidth:7MHz\n");
                        printf("\t4: DVB-T2 Bandwidth:8MHz\n");
                        printf("\t5: ATSC   Bandwidth:6MHz\n");
                        printf("\t6: DTMB   Bandwidth:6MHz\n");//add by madianqi
                        printf("\t7: DTMB   Bandwidth:8MHz\n");//add by madianqi
                        printf("\t8: ISDB-T Bandwidth:6MHz\n");//add by madianqi
                        printf("\t9: J83B   Bandwidth:6MHz\n");//add by madianqi

                        c = getinput();
                        switch(c)
                        {
                            case '1':
                                {
                                    R842_Info.R842_Standard = R842_DVB_T_7M_IF_5M;
                                    break;
                                }
                            case '2':
                                {
                                    R842_Info.R842_Standard = R842_DVB_T_8M_IF_5M;
                                    break;
                                }
                            case '3':
                                {
                                    R842_Info.R842_Standard = R842_DVB_T2_7M_IF_5M;
                                    break;
                                }
                            case '4':
                                {
                                    R842_Info.R842_Standard = R842_DVB_T2_8M_IF_5M;
                                    break;
                                }
                            case '5':
                                {
                                    R842_Info.R842_Standard = R842_ATSC_IF_5M;
                                    break;
                                }
                                /***********************add by MDQ***************************/
                            case '6':
                                {
                                    R842_Info.R842_Standard = R842_DTMB_6M_IF_5M;
                                    break;
                                }
                            case '7':
                                {
                                    R842_Info.R842_Standard = R842_DTMB_8M_IF_5M;
                                    break;
                                }
                            case '8':
                                {
                                    R842_Info.R842_Standard = R842_ISDB_T_IF_5M;
                                    break;
                                }
                            case '9':
                                {
                                    R842_Info.R842_Standard = R842_J83B_IF_5M;
                                    break;
                                }
                                /************************************************************/
                            default:
                                {
                                    R842_Info.R842_Standard = R842_DVB_T_8M_IF_5M;
                                    break;
                                }

                        }
                    }
                    else
                    {
                        printf("unknow input '%c'\n", c);
                    }

                    break;
                }
            case 'f':
            case 'F':

                break;

            case 'a':
            case 'A':

                break;

            case 'u':
            case 'U':

                break;

            case 'p':
            case 'P':

                break;

            case 'v':
            case 'V':

                break;

            case 0xe0:// JUST FOR TEST
                {
                    c = getinput();
                    switch (c)
                    {
                        case 0x48:
                            {
                                if(u8chipInited == 1)
                                {
                                    R842_Info.RF_KHz += 8000;
                                    printf("--->input freq: %ld \n", R842_Info.RF_KHz);
                                    R842_SetPllData(R842_Info);
                                    if(R842_PLL_Lock() ==1)
                                    {
                                        printf("--->PLL Locked\n");
                                    }
                                }
                                else
                                {
                                    printf("--->please init first\n");
                                }
                                break;
                            }
                        case 0x50:
                            {
                                if(u8chipInited == 1)
                                {
                                    R842_Info.RF_KHz -= 8000;
                                    printf("--->input freq: %ld \n", R842_Info.RF_KHz);
                                    R842_SetPllData(R842_Info);
                                    if(R842_PLL_Lock() ==1)
                                    {
                                        printf("--->PLL Locked\n");
                                    }
                                }
                                else
                                {
                                    printf("--->please init first\n");
                                }
                                break;
                            }
                        case 0x4b:
                            break;
                        case 0x4d:
                            break;
                        default:
                            break;
                    }
                }
                break;

            case 'x':
            case 'X':
                goto EXITPROC;
                break;

            default:
                break;
        }
    }

EXITPROC:
    printf("================================================\n");
    printf("\nexit Tuner config!\n");
    printf("================================================\n");
}
