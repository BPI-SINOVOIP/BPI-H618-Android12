//
// Created by huangweibin on 2021/4/26.
//

#include <string>
#include <malloc.h>
#include <string.h>
#include "log.h"
#include <unistd.h>
#include <android/bitmap.h>
#include <fcntl.h>
#include "GifPlayer.h"

#define DATA_OFFSET 3


/**************************************私有接口********************************************************/

void GifPlayer::_setPlayState(PlayState state) {
    pthread_mutex_lock(&play_mutex);
    play_state = state;
    pthread_mutex_unlock(&play_mutex);
}

void GifPlayer::_getPlayState(PlayState *playState) {
    pthread_mutex_lock(&play_mutex);
    *playState = play_state;
    pthread_mutex_unlock(&play_mutex);
}

void GifPlayer::_printGifError(int Error) {
    LOGE("error: %s", GifErrorString(Error));
}

int GifPlayer::_prepareGif(JNIEnv *env, const char *filename){
    int Error;
    gif_width = 0;
    gif_height = 0;
    is_play_quit = false;
    threadSleep.reset();
    transparentColorIndex = NO_TRANSPARENT_COLOR;
    //防止重复使用上一次的gifFile句柄
    if (NULL != gifFile) {
        DGifCloseFile(gifFile, &Error);
        gifFile = NULL;
    }
    _setPlayState(PREPARE);
    if (NULL == (gifFile = DGifOpenFileName(filename, &Error))) {
        _printGifError(Error);
        return -1;
    }
    gif_width = gifFile->SWidth;
    gif_height = gifFile->SHeight;
    LOGD("gif SWidth: %d SHeight: %d", gifFile->SWidth, gifFile->SHeight);
    return 0;
}

uint32_t GifPlayer::_gifColorToColorARGB(const GifColorType &color) {
    return (uint32_t) (MAKE_COLOR_ABGR(color.Red, color.Green, color.Blue));
}

void GifPlayer::_setColorARGB(uint32_t *sPixels, int imageIndex,
                             ColorMapObject *colorMap, GifByteType colorIndex) {
    if (imageIndex > 0 && disposalMode == DISPOSE_DO_NOT && colorIndex == transparentColorIndex) {
        return;
    }
    if (colorIndex != transparentColorIndex || transparentColorIndex == NO_TRANSPARENT_COLOR) {
        *sPixels = _gifColorToColorARGB(colorMap->Colors[colorIndex]);
    } else {
        *sPixels = 0;
    }
}

void GifPlayer::_drawBitmap(JNIEnv *env, int imageIndex, jobject bitmap,
                           SavedImage *SavedImages, ColorMapObject *ColorMap,
                           GifRowType *ScreenBuffer,
                           int bitmapWidth,
                           int left, int top,
                           int width, int height) {
    if(NULL == SavedImages || NULL == ScreenBuffer || NULL == ColorMap){
        LOGE("_drawBitmap parameter is NULL");
        return;
    }
    //锁定像素以确保像素的内存不会被移动，一副图片是二维数组
    void *pixels;
    AndroidBitmap_lockPixels(env, bitmap, &pixels);
    //拿到像素地址
    uint32_t *sPixels = (uint32_t *) pixels;
    //数据会偏移3，这个是固定的
    int dataOffset = sizeof(int32_t) * DATA_OFFSET;
    int dH = bitmapWidth * top;
    GifByteType colorIndex;
    //从左到右，一层一层设置Bitmap像素
    for (int h = top; h < height; h++) {
        for (int w = left; w < width; w++) {
            //像素点下标，给一个像素点设置ARGB
            colorIndex = (GifByteType) ScreenBuffer[h][w];
            //sPixels[dH + w] Bitmap像素地址，通过遍历给每个像素点设置argb，Bitmap就有颜色了
            _setColorARGB(&sPixels[dH + w],
                         imageIndex,
                         ColorMap,
                         colorIndex);
            if (NULL != SavedImages && NULL != SavedImages->RasterBits) {
                //将颜色下标保存，循环播放的时候需要知道这个下标，直接取出来就可以
                SavedImages->RasterBits[dataOffset++] = colorIndex;
            }
        }
        dH += bitmapWidth;
    }
    AndroidBitmap_unlockPixels(env, bitmap);
}

void GifPlayer::_playGif(JNIEnv *env, bool loop, jobject bitmap, jobject runnable) {

    GifRecordType RecordType;
    GifByteType *Extension;
    GifRowType *ScreenBuffer;
    GifByteType *GifExtension;
    ColorMapObject *ColorMap;

    int InterlacedOffset[] = {0, 4, 2, 1}; /* The way Interlaced image should. 隔行扫描*/
    int InterlacedJumps[] = {8, 8, 4, 2};    /* be read - offsets and jumps... */
    int ImageNum = 0;
    int Error;
    int i, j, Row, Col, Width, Height, ExtCode,imageCount;
    SavedImage *sp = NULL;
    int32_t delayTime = 0;
    unsigned int dt = 0;
    int32_t *user_image_data;
    size_t rowSize = 0;
    AndroidBitmapInfo bitmapInfo;
    int bitmapWidth;

    _setPlayState(PLAYING);

    AndroidBitmap_getInfo(env, bitmap, &bitmapInfo);
    bitmapWidth = bitmapInfo.stride / 4;

    jclass runClass = env->GetObjectClass(runnable);
    jmethodID runMethod = env->GetMethodID(runClass, "run", "()V");

    //所有的gif图像共享一个屏幕（Screen），这个屏幕和我们的电脑屏幕不同，只是一个逻辑概念。所有的图像都会绘制到屏幕上面。
    //首先我们需要给屏幕分配内存：
    if ((ScreenBuffer = (GifRowType *)
            malloc(gif_height * sizeof(GifRowType))) == NULL) {
        LOGE("Failed to allocate memory required, aborted.");
        goto end;
    }
    //一行像素占用的内存大小
    rowSize = gif_width * sizeof(GifPixelType);
    if ((ScreenBuffer[0] = (GifRowType) malloc(rowSize)) == NULL) {
        LOGE("Failed to allocate memory required, aborted.");
        goto end;
    }
    GifPixelType *buffer;
    buffer = (GifPixelType *) (ScreenBuffer[0]);

    /***** 给 ScreenBuffer 设置背景颜色为gif背景*/
    //设置第一行背景颜色
    for (i = 0; i < gif_width; i++)
        buffer[i] = (GifPixelType) gifFile->SBackGroundColor;
    //其它行拷贝第一行
    for (i = 1; i < gif_height; i++) {
        if ((ScreenBuffer[i] = (GifRowType) malloc(rowSize)) == NULL) {
            LOGE("Failed to allocate memory required, aborted.");
            goto end;
        }
        memcpy(ScreenBuffer[i], ScreenBuffer[0], rowSize);
    }
    syncTime.set_clock();
    /***** 循环解析gif数据，并根据不同的类型进行不同的处理*/
    do {
        //DGifGetRecordType函数用来获取下一块数据的类型
        if (DGifGetRecordType(gifFile, &RecordType) == GIF_ERROR) {
            _printGifError(gifFile->Error);
            goto end;
        }
        switch (RecordType) {
            //如果是图像数据块，需要绘制到 ScreenBuffer 中
            case IMAGE_DESC_RECORD_TYPE:
                // DGifGetImageDesc 函数是 获取gif的详细信息，例如 是否是隔行扫描，每个像素点的颜色信息等等
                if (DGifGetImageDesc(gifFile) == GIF_ERROR) {
                    _printGifError(gifFile->Error);
                    goto end;
                }
                //sp 表示当前这一帧图片信息，gifFile->ImageCount 会从1开始递增，表示当前解析到第几张图片
                //这里获取sp主要是想保存一些数据到当前解析的这帧图片中，循环播放的时候可以直接取出来用
                imageCount = gifFile->ImageCount;
                // LOGE("imageCount=%d\n",imageCount);
                //sp = &gifFile->SavedImages[gifFile->ImageCount - 1];
                sp = &gifFile->SavedImages[imageCount - 1];
                //RasterBits 字段分配内存
                sp->RasterBits = (unsigned char *) malloc(
                        sizeof(GifPixelType) * gif_width * gif_height +
                        sizeof(int32_t) * 2);
                //RasterBits 这块内存用来保存一些数据，延时、透明颜色下标等，循环播放的时候要用到
                user_image_data = (int32_t *) sp->RasterBits;
                user_image_data[0] = delayTime;
                user_image_data[1] = transparentColorIndex;
                user_image_data[2] = disposalMode;


                Row = gifFile->Image.Top;
                Col = gifFile->Image.Left;
                Width = gifFile->Image.Width;
                Height = gifFile->Image.Height;


                //正常情况下这个条件不成立
                if (gifFile->Image.Left + gifFile->Image.Width > gif_width ||
                    gifFile->Image.Top + gifFile->Image.Height > gif_height) {
                    LOGE("Image %d is not confined to screen dimension, aborted", ImageNum);
                    goto end;
                }
                //隔行扫描
                if (gifFile->Image.Interlace) {
                    //隔行扫描，要执行扫描4次才完整绘制完
                    for (i = 0; i < 4; i++)
                        // 从GifFile 中获取一行数据，放到ScreenBuffer 中去
                        for (j = Row + InterlacedOffset[i]; j < Row + Height;j += InterlacedJumps[i]) {
                            if (DGifGetLine(gifFile, &ScreenBuffer[j][Col],
                                            Width) == GIF_ERROR) {
                                _printGifError(gifFile->Error);
                                goto end;
                            }
                        }
                } else {
                    //没有隔行扫描，顺序一行一行来
                    for (i = 0; i < Height; i++) {
                        if (DGifGetLine(gifFile, &ScreenBuffer[Row++][Col],
                                        Width) == GIF_ERROR) {
                            _printGifError(gifFile->Error);
                            goto end;
                        }
                    }
                }
                //扫描完成，ScreenBuffer 中每个像素点是什么颜色就确定好了，就差绘制到Bitmap上了
                ColorMap = (gifFile->Image.ColorMap
                            ? gifFile->Image.ColorMap
                            : gifFile->SColorMap);
                //睡眠，delayTime 表示帧间隔时间，是从另一个数据块计算出来的
                //LOGE("1. before delayTime =%d\n",delayTime);
                dt = syncTime.synchronize_time(delayTime * 10);
                //LOGE("1. after delayTime =%d\n",dt);
                threadSleep.msleep(dt);
                //threadSleep.msleep(delayTime * 10);
                delayTime = 0;

                //暂停播放
                pthread_mutex_lock(&play_mutex);
                if (is_pause) {
                    is_pause = false;
                    pthread_cond_wait(&play_cond, &play_mutex);
                }
                pthread_mutex_unlock(&play_mutex);
                //将数据绘制到Bitmap上
                _drawBitmap(env, imageCount - 1,
                           bitmap, sp, ColorMap, ScreenBuffer,
                           bitmapWidth,
                           gifFile->Image.Left, gifFile->Image.Top,
                           gifFile->Image.Left + Width, gifFile->Image.Top + Height);

                //Bitmap绘制好了，回调runnable的run方法，Java层刷新ImageView即可看到新的一帧图片
                env->CallVoidMethod(runnable, runMethod);
                syncTime.set_clock();
                break;
                //额外信息块，获取帧之间间隔、透明颜色下标
            case EXTENSION_RECORD_TYPE:
                //获取额外数据，这个函数只会返回一个数据块，调用这个函数后要调用DGifGetExtensionNext
                if (DGifGetExtension(gifFile, &ExtCode, &Extension) == GIF_ERROR) {
                    _printGifError(gifFile->Error);
                    goto end;
                }
                if (ExtCode == GRAPHICS_EXT_FUNC_CODE) {
                    if (Extension[0] != 4) {
                        _printGifError(GIF_ERROR);
                        goto end;
                    }
                    GifExtension = Extension + 1;
                    //获取帧间隔，这些计算方法就先不追究了，必须要知道Gif格式，帧之间间隔单位是 10 ms
                    delayTime = UNSIGNED_LITTLE_ENDIAN(GifExtension[1], GifExtension[2]);
                    if(delayTime < 1){ // 如果没有时间，写个默认5
                        delayTime = 5;
                    }
                    if (GifExtension[0] & 0x01) {
                        //获取透明颜色下标，设置argb的时候要特殊处理//获取透明颜色下标，设置argb的时候要特殊处理
                        transparentColorIndex = (int) GifExtension[3];
                    } else {
                        transparentColorIndex = NO_TRANSPARENT_COLOR;
                    }
                    disposalMode = (GifExtension[0] >> 2) & 0x07;
                }
                while (Extension != NULL) {
                    //跳过其它块
                    if (DGifGetExtensionNext(gifFile, &Extension) == GIF_ERROR) {
                        _printGifError(gifFile->Error);
                        goto end;
                    }
                }
                break;
            case TERMINATE_RECORD_TYPE:
                break;
            default:
                break;
        }
    } while (RecordType != TERMINATE_RECORD_TYPE && !is_play_quit);


    if(NULL!= gifFile && NULL != ScreenBuffer){
        for (i = 0; i < gif_height; i++) {
            free(ScreenBuffer[i]);
        }
        free(ScreenBuffer);
        ScreenBuffer = NULL;
    } 
    syncTime.set_clock();

    while (!is_play_quit && loop) {
        for (int t = 0; t < imageCount; t++) {
            if (is_play_quit) {
                break;
            }
            SavedImage frame = gifFile->SavedImages[t];
            GifImageDesc frameInfo = frame.ImageDesc;

            ColorMap = (frameInfo.ColorMap
                        ? frameInfo.ColorMap
                        : gifFile->SColorMap);

            user_image_data = (int32_t *) frame.RasterBits;
            delayTime = user_image_data[0];
            transparentColorIndex = user_image_data[1];
            disposalMode = user_image_data[2];
            if (delayTime < 1) { // 如果没有时间，写个默认5
                delayTime = 5;
            }
            //LOGD("2. before delayTime = %d\n",delayTime);
            delayTime = syncTime.synchronize_time(delayTime * 10);
            //LOGD("2. after delayTime = %d\n",delayTime);
            threadSleep.msleep(delayTime);


            pthread_mutex_lock(&play_mutex);
            if (is_pause) {
                is_pause = false;
                pthread_cond_wait(&play_cond, &play_mutex);
            }
            pthread_mutex_unlock(&play_mutex);

            void *pixels;
            AndroidBitmap_lockPixels(env, bitmap, &pixels);
            uint32_t *sPixels = (uint32_t *) pixels;

            int pointPixelIdx = sizeof(int32_t) * DATA_OFFSET;
            int dH = bitmapWidth * frameInfo.Top;
            for (int h = frameInfo.Top; h < frameInfo.Top + frameInfo.Height; h++) {
                for (int w = frameInfo.Left; w < frameInfo.Left + frameInfo.Width; w++) {
                    _setColorARGB(&sPixels[dH + w],
                                 t,
                                 ColorMap,
                                 frame.RasterBits[pointPixelIdx++]);
                }
                dH += bitmapWidth;
            }
            AndroidBitmap_unlockPixels(env, bitmap);
            env->CallVoidMethod(runnable, runMethod);

            syncTime.set_clock();
        }
    }

    end:
    if (NULL != ScreenBuffer) {
        free(ScreenBuffer);
        ScreenBuffer = NULL;
    }

    if (DGifCloseFile(gifFile, &Error) == GIF_ERROR) {
            _printGifError(Error);
    }
    gifFile = NULL;
    _setPlayState(IDLE);
}
/**************************************私有接口********************************************************/


/**************************************JNI接口********************************************************/

jboolean GifPlayer::load(JNIEnv *env,const char *gifPath) {
    PlayState playState;
    int ret;
    _getPlayState(&playState);
    if (playState == IDLE) {
        ret = _prepareGif(env, gifPath);
    } else {
        ret = -1;
    }
    return (jboolean) (ret == 0 ? JNI_TRUE : JNI_FALSE);
}

void GifPlayer::play(JNIEnv *env, jboolean loop,
                      jobject bitmap, jobject runnable) {
    PlayState playState;
    _getPlayState(&playState);
    if (playState == PREPARE) {
        _playGif(env, loop, bitmap, runnable);
    }
}

jint GifPlayer::getWidth() {
    return gif_width;
}

jint GifPlayer::getHeight() {
    return gif_height;
}

void GifPlayer::pause() {
    pthread_mutex_lock(&play_mutex);
    if (!is_pause) {
        is_pause = true;
    }
    pthread_mutex_unlock(&play_mutex);
}

void GifPlayer::resume() {
    pthread_mutex_lock(&play_mutex);
    if (is_pause) {
        is_pause = false;
    }
    pthread_cond_signal(&play_cond);
    pthread_mutex_unlock(&play_mutex);
}

void GifPlayer::stop() {
    pthread_mutex_lock(&play_mutex);
    is_play_quit = true;
    is_pause = false;
    pthread_cond_signal(&play_cond);
    pthread_mutex_unlock(&play_mutex);
    threadSleep.interrupt();
}

/**************************************JNI接口********************************************************/
