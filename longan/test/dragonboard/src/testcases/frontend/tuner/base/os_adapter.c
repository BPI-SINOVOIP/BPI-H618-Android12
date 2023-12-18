
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>
#include <pthread.h>
#include <string.h>
#include <sys/ioctl.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>

#include "os_adapter.h"
#include "fe_log.h"

#define RETRY_TIMES             (5)
#define SINGLE_I2C_MSG          (1)
#define DOUBLE_I2C_MSG          (2)

static I2C_T *i2cdev = NULL;

static INT32 R842_Convert(int InvertNum);

void msleep(UINT16 millisecond)
{
    usleep(1000 * millisecond);//ms
}

void mdelay(UINT16 millisecond)
{
    usleep(1000 * millisecond);//ms
}

static INT32 i2c_open(const char* dev, U8 chip_addr)
{
    int fd;

    if (!i2cdev) {
        FE_LOGE("i2cdev pointer is NULL! please check\n");
        return -1;
    }

    fd = open(dev, O_RDWR);
    if (fd < 0) {
        FE_LOGE("open %s failed (%d)\n", dev, fd);
        return -1;
    }

    i2cdev->fd = fd;
    i2cdev->chip_addr = chip_addr;

    return 0;
}

static INT32 i2c_close()
{
    if (!i2cdev) {
        FE_LOGE("i2cdev pointer is NULL! please check\n");
        return -1;
    }

    close(i2cdev->fd);

    return 0;
}

INT32 i2c_connection_check(const INT8* path, U8 chip_addr)
{
    INT32 ret;
    I2C_T *i2c;

    if (!i2cdev) {
        i2c = (I2C_T *)malloc(sizeof(I2C_T));
        if(!i2c) {
            FE_LOGE("malloc i2c device error!\n");
            return -1;
        }
        memset(i2c, 0, sizeof(I2C_T));
        i2cdev = i2c;
    } else {
        i2c_close();
    }

    ret = i2c_open(path, chip_addr);
    if (ret) {
        FE_LOGE("open i2cdev[%s] chip_addr=0x%x failed\n", path, chip_addr);
        return -1;
    }

    return 0;
}

INT32 I2cWrite(U8 *pData, UINT32 size)
{
    U8 *buf = pData;
    struct i2c_msg msgs[SINGLE_I2C_MSG];
    struct i2c_rdwr_ioctl_data ioctl_data;
    int nmsgs_sent;

    if (!i2cdev) {
        FE_LOGE("Please call i2c_connection_check first\n");
        return -1;
    }

    memset(msgs, 0, sizeof(struct i2c_msg));
    memset(&ioctl_data, 0, sizeof(struct i2c_rdwr_ioctl_data));

    msgs[0].addr = i2cdev->chip_addr >> 1;
    msgs[0].flags = 0;
    msgs[0].buf = buf;
    msgs[0].len = size;
    ioctl_data.nmsgs = SINGLE_I2C_MSG;
    ioctl_data.msgs = msgs;

#if IIC_DEBUG
    FE_LOGD("write: %d bytes\n", size);
    for (int i = 0; i < size; i++)
        FE_LOGD("0x%02x ", buf[i]);
    FE_LOGD(" END!\n");
#endif

    nmsgs_sent = ioctl(i2cdev->fd, I2C_RDWR, &ioctl_data);
    if (nmsgs_sent < 0) {
        FE_LOGE("Read I2C Failed, errno:%s!", strerror(errno));
        return -1;
    } else if (nmsgs_sent < SINGLE_I2C_MSG) {
        FE_LOGE("Read I2C: only %d/%d messages were sent\n", nmsgs_sent, ioctl_data.nmsgs);
        return -1;
    }

    return nmsgs_sent == SINGLE_I2C_MSG ? size : -1;
}

INT32 I2cRead(U8 *pData, UINT32 size)
{
    struct i2c_msg msgs[SINGLE_I2C_MSG];
    struct i2c_rdwr_ioctl_data ioctl_data;
    int nmsgs_sent;

    if (!i2cdev) {
        FE_LOGE("Please call i2c_connection_check first\n");
        return -1;
    }

    memset(msgs, 0, sizeof(struct i2c_msg));
    memset(&ioctl_data, 0, sizeof(struct i2c_rdwr_ioctl_data));

    msgs[0].addr = i2cdev->chip_addr >> 1;
    msgs[0].flags = I2C_M_RD;
    msgs[0].buf = pData;
    msgs[0].len = size;
    ioctl_data.nmsgs = SINGLE_I2C_MSG;
    ioctl_data.msgs = msgs;
    nmsgs_sent = ioctl(i2cdev->fd, I2C_RDWR, &ioctl_data);
    if (nmsgs_sent < 0) {
        FE_LOGE("Read I2C Failed, errno:%s!", strerror(errno));
        return -1;
    } else if (nmsgs_sent < SINGLE_I2C_MSG) {
        FE_LOGE("Read I2C: only %d/%d messages were sent\n", nmsgs_sent, ioctl_data.nmsgs);
        return -1;
    }

    return nmsgs_sent == SINGLE_I2C_MSG ? size : -1;
}

/*****************************************************************************
 Prototype    : BYTE WriteRegWithDataLen_tv(BYTE chip_addr, BYTE* reg_addr, BYTE addr_size, BYTE* Data,  BYTE size);
 Description  : Write Reg
 Input        : chip_addr: 芯片IIC地址，reg_addr: 寄存器地址数组名，addr_size: 地址数组大小， Data:写数据数组，size: 
数据数组大小
 Return Value : 通讯中产生的错误信息，当通讯错误时可供参考
*****************************************************************************/
INT8 WriteRegWithDataLen_tv(U8* reg_addr, UINT16 addr_size, U8* data,  UINT16 size)
{
    int nmsgs_sent = 0;
    struct i2c_rdwr_ioctl_data ioctlmsg;

    if (!i2cdev) {
        FE_LOGE("Please call i2c_connection_check first\n");
        return -1;
    }

#if IIC_DEBUG
    FE_LOGD("write addr: 0x%x, reg:0x%x, addr_size=%d, data:0x%x, size=%d",
             i2cdev->chip_addr, reg_addr[0], addr_size, data[0], size);
#endif

    struct i2c_msg msgs[DOUBLE_I2C_MSG] =
    {
        {
            .addr = i2cdev->chip_addr >> 1,
            .flags = 0,
            .len = addr_size,
            .buf = reg_addr,
        }, {
            .addr = i2cdev->chip_addr >> 1,
            .flags = 0,
            .len = size,
            .buf = data,
        },
    };
    ioctlmsg.msgs = msgs;
    ioctlmsg.nmsgs = DOUBLE_I2C_MSG;

    nmsgs_sent = ioctl(i2cdev->fd, I2C_RDWR, (unsigned long)&ioctlmsg);
    if (nmsgs_sent < 0) {
        FE_LOGE("Write I2C Failed, errno:%s!", strerror(errno));
        return -1;
    } else if (nmsgs_sent < DOUBLE_I2C_MSG) {
        FE_LOGE("Write I2C: only %d/%d messages were sent\n", nmsgs_sent, ioctlmsg.nmsgs);
        return -1;
    }

    return 0;
}

/*****************************************************************************
 Prototype    : BYTE ReadRegWithDataLen_tv(BYTE chip_addr, BYTE* reg_addr, BYTE addr_size, BYTE* pData,  BYTE size);
 Description  : Write Reg
 Input        : chip_addr: 芯片IIC地址，reg_addr: 寄存器地址数组名，addr_size: 地址数组大小， size: 数据数组大小
 Output       : pData:读数据数组
 Return Value : 通讯中产生的错误信息，当通讯错误时可供参考
*****************************************************************************/
INT8 ReadRegWithDataLen_tv(U8* reg_addr, UINT16 addr_size, U8* data,  UINT16 size)
{
    int nmsgs_sent = 0;
    struct i2c_rdwr_ioctl_data ioctlmsg;

    if (!i2cdev) {
        FE_LOGE("Please call i2c_connection_check first\n");
        return -1;
    }

#if IIC_DEBUG
    FE_LOGD("read addr: 0x%x, reg:0x%x %x, addr_size=%d, data:0x%x, size=%d",
             chip_addr, reg_addr[0], reg_addr[1], addr_size, data[0], size);
#endif

    struct i2c_msg msgs[DOUBLE_I2C_MSG] =
    {
        {
            .addr = i2cdev->chip_addr >> 1,
            .flags = 0,
            .len = addr_size,//  2
            .buf = reg_addr,// 0x0000
        }, {
            .addr = i2cdev->chip_addr >> 1,
            .flags = I2C_M_RD,
            .len = size,
            .buf = data,
        },
    };
    ioctlmsg.msgs = msgs;
    ioctlmsg.nmsgs = DOUBLE_I2C_MSG;

    nmsgs_sent = ioctl(i2cdev->fd, I2C_RDWR, (unsigned long)&ioctlmsg);
    if (nmsgs_sent < 0) {
        FE_LOGE("Read I2C Failed, errno:%s!", strerror(errno));
        return -1;
    } else if (nmsgs_sent < DOUBLE_I2C_MSG) {
        FE_LOGE("Read I2C: only %d/%d messages were sent\n", nmsgs_sent, ioctlmsg.nmsgs);
        return -1;
    }

    return 0;
}

static INT32 R842_Convert(int InvertNum)
{
    int ReturnNum = 0;
    int AddNum    = 0x80;
    int BitNum    = 0x01;
    int CuntNum   = 0;

    for(CuntNum = 0;CuntNum < 8;CuntNum ++) {
        if(BitNum & InvertNum)
            ReturnNum += AddNum;

        AddNum /= 2;
        BitNum *= 2;
    }

    return ReturnNum;
}

/* return value: 1 - ok, 0 - fail */
U8 I2C_Read_Len_R842(I2C_LEN_TYPE *I2C_Info)
{
    U8 u8i= 0, u8Ret;
    U8 u8Data[255] = {0};

    for(u8i = 0; u8i < RETRY_TIMES; u8i++) {
        u8Ret = I2cRead(u8Data, I2C_Info->Len);
        if(u8Ret == I2C_Info->Len) {
            for(u8i = 0; u8i < I2C_Info->Len; u8i++)
            I2C_Info->Data[u8i] = R842_Convert(u8Data[u8i]);

            return 1;
        }
    }

    FE_LOGE("I2C_Read_Len_R842 fail\n");
    return 0;
}

/* return value: 1 - ok, 0 - fail */
U8 I2C_Write_Len(I2C_LEN_TYPE *I2C_Info)
{
    U8 u8i= 0, totalmsgLen;
    U8 *buf = NULL;
    INT32 ret;

    totalmsgLen = I2C_Info->Len + 1;
    buf = malloc(totalmsgLen);
    if (!buf) {
        FE_LOGE("I2C_Write_Len malloc fail\n");
        return 0;
    }
    buf[0] = I2C_Info->RegAddr;
    memcpy(buf+1, I2C_Info->Data, I2C_Info->Len);

    for(u8i = 0; u8i < RETRY_TIMES; u8i++) {
#if IIC_DEBUG
        U8 i;
        FE_LOGD("read_len: reg:0x%x ",I2C_Info->RegAddr);
        for (i = 0; i < I2C_Info->Len; i++)
            FE_LOGD(" data:0x%x ", I2C_Info->Data[i]);

        FE_LOGD("size = %d\n", I2C_Info->Len);
#endif
        ret = I2cWrite(buf, totalmsgLen);
        if(ret == totalmsgLen) {
            /* check ok */
            free(buf);
            return 1;
        }
    }

    free(buf);
    FE_LOGE("I2C_Write_Len fail\n");
    return 0;
}

/* return value: 1 - ok, 0 - fail */
U8 I2C_Write(I2C_TYPE *I2C_Info)
{
    U8 u8i= 0, buf[2];
    INT32 ret;

    buf[0] = I2C_Info->RegAddr;
    buf[1] = I2C_Info->Data;

    for(u8i = 0; u8i < RETRY_TIMES; u8i++) {
#if IIC_DEBUG
        FE_LOGD("I2C_Write  reg:0x%x , addr_size=%d, data:0x%x, size=%d",
                 I2C_Info->RegAddr, 1, I2C_Info->Data, 1);
#endif
        ret = I2cWrite(buf, 2);
        if(ret == 2) {
            /* check ok */
            return 1;
        }
    }

    FE_LOGE("I2C_Write fail\n");
    return 0;
}