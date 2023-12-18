/******************************************************************************
 *
 *  Copyright(C), 2015, Xradio Technology Co., Ltd.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at:
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 ******************************************************************************/

/******************************************************************************
 *
 *  Filename:      cmd.h
 *
 *  Description:   Data structure definition for brom protocol
 *
 ******************************************************************************/

#ifndef _COMMAND_H_
#define _COMMAND_H_

#include "type.h"

#define ENABLE_DCS     0

#define CMD_REVESION   0x0000 // 0.0.0.0
#define CMD_SYNC_WORD  0x55
#define CMD_ID(group, key)   (((group) << 3) | (key))

/*----------------------------*/
/*   COMMANDS FORM PC TO MCU  */
/*----------------------------*/
#define CMD_ID_MEMRW   0x00
#define CMD_ID_SEQRQ   0x01
#define CMD_ID_SYSCTL  0x02
#define CMD_ID_FLASH   0x03
/* memory access commands */
#define CMD_ID_READ1   CMD_ID(CMD_ID_MEMRW, 0)
#define CMD_ID_WRITE1  CMD_ID(CMD_ID_MEMRW, 1)
#define CMD_ID_READ2   CMD_ID(CMD_ID_MEMRW, 2)
#define CMD_ID_WRITE2  CMD_ID(CMD_ID_MEMRW, 3)
#define CMD_ID_READ4   CMD_ID(CMD_ID_MEMRW, 4)
#define CMD_ID_WRITE4  CMD_ID(CMD_ID_MEMRW, 5)
#define CMD_ID_READ8   CMD_ID(CMD_ID_MEMRW, 6)
#define CMD_ID_WRITE8  CMD_ID(CMD_ID_MEMRW, 7)

#define CMD_ID_SEQRD   CMD_ID(CMD_ID_SEQRQ, 0)
#define CMD_ID_SEQWR   CMD_ID(CMD_ID_SEQRQ, 1)
/* uart commands */
#define CMD_ID_SETUART CMD_ID(CMD_ID_SYSCTL, 0)
#define CMD_ID_SETJTAG CMD_ID(CMD_ID_SYSCTL, 1)
#define CMD_ID_REBOOT  CMD_ID(CMD_ID_SYSCTL, 2)
#define CMD_ID_SETPC   CMD_ID(CMD_ID_SYSCTL, 3)
#define CMD_ID_SETCKCS CMD_ID(CMD_ID_SYSCTL, 4)
/* flash operation commands */
#define CMD_ID_FLASH_GETINFO CMD_ID(CMD_ID_FLASH, 0)
#define CMD_ID_FLASH_ERASE   CMD_ID(CMD_ID_FLASH, 1)
#define CMD_ID_FLASH_READ    CMD_ID(CMD_ID_FLASH, 2)
#define CMD_ID_FLASH_WRITE   CMD_ID(CMD_ID_FLASH, 3)

/*----------------------------*/
/*   COMMANDS FORM MCU TO PC  */
/*----------------------------*/
/* message output */
#define CMD_ID_SENDMSG CMD_ID(0, 0)

#pragma pack(1)
/* command header
 *
 *    byte 0    byte 1    byte 2   byte 3     byte 4    byte 5    byte 6 -7          byte 8-11
 *  ___________________________________________________________________________________________________
 * |         |         |         |         |         |         |                   |                   |
 * |   'B'   |   'R'   |   'O'   |   'M'   |  Flags  |Reserved | Checksum          | Playload Length   |
 * |_________|_________|_________|_________|_________|_________|__________ ________|___________________|
 */
typedef struct {
    u8 magic[4]; // magic "BROM"
        #define CMD_BROM_MAGIC    "BROM"
    u8 flags;
        #define CMD_HFLAG_ERROR   (0x1U << 0)
        #define CMD_HFLAG_ACK     (0x1U << 1)
        #define CMD_HFLAG_CHECK   (0x1U << 2)
        #define CMD_HFLAG_RETRY   (0x1U << 3)
        #define CMD_HFLAG_EXE     (0x1U << 4)
    u8 version:4;
    u8 reserved:4;
    u16 checksum;
    u32 payload_len;
} __attribute__((packed)) cmd_header_t;
#define MB_CMD_HEADER_SIZE (sizeof(cmd_header_t))

typedef struct {
    cmd_header_t h;
    u8 cmdid;
} __attribute__((packed)) cmd_header_id_t;
#define MB_HEADER_TO_ID(h) (((cmd_header_id_t*)(h))->cmdid)

/* acknownledge structure */
typedef struct {
    cmd_header_t h;
    u8 err;
} __attribute__((packed)) cmd_ack_t;

/* memory read/write command structure */
#define CMD_RW_DATA_POS (MB_CMD_HEADER_SIZE + 5)
#define CMD_RW_DATA_LEN(id) (1 << ((id >> 1) & 0x3))
typedef struct {
    cmd_header_t h;
    u8  cmdid;
    u32 addr;
} __attribute__((packed)) cmd_rw_t;

/* sequence read/write command structure */
#define CMD_SEQRW_DATA_POS (MB_CMD_HEADER_SIZE + 5)
typedef struct {
    cmd_header_t h;
    u8  cmdid;
    u32 addr;
    u32 dlen;
    u16 dcs;
} __attribute__((packed)) cmd_seq_wr_t;

typedef struct {
    cmd_header_t h;
    u8  cmdid;
    u32 addr;
    u32 dlen;
} __attribute__((packed)) cmd_seq_rd_t;

/* io change command structure */
#define CMD_SYS_DATA_POS (MB_CMD_HEADER_SIZE + 1)
typedef struct {
    cmd_header_t h;
    u8  cmdid;
    u32 val;
} __attribute__((packed)) cmd_sys_t;

typedef struct {
    cmd_header_t h;
    u8  cmdid;
    u32 lcr;
} __attribute__((packed)) cmd_sys_setuart_t;

typedef struct {
    cmd_header_t h;
    u8 cmdid;
    u8 mode;
} __attribute__((packed)) cmd_sys_setjtag_t;

typedef struct {
    cmd_header_t h;
    u8 cmdid;
    u8 mode;
} __attribute__((packed)) cmd_sys_setcs_t;

/* flash command structure */
typedef struct {
    cmd_header_t h;
    u8 cmdid;
    u8 erase_cmd;
    u32 addr;
} __attribute__((packed)) cmd_flash_er_t;

typedef struct {
    cmd_header_t h;
    u8  cmdid;
    u32 sector;
    u32 num;
    u16 dcs;
} __attribute__((packed)) cmd_flash_wr_t;

typedef struct {
    cmd_header_t h;
    u8  cmdid;
    u32 sector;
    u32 num;
} __attribute__((packed)) cmd_flash_rd_t;

#pragma pack()

/* response error type */
#define MB_ERR_UNKNOWNCMD   (1)
#define MB_ERR_TIMEOUT      (2)
#define MB_ERR_CHECKSUM     (3)
#define MB_ERR_INVALID      (4)
#define MB_ERR_NOMEM        (5)

s32 cmd_sync_uart(void);
s32 cmd_sync_baud(u32 lcr);
s32 cmd_write_seq(u32 addr, u32 len, u8 *data);
s32 cmd_read_seq(u32 addr, u32 len, u8 *data);
s32 cmd_write_byte(u32 addr, u32 len, u8 *data);
s32 cmd_set_pc(u32 pc);

#endif
