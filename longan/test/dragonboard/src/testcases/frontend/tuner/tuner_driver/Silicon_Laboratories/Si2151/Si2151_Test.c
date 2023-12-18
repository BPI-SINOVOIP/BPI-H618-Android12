/***************************************************************************************
                  Silicon Laboratories Broadcast Si2151 Layer 3 API

   EVALUATION AND USE OF THIS SOFTWARE IS SUBJECT TO THE TERMS AND CONDITIONS OF
     THE SOFTWARE LICENSE AGREEMENT IN THE DOCUMENTATION FILE CORRESPONDING
     TO THIS SOURCE FILE.
   IF YOU DO NOT AGREE TO THE LIMITED LICENSE AND CONDITIONS OF SUCH AGREEMENT,
     PLEASE RETURN ALL SOURCE FILES TO SILICON LABORATORIES.

   L3 sample application
   FILE: Si2151_L3_Test.c
   Supported IC : Si2141-A10, Si2141-B10, Si2144-A20, Si2124-A20
   Compiled for ROM 61 firmware 1_1_build_10
   Revision: 0.2
   Tag:  ROM61_1_1_build_10_V0.2
   Date: August 04 2015
  (C) Copyright 2015, Silicon Laboratories, Inc. All rights reserved.
****************************************************************************************/
#include "Si2151_L3_Test.h"
#include <stdio.h>
#include "os_adapter.h"

#ifndef RINT8
typedef char RINT8;
#endif

typedef int32_t  __s32;
typedef uint32_t u32;
typedef uint8_t  u8;

#define Error_NO_ERROR 0x00

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

/* define TUNER_ONLY if using a Si2151 tuner without demod                                                    */
/* (It should be defined at project/makefile level to use 100% identical code for tuner-only and tuner+demod)   */
#define TUNER_ONLY

#ifdef    TUNER_ONLY

/* define SILABS_DEMOD_EVB if used on a tuner_only application on a demod EVB (opens i2c_passthru in main)      */
/* (It should be defined at project/makefile level to use 100% identical code for tuner-only EVB and demod EVB) */
/* #define SILABS_DEMOD_EVB*/

/* define Si2151_COMMAND_LINE_APPLICATION if using this code in console mode                                                     */
/* (Si2151_COMMAND_LINE_APPLICATION should be defined at project/makefile level to use 100% identical code for console and TV )  */
/* #define Si2151_COMMAND_LINE_APPLICATION */

/* define FRONT_END_COUNT 1 for a single tuner application                                                                         */
/* (FRONT_END_COUNT should be defined at project/makefile level to use 100% identical code for single-frontend and multi-frontend) */
#define FRONT_END_COUNT 1

/* Only 1 VCO blocking method is needed. Use the POSTTUNE Method for most applications. */
/* No blocking method is needed for a single tuner application -- both flags can be undef*/
/* If using a Silabs Demod and > 1 tuner then undef both flags and define Si2151_DEMOD_WRAPPER_VCO (see Si2151_L2_API.c) */
#if FRONT_END_COUNT > 1
#define Si2151_USE_POSTTUNE_VCO_BLOCKING_METHOD
#undef  Si2151_USE_PRETUNE_VCO_BLOCKING_METHOD
#else
#undef Si2151_USE_POSTTUNE_VCO_BLOCKING_METHOD
#undef  Si2151_USE_PRETUNE_VCO_BLOCKING_METHOD
#endif

int  commandCount  = 0;
int  fe;

L1_Si2151_Context          FrontEnd_Table   [FRONT_END_COUNT];
L1_Si2151_Context         *tuners[FRONT_END_COUNT];
L1_Si2151_Context         *Si2151;
L1_Si2151_Context         *front_end;

/* Also init a simple i2c context to allow i2c communication checking */
L0_Context* i2c;
L0_Context  i2c_context;
CONNECTION_TYPE mode;

#ifdef    SILABS_DEMOD_EVB
/* To be able to test this on SiLabs demod EVB, it must be possible to close the i2c passthru in the demod */
typedef struct demod_Context {
  L0_Context    *i2c;
  L0_Context     i2cObj;
} demod_Context;

/* i2c_passthru                   */
 #define    i2c_passthru_ADDRESS               49165
 #define    i2c_passthru_OFFSET                0
 #define    i2c_passthru_NBBIT                 1
 #define    i2c_passthru_ALONE                 1
 #define    i2c_passthru_SIGNED                0
  #define           i2c_passthru_disabled                      0
  #define           i2c_passthru_enabled                       1

  demod_Context  demod_table[FRONT_END_COUNT];
  demod_Context *demod;
#endif /* SILABS_DEMOD_EVB */

#ifdef    SiTRACES
/************************************************************************************************************************
  Si2151_UserInput_traces function
  Use:        send a traces configuration message to the L0 SiTRACES functions
              the Si2151_UserInput_traces function asks the user which type of configuration he wishes.
              Then, memorize the message and calls SiTraceConfiguration.
  Returns:    void
  Porting:    Not compiled if SiTRACES is not defined in Silabs_L0_API.h
************************************************************************************************************************/
void Si2151_UserInput_traces               (void)
{
    char *configuration;
    char *msg;
    configuration = (char*)malloc(sizeof(char)*100);
    msg           = (char*)malloc(sizeof(char)*100);
    printf("Enter the traces configuration string (-<param> ,<value>): ");
    //gets(configuration);
    fgets(configuration, 100, stdin);
    sprintf(msg, "traces %s", configuration);
    printf("%s",SiTraceConfiguration (msg));
}
#endif /* SiTRACES */
/************************************************************************************************************************
  Si_I2C_UserInput_read function
  Use:        i2c read test function
              Used to retrieve user input before an i2c read
  Behavior:   split user input on spaces to find the address of the chip, the index of the first byte to read
              and the number of bytes to read, then perform the read and display the resulting bytes.
  Porting:    This is for i2c verification only.
  **********************************************************************************************************************/
void Si_I2C_UserInput_read                 (L0_Context* i2c)
{
    int i;
    int readBytes;
    int loop;
    int nbArgs;
    int indexSize;
    unsigned int address;
    unsigned int uintval;
    unsigned int hexval;
    char *input;
    char *array[50];
    int  bytes[50];
    unsigned int iI2CIndex;
    int iNbBytes;
    unsigned char *pbtDataBuffer;

    input = (char*) malloc(sizeof(char)*1000);
    printf("i2c read ");
    //gets(input);
    fgets(input, 1000, stdin);

    /* strtok splitting input and storing all items, returning first item */
    array[0] = strtok(input," ");
    if(array[0]==NULL) {return;}
    /* retrieving all remaining items */
    for(loop=1;loop<50;loop++) {
        array[loop] = strtok(NULL," ");
        if(array[loop]==NULL) break;
    }
    nbArgs = loop;
    /* scanning arguments, allowing decimal or hexadecimal input */
    for(loop=0;loop<nbArgs;loop++) {
        hexval=0;
        sscanf(array[loop],"%d", &uintval);
        if (!uintval) sscanf(array[loop],"%x", &hexval);
        bytes[loop] = hexval + uintval;
    }

    address   = bytes[0];
    indexSize = nbArgs - 2;
    if (indexSize <0) return;
    iI2CIndex = 0;
    for (i=1; i<= indexSize; i++) {
        iI2CIndex = iI2CIndex + bytes[i];
    }
    iNbBytes  = bytes[loop-1];
    pbtDataBuffer = (unsigned char*)malloc(sizeof(unsigned char)*iNbBytes);
    L0_SetAddress   (i2c, address, indexSize);
    readBytes = L0_ReadRawBytes (i2c, iI2CIndex, iNbBytes, pbtDataBuffer);
    for (i=0; i<readBytes; i++) { printf ("0x%02x ", pbtDataBuffer[i]); }
    if (readBytes) printf("\n");
}
/************************************************************************************************************************
  Si_I2C_UserInput_write function
  Use:        i2c write test function
              Used to retrieve user input before an i2c write
  Behavior:   split user input on spaces to find the address of the chip and the bytes to write,
              then perform the write.
  Porting:    This is for i2c verification only.
  **********************************************************************************************************************/
void Si_I2C_UserInput_write                (L0_Context* i2c)
{
    int i;
    int writeBytes;
    int loop;
    int nbArgs;
    int indexSize;
    unsigned int address;
    unsigned int uintval;
    unsigned int hexval;
    char *input;
    char *array[50];
    int  bytes[50];
    unsigned int iI2CIndex;
    int iNbBytes;
    unsigned char *pbtDataBuffer;

    input = (char*) malloc(sizeof(char)*1000);
    printf("i2c write ");
    //gets(input);
    fgets(input, 1000, stdin);

    /* strtok splitting input and storing all items, returning first item */
    array[0] = strtok(input," ");
    if(array[0]==NULL) {return;}
    /* retrieving all remaining items */
    for(loop=1;loop<50;loop++) {
        array[loop] = strtok(NULL," ");
        if(array[loop]==NULL) break;
    }
    nbArgs = loop;
    /* scanning arguments, allowing decimal or hexadecimal input */
    for(loop=0;loop<nbArgs;loop++) {
        hexval=0;
        sscanf(array[loop],"%d", &uintval);
        if (!uintval) sscanf(array[loop],"%x", &hexval);
        bytes[loop] = hexval + uintval;
    }

    address   = bytes[0];
    indexSize = 0;
    if (indexSize <0) return;
    iI2CIndex = 0;
    iNbBytes  = nbArgs-1;
    pbtDataBuffer = (unsigned char*)malloc(sizeof(unsigned char)*iNbBytes);
    for (i=0; i<iNbBytes; i++) { pbtDataBuffer[i] = bytes[i+1]; }
    L0_SetAddress   (i2c, address, indexSize);
    writeBytes = L0_WriteRawBytes (i2c, iI2CIndex, iNbBytes, pbtDataBuffer);
    if (writeBytes) {printf("%d bytes written\n", writeBytes);} else {printf("Write error!\n");}
}
/************************************************************************************************************************
  Si2151_UserInputString function
  Use:        retrieve a string entered by the user
  Parameters: text, a pointer to the string buffer
              max_size, the size allocated by the calling function for the text buffer
  Behavior:   Retrieves the string using fgets to avoid overflows. As fgets keeps the 'carriage return' except
                if the string is longer than max_size, replace it (if present) by the terminating character
  Returns:    the length of the string
************************************************************************************************************************/
int  Si2151_UserInputString                (char* text, int max_size) {
  char *newline;
  if ( fgets(text, max_size, stdin) != NULL ) {
    newline = strchr(text, '\n');             /* search for newline character        */
    if ( newline != NULL ) { *newline = '\0'; /* overwrite trailing newline (if any) */ }
  }
  return (int)(strlen(text));
}
/************************************************************************************************************************
  Si2151_trace function
  Use:        trace toggle function
              Used to toggle tracing for all register accesses, at byte level
  Behavior:   Calls the tracking functions for read and write for both demod and tuner
************************************************************************************************************************/
void Si2151_trace                          (L1_Si2151_Context *api) {
    int trackFlag;
    trackFlag = !api->i2c->trackRead;
    L0_TrackWrite(i2c     , trackFlag);
    L0_TrackRead (i2c     , trackFlag);
    L0_TrackWrite(api->i2c, trackFlag);
    L0_TrackRead (api->i2c, trackFlag);
}
#endif /* TUNER_ONLY */
/************************************************************************************************************************
  NAME: Si2151_configure_i2c_communication
  DESCRIPTION:        Setup USB communication with Si2151 EVB
  Parameter:  Pointer to Si2151 Context (I2C address)
  Returns:    nothing
************************************************************************************************************************/
#if 0	//Si2151_configure_i2c_communication multiply defined Si2151_L3_Test.c, change by madianqi,20201015
void Si2151_configure_i2c_communication    (L1_Si2151_Context *api)
{
#ifdef    USB_Capability
    double        retdval;
    char rettxtBuffer[256];
    char *rettxt;
    rettxt = rettxtBuffer;
    L0_Connect (api->i2c, USB);
    L0_Cypress_Configure("-i2c"                 ,"400khz", 0   , &retdval, &rettxt);  /* at 400kHz SCL rate     */
 #ifdef    SILABS_DEMOD_EVB
    L0_Connect (demod->i2c, USB);
 #endif /* SILABS_DEMOD_EVB */
#endif /* USB_Capability */
#ifdef    SILABS_DEMOD_EVB
    L1_WRITE(demod, i2c_passthru, i2c_passthru_enabled);
#endif /* SILABS_DEMOD_EVB */
}
#endif

#ifdef Si2151_COMMAND_LINE_APPLICATION
/************************************************************************************************************************
  NAME: Si2151_DisplayStatusByte
  DESCRIPTION:Read Si2151 STATUS byte and display status bits
  Parameter:  Si2151 Context (I2C address)
  Returns:    Si2151/I2C transaction error code
************************************************************************************************************************/
int  Si2151_DisplayStatusByte              (L1_Si2151_Context *api)
{
    int error_code;

    error_code = Si2151_L1_CheckStatus(api);
    if (error_code)
    {
        SiTRACE("Error %d reading STATUS byte\n",error_code);
        printf ("Error %d reading STATUS byte\n",error_code);
        return error_code;
    }

    printf("STATUS: CTS=%d, ERR=%d, DTVINT=%d, ATVINT=%d, TUNINT=%d\n", api->status->cts, api->status->err, api->status->dtvint, api->status->atvint, api->status->tunint);

    return 0;
}
/************************************************************************************************************************
  NAME : Si2151_GetRevision
  DESCRIPTION:  Execute Si2151 GET_REV function and display response fields
  Parameter:  Si2151 Context (I2C address)
  Returns:    I2C transaction error code
************************************************************************************************************************/
int  Si2151_GetRevision                    (L1_Si2151_Context *api)
{
    char message[1000];

    if (Si2151_L1_GET_REV(api) != NO_Si2151_ERROR)          /* execute GET_REV and always read response */
    {
        printf("Error reading GET_REV\n");
        return ERROR_Si2151_SENDING_COMMAND;
    }
    Si2151_L1_GetCommandResponseString(api, Si2151_GET_REV_CMD_CODE,"\n", message);
    printf("%s\n",message);

    return 0;
}
/************************************************************************************************************************
  NAME: Si2151_TunerStatus
  DESCRIPTION:        Reports tuner status from TUNER_STATUS response
  Parameter:  Pointer to Si2151 Context (I2C address)
  Returns:    I2C transaction error code
************************************************************************************************************************/
int Si2151_TunerStatus (L1_Si2151_Context *Si2151)
{
    char *message=(char *) malloc(BUF_SIZE);

    if (Si2151_L1_TUNER_STATUS(Si2151) != NO_Si2151_ERROR) { /* execute TUNER_STATUS and always read response */
        printf("Error reading TUNER_STATUS\n");
        return ERROR_Si2151_SENDING_COMMAND;
    }
  Si2151_L1_GetCommandResponseString(Si2151, Si2151_TUNER_STATUS_CMD_CODE, "\n", message);
  printf("%s\n",message);/* display results */

    return 0;
}
/************************************************************************************************************************
  NAME: Si2151_GetUserFrequency
  DESCRIPTION: Function to process user input frequency and return an error if invalid.
  Parameter: unsigned long *fIn - returns the user entered frequency in Hz.
  Returns : 1 if an error occurred, 0 otherwise.
************************************************************************************************************************/
int Si2151_GetUserFrequency(unsigned long *fIn)
{
  char entry[MAX_LENGTH];
  fgets(entry,MAX_LENGTH,stdin);
  if (strlen(entry) > 1)
  {
    if (sscanf(entry,"%ld",fIn) < 1)
          { printf ("Error invalid frequency\nPlease press <Enter> to continue\n"); getc(stdin); return 1;}
  }
  else
  { printf ("Error invalid frequency \nPlease press <Enter> to continue\n"); getc(stdin); return 1;}
  SiTRACE("fIn %ld\n",*fIn);
  return 0;
}
/************************************************************************************************************************
  Si2151_help function
  Use:        console application help function
              Used to explain how to init the EVB, tune and scan
************************************************************************************************************************/
int  Si2151_help                           (void) {
    printf("\n\
----------------------------------------------------------------------------\n\
       This is a demonstration application used to illustrate how to use    \n\
            the Si2151 API delivered by Silicon Laboratories                \n\
                                                                            \n\
       It demonstrates a dual front-end case                                \n\
                                                                            \n\
       Most platforms will only have one front-end, therefore               \n\
       the second one will not work correctly                               \n\
                                                                            \n\
                                                                            \n\
    Enter 'help' to display the help.                                       \n\
    Enter 'm'    to display the menu.                                       \n\
                                                                            \n\
----------------------------------------------------------------------------\n");
  return 0;
}
/************************************************************************************************************************
  NAME: Si2151_menu
  DESCRIPTION: Si2151 user menu function
              Used to display the various possibilities offered to the user
  Behavior:   Prints the menu in the console
************************************************************************************************************************/
void Si2151_menu                           (unsigned char full) {
  if (full) {
  printf("\
-----------------------------------------------------------------------\n\
   Console mode Application Menu:\n\
-----------------------------------------------------------------------\n\
 ------  i2c ---------\n\
read           : read bytes from i2c       \n\
write          : write bytes to i2c        \n\
USB            : connect i2c in USB  mode  \n\
CUST           : connect i2c in CUST mode  \n\
trace          : toggle L0 traces\n\
traces         : manage L0 traces (use 'traces' then 'help' for details)\n\
------  Si2151 ------\n\
");
}
  printf("\
InitAndConfig  : Initialize and Configure the Si2151\n\
Status         : Read Si2151 STATUS\n\
GetRevision    : Display revision info\n");
#ifdef USING_ALIF_FILTER
printf ("LoadALIFVidfilt : Load the ALIF videofilter\n");
#endif
#ifdef USING_DLIF_FILTER
printf("LoadDLIFVidfilt : Load the DLIF videofilter\n");
#endif
printf("ATVConfig      : Configure the Properties for ATV mode\n\
DTVConfig      : Configure the Properties for DTV mode\n\
TunerConfig    : Configure the Properties for the Tuner\n\
CommonConfig   : Configure the Common Properties \n\
ATVTune        : Tune an ATV channel\n\
DTVTune        : Tune a DTV channel\n\
TunerStatus    : Display Tuner Status (TC, RSSI, ...)\n\
ChannelScanM   : Do a channel scan for ATV system M\n\
ChannelScanPal : Do a channel scan for ATV system PAL\n\
AGCOverride    : Override the AGC Control\n\
Standby        : Put the Part in Standby Mode\n\
Powerdown      : Powerdown the Part\n\
XOUTOn         : Turn on XOUT\n\
XOUTOff        : Turn off XOUT\n");
if (full) {
  printf(" ------ console -----\n\
help           : display application help\n\
cls            : clear screen\n\
m              : display this menu\n\
exit           : exit application\n\
");
  }
  printf("----------------------------------------------------------------------------\n");
}
/************************************************************************************************************************
  NAME: Si2151_demoLoop
  DESCRIPTION: Demo application loop function
              Used to call all available demod functions, based on the user's entry
  Behavior:   Wait for the user to enter a string, then calls the selected function
************************************************************************************************************************/
int Si2151_demoLoop (L1_Si2151_Context *Si2151, char* choice) {
    char entry[MAX_LENGTH];
    int errcode;
    unsigned long freq;
    unsigned long  minRange, maxRange;
    int i;
    freq = 0;

    sprintf(entry, "%s", choice);

#ifdef    TUNER_ONLY
    if (fe > FRONT_END_COUNT) return 0;
    Si2151 = &(FrontEnd_Table[fe]);

    if (strlen(choice) > 0) {
        SiTRACE("choice '%s'\n",choice);
    } else {
        printf("\n%04d FrontEnd[%d] Command > ", ++commandCount, fe);
        fgets(entry,MAX_LENGTH,stdin);
        if (strlen(entry) > 0) entry[strlen(entry)-1]='\0';
    }

    /* front end selection */
    if ( (strcmp_nocase(entry, "0")==0) & (FRONT_END_COUNT>0) ) { fe = 0; printf ("Controlling FrontEnd %d\n",fe); return 1;}
    if ( (strcmp_nocase(entry, "1")==0) & (FRONT_END_COUNT>1) ) { fe = 1; printf ("Controlling FrontEnd %d\n",fe); return 1;}
    if ( (strcmp_nocase(entry, "2")==0) & (FRONT_END_COUNT>2) ) { fe = 2; printf ("Controlling FrontEnd %d\n",fe); return 1;}
    if ( (strcmp_nocase(entry, "3")==0) & (FRONT_END_COUNT>3) ) { fe = 3; printf ("Controlling FrontEnd %d\n",fe); return 1;}
#endif /* TUNER_ONLY */

    if (strcmp_nocase(entry, "exit"          )==0) { return 0;}
#ifdef    TUNER_ONLY
#ifdef    SiTRACES
    else if (strcmp_nocase(entry, "traces"   )==0) { Si2151_UserInput_traces();}
#endif /* SiTRACES */
    else if (strcmp_nocase(entry, "read"     )==0) { Si_I2C_UserInput_read (i2c);}
    else if (strcmp_nocase(entry, "write"    )==0) { Si_I2C_UserInput_write(i2c);}
    else if (strcmp_nocase(entry, "usb"      )==0) { mode = USB;}
    else if (strcmp_nocase(entry, "cust"     )==0) { mode = CUSTOMER;}
    else if (strcmp_nocase(entry, "simu"     )==0) { mode = SIMU;}
    else if (strcmp_nocase(entry, "trace"    )==0) { Si2151_trace (Si2151); }
#endif /* TUNER_ONLY */
    else if (strcmp_nocase(entry, "InitAndConfig"          )==0)
    {
         if (Si2151_Init(Si2151) == 0 ) {
           Si2151_Configure(Si2151);
         } else {
          SiTRACE("\n");
          SiTRACE("ERROR ----------------------------------\n");
          SiTRACE("ERROR initializing the Si2151 at 0x%02x!\n", Si2151->i2c->address );
          SiTRACE("ERROR ----------------------------------\n");
          SiTRACE("\n");
         }
    }
    else if (strcmp_nocase(entry, "Status"        )==0) { Si2151_DisplayStatusByte     (Si2151); }
    else if (strcmp_nocase(entry, "GetRevision"   )==0) { Si2151_GetRevision           (Si2151); }
#ifdef USING_ALIF_FILTER
    else if (strcmp_nocase(entry, "LoadALIFVidFilt"   )==0) { Si2151_LoadVideofilter(Si2151,ALIF_VIDFILT_TABLE,ALIF_VIDFILT_LINES); }
#endif
#ifdef USING_DLIF_FILTER
    else if (strcmp_nocase(entry, "LoadDLIFVidFilt"   )==0) { Si2151_LoadVideofilter(Si2151,DLIF_VIDFILT_TABLE,DLIF_VIDFILT_LINES); }
#endif
    else if (stricmp(entry, "ATVConfig"     )==0)
    {
         Si2151_setupATVProperties(Si2151);
         printf("Sending Properties to Si2151\n");
         Si2151_downloadATVProperties(Si2151);
         printf("Done \n");
    }
    else if (stricmp(entry, "DTVConfig"     )==0)
    {
         Si2151_setupDTVProperties(Si2151);
         printf("Sending Properties to Si2151\n");
         Si2151_downloadDTVProperties(Si2151);
         printf("Done \n");
     }
    else if (stricmp(entry, "TunerConfig"   )==0)
    {
         Si2151_setupTUNERProperties(Si2151);
         printf("Sending Properties to Si2151\n");
         Si2151_downloadTUNERProperties(Si2151);
         printf("Done \n");
    }
    else if (stricmp(entry, "CommonConfig"  )==0)
    {
         Si2151_setupCOMMONProperties(Si2151);
         printf("Sending Properties to Si2151\n");
         Si2151_downloadCOMMONProperties(Si2151);
         printf("Done \n");
    }
    else if (stricmp(entry, "ATVTune"       )==0)
    {
        printf("ATV Center Frequency (in Hz)? ");

        if ((errcode=Si2151_GetUserFrequency(&freq)) != NO_Si2151_ERROR) {
           SiTRACE ("Si2151_GetUserFrequency error 0x%02x\n", errcode);
           return errcode;
        }
        Si2151_setupProperty(Si2151,Si2151_PropertyIndex(Si2151,"ATV_VIDEO_MODE"));

    #ifdef Si2151_USE_PRETUNE_VCO_BLOCKING_METHOD
         if ((errcode =  Si2151_L2_VCO_Blocking_PreTune( tuners, fe, FRONT_END_COUNT)) != NO_Si2151_ERROR)
         {
             return errcode;
         }
    #endif // Si2151_USE_PRETUNE_VCO_BLOCKING_METHOD
        errcode=Si2151_ATVTune (Si2151, freq,  Si2151->prop->atv_video_mode.video_sys, Si2151->prop->atv_video_mode.invert_spectrum);
        if ((errcode != NO_Si2151_ERROR ) && (errcode != ERROR_Si2151_xTVINT_TIMEOUT))
          { printf ("Error returned from ATVTune error = %s\nPlease press <Enter> to continue\n",Si2151_L1_API_ERROR_TEXT(errcode)); getc(stdin); return 1;}

    #ifdef Si2151_USE_POSTTUNE_VCO_BLOCKING_METHOD
          if ((errcode =  Si2151_L2_VCO_Blocking_PostTune( tuners, fe, FRONT_END_COUNT)) != NO_Si2151_ERROR)
          {
              return errcode;
          }
    #endif // Si2151_USE_POSTTUNE_VCO_BLOCKING_METHOD
  }
  else if (stricmp(entry, "DTVTune"       )==0)
  {
        printf("DTV Center Frequency (in Hz)? ");
        if ((errcode=Si2151_GetUserFrequency(&freq)) != NO_Si2151_ERROR)
           return errcode;
        Si2151_setupProperty(Si2151,Si2151_PropertyIndex(Si2151, "DTV_MODE"));
    #ifdef Si2151_USE_PRETUNE_VCO_BLOCKING_METHOD
         if ((errcode =  Si2151_L2_VCO_Blocking_PreTune( tuners, fe, FRONT_END_COUNT)) != NO_Si2151_ERROR)
         {
             return errcode;
         }
    #endif // Si2151_USE_PRETUNE_VCO_BLOCKING_METHOD

        errcode=Si2151_DTVTune (Si2151, freq, Si2151->prop->dtv_mode.bw, Si2151->prop->dtv_mode.modulation, Si2151->prop->dtv_mode.invert_spectrum);
        if ((errcode != NO_Si2151_ERROR ) && (errcode != ERROR_Si2151_xTVINT_TIMEOUT))
          { printf ("Error returned from DTVTune error = %s\nPlease press <Enter> to continue\n",Si2151_L1_API_ERROR_TEXT(errcode)); getc(stdin); return 1;}

    #ifdef Si2151_USE_POSTTUNE_VCO_BLOCKING_METHOD
          if ((errcode =  Si2151_L2_VCO_Blocking_PostTune( tuners, fe, FRONT_END_COUNT)) != NO_Si2151_ERROR)
          {
              return errcode;
          }
    #endif // Si2151_USE_POSTTUNE_VCO_BLOCKING_METHOD


  }
    else if (stricmp(entry, "TunerStatus"   )==0) { Si2151_TunerStatus(Si2151);}
    else if (stricmp(entry, "CLS"           )==0) { system("cls");}
    else if (stricmp(entry, "XOUTOn"           )==0) { Si2151_XoutOn(Si2151);}
    else if (stricmp(entry, "XOUTOff"           )==0) { Si2151_XoutOff(Si2151);}
    else if (stricmp(entry, "Powerdown"           )==0) { Si2151_Powerdown(Si2151);}
    else if (stricmp(entry, "Standby"           )==0) { Si2151_Standby(Si2151);}

    else if (stricmp(entry, "ChannelScanM"  )==0)
  {
     printf("Frequency Scan Start Hz ? ");
    if ((errcode=Si2151_GetUserFrequency(&minRange)) != NO_Si2151_ERROR)
       return errcode;

    printf("Frequency Scan Stop Hz ? ");
    if ((errcode=Si2151_GetUserFrequency(&maxRange)) != NO_Si2151_ERROR)
       return errcode;

    Si2151_ATV_Channel_Scan_M(Si2151,minRange,maxRange);
    if ((Si2151->ChannelListSize) > 0)
    {
      printf("Channels Found \n");
      for (i=0; i< Si2151->ChannelListSize;++i)
        printf("%d\t%ld\n",i+1,Si2151->ChannelList[i]);
    }
    else
    {
      printf("No Channels Found \n");
    }
  }
  else if (stricmp(entry, "ChannelScanPal"  )==0)
  {
    printf("Frequency Scan Start Hz ? ");
       if ((errcode=Si2151_GetUserFrequency(&minRange)) != NO_Si2151_ERROR)
       return errcode;


    printf("Frequency Scan Stop Hz ? ");
      if ((errcode=Si2151_GetUserFrequency(&maxRange)) != NO_Si2151_ERROR)
       return errcode;


    Si2151_ATV_Channel_Scan_PAL(Si2151,minRange,maxRange);
    if ((Si2151->ChannelListSize) > 0)
    {
      printf("Channels Found \n");
      printf("Num\tType\tChannel \n");
      for (i=0; i< Si2151->ChannelListSize;++i)
        printf("%d\t%s\t%ld\n",i,Si2151->ChannelType[i],Si2151->ChannelList[i]);
    }
    else
    {
      printf("No Channels Found \n");
    }
  }
   else if (stricmp(entry,"AGCOverride"    )==0)
    {
        printf("AGC Override Mode (MAX,TOP,NORMAL)?");
        fgets(entry,MAX_LENGTH,stdin);
        if (strlen(entry) > 0) entry[strlen(entry)-1]='\0';

        if      ( (strcmp_nocase(entry, "MAX")==0) ) { Si2151_AGC_Override(Si2151, Si2151_FORCE_MAX_AGC); }
        else if ( (strcmp_nocase(entry, "TOP")==0) ) { Si2151_AGC_Override(Si2151, Si2151_FORCE_TOP_AGC); }
        else if ( (strcmp_nocase(entry, "NORMAL")==0) ) { Si2151_AGC_Override(Si2151, Si2151_FORCE_NORMAL_AGC); }
        else    { printf ("Error Invalid Choice\nPlease press <Enter> to continue\n"); getc(stdin); return 1;}
    }
    else if (strcmp_nocase(entry, "m"             )==0) { Si2151_menu(1);}
    else if (strcmp_nocase(entry, "help"          )==0) { Si2151_help();}
    else if (strcmp_nocase(entry, "cls"           )==0) { system("cls");}
    else                                          { Si2151_menu(1);}

  return 1;
}
#endif  /* Si2151_COMMAND_LINE_APPLICATION  */

#ifdef    TUNER_ONLY

#if 1
#include "stdio.h"
#include "stdlib.h"
#include "stdarg.h"
//#include "ansidecl.h"

//#include "Platform.h"
//#include "x86dataType.h"
//#include "pfdriver.h"
//#include "pferror.h"

/******************************************************************************
**
**	I2c Global Functions
**
*******************************************************************************/
HANDLE I2c_handle = NULL;
//STCHLIST dwReceivedChFreqList[256];

BYTE CreatePort(void)
{
    return 0;
}

BYTE DestroyPort(void)
{
    printf("================================================\n");
    printf("\nexit Tuner config!\n");
    printf("================================================\n");
    return 0;
}

/*#define RST SetPinAH( ); \
    Sleep( 25 ); \
    SetPinAL( ); \
    Sleep( 60 )
*/

typedef enum
{
    CDTMTV_ATV,
    CDTMTV_DTV,
} BB_SYSTEM;
/************************************************************************************************************************
  NAME: Si2151_Test
  DESCRIPTION:  Main Routine
 ************************************************************************************************************************/
int main(int argc, char *argv[])
{
    UINT8 c;
    UINT8 u8_IIC_port = 0;
    UINT32 err = Error_NO_ERROR;
    char freq[15];FILE *file;

    char bubffer[5] = {0};
	UINT8 TvMode;
	UINT32 FreqOffset;
    /* define the tuner address(es) -- up to 4 */
    int tuner_addr[] = {0xc0,0xc2,0xc4,0xc6};
    /* define the demod address(es) -- only needed to open i2c passthru */
    int demod_addr[] = {0xc8,0xce,0xca,0xcc};
    unsigned char g_u8_mode = Si2151_ATV_VIDEO_MODE_PROP_VIDEO_SYS_B;
    unsigned char g_u8_invert_spectrum = Si2151_ATV_VIDEO_MODE_PROP_INVERT_SPECTRUM_NORMAL;
    // set freq, bw, mode.
    unsigned long u32_freq_hz = 858000000;
    unsigned char u8_bw = Si2151_DTV_MODE_PROP_BW_BW_8MHZ;
    unsigned char u8_mode = Si2151_DTV_MODE_PROP_MODULATION_DVBC;
    unsigned char u8_invert_spectrum = Si2151_DTV_MODE_PROP_INVERT_SPECTRUM_INVERTED;

    SiTraceDefaultConfiguration();
	SiTraceConfiguration("traces -output file -file on -verbose off -function on -line on\n");

    if(CreatePort())
    {
        //system("PAUSE");
        return 1;
    }

    #if 1
    while(1)
	{
		printf("================================================\n");
		printf("Please input blew command!\n");
		printf("c: Select Tuner Addr:0xc0,0xc2,0xc4,0xc6\n");
        printf("a : Select ATV or DTV Mode\n");
        printf("i : Tuner Init\n");
        printf("m : Set TV Standard\n");
        printf("s : Set Frequency\n");
		printf("r : read RSSI vaule\n");
		printf("1 : TUNER_1_0xC0 : StandardDVB-C Bandwitch: 8MHz Freq:474MHz  \n");
		printf("2 : TUNER_2_0xC6 : StandardDVB-T Bandwitch: 8MHz Freq:858MHz  \n");
		printf("================================================\n");	
		c = getinput();

		switch(c)
		{
            case 'r':
            case 'R':
            {
                //unsigned int RSSI=0;
                while(1)
                {

                    Si2151 = &(FrontEnd_Table[0]);
                    Si2151_L1_TUNER_STATUS(Si2151);

                    //RSSI=Si2151->rsp->tuner_status.rssi;
                    system("CLS");
                    system("color 0c");
                    printf ("\r read RSSI=%ddBm",Si2151->rsp->tuner_status.rssi);
                }
                break;
            }
            case 'a':
            case 'A':
            {
                printf("================================================\n");
                printf("1 : Select ATV\n");
                printf("2 : Select DTV\n");
                printf("================================================\n");
                c = getinput();
                switch(c)
                {
                case '1':
                    printf("Select ATV Done\n");
                    TvMode=CDTMTV_ATV;
                    break;
                case '2':
                    printf("Select DTV Done\n");
                    TvMode=CDTMTV_DTV;
                    break;
                }
                break;
            }
            case 'c':
            case 'C':
            {
                printf("================================================\n");
                printf("1 : TUNER_1_0xC0 \n");
                printf("2 : TUNER_0xC2 \n");
                printf("3 : TUNER_0xC4 \n");
                printf("4 : TUNER_2_0xC6 \n\n");
                printf("================================================\n");
                c = getinput();
                switch(c)
                {
                case '1':
                    printf("选择地址: TUNER_1_0xC0 \n");
                    u8_IIC_port=0;
                    break;
                case '2':
                    printf("选择地址: 0xc2\n");
                    u8_IIC_port=1;
                    break;
                case '3':
                    printf("选择地址: 0xc4 \n");
                    u8_IIC_port=2;
                    break;
                case '4':
                    printf("选择地址: TUNER_2_0xC6 \n");
                    u8_IIC_port=3;
                    break;
                }
                break;
            }

            case '1':
            {
                UINT8 reg= 0;
                u8_IIC_port=0;
                i2c_connection_check(I2C_BUS_DEV, tuner_addr[u8_IIC_port]);
                I2cRead(&reg,1);
                printf("0x%02x <<0x%02x\n", tuner_addr[u8_IIC_port]+1, reg);
                if(reg&0x85)
                {
                    /* Software Init */
                    for (fe=0; fe<FRONT_END_COUNT; fe++)
                    {
                        /* Software Init */
                        #ifdef    SILABS_DEMOD_EVB
                            demod = &(demod_table[fe]);
                            demod->i2c = &(demod->i2cObj);
                            demod->i2c->address    = demod_addr[fe];
                            demod->i2c->indexSize  = 2;
                            demod->i2c->trackWrite = 1;
                        #endif /* SILABS_DEMOD_EVB */
                        /* initialize some pointers */
                        front_end = &(FrontEnd_Table[fe]);
                        tuners[fe]= &(FrontEnd_Table[fe]);

                        Si2151_L1_API_Init(front_end, tuner_addr[u8_IIC_port]);
                        if (fe==0)
                        {
                            front_end->cmd->power_up.clock_mode =  Si2151_POWER_UP_CMD_CLOCK_MODE_XTAL;
                            front_end->cmd->power_up.en_xout    =  Si2151_POWER_UP_CMD_EN_XOUT_EN_XOUT;
                            front_end->cmd->config_clocks.clock_mode = Si2151_CONFIG_CLOCKS_CMD_CLOCK_MODE_XTAL;
                        }
                        else
                        {
                            front_end->cmd->power_up.clock_mode = Si2151_POWER_UP_CMD_CLOCK_MODE_EXTCLK;
                            front_end->cmd->power_up.en_xout    = Si2151_POWER_UP_CMD_EN_XOUT_DIS_XOUT;
                            front_end->cmd->config_clocks.clock_mode = Si2151_CONFIG_CLOCKS_CMD_CLOCK_MODE_EXTCLK;
                        }

                        front_end->i2c->trackRead = front_end->i2c->trackWrite = 1;
                        Si2151_configure_i2c_communication  (front_end);

                    }
                    /* If using 2 tuners (or more) startup both tuners using CONFIG_I2C command to broadcast firmware download */
                    #if FRONT_END_COUNT > 1
                    if (Si2151_PowerUpUsingBroadcastI2C(tuners,FRONT_END_COUNT) != NO_Si2151_ERROR )
                    {
                        SiTRACE("\n");
                        SiTRACE("ERROR ----------------------------------\n");
                        SiTRACE("ERROR initializing the Si2151's!\n");
                        SiTRACE("ERROR ----------------------------------\n");
                        SiTRACE("\n");
                    }
                    #else /* use standard init for a single tuner */
                    if (Si2151_Init(&(FrontEnd_Table[0])) != NO_Si2151_ERROR )
                    {
                        SiTRACE("\n");
                        SiTRACE("ERROR ----------------------------------\n");
                        SiTRACE("ERROR initializing the Si2151's!\n");
                        SiTRACE("ERROR ----------------------------------\n");
                        SiTRACE("\n");
                    }
                    else
                    {
                        printf("TUNER_1:Si2151 init Success...\n");
                    }

                    #endif/*#if FRONT_END_COUNT > 1*/
                }
                else
                {
                    printf("TUNER_1: IIC communication fail!!!!!\n\n");
                    goto EXITPROC;
                }

                u8_bw = 8;
                u8_mode = Si2151_DTV_MODE_PROP_MODULATION_DVBC;
                printf ("what freq to acquire? (KHz)\n");
                scanf ("%ld", &u32_freq_hz);
                u32_freq_hz *= 1000;
                printf ("Standard:DVB-C ,Bandwidth:%dMHz  ,Feq:%luHz mode=%d spec=%d\n",u8_bw, u32_freq_hz, u8_mode, u8_invert_spectrum);
                if(Si2151_DTVTune(&(FrontEnd_Table[0]), u32_freq_hz, u8_bw, u8_mode, u8_invert_spectrum) != NO_Si2151_ERROR)
                {
                    printf("TUNER_1:DVB-C ret = %02x\r\n Please Correct the strength of RF Signal!!!\r\n", err);
                }
                else
                {
                    printf("TUNER_1:DVB-C Lock\n");
                }
                break;
            }

            case '2':
            {
                UINT8 reg= 0;
                u8_IIC_port=3;
                i2c_connection_check(I2C_BUS_DEV, tuner_addr[u8_IIC_port]);
                I2cRead(&reg,1);
                printf("0x%02x <<0x%02x\n", tuner_addr[u8_IIC_port]+1, reg);
                if(reg&0x85)
                {
                    /* Software Init */
                    for (fe=0; fe<FRONT_END_COUNT; fe++)
                    {
                        /* Software Init */
                        #ifdef    SILABS_DEMOD_EVB
                            demod = &(demod_table[fe]);
                            demod->i2c = &(demod->i2cObj);
                            demod->i2c->address    = demod_addr[fe];
                            demod->i2c->indexSize  = 2;
                            demod->i2c->trackWrite = 1;
                        #endif /* SILABS_DEMOD_EVB */
                        /* initialize some pointers */
                        front_end = &(FrontEnd_Table[fe]);
                        tuners[fe]= &(FrontEnd_Table[fe]);
                        Si2151_L1_API_Init(front_end, tuner_addr[u8_IIC_port]);
                        if (fe==0)
                        {
                            front_end->cmd->power_up.clock_mode =  Si2151_POWER_UP_CMD_CLOCK_MODE_XTAL;
                            front_end->cmd->power_up.en_xout    =  Si2151_POWER_UP_CMD_EN_XOUT_EN_XOUT;
                            front_end->cmd->config_clocks.clock_mode = Si2151_CONFIG_CLOCKS_CMD_CLOCK_MODE_XTAL;
                        }
                        else
                        {
                            front_end->cmd->power_up.clock_mode = Si2151_POWER_UP_CMD_CLOCK_MODE_EXTCLK;
                            front_end->cmd->power_up.en_xout    = Si2151_POWER_UP_CMD_EN_XOUT_DIS_XOUT;
                            front_end->cmd->config_clocks.clock_mode = Si2151_CONFIG_CLOCKS_CMD_CLOCK_MODE_EXTCLK;
                        }

                        front_end->i2c->trackRead = front_end->i2c->trackWrite = 1;
                        Si2151_configure_i2c_communication  (front_end);

                    }
                    /* If using 2 tuners (or more) startup both tuners using CONFIG_I2C command to broadcast firmware download */
                    #if FRONT_END_COUNT > 1

                    if (Si2151_PowerUpUsingBroadcastI2C(tuners,FRONT_END_COUNT) != NO_Si2151_ERROR )
                    {
                        SiTRACE("\n");
                        SiTRACE("ERROR ----------------------------------\n");
                        SiTRACE("ERROR initializing the Si2151's!\n");
                        SiTRACE("ERROR ----------------------------------\n");
                        SiTRACE("\n");
                    }

                    #else /* use standard init for a single tuner */
                    if (Si2151_Init(&(FrontEnd_Table[0])) != NO_Si2151_ERROR )
                    {
                        SiTRACE("\n");
                        SiTRACE("ERROR ----------------------------------\n");
                        SiTRACE("ERROR initializing the Si2151's!\n");
                        SiTRACE("ERROR ----------------------------------\n");
                        SiTRACE("\n");
                    }
                    else
                    {
                        printf("TUNER_2:Si2151 init success...\n");
                    }

                    #endif/*#if FRONT_END_COUNT > 1*/
                }
                else
                {
                    printf("TUNER_2:IIC communication fail!!!!!\n\n");
                    goto EXITPROC;
                }

                u8_bw = 8;
                u8_mode = Si2151_DTV_MODE_PROP_MODULATION_DVBT;
                //scanf ("%d", &u32_freq_hz);
                u32_freq_hz=858000;
                u32_freq_hz *= 1000;
                //printf ("standard:DVB-C ,Bandwitch:%dMHz  ,Freq:%dMHz\n",u8_bw,u32_freq_hz/1000000);
                if(Si2151_DTVTune(&(FrontEnd_Table[0]), u32_freq_hz, u8_bw, u8_mode, u8_invert_spectrum) != NO_Si2151_ERROR)
                {
                    printf("TUNER_2:DVB-T ret = %02x\r\nPlease Correct the Strength of RF Signal!!!\r\n", err);
                }
                else
                {
                    printf("TUNER_2:DVB-T lock\n");
                }
                break;
            }

            case 'i':
            case 'I':
            {
                UINT8 reg= 0,u8cnt=0;
                //I2cRead(tuner_addr[u8_IIC_port], &reg,1);
                //printf("0x%02x <<0x%02x\n", tuner_addr[u8_IIC_port]+1, reg);
                while(1)
                {
                    reg=0;
                    I2cRead(&reg,1);
                    //printf("\r0x%02x <<0x%02x", tuner_addr[u8_IIC_port]+1, reg);
                    if(reg&0x80)
                    {
                        u8cnt++;
                    }
                    else
                    {
                        u8cnt=0;
                    }
                    if(u8cnt>3)
                    {
                        break;
                    }
                }
                printf("\r0x%02x <<0x%02x", tuner_addr[u8_IIC_port]+1, reg);
                if(reg&0x80)
                {
                    printf("\n");
                    /* Software Init */
                    for (fe=0; fe<FRONT_END_COUNT; fe++)
                    {
                        /* Software Init */
                        #ifdef    SILABS_DEMOD_EVB
                            demod = &(demod_table[fe]);
                            demod->i2c = &(demod->i2cObj);
                            demod->i2c->address    = demod_addr[fe];
                            demod->i2c->indexSize  = 2;
                            demod->i2c->trackWrite = 1;
                        #endif /* SILABS_DEMOD_EVB */
                        /* initialize some pointers */
                        front_end = &(FrontEnd_Table[fe]);
                        tuners[fe]= &(FrontEnd_Table[fe]);

                        Si2151_L1_API_Init(front_end, tuner_addr[u8_IIC_port]);
                        if (fe==0)
                        {
                            front_end->cmd->power_up.clock_mode =  Si2151_POWER_UP_CMD_CLOCK_MODE_XTAL;
                            front_end->cmd->power_up.en_xout    =  Si2151_POWER_UP_CMD_EN_XOUT_EN_XOUT;
                            front_end->cmd->config_clocks.clock_mode = Si2151_CONFIG_CLOCKS_CMD_CLOCK_MODE_XTAL;
                        }
                        else
                        {
                            front_end->cmd->power_up.clock_mode = Si2151_POWER_UP_CMD_CLOCK_MODE_EXTCLK;
                            front_end->cmd->power_up.en_xout    = Si2151_POWER_UP_CMD_EN_XOUT_DIS_XOUT;
                            front_end->cmd->config_clocks.clock_mode = Si2151_CONFIG_CLOCKS_CMD_CLOCK_MODE_EXTCLK;
                        }

                        front_end->i2c->trackRead = front_end->i2c->trackWrite = 1;
                        Si2151_configure_i2c_communication  (front_end);

                    }
                    /* If using 2 tuners (or more) startup both tuners using CONFIG_I2C command to broadcast firmware download */
                    #if FRONT_END_COUNT > 1

                    if (Si2151_PowerUpUsingBroadcastI2C(tuners,FRONT_END_COUNT) != NO_Si2151_ERROR )
                    {
                        SiTRACE("\n");
                        SiTRACE("ERROR ----------------------------------\n");
                        SiTRACE("ERROR initializing the Si2151's!\n");
                        SiTRACE("ERROR ----------------------------------\n");
                        SiTRACE("\n");
                    }

                    #else /* use standard init for a single tuner */
                    if (Si2151_Init(&(FrontEnd_Table[0])) != NO_Si2151_ERROR )
                    {
                        SiTRACE("\n");
                        SiTRACE("ERROR ----------------------------------\n");
                        SiTRACE("ERROR initializing the Si2151's!\n");
                        SiTRACE("ERROR ----------------------------------\n");
                        SiTRACE("\n");
                    }
                    else
                    {
                        printf("================================================\n");
                        printf("Si2151 init success...\n");
                        printf("================================================\n");
                    }

                    #endif/*#if FRONT_END_COUNT > 1*/
                }
                else
                {
                        printf("================================================\n");
                        printf("IIC communication fail!!!!!\n\n");
                        printf("================================================\n");

                }
                break;
            }
            case 'm':
            case 'M':
            {
                if(TvMode==CDTMTV_ATV)
                {
                    printf("=============ATV===================================\n");
                    printf("1:NTSC-M\n");
                    printf("2:PAL-M\n");
                    printf("3:PAL-N\n");
                    printf("4:PAL-G\n");
                    printf("5:PAL-GH\n");
                    printf("6:PAL-I\n");
                    printf("5:PAL-DK\n");
                    printf("6:SECAM-L\n");
                    printf("================================================\n");
                    c = getinput();
                    switch ( c )
                    {
                    case '1':
                        //偏频参与tda8296芯片
                        //tmbslTDA8296SetStandardMode(0,tmTDA8296StandardMN);
                        FreqOffset = 760000;   //(5.76MHz 2017.05.17)
                        g_u8_mode = Si2151_ATV_VIDEO_MODE_PROP_VIDEO_SYS_M;
                        printf("Standard: NTSC-M\n");
                        break;
                    case '2' :
                        FreqOffset = 400000;  // 5.4Mhz
                        g_u8_mode = Si2151_ATV_VIDEO_MODE_PROP_VIDEO_SYS_M;
                        printf("Standard: PAL-M\n");
                        break;
                    case '3':

                        FreqOffset = 400000;  // 5.4Mhz
                        g_u8_mode = Si2151_ATV_VIDEO_MODE_PROP_VIDEO_SYS_N;
                        printf("Standard: PAL-N\n");
                        break;
                    case '4' :
                        FreqOffset = 1400000;  // 6.4Mhz
                        g_u8_mode = Si2151_ATV_VIDEO_MODE_PROP_VIDEO_SYS_B;
                        printf("Standard: PAL-BG\n");

                        break;
                    case '5':
                        FreqOffset = 1750000;  // 6.75HMz
                        g_u8_mode = Si2151_ATV_VIDEO_MODE_PROP_VIDEO_SYS_GH;
                        printf("Standard: PAL-BG\n");
                        break;
                    case '6' :

                        FreqOffset = 2250000;  // 7.25Mh
                        g_u8_mode = Si2151_ATV_VIDEO_MODE_PROP_VIDEO_SYS_I;
                        printf("Standard: PAL-I\n");
                        break;
                    case '7' :
                        FreqOffset = 1850000;  // 6.85Mhz
                        g_u8_mode = Si2151_ATV_VIDEO_MODE_PROP_VIDEO_SYS_DK;
                        printf("Standard: PAL-DK\n");

                        break;
                    case '8' :
                        FreqOffset = 2750000;
                        g_u8_mode = Si2151_ATV_VIDEO_MODE_PROP_VIDEO_SYS_L;
                        printf("Standard: SECAM-L\n");
                        break;
                    default:
                        break;
                    }
                }
                else
                {
                    printf("================================================\n");
                    printf("1:DVB-C   8MHz\n");
                    printf("2:DVB-T   7MHz\n");
                    printf("3:DVB-T   8MHz\n");
                    printf("4:ISDB-T  8MHz\n");
                    printf("5:DTMB    8MHz\n");
                    printf("6:ATSC    6MHz\n");
                    printf("================================================\n");
                    c = getinput();
                    switch (c)
                    {
                    case '1':
                    {
                        u8_bw = 8;
                        u8_mode = Si2151_DTV_MODE_PROP_MODULATION_DVBC;
                        printf("-->DVB-C 8MHz\n");
                        break;
                    }
                    case '2':
                    {
                        u8_bw = 7;
                        u8_mode = Si2151_DTV_MODE_PROP_MODULATION_DVBT;
                        printf("-->DVB-T 7MHz\n");
                        break;
                    }
                    case '3':
                    {
                        u8_bw = 8;
                        u8_mode = Si2151_DTV_MODE_PROP_MODULATION_DVBT;
                        printf("-->DVB-T 8MHz\n");
                        break;
                    }
                    case '4':
                    {
                        u8_bw = 8;
                        u8_mode = Si2151_DTV_MODE_PROP_MODULATION_ISDBT;
                        printf("-->ISDB-T 8MHz\n");
                        break;
                    }
                    case '5':
                    {
                        u8_bw = 8;
                        u8_mode = Si2151_DTV_MODE_PROP_MODULATION_DTMB;
                        printf("-->DTMB  8MHz\n");
                        break;
                    }
                    case '6':
                    {
                        u8_bw = 6;
                        u8_mode = Si2151_DTV_MODE_PROP_MODULATION_ATSC;
                        printf("-->ATSC  6MHz\n");
                        break;
                    }
                    }
                }
                break;
            }
            case 's':
            case 'S':
            {
                if(TvMode==CDTMTV_ATV)
                {
                    printf ("Please input Frequency(KHz):");
                    scanf ("%ld", &u32_freq_hz);
                    u32_freq_hz *= 1000;
                    printf ("standard:%d, freq:%ldHz invert=%d\n", g_u8_mode, u32_freq_hz  + FreqOffset, g_u8_invert_spectrum);
                    if(Si2151_ATVTune (front_end, u32_freq_hz  + FreqOffset, g_u8_mode,g_u8_invert_spectrum))
                    {
                        printf("ATV lock fail\n");
                    }
                    else
                    {
                        printf("ATV lock success!\n");
                    }
                }
                else
                {
                    printf ("Please input Frequency(KHz):");
                    scanf ("%ld", &u32_freq_hz);
                    u32_freq_hz *= 1000;
                    //printf ("制式:DVB-C ,带宽:%dMHz  ,频率:%dMHz\n",u8_bw,u32_freq_hz/1000000);
                    if(Si2151_DTVTune(&(FrontEnd_Table[0]), u32_freq_hz, u8_bw, u8_mode, u8_invert_spectrum) != NO_Si2151_ERROR)
                    {
                        printf("DTV lock fail!\n");
                    }
                    else
                    {
                        printf("DTV lock success!\n");
                    }
                }
                break;
            }

            case 'x':
            case 'X':
            {
                goto EXITPROC;
                break;
            }
            case 0xe0:// JUST FOR TEST
            {
                c = getinput();
                switch (c)
                {
                    case 0x48:
                    {
                        if(u8_bw == Si2151_DTV_MODE_PROP_BW_BW_8MHZ)
                        {
                            u32_freq_hz += 8000000;
                        }
                        else if(u8_bw == Si2151_DTV_MODE_PROP_BW_BW_7MHZ)
                        {
                            u32_freq_hz += 7000000;
                        }
                        Si2151_DTVTune(&(FrontEnd_Table[0]), u32_freq_hz, u8_bw, u8_mode, u8_invert_spectrum);
                        break;
                    }
                    case 0x50:
                    {
                        if(u8_bw == Si2151_DTV_MODE_PROP_BW_BW_8MHZ)
                        {
                            u32_freq_hz -= 8000000;
                        }
                        else if(u8_bw == Si2151_DTV_MODE_PROP_BW_BW_7MHZ)
                        {
                            u32_freq_hz -= 7000000;
                        }
                        Si2151_DTVTune(&(FrontEnd_Table[0]), u32_freq_hz, u8_bw, u8_mode, u8_invert_spectrum);
                        break;
                    }
                    case 0x4b:
                    {
                        break;
                    }
                    case 0x4d:
                    {
                        break;
                    }

                    default:
                        break;
                }
                break;
            }
            default:
            {
                break;
            }
		}
    }
    #else
    UINT8 reg= 0;
    UINT8 u8cnt = 0;
    UINT8 u8Ret = 0;
    UINT32 u32testCount = 0;
    while(1)
    {
        RST;
        //printf("\n");
        while(1)
        {
            reg=0;
            I2cRead(tuner_addr[u8_IIC_port], &reg,1);
	        //printf("\r0x%02x <<0x%02x", tuner_addr[u8_IIC_port]+1, reg);
            if(reg&0x80)
            {
                u8cnt++;
            }
            else
            {
                u8cnt=0;
            }
            if(u8cnt>3)
            {
                break;
            }
            mdelay(5);//Sleep(5);
        }
        //printf("\n");
        fe = 0;
        /* initialize some pointers */
        front_end = &(FrontEnd_Table[fe]);
        tuners[fe]= &(FrontEnd_Table[fe]);

        Si2151_L1_API_Init(front_end, tuner_addr[fe]);
        if (fe==0)
        {
          front_end->cmd->power_up.clock_mode =  Si2151_POWER_UP_CMD_CLOCK_MODE_XTAL;
          front_end->cmd->power_up.en_xout    =  Si2151_POWER_UP_CMD_EN_XOUT_EN_XOUT;
          front_end->cmd->config_clocks.clock_mode = Si2151_CONFIG_CLOCKS_CMD_CLOCK_MODE_XTAL;
        }
        else
        {
          front_end->cmd->power_up.clock_mode = Si2151_POWER_UP_CMD_CLOCK_MODE_EXTCLK;
          front_end->cmd->power_up.en_xout    = Si2151_POWER_UP_CMD_EN_XOUT_DIS_XOUT;
          front_end->cmd->config_clocks.clock_mode = Si2151_CONFIG_CLOCKS_CMD_CLOCK_MODE_EXTCLK;
        }

        front_end->i2c->trackRead = front_end->i2c->trackWrite = 0;
        Si2151_configure_i2c_communication  (front_end);
        
        printf("=============================>\n");
        /* If using 2 tuners (or more) startup both tuners using CONFIG_I2C command to broadcast firmware download */
        if (Si2151_Init(&(FrontEnd_Table[0])) != NO_Si2151_ERROR )
        {
            SiTRACE("\n");
            SiTRACE("ERROR ----------------------------------\n");
            SiTRACE("ERROR initializing the Si2151's!\n");
            SiTRACE("ERROR ----------------------------------\n");
            SiTRACE("\n");
        }
        else
        {
            printf("Si2151_锟斤拷始锟斤拷锟缴癸拷...\n");
        }
        
        switch(u8_mode)
        {
            case Si2151_DTV_MODE_PROP_MODULATION_DVBT:
            {
                printf("Mode: DVBT\n");
                break;
            }
            case Si2151_DTV_MODE_PROP_MODULATION_ATSC:
            {
                printf("Mode: ATSC\n");
                break;
            }
            case Si2151_DTV_MODE_PROP_MODULATION_DVBC:
            {
                printf("Mode: DVBC\n");
                break;
            }
            case Si2151_DTV_MODE_PROP_MODULATION_ISDBT:
            {
                printf("Mode: ISDBT\n");
                break;
            }
            case Si2151_DTV_MODE_PROP_MODULATION_ISDBC:
            {
                printf("Mode: ISDBC\n");
                break;
            }
            case Si2151_DTV_MODE_PROP_MODULATION_DTMB:
            {
                printf("Mode: DTMB\n");
                break;
            }
        }
        printf("Freq: %d Hz\n", u32_freq_hz);
        printf("BW: %d\n", u8_bw);
       
        if(Si2151_DTVTune(&(FrontEnd_Table[0]), u32_freq_hz, u8_bw, u8_mode, u8_invert_spectrum) != NO_Si2151_ERROR)
        {
            printf("Error returned from DTVTune error = %02x\r\nPlease check RF!!!\r\n", err);
        }
        else
        {
            u32testCount ++ ;
            printf("锟斤拷锟斤拷锟斤拷锟街碉拷锟接碉拷谐锟缴癸拷!\n");
        }
        printf("锟斤拷锟斤拷锟斤拷锟絓n锟斤拷锟斤拷:%d\n锟诫换锟斤拷.\n", u32testCount);
        printf("<=============================\n");
                        
        while(1)
        {
            reg=0;
            I2cRead(tuner_addr[u8_IIC_port], &reg,1);
	        //printf("\r0x%02x <<0x%02x", tuner_addr[u8_IIC_port]+1, reg);
            if(reg&0x80)
            {
                u8cnt=0;
            }
            else
            {
                u8cnt++;
            }
            if(u8cnt>3)
            {
                break;
            }
            mdelay(5);//Sleep(5);
        }

    }
    #endif

EXITPROC:
    err = DestroyPort();
    printf("err = %x\n", err);
    //system("PAUSE");
    return err;
}
#endif /* end Si2151_Test */
#endif /* TUNER_ONLY */

void connect_test(void)
{

    UINT8 u8_IIC_port = 0;
    /* define the tuner address(es) -- up to 4 */
    int tuner_addr[] = {0xc0,0xc2,0xc4,0xc6};

    UINT8 reg0= 0, u8cnt=0, reg1[] = {0x55, 0xAA, 0x55, 0xAA};
    UINT8 reg2[4]={0};
    u8_IIC_port = 0;

    i2c_connection_check(I2C_BUS_DEV, tuner_addr[u8_IIC_port]);
    I2cRead(&reg0,1);
    I2cRead(reg2, 16);

    I2cWrite(reg1, 4);
    I2cWrite(reg1, 4);
    I2cWrite(reg1, 4);
    I2cRead(reg2, 4);

    I2cRead(&reg0,1);
    printf("0x%02x <<0x%02x\n", tuner_addr[u8_IIC_port]+1, reg0);

}



