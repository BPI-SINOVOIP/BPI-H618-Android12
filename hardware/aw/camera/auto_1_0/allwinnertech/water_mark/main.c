#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include <water_mark.h>

long long GetNowUs() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (long long)tv.tv_sec * 1000000ll + tv.tv_usec;
}

void argb2yuv420sp(unsigned char* src_p, unsigned char* alph,
                unsigned int width, unsigned int height,
                unsigned char* dest_y, unsigned char* dest_c)
{
    int i,j;
    for (i = 0; i < (int)height; i++) {
        if ((i & 1) == 0) {
            for (j = 0; j< (int)width; j++) {
                *dest_y = (299 * src_p[2] + 587 * src_p[1] + 114 * src_p[0]) / 1000;

                if ((j & 1) == 0) {
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

int main()
{
    FILE *bg_file_y = NULL;
    FILE *bg_file_c = NULL;

    FILE *out_file_y = NULL;
    FILE *out_file_c = NULL;
    int i;
    long long time1 = 0;
    long long time2 = 0;
    char filename[64];
    int watermark_pic_num = 13;

    unsigned char *tmp_argb = NULL;

    BackGroudLayerInfo BG_info;
    WaterMarkInfo WM_info;
    ShowWaterMarkParam WM_Param;
    AjustBrightnessParam AD_bright;

    bg_file_y = fopen("/data/camera/y_in.dat", "r");
    if (bg_file_y == NULL) {
        printf("open file error\n");
        return 0;
    }

    bg_file_c = fopen("/data/camera/c_in.dat", "r");
    if (bg_file_c == NULL) {
        printf("open file error\n");
        fclose(bg_file_y);//avoid resource leak
        return 0;
    }

    out_file_y = fopen("/data/camera/y_out.dat", "wb");
    if (out_file_y == NULL) {
        printf("open file error\n");
        fclose(bg_file_y);//avoid resource leak
        fclose(bg_file_c);//avoid resource leak
        return 0;
    }

    out_file_c = fopen("/data/camera/c_out.dat", "wb");
    if(out_file_c == NULL) {
        printf("open file error\n");
        fclose(bg_file_y);//avoid resource leak
        fclose(bg_file_c);//avoid resource leak
        fclose(out_file_y);//avoid resource leak
        return 0;
    }

    memset(&BG_info, 0, sizeof(BackGroudLayerInfo));
    memset(&WM_info, 0, sizeof(WaterMarkInfo));
    memset(&AD_bright, 0, sizeof(AjustBrightnessParam));

    AD_bright.recycle_frame = 30; // change brightness  30 frame

    // init backgroud info
    BG_info.width = 1280;
    BG_info.height= 720;
    BG_info.y = (unsigned char *)malloc(BG_info.width * BG_info.height * 3 / 2);
    BG_info.c = BG_info.y + BG_info.width * BG_info.height;

    fread(BG_info.y, 1, BG_info.width * BG_info.height, bg_file_y);
    fread(BG_info.c, 1, BG_info.width * BG_info.height / 2, bg_file_c);

    // init watermark pic info

    for(i = 0; i< watermark_pic_num; i++)
    {
        FILE* icon_hdle = NULL;
        int width  = 0;
        int height = 0;

        sprintf(filename, "%s%d.bmp", "/data/camera/icon_720p_", i);

        icon_hdle = fopen(filename, "r");
        if (icon_hdle == NULL) {
            printf("get wartermark %s error\n", filename);
            return 0;
        }

        //get watermark picture size
        fseek(icon_hdle, 18, SEEK_SET);
        fread(&width, 1, 4, icon_hdle);
        fread(&height, 1, 4, icon_hdle);
        WM_info.single_pic[i].width = abs(width);
        WM_info.single_pic[i].height = abs(height);

        fseek(icon_hdle, 54, SEEK_SET);

        WM_info.single_pic[i].y = (unsigned char*)malloc(WM_info.single_pic[i].width * WM_info.single_pic[i].height * 5 / 2);
        WM_info.single_pic[i].alph = WM_info.single_pic[i].y + WM_info.single_pic[i].width * WM_info.single_pic[i].height;
        WM_info.single_pic[i].c = WM_info.single_pic[i].alph + WM_info.single_pic[i].width * WM_info.single_pic[i].height;
        WM_info.single_pic[i].id = i;

        if (tmp_argb == NULL) {
            tmp_argb = (unsigned char*)malloc( WM_info.single_pic[i].width * WM_info.single_pic[i].height * 4);
        }

        fread(tmp_argb, WM_info.single_pic[i].width * WM_info.single_pic[i].height*4, 1, icon_hdle);

        argb2yuv420sp(tmp_argb, WM_info.single_pic[i].alph,
                WM_info.single_pic[i].width, WM_info.single_pic[i].height,
                WM_info.single_pic[i].y, WM_info.single_pic[i].c);

        fclose(icon_hdle);
        icon_hdle = NULL;
    }

    WM_info.picture_number = i;

    // init watermark show para
    WM_Param.pos.x = 32;
    WM_Param.pos.y = 32;

    // 2013-11-17 18:28:26
    WM_Param.id_list[0] = 2;
    WM_Param.id_list[1] = 0;
    WM_Param.id_list[2] = 1;
    WM_Param.id_list[3] = 3;
    WM_Param.id_list[4] = 11;
    WM_Param.id_list[5] = 1;
    WM_Param.id_list[6] = 1;
    WM_Param.id_list[7] = 11;
    WM_Param.id_list[8] = 1;
    WM_Param.id_list[9] = 7;
    WM_Param.id_list[10] = 10;
    WM_Param.id_list[11] = 1;
    WM_Param.id_list[12] = 8;
    WM_Param.id_list[13] = 12;
    WM_Param.id_list[14] = 2;
    WM_Param.id_list[15] = 8;
    WM_Param.id_list[16] = 12;
    WM_Param.id_list[17] = 2;
    WM_Param.id_list[18] = 6;

    WM_Param.number = 19;

    time1 = GetNowUs();

#if 0
    watermark_blending(&BG_info, &WM_info, &WM_Param);
#else
    watermark_blending_ajust_brightness(&BG_info, &WM_info, &WM_Param, &AD_bright);
#endif

    time2 = GetNowUs();

    printf("11water_mark_blending time: %lld(us)\n", time2 - time1);

    fwrite(BG_info.y, 1, BG_info.width*BG_info.height, out_file_y);
    fwrite(BG_info.c, 1, BG_info.width*BG_info.height / 2, out_file_c);

    if (tmp_argb) {
        free(tmp_argb);
        tmp_argb = NULL;
    }

    if (BG_info.y) {
        free(BG_info.y);
        BG_info.y = NULL;
    }

    for (i= 0; i< watermark_pic_num; i++) {
        if (WM_info.single_pic[i].y) {
            free(WM_info.single_pic[i].y);
            WM_info.single_pic[i].y = NULL;
        }
    }

    fclose(bg_file_y);
    fclose(bg_file_c);
    fclose(out_file_y);
    fclose(out_file_c);

    bg_file_y = NULL;
    bg_file_c = NULL;
    out_file_y = NULL;
    out_file_c = NULL;

    return 0;
}

