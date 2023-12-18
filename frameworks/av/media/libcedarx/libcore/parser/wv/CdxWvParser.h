

#ifndef CDX_WAVPACK_PARSER_H
#define CDX_WAVPACK_PARSER_H

#define WV_HEADER_SIZE 32
#define WV_FLAG_INITIAL_BLOCK (1 << 11)
#define WV_FLAG_FINAL_BLOCK   (1 << 12)

// specs say that maximum block size is 1Mb
#define WV_BLOCK_LIMIT 1048576

enum WV_FLAGS
{
    WV_MONO   = 0x0004,
    WV_HYBRID = 0x0008,
    WV_JOINT  = 0x0010,
    WV_CROSSD = 0x0020,
    WV_HSHAPE = 0x0040,
    WV_FLOAT  = 0x0080,
    WV_INT32  = 0x0100,
    WV_HBR    = 0x0200,
    WV_HBAL   = 0x0400,
    WV_MCINIT = 0x0800,
    WV_MCEND  = 0x1000,
    WV_DSD    = 0x80000000,
};

static const int WvRates[16] =
{
     6000, 8000, 9600, 11025, 12000, 16000, 22050, 24000,
    32000, 44100, 48000, 64000, 88200, 96000, 192000, -1
};

typedef struct WvFormatStruct
{
    cdx_uint32 nBlockSize;     //size of block data (excluding headers)
    cdx_uint16 nVersion;       // bitstream version
    cdx_uint32 nTotalSamples; // total number of samples in the entire file, related to duration
    cdx_uint32 nBlockIndex;     //Index of the first sample in the block ,and related to pts
    cdx_uint32 nBlockSamples; //numbers of samples in this block
    cdx_uint32 nFlags;        // various flags for id and decoding
    cdx_uint32 nCrc;         ///crc for actual decoded data
    cdx_int32  nInitial;
    cdx_int32  nFinal;
} WvFormatT;

typedef struct WvParserImplS
{
     //audio common
    CdxParserT base;
    CdxStreamT *stream;
    cdx_int64 ulDuration;//ms
    pthread_cond_t cond;
    cdx_int64 nFileSize;//total file length
    cdx_int64 nFileOffset; //now read location
    cdx_int32 nErrno; //Parser Status
    cdx_uint32 nFlags; //cmd

     //wv base
    uint8_t nBlockHeader[WV_HEADER_SIZE];
    WvFormatT WvFormat;
    cdx_int32 nRate; //Sampling Rate
    cdx_int32 nChanNum;  //Number of channels
    cdx_int32 nFrames;
    cdx_int32 nBpp;     //bits per coded sample
    uint32_t  nChanMask;
    cdx_int32 nMultiChannel;
    cdx_int32 nBlockParsed;
    cdx_int64 nPos;
    cdx_int64 nApetagStart;
    cdx_int32 nSetFinalPts;
    cdx_int32 nSaveWvBitStreamFlag;
}WvParserImplS;

#endif
