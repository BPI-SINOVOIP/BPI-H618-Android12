//
// Created by huangweibin on 2021/4/26.
//

#ifndef GIFTEST_GIFPLAYER_H
#define GIFTEST_GIFPLAYER_H

#include <jni.h>
#include <gif_lib.h>
#include "PthreadSleep.h"
#include "SyncTime.h"

#define UNSIGNED_LITTLE_ENDIAN(lo, hi)    ((lo) | ((hi) << 8))
#define  MAKE_COLOR_ABGR(r, g, b) ((0xff) << 24 ) | ((b) << 16 ) | ((g) << 8 ) | ((r) & 0xff)

enum PlayState {
    IDLE,
    PREPARE,
    PLAYING
};

class GifPlayer {

public:

    jboolean load(JNIEnv *env,const char *gifPath);

    void play(JNIEnv *env, jboolean loop, jobject bitmap, jobject runnable);

    jint getWidth();

    jint getHeight();

    void pause();

    void resume();

    void stop();

private:
    GifFileType *gifFile = NULL;
    PthreadSleep threadSleep;
    SyncTime syncTime;
    pthread_mutex_t play_mutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_cond_t play_cond = PTHREAD_COND_INITIALIZER;
    bool is_pause = false;
    bool is_play_quit = false;
    int gif_width = 0;
    int gif_height = 0;
    int transparentColorIndex = 0;
    int disposalMode = DISPOSAL_UNSPECIFIED;
    enum PlayState play_state = IDLE;

    void _setPlayState(PlayState state);

    void _getPlayState(PlayState *playState);

    void _printGifError(int Error);

    int _prepareGif(JNIEnv *env, const char *filename);

    uint32_t _gifColorToColorARGB(const GifColorType &color);

    void _setColorARGB(uint32_t *sPixels, int imageIndex, ColorMapObject *colorMap,
                      GifByteType colorIndex);

    void _drawBitmap(JNIEnv *env, int imageIndex, jobject bitmap,
                    SavedImage *SavedImages, ColorMapObject *ColorMap,
                    GifRowType *ScreenBuffer,
                    int bitmapWidth,
                    int left, int top,
                    int width, int height);

    void _playGif(JNIEnv *env, bool loop, jobject bitmap, jobject runnable);

};

#endif //GIFTEST_GIFPLAYER_H
