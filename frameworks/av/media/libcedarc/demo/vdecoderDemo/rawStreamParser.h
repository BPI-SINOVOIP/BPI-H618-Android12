/*
 * Copyright (c) 2008-2016 Allwinner Technology Co. Ltd.
 * All rights reserved.
 *
 * File : rawStreamParser.h
 * Description : rawStreamParser
 * History :
 *
 */

#include <stdio.h>
#include <stdint.h>

#ifndef RAW_STREAM_PARSER_H
#define RAW_STREAM_PARSER_H

#define VIDEO_FORMAT_H264 0x1
#define VIDEO_FORMAT_H265 0x2
#define VIDEO_FORMAT_AV1  0x3

typedef struct DataPacketS
{
    void *buf;
    void *ringBuf;
    int buflen;
    int ringBufLen;
    int64_t pts;
    int64_t length;
    int64_t pcr;
}DataPacketT;

typedef struct StreamBufMan{
    char* pStreamBuffer;
    int nCurIndex;
    int nBufferLen;
    int nValidSize;
}StreamBufMan;

typedef struct RawParserContext{
    FILE* fd;
    int streamType;
    StreamBufMan mBufManager;
}RawParserContext;

typedef struct {
    uint64_t obu_header_size;
    unsigned obu_type;
    uint64_t obu_size;  //leb128(), contains the size in bytes of the OBU not including the bytes within obu_header or the obu_size syntax
    int extension_flag;
    int has_size_field;

    //extension_flag == 1
    int temporal_id;
    int spatial_id;
} OBU_t;

RawParserContext* RawParserOpen(FILE* fd);
int RawParserClose(RawParserContext* pCtx);
int RawParserProbe(RawParserContext* pCtx);
int RawParserPrefetch(RawParserContext* pCtx, DataPacketT* pPkt);
int RawParserRead(RawParserContext* pCtx, DataPacketT* pPkt);
int get_obu(OBU_t *obu, char* src);

#endif
