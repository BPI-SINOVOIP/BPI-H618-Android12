/*
**********************************************************************************************************************
*											        eGon
*						                     the Embedded System
*									       boot pack sub-system
*
*						  Copyright(C), 2006-2017, SoftWinners Microelectronic Co., Ltd.
*                                           All Rights Reserved
*
* File    : script.c
*
* Version : V2.00
*
* Date	  :
*
* Descript:
**********************************************************************************************************************
*/
#ifndef __PART_H__
#define __PART_H__

#define	MAX_PATH				320

#define	EMMC_CAPACITY			(8 * 1024 * 1024)
extern  int get_partition_file(char *part_name, char *file_name);
extern  int get_img_name(char *img_name);
extern  int set_img_name(char *img_name);
extern  int write_normal_part2img(FILE *img_file, sunxi_mbr_t *mbr_info, int mbr_size);
extern  int write_boot_part(char *boot0_name, char *boot1_name, FILE *img_file);
extern 	int write_mbr_file(char *mbr_name, FILE *img_file, int mbr_fill_size);
extern  int write_standar_mbr_file(sunxi_mbr_t *mbr_info, FILE *img_file);
extern  int write_gpt_file(char *gpt_name, FILE *img_file);
#endif

