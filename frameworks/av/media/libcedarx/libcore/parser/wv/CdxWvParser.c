#include <CdxTypes.h>
#include <CdxParser.h>
#include <CdxStream.h>
#include <CdxMemory.h>
#include <CdxWvParser.h>

const char* fpStreamPath = "/data/camera/wvcodestream.dat";
static FILE* fpStream = NULL;

#define MKTAG(a, b, c, d) ((a) | ((b) << 8) | ((c) << 16) | ((unsigned)(d) << 24))

#define WAV_MAX(a,b) (a>b?a:b)

#define AUDIO_CHANNEL_OUT_MONO  0x1u
#define AUDIO_CHANNEL_OUT_STEREO  0x3u

#define ReadL32(x)     \
                              (((uint32_t)((const uint8_t*)(x))[3] << 24) |    \
                              (((const uint8_t*)(x))[2] << 16) |    \
                              (((const uint8_t*)(x))[1] <<  8) |    \
                              ((const uint8_t*)(x))[0])

#define ReadL16(x)  ((((const cdx_uint8*)(x))[1] << 8) | \
                                  ((const cdx_uint8*)(x))[0])

static cdx_int32 CdxIoRL8(CdxStreamT *s)
{
    char c[4] = {0};
    CdxStreamRead(s, c, 1);
    return (c[0]&0xff);
}

static cdx_int32 CdxIoRL16(CdxStreamT *s)
{
    unsigned int nVal;
    nVal = CdxIoRL8(s);
    nVal |= CdxIoRL8(s) << 8;
    return nVal;
}
static cdx_int32 CdxIoRL24(CdxStreamT *s)
{
    unsigned int nVal;
    nVal = CdxIoRL16(s);
    nVal |= CdxIoRL8(s) << 16;
    return nVal;
}

static cdx_int32 CdxIoRL32(CdxStreamT *s)
{
    unsigned int nVal;
    nVal = CdxIoRL16(s);
    nVal |= CdxIoRL16(s) << 16;
    return nVal;
}

static cdx_int32 WvParseHeader(WvFormatT *wv, const uint8_t *data)
{
    memset(wv, 0, sizeof(*wv));

    if (ReadL32(data) != MKTAG('w', 'v', 'p', 'k'))
    {
        CDX_LOGE( "This is not a wavpack header");
        return PSR_INVALID;
    }

    wv->nBlockSize = ReadL32(data + 4);
    if (wv->nBlockSize < 24 || wv->nBlockSize > WV_BLOCK_LIMIT)
    {
        CDX_LOGE( "blocksize:%d is not in [24, 1048576]",wv->nBlockSize);
        return PSR_INVALID;
    }
    wv->nBlockSize = wv->nBlockSize - 24;
    wv->nVersion = ReadL16(data + 8);
    wv->nTotalSamples = ReadL32(data + 12);
    wv->nBlockIndex = ReadL32(data + 16);
    wv->nBlockSamples = ReadL32(data + 20);
    wv->nFlags = ReadL32(data + 24);
    wv->nCrc = ReadL32(data + 28);

    wv->nInitial = !!(wv->nFlags & WV_FLAG_INITIAL_BLOCK);
    wv->nFinal = !!(wv->nFlags & WV_FLAG_FINAL_BLOCK);
    CDX_LOGD( "blocksize %d,version:%d, total_samples:%d, block_idx:%d, block_samples:%d, ",\
        wv->nBlockSize, wv->nVersion,wv->nTotalSamples,wv->nBlockIndex,wv->nBlockSamples );
    return 0;
}

static cdx_int32 WvReadBlockHeader(CdxParserT* parser)
{
    cdx_int32 nRet;
    cdx_int32 nRate;
    cdx_int32 nBpp;//bits per coded sample
    cdx_int32 nChanNum;
    cdx_uint32 nChanMask, nFlags;
    struct WvParserImplS *impl = (WvParserImplS *)parser;
    impl->nPos = CdxStreamTell(impl->stream);

    // don't return bogus packets with the ape tag data
    if (impl->nApetagStart && impl->nPos >= impl->nApetagStart)
    {
        CDX_LOGE( "Bogus packets with the ape tag data");
        return PSR_EOS;
    }

    //read 32 bytes for block header
    nRet = CdxStreamRead(impl->stream,  impl->nBlockHeader, WV_HEADER_SIZE);

    if (nRet != WV_HEADER_SIZE)
    {
        CDX_LOGE( "Readed bytes = ret:%d, less than 32 bytes", nRet);
        return (nRet < 0) ? nRet : PSR_EOS;
    }

    nRet = WvParseHeader(&impl->WvFormat, impl->nBlockHeader);
    if (nRet != 0)
    {
        CDX_LOGE("Invalid block header.\n");
        return nRet;
    }

    //we don't support DSD
    if (impl->WvFormat.nFlags & WV_DSD)
    {
        CDX_LOGE( " We don't support DSD.\n");
        return PSR_INVALID;
    }

    if (impl->WvFormat.nVersion < 0x402 || impl->WvFormat.nVersion > 0x410)
    {
        CDX_LOGE( "Wv version is :0x%03X, not in [0x402, 0x410].\n", impl->WvFormat.nVersion);
        return PSR_INVALID;
    }

     // Blocks with zero samples don't contain actual audio information and should be ignored
    if (!impl->WvFormat.nBlockSamples)
    {
        CDX_LOGE( "Block without a sample.\n");
        return 0;
    }

    // parse flags
    nFlags = impl->WvFormat.nFlags;
    nBpp = ((nFlags & 3) + 1) << 3;
    nChanNum = 1 + !(nFlags & WV_MONO);
    nChanMask = nFlags & WV_MONO ? AUDIO_CHANNEL_OUT_MONO : AUDIO_CHANNEL_OUT_STEREO;

    nRate = WvRates[(nFlags >> 23) & 0xF];
    impl->nMultiChannel = !(impl->WvFormat.nInitial && impl->WvFormat.nFinal);
    if (impl->nMultiChannel)
    {
        nChanNum = impl->nChanNum;
        nChanMask = impl->nChanMask;
    }
    if ((nRate == -1 || !nChanNum) && !impl->nBlockParsed)
    {
        int64_t block_end = CdxStreamTell(impl->stream) + impl->WvFormat.nBlockSize;

        while (CdxStreamTell(impl->stream) < block_end && !CdxStreamEos(impl->stream))
        {
            int id, size;
            id = CdxIoRL8(impl->stream);
            size = (id & 0x80) ? CdxIoRL24(impl->stream) : CdxIoRL8(impl->stream);
            size <<= 1;
            if (id & 0x40)
                size--;
            switch (id & 0x3F)
            {
                case 0xD:
                if (size <= 1)
                {
                    CDX_LOGE( "Insufficient channel information\n");
                    return PSR_INVALID;
                }
                nChanNum = CdxIoRL8(impl->stream);
                switch (size - 2)
                {
                    case 0:
                        nChanMask = CdxIoRL8(impl->stream);
                        break;
                    case 1:
                        nChanMask =  CdxIoRL8(impl->stream);
                        break;
                    case 2:
                        nChanMask =  CdxIoRL24(impl->stream);
                        break;
                    case 3:
                        nChanMask = CdxIoRL32(impl->stream);
                        break;
                    case 4:
                        CdxStreamSkip(impl->stream, 1);
                        nChanNum  |= (CdxIoRL8(impl->stream)& 0xF) << 8;
                        nChanNum  += 1;
                        nChanMask = CdxIoRL24(impl->stream);
                        break;
                    case 5:
                        CdxStreamSkip(impl->stream, 1);
                        nChanNum  |= (CdxIoRL8(impl->stream)& 0xF) << 8;
                        nChanNum  += 1;
                        nChanMask = CdxIoRL32(impl->stream);
                        break;
                    default:
                        CDX_LOGD("Invalid channel info size %d\n", size);
                        return PSR_INVALID;
                }
                break;
            case 0x27:
                nRate = CdxIoRL24(impl->stream);
                break;
            default:
                CdxStreamSkip(impl->stream, size);
            }
            if(id & 0x40)
                CdxStreamSkip(impl->stream, 1);
        }
        if(nRate == -1)
        {
            CDX_LOGE( "Cannot determine custom sampling rate\n");
            return PSR_INVALID;
        }

        CdxStreamSeek(impl->stream, block_end - impl->WvFormat.nBlockSize, SEEK_SET);
    }

    if (!impl->nBpp)
        impl->nBpp = nBpp;
    if (!impl->nChanNum)
        impl->nChanNum = nChanNum;
    if (!impl->nChanMask)
        impl->nChanMask = nChanMask;
    if (!impl->nRate)
        impl->nRate = nRate;

    if (nFlags && nBpp != impl->nBpp)
    {
        CDX_LOGE("Bits per sample differ, this block: %i, header block: %i\n", nBpp, impl->nBpp);
        return PSR_INVALID;
    }
    if (nFlags && !impl->nMultiChannel && nChanNum != impl->nChanNum)
    {
        CDX_LOGE("Channels differ, this block: %i, header block: %i\n", nChanNum, impl->nChanNum);
        return PSR_INVALID;
    }
    if (nFlags && nRate != -1 && nRate != impl->nRate)
    {
        CDX_LOGE("Sampling rate differ, this block: %i, header block: %i\n", nRate, impl->nRate);
        return PSR_INVALID;
    }
    return 0;
}

static int WvInit(CdxParserT* parser)
{
    struct WvParserImplS *impl = NULL;
    CdxStreamT *cdxStream = NULL;
    cdx_int32 nRet = 0;

    impl = (WvParserImplS *)parser;
    cdxStream = impl->stream;

//save codestream for debug
#ifdef __ANDROID__
    char nVal[256];
    property_get("vendor.vdec.savewvstream", nVal, "0");
    impl->nSaveWvBitStreamFlag = atoi(nVal);
#endif

    if(impl->nSaveWvBitStreamFlag == 0)
    {
        CDX_LOGW("save  Wv code stream fail! cause nSaveWvBitStreamFlag == 0");
    }
    else
    {
        fpStream = fopen(fpStreamPath, "wb");
        if(fpStream == NULL)
        {
            impl->nSaveWvBitStreamFlag = 0;
            CDX_LOGW("open codestream file failed!");
        }
    }

    impl->nBlockParsed = 0;
    for (;;) //skip the block of zero samples
    {
        if ((nRet = WvReadBlockHeader(parser)) < 0)
            goto WVERROR;
        if (!impl->WvFormat.nBlockSamples)
            CdxStreamSkip(cdxStream, impl->WvFormat.nBlockSize);
        else
            break;
    }
    impl->ulDuration = (cdx_int64)(impl->WvFormat.nTotalSamples*1000/impl->nRate);   //for duration  ms
    impl->nFileSize = CdxStreamSize(impl->stream);   //get the size of file

    impl->nErrno = PSR_OK;
    pthread_cond_signal(&impl->cond);
    return 0;

WVERROR:
    CDX_LOGE("WvOpenThread fail!!!");
    impl->nErrno = PSR_OPEN_FAIL;
    pthread_cond_signal(&impl->cond);
    return -1;
}

static cdx_int32 __WvParserControl(CdxParserT *parser, cdx_int32 cmd, void *param)
{
    struct WvParserImplS *impl = NULL;
    impl = CdxContainerOf(parser, struct WvParserImplS, base);
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

static cdx_int32 __WvParserPrefetch(CdxParserT *parser, CdxPacketT *pkt)
{
    int nRet;
    struct WvParserImplS *impl = NULL;
    impl = CdxContainerOf(parser, struct WvParserImplS, base);

    if (CdxStreamEos(impl->stream))
    {
        CDX_LOGW( "EOS" );
        return PSR_EOS;
    }
    if (impl->nBlockParsed)
    {
        if ((nRet = WvReadBlockHeader(parser)) < 0)
        {
            CDX_LOGE( "__WvParserPrefetch failed to run WvReadBlockHeader.");
            return nRet;
        }
    }

    pkt->type = CDX_MEDIA_AUDIO;
    pkt->length = impl->WvFormat.nBlockSize + 32;
    pkt->pts = (cdx_int64)impl->nFrames* impl->WvFormat.nBlockSamples*1000000/impl->nRate;//A block containing 44100 samples is 1s
    if(impl->WvFormat.nBlockSamples < 44100)
    {
        if(!impl->nSetFinalPts)
        {
            pkt->pts = (impl->nFrames)*1000000;
            impl->nSetFinalPts = 1;
        }
        else
        {
            pkt->pts = impl->WvFormat.nBlockSamples*10000/441;//The last block  is less than 44100 samples, calculated as a percentage
            pkt->pts = pkt->pts+ (impl->nFrames -1)*1000000;
        }
    }

    CDX_LOGD( " pkt->length:%d, pts:%lld, nBlockSize%d", pkt->length, pkt->pts,impl->WvFormat.nBlockSize);
    pkt->flags |= (FIRST_PART|LAST_PART);

    return CDX_SUCCESS;
}

static cdx_int32 __WvParserRead(CdxParserT *parser, CdxPacketT *pkt)
{
    cdx_int32 nReadLength;
    struct WvParserImplS *impl = CdxContainerOf(parser, struct WvParserImplS, base);
    CdxStreamT *cdxStream = impl->stream;

    //memcpy(pkt->buf, impl->nBlockHeader, WV_HEADER_SIZE);
    if(pkt->length <= pkt->buflen)
    {
        memcpy(pkt->buf, impl->nBlockHeader, WV_HEADER_SIZE);
        nReadLength = CdxStreamRead(cdxStream,  (char*)pkt->buf + WV_HEADER_SIZE, pkt->length - 32);
    }
    else
    {
        memcpy(pkt->buf, impl->nBlockHeader, WV_HEADER_SIZE);
        nReadLength = CdxStreamRead(cdxStream, (char*)pkt->buf + WV_HEADER_SIZE, pkt->buflen);
        nReadLength += CdxStreamRead(cdxStream, pkt->ringBuf,    pkt->length - pkt->buflen - 32);//ring buffer
    }

    //save Wv bit stream for debug
    if(impl->nSaveWvBitStreamFlag == 1 && fpStream)
    {
        if(pkt->length> 0)
            fwrite(pkt->buf, 1, pkt->length, fpStream);
    }

    if(nReadLength < 0)
    {
        CDX_LOGE("CdxStreamRead fail");
        impl->nErrno = PSR_IO_ERR;
        return CDX_FAILURE;
    }
    else if(nReadLength == 0)
    {
        CDX_LOGE("CdxStream EOS");
        impl->nErrno = PSR_EOS;
        return CDX_FAILURE;
    }
    impl->nBlockParsed  = 1;
    nReadLength += 32;
    pkt->length = nReadLength;
    impl->nFileOffset += nReadLength;
    impl->nFrames++;

    CDX_LOGD( "pkt length :%d,file_offset:%lld,  impl frames:%d",pkt->length, impl->nFileOffset , impl->nFrames);
    if(nReadLength == 0)
    {
        CDX_LOGW("audio parse no data");
        return CDX_FAILURE;
    }

    // TODO: fill pkt
    return CDX_SUCCESS;
}

static cdx_int32 __WvParserGetMediaInfo(CdxParserT *parser, CdxMediaInfoT *mediaInfo)
{
    struct WvParserImplS *impl;
    struct CdxProgramS *cdxProgram = NULL;

    impl = CdxContainerOf(parser, struct WvParserImplS, base);
    memset(mediaInfo, 0x00, sizeof(*mediaInfo));

    if(impl->nErrno != PSR_OK)
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
    mediaInfo->fileSize = impl->nFileSize;

    cdxProgram->duration = impl->ulDuration;
    cdxProgram->audio[0].eCodecFormat = AUDIO_CODEC_FORMAT_WV;
    cdxProgram->audio[0].eSubCodecFormat = 0;
    cdxProgram->audio[0].nChannelNum = impl->nChanNum;
    //cdxProgram->audio[0].nBitsPerSample = impl->nBpp;
    cdxProgram->audio[0].nBitsPerSample = 16;
    cdxProgram->audio[0].nSampleRate = impl->nRate;

    CDX_LOGD("fileSize:%lld,  duration:%d, ChannelNum: = %d,  BitsPerSample:%d, SampleRate:%d", \
        mediaInfo->fileSize, cdxProgram->duration, cdxProgram->audio[0].nChannelNum ,\
        cdxProgram->audio[0].nBitsPerSample,cdxProgram->audio[0].nSampleRate);
    return CDX_SUCCESS;
}

static cdx_int32 __WvParserSeekTo(CdxParserT *parser, cdx_int64 timeUs, SeekModeType seekModeType)
{
    CDX_UNUSE(seekModeType);
    struct WvParserImplS *impl = NULL;
    cdx_int32 nFileLocation = 0;
    impl = CdxContainerOf(parser, struct WvParserImplS, base);
    cdx_int32 nRet;
    cdx_int32 nFrame = 0;
    cdx_int32 i = 0;


    nFrame = timeUs/1000000;
    CDX_LOGD(" seek ,the timeUs :%lld,and nFrame%d", timeUs ,nFrame);
    if(CdxStreamSeek(impl->stream, 0, SEEK_SET))
    {
        CDX_LOGE("Function:%s, Line:%d,  CdxStreamSeek fail",__FUNCTION__, __LINE__);
        return CDX_FAILURE;
    }
    while(i++ < nFrame )//Find the block where the skip location is
    {
        if ((nRet = WvReadBlockHeader(parser)) < 0)
        {
            CDX_LOGE( "Function:%s, Line:%d,  WvReadBlockHeader  fail", __FUNCTION__, __LINE__);
            return nRet;
        }
        nFileLocation = nFileLocation + impl->WvFormat.nBlockSize;
        CdxStreamSkip(impl->stream, impl->WvFormat.nBlockSize);//Skip this block and find the next one
    }
    nFileLocation = (nFileLocation + 32*nFrame);
    nFileLocation = WAV_MAX(nFileLocation, 0);
    impl->nFrames = nFrame;
    impl->nSetFinalPts = 0;

    CDX_LOGI("Now we seek!, seek location:%d", nFileLocation);
    if(CdxStreamSeek(impl->stream, nFileLocation, SEEK_SET))
    {
        CDX_LOGE("Function:%s, Line:%d,  CdxStreamSeek fail",__FUNCTION__, __LINE__);
        return CDX_FAILURE;
    }

    impl->nFileOffset = nFileLocation;

    // TODO: not implement now...
    CDX_LOGI("TODO, seek to now...");

    return CDX_SUCCESS;
}

static cdx_uint32 __WvParserAttribute(CdxParserT *parser)
{
    struct WvParserImplS *impl = NULL;
    impl = CdxContainerOf(parser, struct WvParserImplS, base);
    return 0;
}

static cdx_int32 __WvParserGetStatus(CdxParserT *parser)
{
    struct WvParserImplS *impl = NULL;
    impl = CdxContainerOf(parser, struct WvParserImplS, base);
    return impl->nErrno;
}

static cdx_int32 __WvParserClose(CdxParserT *parser)
{
    struct WvParserImplS *impl = NULL;

    impl = CdxContainerOf(parser, struct WvParserImplS, base);
    CdxStreamClose(impl->stream);
    pthread_cond_destroy(&impl->cond);
    CdxFree(impl);

    if(fpStream)
    {
        fclose(fpStream);
        fpStream = NULL;
    }

    return CDX_SUCCESS;
}

static struct CdxParserOpsS wvParserOps =
{
    .control = __WvParserControl,
    .prefetch = __WvParserPrefetch,
    .read = __WvParserRead,
    .getMediaInfo = __WvParserGetMediaInfo,
    .seekTo = __WvParserSeekTo,
    .attribute = __WvParserAttribute,
    .getStatus = __WvParserGetStatus,
    .close = __WvParserClose,
    .init = WvInit
};

static cdx_uint32 __WvParserProbe(CdxStreamProbeDataT *probeData)
{
    int i;
    CDX_CHECK(probeData);
    if(probeData->len < 32)
    {
        CDX_LOGE("Probe data is not enough.");
        return 0;
    }
    for(i = 0; i<4; i++)
    {
        CDX_LOGD("__WvParserProbe: %c",probeData->buf[i]);
    }

    if (ReadL32(&probeData->buf[0]) == MKTAG('w', 'v', 'p', 'k') &&
        ReadL32(&probeData->buf[4]) >= 24 &&
        ReadL32(&probeData->buf[4]) <= WV_BLOCK_LIMIT &&
        ReadL16(&probeData->buf[8]) >= 0x402 &&
        ReadL16(&probeData->buf[8]) <= 0x410)
        return 100;
    else
        return 0;
}

static CdxParserT *__WvParserOpen(CdxStreamT *stream, cdx_uint32 nFlags)
{
    struct WvParserImplS *impl;
    impl = CdxMalloc(sizeof(*impl));
    (void)nFlags;

    memset(impl, 0x00, sizeof(*impl));
    impl->stream = stream;
    impl->base.ops = &wvParserOps;

    pthread_cond_init(&impl->cond, NULL);
    impl->nErrno = PSR_INVALID;
    return &impl->base;
}

struct CdxParserCreatorS wvParserCtor =
{
    .probe = __WvParserProbe,
    .create  = __WvParserOpen
};

