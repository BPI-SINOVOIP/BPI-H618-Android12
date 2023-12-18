#define LOG_TAG "Water_mark_interface"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <cutils/log.h>
#include <water_mark_interface.h>

#include "water_mark.h"

#define ALIGN_2B(x) (((x) + (1)) & ~(1))

static void argb2yuv420sp(unsigned char* src_p, unsigned char* alph,
                int width, int height,
                unsigned char* dest_y, unsigned char* dest_c)
{
    int i, j;
    for (i = 0; i < (int)height; i++) {
        if ((i & 1) == 0) {
            for(j= 0; j< (int)width; j++) {
                *dest_y = (299 * src_p[2] + 587 * src_p[1] + 114 * src_p[0]) / 1000;

                if((j & 1) == 0) {
                   //cb
                   *dest_c++ = 128 + (564 * (src_p[0] - *dest_y) / 1000);
                } else {
                   // cr
                   *dest_c++ = 128 + (713 * (src_p[2] - *dest_y) / 1000);
                }

                *alph++ = src_p[3];
                src_p += 4;
                dest_y++;
            }
        } else {
            for (j= 0; j< (int)width; j++) {
                *dest_y = (299 * src_p[2] + 587 * src_p[1] + 114 * src_p[0]) / 1000;
                *alph++ = src_p[3];
                src_p += 4;
                dest_y++;
            }
        }
    }

    return;
}

static int getWordCharNum(char val)
{
    int i;
    int cnt = 0;

    for (i=7; i>=0; i--) {
        if (!(val & (1<<i))) {
            break;
        } else {
            cnt++;
        }
    }

    if (cnt == 0) {
        return 1;
    }
    return cnt;
}

static int getWordIndex(char *data, int len)
{
    if (len == 1) {
        if (*data >= '0' && *data <= '9') return (*data - 0x30);
        //add capital support,index start from 22(A)
        else if (*data >= 'A' && *data <= 'Z') return (*data - 0x2B);
        else if (*data == ' ') return 19;
        else if (*data == '-') return 20;
        else if (*data == ':') return 21;
        else if (*data == '/') return 48;
        else return 19;
    } else if (len == 2) {
        return 19;
    } else if (len == 3) {
        if (!memcmp(data, "日", 3)) return 10;
        else if (!memcmp(data, "一", 3)) return 11;
        else if (!memcmp(data, "二", 3)) return 12;
        else if (!memcmp(data, "三", 3)) return 13;
        else if (!memcmp(data, "四", 3)) return 14;
        else if (!memcmp(data, "五", 3)) return 15;
        else if (!memcmp(data, "六", 3)) return 16;
        else if (!memcmp(data, "星", 3)) return 17;
        else if (!memcmp(data, "期", 3)) return 18;
    } else {
        return 19;
    }
    return 19;
}

void *initialwaterMark(int wm_height)
{
    FILE *fp = NULL;
    int watermark_pic_num = MAX_ICON_PIC;
    int i;
    int width = 0, height = 0;
    char filename[256];
    unsigned char *buf = NULL;
    WATERMARK_CTRL *wm_ctrl = NULL;

    ALOGI("<initialwaterMark>water mark initialize, height=%d", wm_height);
    wm_ctrl = (WATERMARK_CTRL*)malloc(sizeof(WATERMARK_CTRL));
    if (wm_ctrl == NULL) {
        ALOGE("<initialwaterMark> malloc error");
        return NULL;
    }
    memset(wm_ctrl, 0, sizeof(WATERMARK_CTRL));
    wm_ctrl->ADBright.recycle_frame = 30;

    for(i = 0; i < watermark_pic_num; ++i) {
        sprintf(filename, "/system/watermark/wm_%dp_%d.bmp", wm_height, i);

        fp = fopen(filename, "r");
        if (NULL == fp) {
            ALOGE("<initialwaterMark>Fail to open file %s!", filename);
            goto OPEN_FILE_ERR;
        }

        //get watermark picture size
        fseek(fp, 18, SEEK_SET);
        fread(&width, 1, 4, fp);
        fread(&height, 1, 4, fp);
        wm_ctrl->wminfo.single_pic[i].width = abs(width);
        wm_ctrl->wminfo.single_pic[i].height = abs(height);

        fseek(fp, 54, SEEK_SET);

        wm_ctrl->wminfo.single_pic[i].y =
            (unsigned char*)malloc(wm_ctrl->wminfo.single_pic[i].width * ALIGN_2B(wm_ctrl->wminfo.single_pic[i].height) * 5 / 2);
        if (NULL == wm_ctrl->wminfo.single_pic[i].y) {
            fclose(fp);
            goto ALLOC_Y_ERR;
        }
        wm_ctrl->wminfo.single_pic[i].alph =
            wm_ctrl->wminfo.single_pic[i].y + wm_ctrl->wminfo.single_pic[i].width * wm_ctrl->wminfo.single_pic[i].height;
        wm_ctrl->wminfo.single_pic[i].c =
            wm_ctrl->wminfo.single_pic[i].alph + wm_ctrl->wminfo.single_pic[i].width * wm_ctrl->wminfo.single_pic[i].height;
        wm_ctrl->wminfo.single_pic[i].id = i;

        buf = (unsigned char *)malloc(wm_ctrl->wminfo.single_pic[i].width * wm_ctrl->wminfo.single_pic[i].height * 4);
        if (NULL == buf) {
            fclose(fp);
            goto ALLOC_BUF_ERR;
        }

        fread(buf, wm_ctrl->wminfo.single_pic[i].width * wm_ctrl->wminfo.single_pic[i].height * 4, 1, fp);

        argb2yuv420sp(buf, wm_ctrl->wminfo.single_pic[i].alph,
                wm_ctrl->wminfo.single_pic[i].width, wm_ctrl->wminfo.single_pic[i].height,
                wm_ctrl->wminfo.single_pic[i].y, wm_ctrl->wminfo.single_pic[i].c);

        fclose(fp);
        free(buf);
    }
    wm_ctrl->wminfo.picture_number = i;

    return (void*)wm_ctrl;

ALLOC_BUF_ERR:
ALLOC_Y_ERR:
OPEN_FILE_ERR:
    for (i = 0; i < wm_ctrl->wminfo.picture_number; ++i) {
        if (wm_ctrl->wminfo.single_pic[i].y != NULL) {
            free(wm_ctrl->wminfo.single_pic[i].y);
            wm_ctrl->wminfo.single_pic[i].y = NULL;
        }
    }
    free(wm_ctrl);
    return NULL;
}

void* initWaterMarkMultiple()
{
    WaterMarkMultiple *mWaterMarkMultiple = NULL ;
    mWaterMarkMultiple = (WaterMarkMultiple *)malloc(sizeof(WaterMarkMultiple));
    if (NULL == mWaterMarkMultiple)
    {
        ALOGE("Fail to initialize mWaterMarkMultiple!");
        return NULL;
    }
    memset(mWaterMarkMultiple, 0, sizeof(WaterMarkMultiple));

    return (void *)mWaterMarkMultiple;
}
int releaseWaterMark(void *ctrl)
{
    int i;
    WATERMARK_CTRL *wm_ctrl = (WATERMARK_CTRL*)ctrl;

    ALOGI("<releaseWaterMark>water mark release");
    for (i = 0; i < wm_ctrl->wminfo.picture_number; ++i) {
        if (wm_ctrl->wminfo.single_pic[i].y != NULL) {
            free(wm_ctrl->wminfo.single_pic[i].y);
            wm_ctrl->wminfo.single_pic[i].y = NULL;
        }
    }
    free(wm_ctrl);
    return 0;
}

int releaseWaterMarkMultiple(void *multi)
{
    WaterMarkMultiple *wm_multi = (WaterMarkMultiple *)multi;

    ALOGI("<releaseWaterMarkMultiple>water mark multiple release");

    free(wm_multi);

    return 0;
}
int doWaterMark(WaterMarkInData *data, void *ctrl)
{
    BackGroudLayerInfo BGInfo;
    ShowWaterMarkParam WMPara;
    AjustBrightnessParam *ADBright;
    WATERMARK_CTRL *wm_ctrl;
    WaterMarkInfo *wm_info;
    int buflen, wordlen;
    int i;
    char *dispBuf;

    if (ctrl == NULL || data == NULL) {
        ALOGE("Input parameters error!");
        return -1;
    }
    if (data->display == NULL || (buflen = strlen(data->display)) == 0) {
        ALOGE(" invalid water mark display buffer!");
        return -1;
    }

    wm_ctrl = (WATERMARK_CTRL*)ctrl;
    BGInfo.width = data->width;
    BGInfo.height = data->height;
    BGInfo.y = data->y;
    BGInfo.c = data->c;
    wm_info = &wm_ctrl->wminfo;
    ADBright = &wm_ctrl->ADBright;
    dispBuf = data->display;

    for (i = 0; i < DISP_ICON_NUM; ++i) {
        wordlen = getWordCharNum(dispBuf[0]);
        buflen -= wordlen;
        if (buflen < 0) {
            break;
        }
        WMPara.id_list[i] = getWordIndex(dispBuf, wordlen);
        dispBuf += wordlen;
    }

    WMPara.number = i;

    WMPara.pos.x =  data->posx;
    WMPara.pos.y =  data->posy;

    //watermark_blending_ajust_brightness(&BGInfo, wm_info, &WMPara, ADBright);
    watermark_blending(&BGInfo, wm_info, &WMPara);

    return 0;
}

int doWaterMarkMultiple(WaterMarkInData *data, void *ctrl, void *multi, char *content, char *time_watermark)
{
    char *delims = ",";
    char *allContent[MAX_WATERMARK_NUM * 3] ;
    char * _ptr = NULL;
    int count = 0;
    WaterMarkMultiple *wm_multi ;

    wm_multi = (WaterMarkMultiple *)multi;

    allContent[count] = strtok_r(content,delims, &_ptr);
    while (allContent[count] != NULL) {
        count++;
        allContent[count] = strtok_r(NULL,delims, &_ptr);
    }

    wm_multi->mWaterMarkNum = count / 3;
    int j = 0;
    int i;
    for ( i = 0;i < count / 3; i++) {
        wm_multi->mSingleWaterMark[i].mPositionX = strtol(allContent[j++],  NULL,10);
        wm_multi->mSingleWaterMark[i].mPositionY = strtol(allContent[j++], NULL, 10);
        strcpy(wm_multi->mSingleWaterMark[i].content, allContent[j++]);
    }

    if (data->height == 540) {//preview watermark
        data->posx = wm_multi->mSingleWaterMark[0].mPositionX*data->resolution_rate;
        data->posy = wm_multi->mSingleWaterMark[0].mPositionY*data->resolution_rate;
        data->display = time_watermark;
        doWaterMark(data, ctrl);

        for( i = 1; i < wm_multi->mWaterMarkNum; i++)
        {
            data->posx = wm_multi->mSingleWaterMark[i].mPositionX * data->resolution_rate;
            data->posy = wm_multi->mSingleWaterMark[i].mPositionY * data->resolution_rate;
            data->display = wm_multi->mSingleWaterMark[i].content;

            doWaterMark(data,ctrl);
        }

    } else {  //record watermark

        data->posx = wm_multi->mSingleWaterMark[0].mPositionX;
        data->posy = wm_multi->mSingleWaterMark[0].mPositionY;
        data->display = time_watermark;
        doWaterMark(data, ctrl);

        for ( i = 1;i < wm_multi->mWaterMarkNum; i++) {
            data->posx = wm_multi->mSingleWaterMark[i].mPositionX;
            data->posy = wm_multi->mSingleWaterMark[i].mPositionY;
            data->display = wm_multi->mSingleWaterMark[i].content;

            doWaterMark(data, ctrl);
        }

    }

    return 0;
}
