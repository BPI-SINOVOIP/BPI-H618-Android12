#ifndef _WATER_MARK_INTERFACE_H
#define _WATER_MARK_INTERFACE_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define MAX_WATERMARK_NUM 5
#define MAX_WATERMARK_LENGTH 20

typedef struct {
    int mPositionX;
    int mPositionY;
    char content[MAX_WATERMARK_LENGTH];
}SingleWaterMark;

typedef struct {
    int mWaterMarkNum;
    SingleWaterMark mSingleWaterMark[MAX_WATERMARK_NUM];
}WaterMarkMultiple;

typedef struct {
    unsigned char *y;
    unsigned char *c;
    int posx;
    int posy;
    int width;
    int height;
    float resolution_rate;
    char *display;
} WaterMarkInData;

extern int doWaterMark(WaterMarkInData *indata, void *ctrl);
extern int doWaterMarkMultiple(WaterMarkInData *indata, void *ctrl, void *multi, char *content, char *time_watermark);
extern void *initialwaterMark(int wm_height);
extern void *initWaterMarkMultiple();
extern int releaseWaterMark(void *ctrl);
extern int releaseWaterMarkMultiple(void *multi);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _WATER_MARK_INTERFACE_H */
