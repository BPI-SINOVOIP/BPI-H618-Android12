/***************************************************************************************
                  Silicon Laboratories Broadcast Si2151 Layer 2 API

   EVALUATION AND USE OF THIS SOFTWARE IS SUBJECT TO THE TERMS AND CONDITIONS OF
     THE SOFTWARE LICENSE AGREEMENT IN THE DOCUMENTATION FILE CORRESPONDING
     TO THIS SOURCE FILE.
   IF YOU DO NOT AGREE TO THE LIMITED LICENSE AND CONDITIONS OF SUCH AGREEMENT,
     PLEASE RETURN ALL SOURCE FILES TO SILICON LABORATORIES.

   L2 API for commands and properties
   FILE: Si2151_L2_API.c
   Supported IC : Si2151
   Compiled for ROM 61 firmware 1_2_build_2
   Revision: 0.1
   Tag:  ROM61_1_2_build_2_V0.1
   Date: October 23 2015
  (C) Copyright 2014, Silicon Laboratories, Inc. All rights reserved.
****************************************************************************************/
#include <string.h>
/* Si2151 API Defines */
 /* define this if using the ALIF video filter */
#undef USING_ALIF_FILTER
/* define this if using the DTV video filter */
#undef USING_DLIF_FILTER
/************************************************************************************************************************/
/* Si2151 API Specific Includes */
#include "Si2151_L2_API.h"               /* Include file for this code */
//#include "Si2151_firmware_1_2_build_2.h"       /* firmware compatible with Si2151-A10 */
#include "Si2151_firmware_1_2_build_2.h"
#include "Si2151_firmware_0_A_build_23.h"      /* firmware compatible for Si2151-A0A (ROM60) */
#define Si2151_BYTES_PER_LINE 8
 #ifdef    USING_ALIF_FILTER
#include "write_ALIF_video_coeffs.h"   /* .h file from custom Video filter Tool output */
#endif /* USING_ALIF_FILTER */
#ifdef USING_DLIF_FILTER
#include "write_DLIF_video_coeffs.h"   /* .h file from custom Video filter Tool output */
#endif

/* define the following token if using >1 tuner and control1ing vco settings via the demod wrappper */
#undef Si2151_DEMOD_WRAPPER_VCO
/* Number of times the powerup command will be attempted before generating an error */
#define RESET_HARDWARE_POWERUP_ATTEMPTS 10
/************************************************************************************************************************
  NAME: Si2151_Configure
  DESCRIPTION: Setup Si2151 video filters, GPIOs/clocks, Common Properties startup, etc.
  Parameter:  Pointer to Si2151 Context
  Returns:    I2C transaction error code, NO_Si2151_ERROR if successful
************************************************************************************************************************/
int Si2151_Configure           (L1_Si2151_Context *api)
{
    int return_code;
    return_code = NO_Si2151_ERROR;

 #ifdef    USING_ALIF_FILTER
    if ((return_code = Si2151_LoadVideofilter(api,ALIF_VIDFILT_TABLE,ALIF_VIDFILT_LINES)) != NO_Si2151_ERROR)
       return return_code;
  #endif /* USING_ALIF_FILTER */
 /* load DTV video filter file */
  #ifdef USING_DLIF_FILTER
     if ((return_code = Si2151_LoadVideofilter(api,DLIF_VIDFILT_TABLE,DLIF_VIDFILT_LINES)) != NO_Si2151_ERROR)
       return return_code;
  #endif
  printf("09876565131\n");
    /* Edit the Si2151_storeUserProperties if you have any properties settings different from the standard defaults */
    Si2151_storeUserProperties     (api->prop);

    /* Download properties different from 'propShadow' if api->propertyWriteMode = Si2151_DOWNLOAD_ON_CHANGE */
    /* Download properties in any case                 if api->propertyWriteMode = Si2151_DOWNLOAD_ALWAYS    */
    Si2151_downloadAllProperties(api);

    return return_code;
}
/************************************************************************************************************************
  NAME: Si2151_PowerUpOnly
  DESCRIPTION: Send Si2151 API PowerUp sequence with PowerUp to bootloader,
  This was created to handle an issue if a PART_INFO error occurs this will reset the part.

  Parameter:  pointer to Si2151 Context
  Returns:    Si2151/I2C transaction error code, NO_Si2151_ERROR if successful
************************************************************************************************************************/
int Si2151_PowerUpOnly    (L1_Si2151_Context *api)
{
    int return_code;
    char err_count;
    return_code = NO_Si2151_ERROR;
    err_count=0;
    /* always wait for CTS prior to POWER_UP command */
    if ((return_code = Si2151_pollForCTS  (api)) != NO_Si2151_ERROR) {
        SiTRACE ("Si2151_pollForCTS error 0x%02x\n", return_code);
        return return_code;
    }

    do {
            if ((return_code = Si2151_L1_RESET_HW(api,
                                                  Si2151_RESET_HW_CMD_SUBCODE_CODE,
                                                  Si2151_RESET_HW_CMD_RESERVED1_RESERVED,
                                                  Si2151_RESET_HW_CMD_RESERVED2_RESERVED)) != NO_Si2151_ERROR)
            {
                SiTRACE ("Si2151_L1_RESET_HW error 0x%02x: %s\n", return_code, Si2151_L1_API_ERROR_TEXT(return_code) );
                return return_code;
            }

            if ((return_code = Si2151_L1_POWER_UP (api,
                                    Si2151_POWER_UP_CMD_SUBCODE_CODE,
                                    api->cmd->power_up.clock_mode,
                                    api->cmd->power_up.en_xout,
                                    Si2151_POWER_UP_CMD_CONFIG_1P8V_INTERNAL_REG,
                                    Si2151_POWER_UP_CMD_RESERVED3_RESERVED,
                                    Si2151_POWER_UP_CMD_RESERVED4_RESERVED,
                                    Si2151_POWER_UP_CMD_RESERVED5_RESERVED,
                                    Si2151_POWER_UP_CMD_RESERVED6_RESERVED,
                                    Si2151_POWER_UP_CMD_RESERVED7_RESERVED,
                                    Si2151_POWER_UP_CMD_RESERVED8_RESERVED
                                    )) != NO_Si2151_ERROR)
            {
                SiTRACE ("Si2151_L1_POWER_UP error 0x%02x: %s\n", return_code, Si2151_L1_API_ERROR_TEXT(return_code) );
                ++err_count;
                if (err_count > RESET_HARDWARE_POWERUP_ATTEMPTS)
                {
                    return return_code;
                }
            }

    } while (Si2151_GetStatus(api) == 0xFE);
      if ((return_code = Si2151_L1_WAKE_UP (api,
                                Si2151_WAKE_UP_CMD_SUBCODE_CODE,
                                Si2151_WAKE_UP_CMD_RESET_RESET,
                                Si2151_WAKE_UP_CMD_CLOCK_FREQ_CLK_24MHZ,
                                Si2151_WAKE_UP_CMD_RESERVED9_RESERVED,
                                Si2151_WAKE_UP_CMD_FUNC_BOOTLOADER,
                                Si2151_WAKE_UP_CMD_WAKE_UP_WAKE_UP
                                )) != NO_Si2151_ERROR)
        {
            SiTRACE ("Si2151_L1_WAKE_UP error 0x%02x: %s\n", return_code, Si2151_L1_API_ERROR_TEXT(return_code) );
            return return_code;
        }
    return return_code;
}

/************************************************************************************************************************
  NAME: Si2151_PowerUpWithPatch
  DESCRIPTION: Send Si2151 API PowerUp Command with PowerUp to bootloader,
  Check the Chip rev and part, and ROMID are compared to expected values.
  Load the Firmware Patch then Start the Firmware.
  Programming Guide Reference:    Flowchart A.2 (POWER_UP with patch flowchart)

  Parameter:  pointer to Si2151 Context
  Returns:    Si2151/I2C transaction error code, NO_Si2151_ERROR if successful
************************************************************************************************************************/
int Si2151_PowerUpWithPatch    (L1_Si2151_Context *api)
{
    int return_code;
    char err_count;

    return_code = NO_Si2151_ERROR;
    err_count=0;
    if (!(api->load_control & SKIP_POWERUP))
    {
    /* always wait for CTS prior to POWER_UP command */
        if ((return_code = Si2151_pollForCTS  (api)) != NO_Si2151_ERROR)
       {
            SiTRACE ("Si2151_pollForCTS error 0x%02x\n", return_code);
            return return_code;
        }

    do {
            if ((return_code = Si2151_L1_RESET_HW(api,
                                                  Si2151_RESET_HW_CMD_SUBCODE_CODE,
                                                  Si2151_RESET_HW_CMD_RESERVED1_RESERVED,
                                                  Si2151_RESET_HW_CMD_RESERVED2_RESERVED)) != NO_Si2151_ERROR)
            {
                SiTRACE ("Si2151_L1_RESET_HW error 0x%02x: %s\n", return_code, Si2151_L1_API_ERROR_TEXT(return_code) );
                return return_code;
            }

            if ((return_code = Si2151_L1_POWER_UP (api,
                                    Si2151_POWER_UP_CMD_SUBCODE_CODE,
                                    api->cmd->power_up.clock_mode,
                                    api->cmd->power_up.en_xout,
                                    Si2151_POWER_UP_CMD_CONFIG_1P8V_INTERNAL_REG,
                                    Si2151_POWER_UP_CMD_RESERVED3_RESERVED,
                                    Si2151_POWER_UP_CMD_RESERVED4_RESERVED,
                                    Si2151_POWER_UP_CMD_RESERVED5_RESERVED,
                                    Si2151_POWER_UP_CMD_RESERVED6_RESERVED,
                                    Si2151_POWER_UP_CMD_RESERVED7_RESERVED,
                                    Si2151_POWER_UP_CMD_RESERVED8_RESERVED
                                    )) != NO_Si2151_ERROR)
            {
                SiTRACE ("Si2151_L1_POWER_UP error 0x%02x: %s\n", return_code, Si2151_L1_API_ERROR_TEXT(return_code) );
                ++err_count;
                if (err_count > RESET_HARDWARE_POWERUP_ATTEMPTS)
                {
                    return return_code;
                }
            }

     } 
    while (Si2151_GetStatus(api) == 0xFE);

        if ((return_code = Si2151_L1_WAKE_UP (api,
                                Si2151_WAKE_UP_CMD_SUBCODE_CODE,
                                Si2151_WAKE_UP_CMD_RESET_RESET,
                                Si2151_WAKE_UP_CMD_CLOCK_FREQ_CLK_24MHZ,
                                Si2151_WAKE_UP_CMD_RESERVED9_RESERVED,
                                Si2151_WAKE_UP_CMD_FUNC_BOOTLOADER,
                                Si2151_WAKE_UP_CMD_WAKE_UP_WAKE_UP
                                )) != NO_Si2151_ERROR)
        {
            SiTRACE ("Si2151_L1_WAKE_UP error 0x%02x: %s\n", return_code, Si2151_L1_API_ERROR_TEXT(return_code) );
            return return_code;
        }

    /* Get the Part Info from the chip.   This command is only valid in Bootloader mode */
    if ((return_code = Si2151_L1_PART_INFO(api)) !=NO_Si2151_ERROR )//NO_Si2151_ERROR
    {
        SiTRACE ("Si2151_L1_PART_INFO error 0x%02x: %s\n", return_code, Si2151_L1_API_ERROR_TEXT(return_code) );
        /* if there is an error at this point retry the powerup sequence and execute the PART_INFO command again */
        if ((return_code = Si2151_PowerUpOnly(api)) == NO_Si2151_ERROR)
        {
        
           if ((return_code = Si2151_L1_PART_INFO(api)) != NO_Si2151_ERROR)
            {
                SiTRACE ("Si2151_L1_PART_INFO error 0x%02x: %s\n", return_code, Si2151_L1_API_ERROR_TEXT(return_code) );
                return return_code;
            }
        }
       else
       {
       
        SiTRACE ("Si2151_PowerUpOnly error 0x%02x: %s\n", return_code, Si2151_L1_API_ERROR_TEXT(return_code) );
        return return_code;
       }
    }
#if 1
    printf("chiprev %d\n",        api->rsp->part_info.chiprev);
    printf("part    Si21%d\n",    api->rsp->part_info.part   );
    printf("part2   '%c'\n",        api->rsp->part_info.part2  );
    printf("romid   %d\n",         api->rsp->part_info.romid  );
    printf("pmajor  %d\n",        api->rsp->part_info.pmajor );
    printf("pminor  %d\n",         api->rsp->part_info.pminor );
    printf("pbuild  %d\n",           api->rsp->part_info.pbuild );
	
#endif
    if ((api->rsp->part_info.pmajor >= 0x30) & (api->rsp->part_info.pminor >= 0x30)) 
{
    //printf("pmajor  '%c'\n",    api->rsp->part_info.pmajor );
    //printf("pminor  '%c'\n",    api->rsp->part_info.pminor );
    //printf("Full Info       'Si21%02d-%c%c%c ROM%x NVM%c_%cb%d'\n\n\n", api->rsp->part_info.part, api->rsp->part_info.chiprev + 0x40, api->rsp->part_info.pmajor, api->rsp->part_info.pminor, api->rsp->part_info.romid, api->rsp->part_info.pmajor, api->rsp->part_info.pminor, api->rsp->part_info.pbuild );
  }
  }
     if (!(api->load_control & SKIP_LOADFIRMWARE))
     {
        /* Check part info values and load the proper firmware */
          if (api->rsp->part_info.romid == 0x61)
          {
             printf("下载固件Si2151_FW_1_2b2\n");
             if ((return_code = Si2151_LoadFirmware_16(api, Si2151_FW_1_2b2, FIRMWARE_LINES_1_2b2)) != NO_Si2151_ERROR) {
               SiTRACE ("Si2151_LoadFirmware error 0x%02x: %s\n", return_code, Si2151_L1_API_ERROR_TEXT(return_code) );
               return return_code;
             }
          }

          else if (api->rsp->part_info.romid == 0x60)
          {
               //printf("下载固件Si2151_FW_0_Ab23\n");
             if ((return_code = Si2151_LoadFirmware_16(api, Si2151_FW_0_Ab23, FIRMWARE_LINES_0_Ab23)) != NO_Si2151_ERROR) {
               SiTRACE ("Si2151_LoadFirmware error 0x%02x: %s\n", return_code, Si2151_L1_API_ERROR_TEXT(return_code) );
               return return_code;
             }
          }
          else
          {
              SiTRACE ("INCOMPATIBLE PART error ROMID 0x%02x\n", api->rsp->part_info.romid);
              return ERROR_Si2151_INCOMPATIBLE_PART;
          }
     }
     if (!(api->load_control & SKIP_STARTFIRMWARE))
     {
        /*Start the Firmware */
        if ((return_code = Si2151_StartFirmware(api)) != NO_Si2151_ERROR)
        { /* Start firmware */
            SiTRACE ("Si2151_StartFirmware error 0x%02x: %s\n", return_code, Si2151_L1_API_ERROR_TEXT(return_code) );
            return return_code;
        }
        Si2151_L1_GET_REV (api);
        printf("Si21%2d Part running 'FW_%c_%cb%d'\n", api->rsp->part_info.part, api->rsp->get_rev.cmpmajor, api->rsp->get_rev.cmpminor, api->rsp->get_rev.cmpbuild );

     }

    return NO_Si2151_ERROR;
}
/************************************************************************************************************************
  NAME: Si2151_PowerUpUsingBroadcastI2C
  DESCRIPTION: This is similar to PowerUpWithPatch() for tuner_count tuners but it uses the I2C Broadcast
  command to allow the firmware download simultaneously to all tuners.

  Parameter:  tuners, a pointer to a table of L1 Si2151 Contexts
  Parameter:  tuner_count, the number of tuners in the table
  Returns:    Si2151/I2C transaction error code, NO_Si2151_ERROR if successful
************************************************************************************************************************/
int Si2151_PowerUpUsingBroadcastI2C    (L1_Si2151_Context *tuners[], int tuner_count )
{

  int return_code;
  int t;
  #define Si2151_TUNER_BROADCAST_INDEX 0
  unsigned char logged_i2c_address;
  return_code = NO_Si2151_ERROR;

  /* for each tuner execute the powerup and part_info command but not the firmware download. */
  for (t = 0; t < tuner_count; t++)
  {
      tuners[t]->load_control = SKIP_LOADFIRMWARE | SKIP_STARTFIRMWARE;
      /* Si2151_PowerUpWithPatch will return right after Si2151_L1_PART_INFO, because SKIP_LOADFIRMWARE and SKIP_STARTFIRMWARE are set */
      if ( (return_code = Si2151_PowerUpWithPatch(tuners[t])) != NO_Si2151_ERROR) 
	{
        SiTRACE ("Tuner %d Si2151_PowerUpWithPatch error 0x%02x\n", t, return_code);
        return return_code;
      }
       if ( (return_code = Si2151_L1_CONFIG_I2C(tuners[t], Si2151_CONFIG_I2C_CMD_SUBCODE_CODE, Si2151_CONFIG_I2C_CMD_I2C_BROADCAST_ENABLED)) != NO_Si2151_ERROR )
       {
          SiTRACE("Tuner %d L1_CONFIG_I2C error 0x%02x\n", t, return_code);
          return return_code;
       }

   }

  /* At this stage, all tuners are connected/powered up and in 'broadcast i2c' mode */

  /* store the address of tuner0 to the broadcast address and restore the original address after we're done.  */
  logged_i2c_address = tuners[Si2151_TUNER_BROADCAST_INDEX]->i2c->address;
  tuners[Si2151_TUNER_BROADCAST_INDEX]->i2c->address = Si2151_BROADCAST_ADDRESS;
  /* set the load_control flag to SKIP_POWERUP so the firmware is downloaded and started on all tuners only */
  tuners[Si2151_TUNER_BROADCAST_INDEX]->load_control = SKIP_POWERUP;

  /* Si2151_PowerUpWithPatch will now broadcast the tuner fw and return
      when all is completed, because load_fw is now '1'                               */
  if ( (return_code = Si2151_PowerUpWithPatch(tuners[Si2151_TUNER_BROADCAST_INDEX])) != NO_Si2151_ERROR) 
  {
    SiTRACE("Tuner %d Si2151_PowerUpWithPatch error 0x%02x\n", t, return_code);
    return return_code;
  }

  /* At this stage, all tuners have received the patch, and have been issued
     'Si2151_StartFirmware'                                                          */
     if ((return_code = Si2151_Configure    (tuners[Si2151_TUNER_BROADCAST_INDEX])) != NO_Si2151_ERROR)
     {
         return return_code;
     }

  /* Return the broadcast tuner address to its 'normal' value                         */
  tuners[Si2151_TUNER_BROADCAST_INDEX]->i2c->address = logged_i2c_address;

  for (t = 0; t < tuner_count; t++)
  {
    if ( (return_code = Si2151_L1_CONFIG_I2C(tuners[t], Si2151_CONFIG_I2C_CMD_SUBCODE_CODE, Si2151_CONFIG_I2C_CMD_I2C_BROADCAST_DISABLED)) != NO_Si2151_ERROR )
     {
      SiTRACE("Tuner %d L1_CONFIG_I2C error 0x%02x\n", t, return_code);
         return return_code;
     }
  }
  for (t = 0; t < tuner_count; t++)
  {
    /* Set Properties startup configuration         */
    Si2151_storePropertiesDefaults (tuners[t]->propShadow);
    /* Edit the Si2151_storeUserProperties if you have any properties settings different from the standard defaults */
    Si2151_storeUserProperties     (tuners[t]->prop);

   /* Reset the load_control flag to execute all phases of PowerUpWithPatch */
      tuners[t]->load_control =  SKIP_NONE;

 /* Check CTS for all tuners */
    if ( (return_code = Si2151_pollForCTS(tuners[t])) != NO_Si2151_ERROR ) {
      SiTRACE("Tuner %d pollForCTS error 0x%02x\n", t, return_code);
      return return_code;
    }

    /* unlock the tuners */
    if ((return_code = Unlock_part(tuners[t])) != NO_Si2151_ERROR) {
       SiTRACE ("Unable to unlock the part error 0x%02x: %s\n", return_code, Si2151_L1_API_ERROR_TEXT(return_code) );
        return return_code;
    }
  }
  return NO_Si2151_ERROR;
}
/************************************************************************************************************************
  NAME: encipher
  DESCRIPTION: Encrypt the part serial number to  use for unlock command
  Parameter:  num_rounds ,  recommended value is 32.
  Parameter:  data words v[0] and v[1]
  Parameter:  key words key[0] - key[3]
  Returns  :  encrypted data in v[0] and v[1]
************************************************************************************************************************/
void encipher(unsigned int num_rounds, unsigned int v[2], const unsigned int key[4])
{
    unsigned int i;
    unsigned int v0=v[0], v1=v[1], sum=0, delta=0x9E3779B9;
    //printf("v[0]=0x%x,v[1]=%x\n",v[0],v[1]);
    for (i=0; i < num_rounds; i++) {
        v0 += (((v1 << 4) ^ (v1 >> 5)) + v1) ^ (sum + key[sum & 3]);
        sum += delta;
        v1 += (((v0 << 4) ^ (v0 >> 5)) + v0) ^ (sum + key[(sum>>11) & 3]);
    }
    v[0]=v0; v[1]=v1;
   // printf("v0=0x%x,v1=%x\n",v0,v1);
}
/************************************************************************************************************************
  NAME: Si2151_LoadFirmware_16
  DESCRIPTION: Load firmware from firmware_struct array in Si2151_Firmware_x_y_build_z.h file into Si2151
              Requires Si2151 to be in bootloader mode after PowerUp
  Programming Guide Reference:    Flowchart A.3 (Download FW PATCH flowchart)

  Parameter:  Si2151 Context (I2C address)
  Parameter:  pointer to firmware_struct array
  Parameter:  number of lines in firmware table array (size in bytes / firmware_struct)
  Returns:    Si2151/I2C transaction error code, NO_Si2151_ERROR if successful
************************************************************************************************************************/
int Si2151_LoadFirmware_16        (L1_Si2151_Context *api, firmware_struct fw_table[], int nbLines)
{
    int return_code;
    int line;
    return_code = NO_Si2151_ERROR;

    SiTRACE ("Si2151_LoadFirmware_16 starting...\n");
    SiTRACE ("Si2151_LoadFirmware_16 nbLines %d\n", nbLines);

    /* for each line in fw_table */
    for (line = 0; line < nbLines; line++)
    {
      if (fw_table[line].firmware_len > 0)  /* don't download if length is 0 , e.g. dummy firmware */
      {
        /* send firmware_len bytes (up to 16) to Si2151 */
        if ((return_code = Si2151_L1_API_Patch(api, fw_table[line].firmware_len, fw_table[line].firmware_table)) != NO_Si2151_ERROR)
        {
          SiTRACE("Si2151_LoadFirmware_16 error 0x%02x patching line %d: %s\n", return_code, line, Si2151_L1_API_ERROR_TEXT(return_code) );
          if (line == 0) {
          SiTRACE("The firmware is incompatible with the part!\n");
          }
          SiTraceConfiguration((char*)"traces resume");
          return ERROR_Si2151_LOADING_FIRMWARE;
        }
        if (line==3) SiTraceConfiguration("traces suspend");
      }
    }
    SiTraceConfiguration("traces resume");
    /* Storing Properties startup configuration in propShadow                              */
    /* !! Do NOT change the content of Si2151_storePropertiesDefaults                   !! */
    /* !! It should reflect the part internal property settings after firmware download !! */
    Si2151_storePropertiesDefaults (api->propShadow);

    SiTRACE ("Si2151_LoadFirmware_16 complete...\n");
    return NO_Si2151_ERROR;
}
/************************************************************************************************************************
  NAME: Si2151_LoadFirmware
  DESCRIPTON: Load firmware from FIRMWARE_TABLE array in Si2151_Firmware_x_y_build_z.h file into Si2151
              Requires Si2151 to be in bootloader mode after PowerUp
  Programming Guide Reference:    Flowchart A.3 (Download FW PATCH flowchart)

  Parameter:  Si2151 Context (I2C address)
  Parameter:  pointer to firmware table array
  Parameter:  number of lines in firmware table array (size in bytes / BYTES_PER_LINE)
  Returns:    Si2151/I2C transaction error code, NO_Si2151_ERROR if successful
************************************************************************************************************************/
int Si2151_LoadFirmware        (L1_Si2151_Context *api, unsigned char fw_table[], int nbLines)
{
    int return_code;
    int line;
    return_code = NO_Si2151_ERROR;

    SiTRACE ("Si2151_LoadFirmware starting...\n");
    SiTRACE ("Si2151_LoadFirmware nbLines %d\n", nbLines);

    /* for each line in fw_table */
    for (line = 0; line < nbLines; line++)
    {
        /* send Si2151_BYTES_PER_LINE fw bytes to Si2151 */
        if ((return_code = Si2151_L1_API_Patch(api, Si2151_BYTES_PER_LINE, fw_table + Si2151_BYTES_PER_LINE*line)) != NO_Si2151_ERROR)
        {
          SiTRACE("Si2151_LoadFirmware error 0x%02x patching line %d: %s\n", return_code, line, Si2151_L1_API_ERROR_TEXT(return_code) );
          if (line == 0) {
          SiTRACE("The firmware is incompatible with the part!\n");
          }
          SiTraceConfiguration((char*)"traces resume");
          return ERROR_Si2151_LOADING_FIRMWARE;
        }
        if (line==3) {SiTraceConfiguration("traces suspend");}
    }
    SiTraceConfiguration("traces resume");

    /* Storing Properties startup configuration in propShadow                              */
    /* !! Do NOT change the content of Si2151_storePropertiesDefaults                   !! */
    /* !! It should reflect the part internal property settings after firmware download !! */
    Si2151_storePropertiesDefaults (api->propShadow);

    SiTRACE ("Si2151_LoadFirmware complete...\n");

    return NO_Si2151_ERROR;
}
/************************************************************************************************************************
  NAME: Si2151_StartFirmware
  DESCRIPTION: Start Si2151 firmware (put the Si2151 into run mode)
  Parameter:   Si2151 Context (I2C address)
  Parameter (passed by Reference):   ExitBootloadeer Response Status byte : tunint, atvint, dtvint, err, cts
  Returns:     I2C transaction error code, NO_Si2151_ERROR if successful
************************************************************************************************************************/
int Si2151_StartFirmware       (L1_Si2151_Context *api)
{

    if (Si2151_L1_EXIT_BOOTLOADER(api, Si2151_EXIT_BOOTLOADER_CMD_FUNC_TUNER, Si2151_EXIT_BOOTLOADER_CMD_CTSIEN_OFF) != NO_Si2151_ERROR)
    {
        return ERROR_Si2151_STARTING_FIRMWARE;
    }

    return NO_Si2151_ERROR;
}
/************************************************************************************************************************
  NAME: Unlock_part
  DESCRIPTION:Send the unlock code to the part
              The key is a specific string for the Si2151R part.
              The v data is a combination of the part serial number and a random number.
              NOTE: the random number generator is seeded in the main() function in Si2151_L3_TEST.C
  Parameter:  Si2151 Context

  Returns:    I2C transaction error code, NO_Si2151_ERROR if successful
************************************************************************************************************************/
int Unlock_part(L1_Si2151_Context *api)
{
    int return_code;
    unsigned int v[2]={0,0};
    unsigned int key[4] = {0x50BA86AF,0xC19304CE,0x891159AE,0x2F7D967D};
    //unsigned int u32_rand;
   
    /* generate a random number for the unlock function */
    //u32_rand = rand();

    /* unlock the part */
   // v[0]=0;
     v[0]=api->rsp->part_info.serial;
     v[1]=0;
     // v[1]=u32_rand;
     // printf("v[0]=0x%x,v[1]=%x\n",v[0],v[1]);
      //printf("v[0]=%d,v[1]=%d\n",v[0],v[1]);
    /*encrypt the data */
      encipher(32, v, key);
     //printf("v[0]=0x%x,v[1]=%x\n",v[0],v[1]);
    if ((return_code = Si2151_L1_UNLOCK(api,v[0],v[1])) != NO_Si2151_ERROR)
	{
        SiTRACE ("Si2151_L1_Unlock error 0x%02x: %s\n", return_code, Si2151_L1_API_ERROR_TEXT(return_code) );
        return return_code;
    }
    return NO_Si2151_ERROR;
}
/************************************************************************************************************************
  NAME: Si2151_Init
  DESCRIPTION:Reset and Initialize Si2151
  Parameter:  Si2151 Context (I2C address)
  Returns:    I2C transaction error code, NO_Si2151_ERROR if successful
************************************************************************************************************************/
int Si2151_Init                (L1_Si2151_Context *api)
{
    int return_code;
    //printf("Si2151_Init starting...\n");

    if ((return_code = Si2151_PowerUpWithPatch(api)) != NO_Si2151_ERROR)
    {   /* PowerUp into bootloader */
        SiTRACE ("Si2151_PowerUpWithPatch error 0x%02x: %s\n", return_code, Si2151_L1_API_ERROR_TEXT(return_code) );
        return return_code;
    }
	
#if 1

/* The Si2151R part must be unlocked for it to function.  The command below will unlock the feature bits */
    if (api->rsp->part_info.part2 == 'R')
    {
      // printf("api->rsp->part_info.part2=%c\n",api->rsp->part_info.part2);
	  //printf("api->rsp->part_info.part2=%x\n",api->rsp->part_info.part2);
        if ((return_code = Unlock_part(api)) != NO_Si2151_ERROR) 
	{
	
           SiTRACE ("Unable to unlock the part error 0x%02x: %s\n", return_code, Si2151_L1_API_ERROR_TEXT(return_code) );
            return return_code;
        }
    }
	#endif

	#if 0
    /* At this point, FW is loaded and started.  */
    Si2151_Configure(api);
    //printf("Si2151_Init complete...\n");
	#endif
    return NO_Si2151_ERROR;
}
 /************************************************************************************************************************
  NAME: Si2151_UpdateChannelScanFrequency
  DESCRIPTION:  This routine allows the user to implement an update of the current channel scan operation with
                the current frequency being scanned.
  Parameter:    Current frequency of the scan routine
  Parameter:  channelsFound = 0 if channel not found at that frequency , channelsFound > 0  the number of channels
        currently found( including this frequency )
  Returns:      1 if scan abort requested, 0 if ok.
************************************************************************************************************************/
int Si2151_UpdateChannelScanFrequency(int freq,int channelsFound)
{
  int abort_scan = 0;
  static int previousFrequency;
/*** Insert user code to display realtime updates of the frequency being scanned and channel status (number found ) **/
/* add check for user abort here */
  if (channelsFound)
  {
    /* Terminate the previous scan message */
    printf("Found Frequency %d\n",freq);
  }
  else
  {
    if (freq==previousFrequency)
      printf("Not Found\n");
    else
      printf("Scanning Frequency %d, ",freq);
  }
  previousFrequency=freq;

  if (abort_scan)
    return 1;
  else
    return 0;
  }
/************************************************************************************************************************
  NAME: Si2151_GetRF
  DESCRIPTION: Retrieve Si2151 tune freq

  Parameter:  Pointer to Si2151 Context (I2C address)
  Returns  :  frequency (Hz) as an int
************************************************************************************************************************/
int  Si2151_GetRF             (L1_Si2151_Context *api)
{
    Si2151_L1_TUNER_STATUS (api);
        return api->rsp->tuner_status.freq;
}
 /************************************************************************************************************************
  NAME: Si2151_Tune
  DESCRIPTIION: Tune Si2151 in specified mode (ATV/DTV) at center frequency, wait for TUNINT and xTVINT with timeout

  Parameter:  Pointer to Si2151 Context (I2C address)
  Parameter:  Mode (ATV or DTV) use Si2151_TUNER_TUNE_FREQ_CMD_MODE_ATV or Si2151_TUNER_TUNE_FREQ_CMD_MODE_DTV constants
  Parameter:  frequency (Hz) as a unsigned long integer
  Returns:    0 if channel found.  A nonzero value means either an error occurred or channel not locked.
  Programming Guide Reference:    Flowchart A.7 (Tune flowchart)
************************************************************************************************************************/
 int  Si2151_Tune              (L1_Si2151_Context *api, unsigned char mode, unsigned long freq)
{
    int start_time  = 0;
    int return_code = NO_Si2151_ERROR;
    int timeout     = 36;


#ifdef Si2151_DEMOD_WRAPPER_VCO
    if (Si2151_SetVcoProperties(api) != NO_Si2151_ERROR)
    {
        return ERROR_Si2151_SENDING_COMMAND;
    }
#endif // Si2151_DEMOD_WRAPPER_VCO

    if (Si2151_L1_TUNER_TUNE_FREQ (api,
                                   mode,
                                   freq) != NO_Si2151_ERROR)
    {
        return ERROR_Si2151_SENDING_COMMAND;
    }

    start_time = system_time();

    /* wait for TUNINT, timeout is 36 ms */
    while ( (system_time() - start_time) < timeout )
    {
        if ((return_code = Si2151_L1_CheckStatus(api)) != NO_Si2151_ERROR)
            return return_code;
        if (api->status->tunint)
            break;
    }
    if (!api->status->tunint) {
      SiTRACE("Timeout waiting for TUNINT\n");
      return ERROR_Si2151_TUNINT_TIMEOUT;
    }

    /* wait for xTVINT, timeout is 110ms for ATVINT and 10 ms for DTVINT */
    start_time = system_time();
    timeout    = ((mode==Si2151_TUNER_TUNE_FREQ_CMD_MODE_ATV) ? 110 : 10);
    while ( (system_time() - start_time) < timeout )
    {
        if ((return_code = Si2151_L1_CheckStatus(api)) != NO_Si2151_ERROR)
            return return_code;
        if (mode==Si2151_TUNER_TUNE_FREQ_CMD_MODE_ATV)
        {
         if (api->status->atvint)
            break;
        }
        else
        {
         if (api->status->dtvint)
            break;
        }
    }

    if (mode==Si2151_TUNER_TUNE_FREQ_CMD_MODE_ATV)
    {
        if (api->status->atvint)
        {
          SiTRACE("ATV Tune Successful\n");
          return_code =  NO_Si2151_ERROR;
        }
        else
        {
            SiTRACE("Timeout waiting for ATVINT\n");
            return_code =  ERROR_Si2151_xTVINT_TIMEOUT;
        }
    }
    else
    {
        if (api->status->dtvint)
        {
          SiTRACE("DTV Tune Successful\n");
          return_code = NO_Si2151_ERROR;
        }
        else
        {
          SiTRACE("Timeout waiting for DTVINT\n");
          return_code =  ERROR_Si2151_xTVINT_TIMEOUT;
        }
    }

    return return_code;
}
 /************************************************************************************************************************
  NAME: Si2151_ATVTune
  DESCRIPTION:Update ATV_VIDEO_MODE and tune ATV mode at center frequency
  Parameter:  Pointer to Si2151 Context (I2C address)
  Parameter:  frequency (Hz)
  Parameter:  Video system , e.g. use constant Si2151_ATV_VIDEO_MODE_PROP_VIDEO_SYS_M for system M
  Parameter:  transport,  e.g. use constant Si2151_ATV_VIDEO_MODE_PROP_TRANS_TERRESTRIAL for terrestrial
  Parameter:  color , e.g. use constant Si2151_ATV_VIDEO_MODE_PROP_COLOR_PAL_NTSC for PAL or NTSC
  Parameter:  invert_spectrum, 0= normal, 1= inverted
  Parameter:  rsp - commandResp structure to returns tune status info.
  Returns:    I2C transaction error code, 0 if successful
  Programming Guide Reference:    Flowchart A.7 (Tune flowchart)
************************************************************************************************************************/
int  Si2151_ATVTune           (L1_Si2151_Context *api, unsigned long freq, unsigned char video_sys, unsigned char invert_spectrum)
{
    /* update ATV_VIDEO_MODE property */
    api->prop->atv_video_mode.video_sys = video_sys;
    api->prop->atv_video_mode.invert_spectrum=invert_spectrum;
    if (Si2151_L1_SetProperty2(api, Si2151_ATV_VIDEO_MODE_PROP) != NO_Si2151_ERROR)
    {
      return ERROR_Si2151_SENDING_COMMAND;
    }

    return Si2151_Tune (api, Si2151_TUNER_TUNE_FREQ_CMD_MODE_ATV, freq);
}
 /************************************************************************************************************************
  NAME: Si2151_DTVTune
  DESCRIPTION: Update DTV_MODE and tune DTV mode at center frequency
  Parameter:  Pointer to Si2151 Context (I2C address)
  Parameter:  frequency (Hz)
  Parameter:  bandwidth , 6,7 or 8 MHz
  Parameter:  modulation,  e.g. use constant Si2151_DTV_MODE_PROP_MODULATION_DVBT for DVBT mode
  Parameter:  invert_spectrum, 0= normal, 1= inverted
  Returns:    I2C transaction error code, 0 if successful
  Programming Guide Reference:    Flowchart A.7 (Tune flowchart)
************************************************************************************************************************/
int  Si2151_DTVTune           (L1_Si2151_Context *api, unsigned long freq, unsigned char bw, unsigned char modulation, unsigned char invert_spectrum)
{
    int return_code;
    return_code = NO_Si2151_ERROR;

    /* update DTV_MODE_PROP property */
    api->prop->dtv_mode.bw = bw;
    api->prop->dtv_mode.invert_spectrum = invert_spectrum;
    api->prop->dtv_mode.modulation = modulation;
    if (Si2151_L1_SetProperty2(api, Si2151_DTV_MODE_PROP) != NO_Si2151_ERROR)
    {
      return ERROR_Si2151_SENDING_COMMAND;
    }

    return_code = Si2151_Tune (api, Si2151_TUNER_TUNE_FREQ_CMD_MODE_DTV, freq);

    return return_code;
}
 /************************************************************************************************************************
  NAME: Si2151_XoutOn
  Parameter:  Pointer to Si2151 Context (I2C address)
  Returns:    I2C transaction error code, NO_Si2151_ERROR if successful
************************************************************************************************************************/
int Si2151_XoutOn             (L1_Si2151_Context *api)
{
    int return_code;
    SiTRACE("Si2151_XoutOn:  Turning Xout ON\n");

    if ((return_code = Si2151_L1_CONFIG_CLOCKS(api,
                                              Si2151_CONFIG_CLOCKS_CMD_SUBCODE_CODE,
                                              api->cmd->config_clocks.clock_mode,
                                              Si2151_CONFIG_CLOCKS_CMD_EN_XOUT_EN_XOUT)) != NO_Si2151_ERROR)
    return return_code;

    return NO_Si2151_ERROR;
}
 /************************************************************************************************************************
  NAME: Si2151_XoutOff
  Parameter:  Pointer to Si2151 Context (I2C address)
  Returns:    I2C transaction error code, NO_Si2151_ERROR if successful
************************************************************************************************************************/
int Si2151_XoutOff            (L1_Si2151_Context *api)
{
    int return_code;
    SiTRACE("Si2151_XoutOff:  Turning Xout OFF\n");

    if ((return_code = Si2151_L1_CONFIG_CLOCKS(api,
                                              Si2151_CONFIG_CLOCKS_CMD_SUBCODE_CODE,
                                              api->cmd->config_clocks.clock_mode,
                                              Si2151_CONFIG_CLOCKS_CMD_EN_XOUT_DIS_XOUT)) != NO_Si2151_ERROR)
    return return_code;

    return NO_Si2151_ERROR;
}
 /************************************************************************************************************************
  NAME: Si2151_Standby
  Parameter:  Pointer to Si2151 Context (I2C address)
  Returns:    I2C transaction error code, NO_Si2151_ERROR if successful
************************************************************************************************************************/
int Si2151_Standby         (L1_Si2151_Context *api)
{
    int return_code;
    SiTRACE("Si2151_Standby: Going to Standby. The part will wake on the next command\n");

    if ((return_code = Si2151_L1_STANDBY(api, Si2151_STANDBY_CMD_TYPE_LNA_ON)) != NO_Si2151_ERROR)
    return return_code;

    return NO_Si2151_ERROR;
}
 /************************************************************************************************************************
  NAME: Si2151_Powerdown
  Parameter:  Pointer to Si2151 Context (I2C address)
  Returns:    I2C transaction error code, NO_Si2151_ERROR if successful
************************************************************************************************************************/
int Si2151_Powerdown         (L1_Si2151_Context *api)
{
    int return_code;
    SiTRACE("Si2151_Powerdown: Powering down the part. Select InitAndConfig to reload.\n");

    if ((return_code = Si2151_L1_POWER_DOWN_HW(api,Si2151_POWER_DOWN_HW_CMD_SUBCODE_CODE,Si2151_POWER_DOWN_HW_CMD_PDB_XO_OSC_XO_OSC_POWER_DOWN,
                                               Si2151_POWER_DOWN_HW_CMD_RESERVED1_RESERVED,Si2151_POWER_DOWN_HW_CMD_EN_XOUT_DIS_XOUT,
                                               Si2151_POWER_DOWN_HW_CMD_RESERVED2_RESERVED,Si2151_POWER_DOWN_HW_CMD_RESERVED3_RESERVED,
                                               Si2151_POWER_DOWN_HW_CMD_RESERVED4_RESERVED,Si2151_POWER_DOWN_HW_CMD_RESERVED5_RESERVED,
                                               Si2151_POWER_DOWN_HW_CMD_RESERVED6_RESERVED,Si2151_POWER_DOWN_HW_CMD_RESERVED7_RESERVED,
                                               Si2151_POWER_DOWN_HW_CMD_RESERVED8_RESERVED,Si2151_POWER_DOWN_HW_CMD_RESERVED9_RESERVED)) != NO_Si2151_ERROR)
    return return_code;

    return NO_Si2151_ERROR;
}
 /************************************************************************************************************************
  NAME: Si2151_LoadVideofilter
  DESCRIPTION:        Load video filters from vidfiltTable in Si2151_write_xTV_video_coeffs.h file into Si2151
  Programming Guide Reference:    Flowchart A.4 (Download Video Filters flowchart)

  Parameter:  Si2151 Context (I2C address)
  Parameter:  pointer to video filter table array
  Parameter:  number of lines in video filter table array(size in bytes / atv_vid_filt_struct)
  Returns:    Si2151/I2C transaction error code, NO_Si2151_ERROR if successful
************************************************************************************************************************/
int Si2151_LoadVideofilter     (L1_Si2151_Context *api, vid_filt_struct vidFiltTable[], int lines)
{
    int line;
    /* for each line in VIDFILT_TABLE  (max 16 bytes) */
    for (line = 0; line < lines; line++)
    {
        /* send up to 16 byte I2C command to Si2151 */
        if (Si2151_L1_API_Patch(api, vidFiltTable[line].vid_filt_len, vidFiltTable[line].vid_filt_table) != NO_Si2151_ERROR)
        {
            return ERROR_Si2151_SENDING_COMMAND;
        }
    }
    return NO_Si2151_ERROR;
}
/*  Channel Scan Functions. */
/* For the following channel scan functions: Si2151_ATV_Channel_Scan_M and Si2151_ATV_Channel_Scan_PAL */
/* the following functions are required:                                */
/* demod_locked(void) returns 1 = demod locked and 0 = demod not locked */
/* demod_afc(void) returns the afc deviation from nominal IF picture carrier in kHz */
    #define demod_locked(void) 0
    #define demod_afc(void) 0
 /************************************************************************************************************************
  NAME: Si2151_ATV_Channel_Scan_M
  DESCRIPTION: Performs a channel scan for NTSC (System M) of the band
  Programming Guide Reference:    Flowchart A.11.0 and A11.1 (ATV Channel Scan flowchart for System M)

  Parameter:  Pointer to Si2151 Context (I2C address)
  Parameter:  starting Frequency Hz
  Parameter:  ending Frequency Hz
  Returns:    0 if successful, otherwise an error.
************************************************************************************************************************/
int Si2151_ATV_Channel_Scan_M    (L1_Si2151_Context *api, unsigned long rangeMinHz, unsigned long rangeMaxHz)
{
    #define SCAN_INTERVAL     1000000
    #define CHANNEL_BANDWIDTH 6000000
    #define CHANNEL_NOT_FOUND       0

    unsigned long freq;
    int i;
    int start_time;
    int timeout=36;
    int return_code;

    /*Clear the channel list size and channel array */
    api->ChannelListSize=0;

    for (i=0; i< MAX_POSSIBLE_CHANNELS;i++)
        api->ChannelList[i]=0;
    /* configure the VideoMode property to System M, NTSC*/
    api->prop->atv_video_mode.video_sys = Si2151_ATV_VIDEO_MODE_PROP_VIDEO_SYS_M;       /* M ATV demodulation       */
    if (Si2151_L1_SetProperty2(api, Si2151_ATV_VIDEO_MODE_PROP) != NO_Si2151_ERROR)
    {
       return ERROR_Si2151_SENDING_COMMAND;
    }

   /* Start Scanning */
    for (freq=rangeMinHz; freq < rangeMaxHz; )
    {

        /* before calling tune provide a callback stub that the user can update the frequency */
        /* if user requested abort then break from the loop */
        if (Si2151_UpdateChannelScanFrequency(freq, CHANNEL_NOT_FOUND))
          break;

        /* Call the Tune command to tune the frequency */
        /* if successful (a station was found) then the return value will be 0. */
       if (Si2151_L1_ATV_SCAN_TUNE(api,freq) != NO_Si2151_ERROR)
        {
             return ERROR_Si2151_SENDING_COMMAND;
        }
        start_time = system_time();

        /* wait for TUNINT, timeout is 36 ms */
        while ( (system_time() - start_time) < timeout )
        {
            if ((return_code = Si2151_L1_CheckStatus(api)) != NO_Si2151_ERROR)
                return return_code;
            if (api->status->tunint)
                break;
        }
        if (!api->status->tunint) {
          SiTRACE("Timeout waiting for TUNINT\n");
          return ERROR_Si2151_TUNINT_TIMEOUT;
        }
        /* start the demod and check for lock */
        if (demod_locked())
        {
             /* Add the afc_freq (khz) to the center frequency and add to the channel list */
              api->ChannelList[api->ChannelListSize]= freq + (demod_afc()  * 1000);
              /* Update the callback to indicate the channel is found */
              Si2151_UpdateChannelScanFrequency( api->ChannelList[api->ChannelListSize], api->ChannelListSize+1);
              freq = api->ChannelList[api->ChannelListSize++] + CHANNEL_BANDWIDTH;
        }
        else
        {
            /* if user requested abort then break from the loop */
          if (Si2151_UpdateChannelScanFrequency(freq, CHANNEL_NOT_FOUND))
            break;
          /*  channel not found! **/
          freq += SCAN_INTERVAL;
        }

    }

 return NO_Si2151_ERROR;

}
 /************************************************************************************************************************
  NAME: Si2151_ATV_Channel_Scan_PAL
  DESCRIPTION: Performs a channel scan for PAL (Systems B/G, D/K, L/L', and I) of the band
  Programming Guide Reference:    Flowchart A.10.0 - A10.3 (ATV Channel Scan flowchart for PAL)

  NOTE: this function requires an external sound processor to determine the PAL standard.  The user is
  required to implement the functions: L0_InitSoundProcessor(); and L0_AcquireSoundStandard(..);

  Parameter:  Pointer to Si2151 Context (I2C address)
  Parameter:  starting Frequency Hz
  Parameter:  ending Frequency Hz
  Returns:    0 if successful, otherwise an error.
************************************************************************************************************************/
int Si2151_ATV_Channel_Scan_PAL(L1_Si2151_Context *api, unsigned long rangeMinHz, unsigned long rangeMaxHz)
{

  #define SCAN_INTERVAL 1000000
  #define CHANNEL_NOT_FOUND 0
  char standard = Si2151_ATV_VIDEO_MODE_PROP_VIDEO_SYS_B;
  unsigned long freq;
  unsigned long channelIncrement;
  int start_time;
  int timeout=36;
  int return_code;
  int i;

  /*Clear the channel list size and channel array */
  api->ChannelListSize=0;

    /* Configure the VideoMode property to System DK, PAL*/
    api->prop->atv_video_mode.video_sys = Si2151_ATV_VIDEO_MODE_PROP_VIDEO_SYS_DK;        /* DK ATV demodulation      */
   if (Si2151_L1_SetProperty2(api, Si2151_ATV_VIDEO_MODE_PROP) != NO_Si2151_ERROR)
    {
       return ERROR_Si2151_SENDING_COMMAND;
    }

  for (i=0; i< MAX_POSSIBLE_CHANNELS;i++)
  {
    api->ChannelList[i]=0;
    api->ChannelType[i][0]='\0';
  }

   /* Start Scanning */
    for (freq=rangeMinHz; freq < rangeMaxHz; )
    {
        /* before calling tune provide a callback stub that the user can update the frequency */
        /* if user requested abort then break from the loop */

        /* Call the Scan Tune command to tune the frequency */
       if (Si2151_L1_ATV_SCAN_TUNE(api,freq) != NO_Si2151_ERROR)
        {
             return ERROR_Si2151_SENDING_COMMAND;
        }
        start_time = system_time();

        /* wait for TUNINT, timeout is 36 ms */
        while ( (system_time() - start_time) < timeout )
        {
            if ((return_code = Si2151_L1_CheckStatus(api)) != NO_Si2151_ERROR)
                return return_code;
            if (api->status->tunint)
                break;
        }
        if (!api->status->tunint) {
          SiTRACE("Timeout waiting for TUNINT\n");
          return ERROR_Si2151_TUNINT_TIMEOUT;
        }
        if (demod_locked())
        {
          /* Update the callback to indicate the channel is found */
          /* if user requested abort then break from the loop */

        /* Run the sound processor and return the standard(s) */
        /*AcquireSoundStandard(&standard);*/
            switch (standard)
            {
                case Si2151_ATV_VIDEO_MODE_PROP_VIDEO_SYS_B:
                      api->ChannelType[api->ChannelListSize][0]='B';
                      api->ChannelType[api->ChannelListSize][1]='\0';
                    /* Add the afc_freq (khz) to the center frequency and add to the channel list */
                      api->ChannelList[api->ChannelListSize]= freq + (demod_afc() * 1000) -500000;
                      channelIncrement = 7000000;
                      /* update the callback with the frequency */
                      Si2151_UpdateChannelScanFrequency( api->ChannelList[api->ChannelListSize], api->ChannelListSize+1);
                      freq = api->ChannelList[api->ChannelListSize++] + channelIncrement;
                break;
                case Si2151_ATV_VIDEO_MODE_PROP_VIDEO_SYS_GH:
                      api->ChannelType[api->ChannelListSize][0]='G';
                      api->ChannelType[api->ChannelListSize][1]='H';
                      api->ChannelType[api->ChannelListSize][2]='\0';
                      api->ChannelList[api->ChannelListSize]= freq + (demod_afc() * 1000);
                      channelIncrement = 8000000;
                      /* update the callback with the frequency */
                      Si2151_UpdateChannelScanFrequency( api->ChannelList[api->ChannelListSize], api->ChannelListSize+1);
                      freq = api->ChannelList[api->ChannelListSize++] + channelIncrement;
                break;
                case Si2151_ATV_VIDEO_MODE_PROP_VIDEO_SYS_I:
                      api->ChannelType[api->ChannelListSize][0]='I';
                      api->ChannelType[api->ChannelListSize][1]='\0';
                      api->ChannelList[api->ChannelListSize]= freq + (demod_afc() * 1000);
                      channelIncrement = 8000000;
                      /* update the callback with the frequency */
                      Si2151_UpdateChannelScanFrequency( api->ChannelList[api->ChannelListSize], api->ChannelListSize+1);
                      freq = api->ChannelList[api->ChannelListSize++] + channelIncrement;
                break;
                case Si2151_ATV_VIDEO_MODE_PROP_VIDEO_SYS_L:
                     api->ChannelType[api->ChannelListSize][0]='L';
                     api->ChannelType[api->ChannelListSize][1]='\0';
                     api->ChannelList[api->ChannelListSize]= freq + (demod_afc() * 1000);
                     channelIncrement = 8000000;
                     /* update the callback with the frequency */
                     Si2151_UpdateChannelScanFrequency( api->ChannelList[api->ChannelListSize], api->ChannelListSize+1);
                     freq = api->ChannelList[api->ChannelListSize++] + channelIncrement;
                break;
                case Si2151_ATV_VIDEO_MODE_PROP_VIDEO_SYS_DK:
                    api->ChannelType[api->ChannelListSize][0]='D';
                    api->ChannelType[api->ChannelListSize][1]='K';
                    api->ChannelType[api->ChannelListSize][2]='\0';
                    api->ChannelList[api->ChannelListSize]= freq + (demod_afc() * 1000);
                    channelIncrement = 8000000;
                   /* update the callback with the frequency */
                    Si2151_UpdateChannelScanFrequency( api->ChannelList[api->ChannelListSize], api->ChannelListSize+1);
                    freq = api->ChannelList[api->ChannelListSize++] + channelIncrement;
                break;
                default:
                    Si2151_UpdateChannelScanFrequency(freq, CHANNEL_NOT_FOUND);
                    freq += SCAN_INTERVAL;
                break;
            }
    }
    else
    {
          /* we didn't find any channels goto flowchart section B */
            freq += SCAN_INTERVAL;
            continue;

            /* if user requested abort then break from the loop */
            if (Si2151_UpdateChannelScanFrequency(freq, CHANNEL_NOT_FOUND))
            break;
    }


  }

 return NO_Si2151_ERROR;
}
 /************************************************************************************************************************
  NAME: Si2151_AGC_Override
  Parameter:  Pointer to Si2151 Context (I2C address)
  Parameter:  Mode 0=Normal, 1= Max, 2=TOP
  Returns:    I2C transaction error code, 0 if successful
************************************************************************************************************************/
int Si2151_AGC_Override    (L1_Si2151_Context *api, unsigned char mode )
{
    int return_code;
    printf("Si2151_AGC_Override: mode = %d\n",mode);
    switch (mode)
    {
        case  Si2151_FORCE_NORMAL_AGC:
            if ((return_code = Si2151_L1_AGC_OVERRIDE (api, Si2151_AGC_OVERRIDE_CMD_FORCE_MAX_GAIN_DISABLE, Si2151_AGC_OVERRIDE_CMD_FORCE_TOP_GAIN_DISABLE)) != NO_Si2151_ERROR)
                return return_code;
            break;
        case  Si2151_FORCE_MAX_AGC:
            if ((return_code = Si2151_L1_AGC_OVERRIDE (api, Si2151_AGC_OVERRIDE_CMD_FORCE_MAX_GAIN_ENABLE, Si2151_AGC_OVERRIDE_CMD_FORCE_TOP_GAIN_DISABLE)) != NO_Si2151_ERROR)
                return return_code;
            break;
        case  Si2151_FORCE_TOP_AGC:
            if ((return_code = Si2151_L1_AGC_OVERRIDE (api, Si2151_AGC_OVERRIDE_CMD_FORCE_MAX_GAIN_DISABLE, Si2151_AGC_OVERRIDE_CMD_FORCE_TOP_GAIN_ENABLE)) != NO_Si2151_ERROR)
                return return_code;
            break;
        default:
            return ERROR_Si2151_PARAMETER_OUT_OF_RANGE;
    }
    return NO_Si2151_ERROR;
}
/************************************************************************************************************************
  NAME: Si2151_SetVcoProperties
  DESCRIPTION: This will set the properties for VCO blocking.
  This function assumes:
  - The vco codes from TUNER_STATUS are assigned to the vco_code property values (e.g.  api->prop->tuner_blocked_vco.vco_code)
  - The api->propertyWriteMode = DOWNLOAD_ON_CHANGE  -- setting propertyWriteMode = DOWNLOAD_ALWAYS will work also but is not optimal.

  If any VCO properties have changed, they will be downloaded.  Normally they will be at their default (0x8000)

  Parameter:  Pointer to Si2151 Context (I2C address)
  Returns  :  error code
************************************************************************************************************************/
int Si2151_SetVcoProperties (L1_Si2151_Context *api)
{
    if (Si2151_L1_SetProperty2(api, Si2151_TUNER_BLOCKED_VCO_PROP) != NO_Si2151_ERROR)
    {
      return ERROR_Si2151_SENDING_COMMAND;
    }
    if (Si2151_L1_SetProperty2(api, Si2151_TUNER_BLOCKED_VCO2_PROP) != NO_Si2151_ERROR)
    {
      return ERROR_Si2151_SENDING_COMMAND;
    }
    if (Si2151_L1_SetProperty2(api, Si2151_TUNER_BLOCKED_VCO3_PROP) != NO_Si2151_ERROR)
    {
      return ERROR_Si2151_SENDING_COMMAND;
    }
    return NO_Si2151_ERROR;
}
/************************************************************************************************************************
  NAME: Si2151_Tuner_Block_VCO
  Parameter:  Pointer to Si2151 Context (I2C address) of the OTHER tuner
  Parameter:  vco_code from TUNER_STATUS
  Returns:    I2C transaction error code, 0 if successful
************************************************************************************************************************/
int  Si2151_Tuner_Block_VCO      (L1_Si2151_Context *api, int vco_code)
{
        /* configure the TUNER_BLOCKED_VCO for the other tuner. */
    api->prop->tuner_blocked_vco.vco_code = vco_code;
SiTRACE("TUNER_BLOCK_VCO  addr=0x%02x, code = 0x%04x, Prop=0x%04x \n",api->i2c->address,vco_code,Si2151_TUNER_BLOCKED_VCO_PROP );
    if (Si2151_L1_SetProperty2(api, Si2151_TUNER_BLOCKED_VCO_PROP) != NO_Si2151_ERROR)
    {
       return ERROR_Si2151_SENDING_COMMAND;
    }
    return NO_Si2151_ERROR;
}
/************************************************************************************************************************
  NAME: Si2151_Tuner_Block_VCO
  Parameter:  Pointer to Si2151 Context (I2C address) of the OTHER tuner
  Parameter:  vco_code from TUNER_STATUS
  Returns:    I2C transaction error code, 0 if successful
************************************************************************************************************************/
int  Si2151_Tuner_Block_VCO2      (L1_Si2151_Context *api, int vco_code)
{
        /* configure the TUNER_BLOCKED_VCO for the other tuner. */
    api->prop->tuner_blocked_vco2.vco_code = vco_code;
SiTRACE("TUNER_BLOCK_VCO2 addr=0x%02x, code = 0x%04x, Prop=0x%04x \n",api->i2c->address,vco_code,Si2151_TUNER_BLOCKED_VCO2_PROP );


    if (Si2151_L1_SetProperty2(api, Si2151_TUNER_BLOCKED_VCO2_PROP) != NO_Si2151_ERROR)
    {
       return ERROR_Si2151_SENDING_COMMAND;
    }
    return NO_Si2151_ERROR;
}
/************************************************************************************************************************
  NAME: Si2151_Tuner_Block_VCO
  Parameter:  Pointer to Si2151 Context (I2C address) of the OTHER tuner
  Parameter:  vco_code from TUNER_STATUS
  Returns:    I2C transaction error code, 0 if successful
************************************************************************************************************************/
int  Si2151_Tuner_Block_VCO3      (L1_Si2151_Context *api, int vco_code)
{
        /* configure the TUNER_BLOCKED_VCO for the other tuner. */
    api->prop->tuner_blocked_vco3.vco_code = vco_code;
SiTRACE("TUNER_BLOCK_VCO3 addr=0x%02x, code = 0x%04x, Prop=0x%04x \n",api->i2c->address,vco_code,Si2151_TUNER_BLOCKED_VCO3_PROP );

    if (Si2151_L1_SetProperty2(api, Si2151_TUNER_BLOCKED_VCO3_PROP) != NO_Si2151_ERROR)
    {
       return ERROR_Si2151_SENDING_COMMAND;
    }
    return NO_Si2151_ERROR;
}
/************************************************************************************************************************
  NAME: Si2151_L2_VCO_Blocking_PreTune
  DESCRIPTION: Si2151 Function for blocking using the pretune model
              Call this function before tuning.  A TUNER_STATUS command will be sent to all the other tuners and the
              TUNER_BLOCK_VCO properties will be updated before the tune command is called.
  Behavior:   The preferred method is the posttune function, since it more efficient.
               There are cases where the pretune method is needed (for example if tuners are powered up and down or put in standby).
               This method will retrieve the vco codes from all the other tuners just before tuning.
************************************************************************************************************************/
int Si2151_L2_VCO_Blocking_PreTune(L1_Si2151_Context *tuners[], int tuner_num, int tuner_count)
{
int errcode;
    if (tuner_count == 1)
    {
        return NO_Si2151_ERROR;
    }
    else if (tuner_count == 2)
    {
        /* get the tuner status of the other tuner !tuner_num */
        if ((errcode=Si2151_L1_TUNER_STATUS (tuners[!tuner_num])) != NO_Si2151_ERROR)
        {
            SiTRACE ("Error returned from TunerStatus error = %s\n",Si2151_L1_API_ERROR_TEXT(errcode));
            return errcode;
        }
        /* check the frequency returned from tuner_status.  If the freq=0 (no tune done) then don't update the vco code */
        if (tuners[!tuner_num]->rsp->tuner_status.freq > 0)
        {
            if ((errcode=Si2151_Tuner_Block_VCO(tuners[tuner_num],tuners[!tuner_num]->rsp->tuner_status.vco_code)) != NO_Si2151_ERROR)
            {
                SiTRACE ("Error returned from Si2151_Tuner_Block_VCO error = %s\n",Si2151_L1_API_ERROR_TEXT(errcode));
                return errcode;
            }
        }
    }
    else if (tuner_count==3)
    {
        switch (tuner_num)
        {
            case 0:

            if ((errcode=Si2151_L1_TUNER_STATUS (tuners[1])) != NO_Si2151_ERROR)
            {
                SiTRACE ("Error returned from TunerStatus error = %s\n",Si2151_L1_API_ERROR_TEXT(errcode));
                return errcode;
            }
            /* check the frequency returned from tuner_status.  If the freq=0 (no tune done) then don't update the vco code */
            if (tuners[1]->rsp->tuner_status.freq > 0)
            {

                if ((errcode=Si2151_Tuner_Block_VCO(tuners[0],tuners[1]->rsp->tuner_status.vco_code)) != NO_Si2151_ERROR)
                {
                    SiTRACE ("Error returned from Si2151_Tuner_Block_VCO error = %s\n",Si2151_L1_API_ERROR_TEXT(errcode));
                    return errcode;
                }
            }
            if ((errcode=Si2151_L1_TUNER_STATUS (tuners[2])) != NO_Si2151_ERROR)
            {
                SiTRACE ("Error returned from TunerStatus error = %s\n",Si2151_L1_API_ERROR_TEXT(errcode));
                return errcode;
            }
            /* check the frequency returned from tuner_status.  If the freq=0 (no tune done) then don't update the vco code */
            if (tuners[2]->rsp->tuner_status.freq > 0)
            {
                if ((errcode=Si2151_Tuner_Block_VCO2(tuners[0],tuners[2]->rsp->tuner_status.vco_code)) != NO_Si2151_ERROR)
                {
                    SiTRACE ("Error returned from Si2151_Tuner_Block_VCO error = %s\n",Si2151_L1_API_ERROR_TEXT(errcode));
                    return errcode;
                }
            }
            break;
            case 1:
            if ((errcode=Si2151_L1_TUNER_STATUS (tuners[0])) != NO_Si2151_ERROR)
            {
                SiTRACE ("Error returned from TunerStatus error = %s\n",Si2151_L1_API_ERROR_TEXT(errcode));
                return errcode;
            }
            /* check the frequency returned from tuner_status.  If the freq=0 (no tune done) then don't update the vco code */
            if (tuners[0]->rsp->tuner_status.freq > 0)
            {
                if ((errcode=Si2151_Tuner_Block_VCO(tuners[1],tuners[0]->rsp->tuner_status.vco_code)) != NO_Si2151_ERROR)
                {
                    SiTRACE ("Error returned from Si2151_Tuner_Block_VCO error = %s\n",Si2151_L1_API_ERROR_TEXT(errcode));
                    return errcode;
                }
            }
            if ((errcode=Si2151_L1_TUNER_STATUS (tuners[2])) != NO_Si2151_ERROR)
            {
                SiTRACE ("Error returned from TunerStatus error = %s\n",Si2151_L1_API_ERROR_TEXT(errcode));
                return errcode;
            }
            /* check the frequency returned from tuner_status.  If the freq=0 (no tune done) then don't update the vco code */
            if (tuners[2]->rsp->tuner_status.freq > 0)
            {
                if ((errcode=Si2151_Tuner_Block_VCO2(tuners[1],tuners[2]->rsp->tuner_status.vco_code)) != NO_Si2151_ERROR)
                {
                    SiTRACE ("Error returned from Si2151_Tuner_Block_VCO error = %s\n",Si2151_L1_API_ERROR_TEXT(errcode));
                    return errcode;
                }
            }
            break;
            case 2:
            if ((errcode=Si2151_L1_TUNER_STATUS (tuners[0])) != NO_Si2151_ERROR)
            {
                SiTRACE ("Error returned from TunerStatus error = %s\n",Si2151_L1_API_ERROR_TEXT(errcode));
                return errcode;
            }
            /* check the frequency returned from tuner_status.  If the freq=0 (no tune done) then don't update the vco code */
            if (tuners[0]->rsp->tuner_status.freq > 0)
            {
                if ((errcode=Si2151_Tuner_Block_VCO(tuners[2],tuners[0]->rsp->tuner_status.vco_code)) != NO_Si2151_ERROR)
                {
                    SiTRACE ("Error returned from Si2151_Tuner_Block_VCO error = %s\n",Si2151_L1_API_ERROR_TEXT(errcode));
                    return errcode;
                }
            }
            if ((errcode=Si2151_L1_TUNER_STATUS (tuners[1])) != NO_Si2151_ERROR)
            {
                SiTRACE ("Error returned from TunerStatus error = %s\n",Si2151_L1_API_ERROR_TEXT(errcode));
                return errcode;
            }
            /* check the frequency returned from tuner_status.  If the freq=0 (no tune done) then don't update the vco code */
            if (tuners[1]->rsp->tuner_status.freq > 0)
            {
                if ((errcode=Si2151_Tuner_Block_VCO2(tuners[2],tuners[1]->rsp->tuner_status.vco_code)) != NO_Si2151_ERROR)
                {
                    SiTRACE ("Error returned from Si2151_Tuner_Block_VCO error = %s\n",Si2151_L1_API_ERROR_TEXT(errcode));
                    return errcode;
                }
            }
            break;
        }
    }
    else if (tuner_count==4)
    {
        switch (tuner_num)
        {
            case 0:
                if ((errcode=Si2151_L1_TUNER_STATUS (tuners[1])) != NO_Si2151_ERROR)
                {
                    SiTRACE ("Error returned from TunerStatus error = %s\n",Si2151_L1_API_ERROR_TEXT(errcode));
                    return errcode;
                }
             /* check the frequency returned from tuner_status.  If the freq=0 (no tune done) then don't update the vco code */
                if (tuners[1]->rsp->tuner_status.freq > 0)
                {
                  if ((errcode=Si2151_Tuner_Block_VCO(tuners[0],tuners[1]->rsp->tuner_status.vco_code)) != NO_Si2151_ERROR)
                    {
                        SiTRACE ("Error returned from Si2151_Tuner_Block_VCO error = %s\n",Si2151_L1_API_ERROR_TEXT(errcode));
                        return errcode;
                    }
                }
                if ((errcode=Si2151_L1_TUNER_STATUS (tuners[2])) != NO_Si2151_ERROR)
                {
                    SiTRACE ("Error returned from TunerStatus error = %s\n",Si2151_L1_API_ERROR_TEXT(errcode));
                    return errcode;
                }
                /* check the frequency returned from tuner_status.  If the freq=0 (no tune done) then don't update the vco code */
                if (tuners[2]->rsp->tuner_status.freq > 0)
                {
                   if ((errcode=Si2151_Tuner_Block_VCO2(tuners[0],tuners[2]->rsp->tuner_status.vco_code)) != NO_Si2151_ERROR)
                    {
                        SiTRACE ("Error returned from Si2151_Tuner_Block_VCO error = %s\n",Si2151_L1_API_ERROR_TEXT(errcode));
                        return errcode;
                    }
                }
                if ((errcode=Si2151_L1_TUNER_STATUS (tuners[3])) != NO_Si2151_ERROR)
                {
                    SiTRACE ("Error returned from TunerStatus error = %s\n",Si2151_L1_API_ERROR_TEXT(errcode));
                    return errcode;
                }
                /* check the frequency returned from tuner_status.  If the freq=0 (no tune done) then don't update the vco code */
                if (tuners[3]->rsp->tuner_status.freq > 0)
                {
                   if ((errcode=Si2151_Tuner_Block_VCO3(tuners[0],tuners[3]->rsp->tuner_status.vco_code)) != NO_Si2151_ERROR)
                    {
                        SiTRACE ("Error returned from Si2151_Tuner_Block_VCO error = %s\n",Si2151_L1_API_ERROR_TEXT(errcode));
                        return errcode;
                    }
                }
            break;
            case 1:
                if ((errcode=Si2151_L1_TUNER_STATUS (tuners[0])) != NO_Si2151_ERROR)
                {
                    SiTRACE ("Error returned from TunerStatus error = %s\n",Si2151_L1_API_ERROR_TEXT(errcode));
                    return errcode;
                }
                /* check the frequency returned from tuner_status.  If the freq=0 (no tune done) then don't update the vco code */
                if (tuners[0]->rsp->tuner_status.freq > 0)
                {
                   if ((errcode=Si2151_Tuner_Block_VCO(tuners[1],tuners[0]->rsp->tuner_status.vco_code)) != NO_Si2151_ERROR)
                    {
                        SiTRACE ("Error returned from Si2151_Tuner_Block_VCO error = %s\n",Si2151_L1_API_ERROR_TEXT(errcode));
                        return errcode;
                    }
                }
                if ((errcode=Si2151_L1_TUNER_STATUS (tuners[2])) != NO_Si2151_ERROR)
                {
                    SiTRACE ("Error returned from TunerStatus error = %s\n",Si2151_L1_API_ERROR_TEXT(errcode));
                    return errcode;
                }
                /* check the frequency returned from tuner_status.  If the freq=0 (no tune done) then don't update the vco code */
                if (tuners[2]->rsp->tuner_status.freq > 0)
                {
                   if ((errcode=Si2151_Tuner_Block_VCO2(tuners[1],tuners[2]->rsp->tuner_status.vco_code)) != NO_Si2151_ERROR)
                    {
                        SiTRACE ("Error returned from Si2151_Tuner_Block_VCO error = %s\n",Si2151_L1_API_ERROR_TEXT(errcode));
                        return errcode;
                    }
                }
                if ((errcode=Si2151_L1_TUNER_STATUS (tuners[3])) != NO_Si2151_ERROR)
                {
                    SiTRACE ("Error returned from TunerStatus error = %s\n",Si2151_L1_API_ERROR_TEXT(errcode));
                    return errcode;
                }
                /* check the frequency returned from tuner_status.  If the freq=0 (no tune done) then don't update the vco code */
                if (tuners[3]->rsp->tuner_status.freq > 0)
                {

                   if ((errcode=Si2151_Tuner_Block_VCO3(tuners[1],tuners[3]->rsp->tuner_status.vco_code)) != NO_Si2151_ERROR)
                    {
                        SiTRACE ("Error returned from Si2151_Tuner_Block_VCO error = %s\n",Si2151_L1_API_ERROR_TEXT(errcode));
                        return errcode;
                    }
                }

            break;
            case 2:
                if ((errcode=Si2151_L1_TUNER_STATUS (tuners[0])) != NO_Si2151_ERROR)
                {
                    SiTRACE ("Error returned from TunerStatus error = %s\n",Si2151_L1_API_ERROR_TEXT(errcode));
                    return errcode;
                }
                /* check the frequency returned from tuner_status.  If the freq=0 (no tune done) then don't update the vco code */
                if (tuners[0]->rsp->tuner_status.freq > 0)
                {
                   if ((errcode=Si2151_Tuner_Block_VCO(tuners[2],tuners[0]->rsp->tuner_status.vco_code)) != NO_Si2151_ERROR)
                    {
                        SiTRACE ("Error returned from Si2151_Tuner_Block_VCO error = %s\n",Si2151_L1_API_ERROR_TEXT(errcode));
                        return errcode;
                    }
                }
                if ((errcode=Si2151_L1_TUNER_STATUS (tuners[1])) != NO_Si2151_ERROR)
                {
                    SiTRACE ("Error returned from TunerStatus error = %s\n",Si2151_L1_API_ERROR_TEXT(errcode));
                    return errcode;
                }
                /* check the frequency returned from tuner_status.  If the freq=0 (no tune done) then don't update the vco code */
                if (tuners[1]->rsp->tuner_status.freq > 0)
                {
                    if ((errcode=Si2151_Tuner_Block_VCO2(tuners[2],tuners[1]->rsp->tuner_status.vco_code)) != NO_Si2151_ERROR)
                    {
                        SiTRACE ("Error returned from Si2151_Tuner_Block_VCO error = %s\n",Si2151_L1_API_ERROR_TEXT(errcode));
                        return errcode;
                    }
                }
                if ((errcode=Si2151_L1_TUNER_STATUS (tuners[3])) != NO_Si2151_ERROR)
                {
                    SiTRACE ("Error returned from TunerStatus error = %s\n",Si2151_L1_API_ERROR_TEXT(errcode));
                    return errcode;
                }
                /* check the frequency returned from tuner_status.  If the freq=0 (no tune done) then don't update the vco code */
                if (tuners[3]->rsp->tuner_status.freq > 0)
                {
                   if ((errcode=Si2151_Tuner_Block_VCO3(tuners[2],tuners[3]->rsp->tuner_status.vco_code)) != NO_Si2151_ERROR)
                    {
                        SiTRACE ("Error returned from Si2151_Tuner_Block_VCO error = %s\n",Si2151_L1_API_ERROR_TEXT(errcode));
                        return errcode;
                    }
                }
            break;
                case 3:
                if ((errcode=Si2151_L1_TUNER_STATUS (tuners[0])) != NO_Si2151_ERROR)
                {
                    SiTRACE ("Error returned from TunerStatus error = %s\n",Si2151_L1_API_ERROR_TEXT(errcode));
                    return errcode;
                }
                /* check the frequency returned from tuner_status.  If the freq=0 (no tune done) then don't update the vco code */
                if (tuners[0]->rsp->tuner_status.freq > 0)
                {
                   if ((errcode=Si2151_Tuner_Block_VCO(tuners[3],tuners[0]->rsp->tuner_status.vco_code)) != NO_Si2151_ERROR)
                    {
                        SiTRACE ("Error returned from Si2151_Tuner_Block_VCO error = %s\n",Si2151_L1_API_ERROR_TEXT(errcode));
                        return errcode;
                    }
                }
                if ((errcode=Si2151_L1_TUNER_STATUS (tuners[1])) != NO_Si2151_ERROR)
                {
                    SiTRACE ("Error returned from TunerStatus error = %s\n",Si2151_L1_API_ERROR_TEXT(errcode));
                    return errcode;
                }
                /* check the frequency returned from tuner_status.  If the freq=0 (no tune done) then don't update the vco code */
                if (tuners[1]->rsp->tuner_status.freq > 0)
                {
                   if ((errcode=Si2151_Tuner_Block_VCO2(tuners[3],tuners[1]->rsp->tuner_status.vco_code)) != NO_Si2151_ERROR)
                    {
                        SiTRACE ("Error returned from Si2151_Tuner_Block_VCO error = %s\n",Si2151_L1_API_ERROR_TEXT(errcode));
                        return errcode;
                    }
                }
                if ((errcode=Si2151_L1_TUNER_STATUS (tuners[2])) != NO_Si2151_ERROR)
                {
                    SiTRACE ("Error returned from TunerStatus error = %s\n",Si2151_L1_API_ERROR_TEXT(errcode));
                    return errcode;
                }
                /* check the frequency returned from tuner_status.  If the freq=0 (no tune done) then don't update the vco code */
                if (tuners[2]->rsp->tuner_status.freq > 0)
                {
                   if ((errcode=Si2151_Tuner_Block_VCO3(tuners[3],tuners[2]->rsp->tuner_status.vco_code)) != NO_Si2151_ERROR)
                    {
                        SiTRACE ("Error returned from Si2151_Tuner_Block_VCO error = %s\n",Si2151_L1_API_ERROR_TEXT(errcode));
                        return errcode;
                    }
                }
            break;
       }

    }
 return NO_Si2151_ERROR;

}
/************************************************************************************************************************
  NAME: Si2151_L2_VCO_Blocking_PostTune
  DESCRIPTION: Si2151 user menu function
              Call this function after tuning.  The VCO_CODE from the TUNER_STATUS command will be sent to all the other
              tuners and the TUNER_BLOCK_VCO properties will be updated after the tune command is called.
  Behavior:   This is a more efficient method than setting up the VCO blocking using the pretune function but there are
               cases where the pretune method is needed (for example if tuners are powered up and down or put in standby).

************************************************************************************************************************/
int Si2151_L2_VCO_Blocking_PostTune(L1_Si2151_Context *tuners[], int tuner_num, int tuner_count)
{

  int  ((*Tuner_Block_VCO_ptr[3])( )) = {Si2151_Tuner_Block_VCO, Si2151_Tuner_Block_VCO2, Si2151_Tuner_Block_VCO3};
  int errcode;
  int vco_dest[]={0,0,0,0};
  int vco_fn[]={0,0,0,0};
  int i;
  int c;
  int vco_code;

/* if only using 1 tuner then no need for this function */
  if (tuner_count == 1)
  {
    return NO_Si2151_ERROR;
  }

  SiTRACE("Si2151_L2_VCO_Blocking_PostTune (tuners, tuner_num %d, tuner_count %d)\n", tuner_num, tuner_count);

  if ((errcode=Si2151_L1_TUNER_STATUS (tuners[tuner_num])) != NO_Si2151_ERROR)
  {
    SiTRACE ("Error returned from TunerStatus error = %s\n",Si2151_L1_API_ERROR_TEXT(errcode));
    return errcode;
  }

  vco_code = tuners[tuner_num]->rsp->tuner_status.vco_code;
  SiTRACE("Si2151_L2_VCO_Blocking_PostTune vco_code 0x%04x\n", vco_code);
/* set up a table of tuner destinations to write the properties to. */
/* tuner 0 = 1 2 3  */
/* tuner 1  = 0 2 3  */
/* tuner 2  = 0 1 3  */
/* tuner 3  = 0 1 2  */

  c = 0;
  for ( i = 0 ; i < tuner_count; i++ ) {
    if (i != tuner_num ) { vco_dest[c++] = i; }
  }
/* set up table of functions to avoid property collisions */
/* tuner 0 = 0 1 2  */
/* tuner 1  = 0 0 0  */
/* tuner 2  = 1 1 1  */
/* tuner 3  = 2 2 2  */
  for (i=0; i< tuner_count; i++)
  {
    if (tuner_num==0)
    {
      vco_fn[i]=i;
    }
    else
    {
      vco_fn[i]=tuner_num-1;
    }
  }

  SiTRACE("Si2151_L2_VCO_Blocking_PostTune tuner_num %d ==> vco_dest[0] =    %d,  vco_dest[1] =    %d,  vco_dest[2] =    %d \n", tuner_num, vco_dest[0], vco_dest[1], vco_dest[2]);
  SiTRACE("Si2151_L2_VCO_Blocking_PostTune tuner_num %d ==> vco_fn  [0] =    %d,  vco_fn  [1] =    %d,  vco_fn  [2]    = %d \n", tuner_num, vco_fn[0], vco_fn[1], vco_fn[2]);

   if (tuner_count >  1) {
    if ((errcode=Tuner_Block_VCO_ptr[vco_fn[0]]  (tuners[vco_dest[0]], vco_code)) != NO_Si2151_ERROR)
    {
      SiTRACE ("Error returned from SiLabs_TER_Tuner_Block_VCO_Code error = %d\n", errcode);
      return errcode;
    }
  }
  if (tuner_count >  2) {
    if ((errcode=Tuner_Block_VCO_ptr[vco_fn[1]]  (tuners[vco_dest[1]], vco_code)) != NO_Si2151_ERROR)
    {
      SiTRACE ("Error returned from SiLabs_TER_Tuner_Block_VCO2_Code error = %d\n", errcode);
      return errcode;
    }
  }
  if (tuner_count >  3) {
    if ((errcode=Tuner_Block_VCO_ptr[vco_fn[2]]  (tuners[vco_dest[2]], vco_code)) != NO_Si2151_ERROR)
    {
      SiTRACE ("Error returned from SiLabs_TER_Tuner_Block_VCO3_Code error = %d\n", errcode);
      return errcode;
    }
  }

  return NO_Si2151_ERROR;
}




