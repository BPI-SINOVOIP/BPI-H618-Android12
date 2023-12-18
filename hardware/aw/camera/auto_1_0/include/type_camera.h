
#ifndef __LIB__CAMERA__TYPE__H__
#define __LIB__CAMERA__TYPE__H__

#include<mutex>

typedef struct PREVIEWINFO_t
{
    int left;
    int top;
    int width;            // preview width
    int height;            // preview height
}PREVIEWINFO_t, RECT_t;

typedef struct V4L2BUF_t
{
    unsigned long            addrPhyY;        // physical Y address of this frame
    unsigned long            addrPhyC;        // physical Y address of this frame
    unsigned long            addrVirY;        // virtual Y address of this frame
    unsigned long            addrVirC;        // virtual Y address of this frame
    unsigned int    width;
    unsigned int    height;
    int             index;            // DQUE id number
    long long        timeStamp;        // time stamp of this frame
    RECT_t            crop_rect;
    int                format;
    void*           overlay_info;

    // thumb
    unsigned char    isThumbAvailable;
    unsigned char    thumbUsedForPreview;
    unsigned char    thumbUsedForPhoto;
    unsigned char    thumbUsedForVideo;
    unsigned long            thumbAddrPhyY;        // physical Y address of thumb buffer
    unsigned long            thumbAddrVirY;        // virtual Y address of thumb buffer
    unsigned int    thumbWidth;
    unsigned int    thumbHeight;
    RECT_t            thumb_crop_rect;
    int             thumbFormat;

    std::mutex refMutex;     // the mutex for refCnt 
    int             refCnt;         // used for releasing this frame
    unsigned int    bytesused;      // used by compressed source
    int             nDmaBufFd;      //dma fd callback to codec
    int             nShareBufFd;    //share fd callback to codec
}V4L2BUF_t;

typedef enum MEDIA_SRC_MODE
{
    MEDIA_SRC_PUSH_MODE,
    MEDIA_SRC_PULL_MODE
}MEDIA_SRC_MODE;

typedef struct VIDEOINFO_t
{
    int video_source;
    int src_height;
    int src_width;
    int height;            // camcorder video frame height
    int width;            // camcorder video frame width
    int frameRate;        // camcorder video frame rate
    int bitRate;        // camcorder video bitrate
    short profileIdc;
    short levelIdc;

    int geo_available;
    int latitudex10000;
    int longitudex10000;

    // rotate
    int rotate_degree;        // only support 0, 90, 180 and 270

    // for video encoder
    unsigned int picEncmode; //0 for frame encoding 1: for field encoding 2:field used for frame encoding
    unsigned int qp_max;
    unsigned int qp_min;

    int is_compress_source; // 0 for common source 1: for mjpeg source 2: for h264 source
}VIDEOINFO_t;

typedef enum AUDIO_ENCODER_TYPE
{
    AUDIO_ENCODER_AAC_TYPE,
    AUDIO_ENCODER_LPCM_TYPE
}AUDIO_ENCODER_TYPE;
typedef struct AUDIOINFO_t
{
    int sampleRate;
    int channels;
    int bitRate;
    int bitsPerSample;
    int audioEncType;  // 0: aac, 1: LPCM
}AUDIOINFO_t;

typedef struct ENCEXTRADATAINFO_t //don't touch it, because it also defined in type.h
{
    char *data;
    int length;
}ENCEXTRADATAINFO_t;

typedef struct ENC_BUFFER_t
{
    unsigned long addrY;
    unsigned long addrCb;
    unsigned long addrCr;
    int width;
    int height;
    RECT_t crop_rect;
    int force_keyframe;
    void*  overlay_info;
    int format;
}ENC_BUFFER_t;

typedef enum JPEG_COLOR_FORMAT
{
    JPEG_COLOR_YUV444,
    JPEG_COLOR_YUV422,
    JPEG_COLOR_YUV420,
    JPEG_COLOR_YUV411,
    JPEG_COLOR_YUV420_NV12,
    JPEG_COLOR_YUV420_NV21,
    JPEG_COLOR_TILE_32X32,
    JPEG_COLOR_CSIARGB,
    JPEG_COLOR_CSIRGBA,
    JPEG_COLOR_CSIABGR,
    JPEG_COLOR_CSIBGRA
}JPEG_COLOR_FORMAT;

typedef struct JPEG_ENC_t
{
    int                src_w;
    int                src_h;
    int                pic_w;
    int                pic_h;
    unsigned long            addrY;
    unsigned long            addrC;
    int                colorFormat;
    int                quality;
    int                rotate;

    int                scale_factor;
    double            focal_length;

    int                thumbWidth;
    int                thumbHeight;

    unsigned char    enable_crop;
    int                crop_x;
    int                crop_y;
    int                crop_w;
    int                crop_h;

    // gps exif
    unsigned char    enable_gps;
    double          gps_latitude;
    double            gps_longitude;
    double            gps_altitude;
    long            gps_timestamp;
    char            gps_processing_method[100];
    int             whitebalance;
    char              CameraMake[64];//for the cameraMake name
    char              CameraModel[64];//for the cameraMode
    char              DateTime[21];//for the data and time
    void*           pover_overlay;
}JPEG_ENC_t;


typedef struct thumb_buffer
{
    int               id;
    unsigned int   width;
    unsigned int   height;
    unsigned char* y_vir;
    unsigned char* uv_vir;
    unsigned long   y_phy;
    unsigned long   uv_phy;
    int size_y;
    int size_uv;
    long long pts;
}thumb_buffer;
#endif // __LIB__CAMERA__TYPE__H__

