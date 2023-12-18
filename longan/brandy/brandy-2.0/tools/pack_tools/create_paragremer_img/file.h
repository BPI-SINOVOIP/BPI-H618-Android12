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
#ifndef __FILE_H__
#define __FILE_H__

extern  int get_file_name(char *path, char *name);
extern  int IsFullName(const char *FilePath);
extern  void GetFullPath(char *dName, const char *sName);
extern  int write_file_data(FILE *src, off_t *src_size, FILE *dest, off_t byte_addr);
extern  int append_file_data(FILE *src, off_t *src_size, FILE *dest);
extern  int fill_file_data(FILE *dest, off_t fill_size);
extern  int write_data2file(FILE *dest, off_t byte_addr, char *data, off_t data_size);
#endif