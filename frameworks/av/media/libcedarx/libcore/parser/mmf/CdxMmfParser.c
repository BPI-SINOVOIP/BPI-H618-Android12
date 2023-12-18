/*
* Copyright (c) 2008-2019 Allwinner Technology Co. Ltd.
* All rights reserved.
*
* File : CdxMmfParser.c
* Description :
*   Date    : 2019/11/19
*   Comment : 创建初始版本，实现MMF 的解复用功能
*/

#include <CdxTypes.h>
#include <CdxParser.h>
#include <CdxStream.h>
#include <CdxMemory.h>
#include <CdxMmfParser.h>

static const cdx_int32 mmf_rate_table[] = { 4000, 8000, 11025, 22050, 44100 };//0-4

static cdx_int32 mmf_sample_rate(cdx_int32 rateBit)
{
    if ((rateBit < 0) || (rateBit > 4))
        return -1;
    return mmf_rate_table[rateBit];
}

static int MmfInit(CdxParserT* parameter)
{
    cdx_int32 ret =0;
    cdx_int64 size = 4;
    cdx_int32 rate = 0;
    cdx_int32 params = 0;
    cdx_int32 channel = 0;
    cdx_int64 headerSize = 0;

    struct MmfParserImplS *impl = NULL;
    MMFObjectT MMFObject;

    impl = (MmfParserImplS *)parameter;

    ret = CdxStreamRead(impl->stream, (void *)&MMFObject, 8);
    headerSize += 8;
    if(ret != 8)
        goto OPENFAILURE;
    if(MMFObject.UID != MMMD)
        goto OPENFAILURE;

    for (;; CdxStreamSkip(impl->stream, size))
    {
        CdxStreamRead(impl->stream, (void *)&MMFObject, 4);
        size = CdxStreamGetBE32(impl->stream);
        headerSize += 8 + size;
        if (MMFObject.UID == CNTI)
            continue;
        if (MMFObject.UID == OPDA)
            continue;
        break;
    }
    headerSize -= size;

    if ((MMFObject.UID & 0xffffff) != ATR)
        goto OPENFAILURE;

    CdxStreamGetU8(impl->stream);//format type
    CdxStreamGetU8(impl->stream);//sequence type

    params = CdxStreamGetU8(impl->stream);//wave type b7->channel, b4-6->format, b0-3->sampleRate
    channel = (params >> 7) + 1;//b7: 0 M, 1 S
    rate = mmf_sample_rate(params & 0x0f);

    CdxStreamGetU8(impl->stream);//wave type base bit
    CdxStreamGetU8(impl->stream);//time base d
    CdxStreamGetU8(impl->stream);//time base g
    headerSize += 6;

    for (;; CdxStreamSkip(impl->stream, size))
    {
        CdxStreamRead(impl->stream, (void *)&MMFObject, 4);
        size = CdxStreamGetBE32(impl->stream);
        headerSize += 8 + size;
        if (MMFObject.UID == Atsq)
            continue;
        if (MMFObject.UID == AspI)
            continue;
        break;
    }
    headerSize -= size;

    if ((MMFObject.UID & 0xffffff) != Awa)
        goto OPENFAILURE;

    impl->nHeadLen = headerSize;
    impl->pkt_length = size;
    impl->dFileSize = CdxStreamSize(impl->stream);

    impl->MmfFormat.formaTag = AUDIO_CODEC_FORMAT_ADPCM;
    impl->MmfFormat.nChannel = channel;
    impl->MmfFormat.nSampleRate = rate;
    impl->MmfFormat.nBitsPerSample = 4;
    impl->MmfFormat.nBitsRate = impl->MmfFormat.nBitsPerSample * rate;
    impl->MmfFormat.nBlockAlign = channel * 16 /8;
    impl->ulDuration = 1000 * size * 8 / impl->MmfFormat.nBitsRate;

    impl->mErrno = PSR_OK;
    pthread_cond_signal(&impl->cond);
    return 0;

OPENFAILURE:
    CDX_LOGE("MmfOpenThread fail!!!");
    impl->mErrno = PSR_OPEN_FAIL;
    pthread_cond_signal(&impl->cond);
    return -1;
}

static cdx_int32 __MmfParserControl(CdxParserT *parser, cdx_int32 cmd, void *param)
{
    struct MmfParserImplS *impl = NULL;
    impl = CdxContainerOf(parser, struct MmfParserImplS, base);
    (void)param;

    switch (cmd)
    {
        case CDX_PSR_CMD_DISABLE_AUDIO:
        case CDX_PSR_CMD_DISABLE_VIDEO:
        case CDX_PSR_CMD_SWITCH_AUDIO:
            break;
        case CDX_PSR_CMD_SET_FORCESTOP:
            CdxStreamForceStop(impl->stream);
            break;
        case CDX_PSR_CMD_CLR_FORCESTOP:
            CdxStreamClrForceStop(impl->stream);
            break;
        default :
            CDX_LOGW("not implement...(%d)", cmd);
        break;
    }
    impl->nFlags = cmd;

    return CDX_SUCCESS;
}

static cdx_int32 __MmfParserPrefetch(CdxParserT *parser, CdxPacketT *pkt)
{
    struct MmfParserImplS *impl = NULL;
    impl = CdxContainerOf(parser, struct MmfParserImplS, base);
    pkt->type = CDX_MEDIA_AUDIO;
    pkt->length = 1024/*(cdx_int32)impl->MmfFormat.nBlockAlign*/;
    pkt->pts = impl->pts;

    pkt->flags |= (FIRST_PART|LAST_PART);
    return CDX_SUCCESS;
}

static cdx_int32 __MmfParserRead(CdxParserT *parser, CdxPacketT *pkt)
{
    cdx_int64 read_length;
    struct MmfParserImplS *impl = NULL;
    CdxStreamT *cdxStream = NULL;

    impl = CdxContainerOf(parser, struct MmfParserImplS, base);
    cdxStream = impl->stream;

    if(pkt->length <= pkt->buflen)
    {
        read_length = CdxStreamRead(cdxStream, pkt->buf, pkt->length);
    }
    else
    {
        read_length = CdxStreamRead(cdxStream, pkt->buf, pkt->buflen);
        read_length += CdxStreamRead(cdxStream, pkt->ringBuf, pkt->length - pkt->buflen);
    }

    if(read_length < 0)
    {
        CDX_LOGE("CdxStreamRead fail");
        impl->mErrno = PSR_IO_ERR;
        return CDX_FAILURE;
    }
    else if(read_length == 0)
    {
        CDX_LOGD("CdxStream EOS");
        impl->mErrno = PSR_EOS;
        return CDX_FAILURE;
    }
    impl->pkt_length = impl->pkt_length - read_length;
    if(read_length == 0)
    {
        CDX_LOGW("audio parse no data");
        return CDX_FAILURE;
    }

    // TODO: fill pkt
    return CDX_SUCCESS;
}

static cdx_int32 __MmfParserGetMediaInfo(CdxParserT *parser, CdxMediaInfoT *mediaInfo)
{
    struct MmfParserImplS *impl;
    struct CdxProgramS *cdxProgram = NULL;

    impl = CdxContainerOf(parser, struct MmfParserImplS, base);
    memset(mediaInfo, 0x00, sizeof(*mediaInfo));

    if(impl->mErrno != PSR_OK)
    {
        CDX_LOGE("audio parse status no PSR_OK");
        return CDX_FAILURE;
    }

    mediaInfo->programNum = 1;
    mediaInfo->programIndex = 0;
    cdxProgram = &mediaInfo->program[0];
    memset(cdxProgram, 0, sizeof(struct CdxProgramS));
    cdxProgram->id = 0;
    cdxProgram->audioNum = 1;
    cdxProgram->videoNum = 0;
    cdxProgram->subtitleNum = 0;
    cdxProgram->audioIndex = 0;
    cdxProgram->videoIndex = 0;
    cdxProgram->subtitleIndex = 0;

    mediaInfo->bSeekable = CdxStreamSeekAble(impl->stream);
    //mediaInfo->fileSize = impl->dFileSize;

    cdxProgram->duration = impl->ulDuration;
    cdxProgram->audio[0].eCodecFormat    = AUDIO_CODEC_FORMAT_ADPCM;
    cdxProgram->audio[0].eSubCodecFormat = ADPCM_CODEC_ID_YAMAHA;
    cdxProgram->audio[0].nChannelNum     = impl->MmfFormat.nChannel;
    cdxProgram->audio[0].nBitsPerSample  = 16;
    cdxProgram->audio[0].nSampleRate     = impl->MmfFormat.nSampleRate;
    cdxProgram->audio[0].nBlockAlign     = impl->MmfFormat.nBlockAlign;
    return CDX_SUCCESS;
}

static cdx_int32 __MmfParserSeekTo(CdxParserT *parser, cdx_int64 timeUs, SeekModeType seekModeType)
{
    CDX_UNUSE(seekModeType);
    struct MmfParserImplS *impl = NULL;
    cdx_int64 size = 0;

    impl = CdxContainerOf(parser, struct MmfParserImplS, base);
    impl->pts = timeUs;
    size = impl->MmfFormat.nBitsRate * timeUs / 8000 / 1000;

    CdxStreamSeek(impl->stream,impl->nHeadLen + size, SEEK_SET);
    return CDX_SUCCESS;
}

static cdx_uint32 __MmfParserAttribute(CdxParserT *parser)
{
    struct MmfParserImplS *impl = NULL;

    impl = CdxContainerOf(parser, struct MmfParserImplS, base);
    return 0;
}

static cdx_int32 __MmfParserGetStatus(CdxParserT *parser)
{
    struct MmfParserImplS *impl = NULL;

    impl = CdxContainerOf(parser, struct MmfParserImplS, base);
    return impl->mErrno;
}

static cdx_int32 __MmfParserClose(CdxParserT *parser)
{
    struct MmfParserImplS *impl = NULL;

    impl = CdxContainerOf(parser, struct MmfParserImplS, base);
    CdxStreamClose(impl->stream);
    pthread_cond_destroy(&impl->cond);
    CdxFree(impl);
    return CDX_SUCCESS;
}

static struct CdxParserOpsS mmfParserOps =
{
    .control      = __MmfParserControl,
    .prefetch     = __MmfParserPrefetch,
    .read         = __MmfParserRead,
    .getMediaInfo = __MmfParserGetMediaInfo,
    .seekTo       = __MmfParserSeekTo,
    .attribute    = __MmfParserAttribute,
    .getStatus    = __MmfParserGetStatus,
    .close        = __MmfParserClose,
    .init         = MmfInit
};

static cdx_int32 MmfProbe(CdxStreamProbeDataT *p)
{
    /* check file header */
    if (p->buf[0] == 'M' && p->buf[1] == 'M' &&
        p->buf[2] == 'M' && p->buf[3] == 'D' &&
        p->buf[8] == 'C' && p->buf[9] == 'N' &&
        p->buf[10] == 'T' && p->buf[11] == 'I')
        return CDX_TRUE;
    else
        return CDX_FALSE;
}

static cdx_uint32 __MmfParserProbe(CdxStreamProbeDataT *probeData)
{
    CDX_CHECK(probeData);
    if(probeData->len < 32)
    {
        CDX_LOGV("Probe data is not enough.");
        return 0;
    }

    if(!MmfProbe(probeData))
    {
        CDX_LOGV("wav probe failed.");
        return 0;
    }
    return 100;
}

static CdxParserT *__MmfParserOpen(CdxStreamT *stream, cdx_uint32 nFlags)
{
    struct MmfParserImplS *impl;
    impl = CdxMalloc(sizeof(*impl));
    (void)nFlags;

    memset(impl, 0x00, sizeof(*impl));
    impl->stream = stream;
    impl->base.ops = &mmfParserOps;
    pthread_cond_init(&impl->cond, NULL);
    impl->mErrno = PSR_INVALID;
    return &impl->base;
}

struct CdxParserCreatorS mmfParserCtor =
{
    .probe = __MmfParserProbe,
    .create  = __MmfParserOpen
};
