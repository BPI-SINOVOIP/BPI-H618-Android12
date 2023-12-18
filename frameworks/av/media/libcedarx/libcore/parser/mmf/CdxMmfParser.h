/*
* Copyright (c) 2008-2019 Allwinner Technology Co. Ltd.
* All rights reserved.
*
* File : CdxMmfParser.h
* Description :
* History :
*   Date    : 2019/11/19
*/

#ifndef CDX_MMF_PARSER_H
#define CDX_MMF_PARSER_H

typedef struct MmfObjectStruct
{
    cdx_int32    UID;
    cdx_int32    Dsize;
}MMFObjectT;

typedef struct MmfFormatStruct
{
    cdx_uint16 formaTag;
    cdx_int16 nChannel;
    cdx_int32 nSampleRate;
    cdx_int32 nBitsPerSample;//码流每样点传输速率
    cdx_int32 nBitsRate;//码流每分传输速率
    cdx_int16 nBlockAlign;//一个采样点所占字节，即一帧的大小
}MmfFormatT;

typedef struct MmfParserImplS
{
    //audio common
    CdxParserT  base;
    CdxStreamT  *stream; //和文件流相关
    pthread_cond_t  cond;//锁

    cdx_int64   ulDuration;//音频时长，单位ms
    cdx_int64   dFileSize;//文件总长
    cdx_int32   mErrno; //Parser 当前状态
    cdx_uint32  nFlags; //控制命令
    //mmf base
    MmfFormatT  MmfFormat;
    cdx_int64   pts;//当前pts
    cdx_int64   nHeadLen;//音频数据前的部分的大小
    cdx_int64   pkt_length;//没有播放的音频数据大小
}MmfParserImplS;

#define    MMMD    0X444D4D4D
#define    CNTI    0X49544E43
#define    OPDA    0x4144504F
#define    ATR     0x00525441
#define    Atsq    0x71737441
#define    AspI    0x49707341
#define    Awa     0x00617741

#endif
