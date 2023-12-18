#ifndef __G2DAPI_H__
#define __G2DAPI_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef struct RECT
{
    int left;
    int top;
    int width;            // preview width
    int height;            // preview height
}stRECT;

typedef struct V4L2BUF
{
    unsigned long            addrPhyY;        // physical Y address of this frame
    unsigned long            addrPhyC;        // physical Y address of this frame
    unsigned long            addrVirY;        // virtual Y address of this frame
    unsigned long            addrVirC;        // virtual Y address of this frame
    unsigned int    width;
    unsigned int    height;
    int             index;            // DQUE id number
    long long        timeStamp;        // time stamp of this frame
    stRECT            crop_rect;
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
    stRECT            thumb_crop_rect;
    int             thumbFormat;

    int             refCnt;         // used for releasing this frame
    unsigned int    bytesused;      // used by compressed source
    int             nDmaBufFd;      //dma fd callback to codec
    int             nShareBufFd;    //share fd callback to codec
}stV4L2BUF;


typedef enum {
    G2D_ROTATE90,
    G2D_ROTATE180,
    G2D_ROTATE270,
    G2D_FLIP_HORIZONTAL,
    G2D_FLIP_VERTICAL,
    G2D_MIRROR45,
    G2D_MIRROR135,
}g2dRotateAngle;


struct G2dOpsS{
    int (*fpG2dInit)();
    int (*fpG2dUnit)(int g2dHandle);
    int (*fpG2dAllocBuff)(stV4L2BUF* bufHandle,int width, int hight);
    int (*fpG2dFreeBuff)(stV4L2BUF* bufHandle);
    int (*fpG2dScale)(int g2dHandle,unsigned int psrc, int src_w, int src_h, int src_x, int src_y, int src_rectw, int src_recth, unsigned int pdst, int dst_w, int dst_h, int dst_x, int dst_y,int dst_rectw, int dst_recth);
    int (*fpG2dClip)(int g2dHandle,void* psrc, int src_w, int src_h, int src_x, int src_y, int width, int height, void* pdst, int dst_w, int dst_h, int dst_x, int dst_y);
    int (*fpG2dRotate)(int g2dHandle,g2dRotateAngle angle, unsigned char *src, int src_width, int src_height, int src_x, int src_y, int width, int height,unsigned char *dst, int dst_width, int dst_height,int dst_x, int dst_y);
};

struct G2dOpsS* GetG2dOpsS();
//void RegisterG2dOps(struct G2dOpsS* pG2dOps);

#ifdef __cplusplus
}
#endif


#endif
