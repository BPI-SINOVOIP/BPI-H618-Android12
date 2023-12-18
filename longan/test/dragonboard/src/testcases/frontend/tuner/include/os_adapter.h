#ifndef OS_ADAPTER_H
#define OS_ADAPTER_H

#include "DataType.h"

#define I2C_BUS_DEV                  "/dev/i2c-2"
typedef struct _I2C_LEN_TYPE
{
	U8 RegAddr;
	U8 Data[50];
	U8 Len;
} I2C_LEN_TYPE;

typedef struct _I2C_TYPE
{
	U8 RegAddr;
	U8 Data;
} I2C_TYPE;

typedef struct i2c_device {
    int fd;
    int chip_addr;
} I2C_T;

void msleep(UINT16 millisecond);
void mdelay(UINT16 millisecond);

INT32 i2c_connection_check(const INT8* path, U8 chip_addr);
INT32 I2cWrite(U8 *pData, UINT32 size);
INT32 I2cRead(U8 *pData, UINT32 size);
INT8 ReadRegWithDataLen_tv(U8* reg_addr, UINT16 addr_size, U8* data,  UINT16 size);
INT8 WriteRegWithDataLen_tv(U8* reg_addr, UINT16 addr_size, U8* data,  UINT16 size);
U8 I2C_Read_Len_R842(I2C_LEN_TYPE *I2C_Info);
U8 I2C_Write_Len(I2C_LEN_TYPE *I2C_Info);
U8 I2C_Write(I2C_TYPE *I2C_Info);

#endif