#include <jni.h>
#include <string>
#include <android/bitmap.h>
#include <iostream>
#include <opencv2/opencv.hpp>
#include <opencv2/core/base.hpp>
#import "opencv2/stitching.hpp"
#import "opencv2/imgcodecs.hpp"
#include <fstream>

#include <android/log.h>
#include "ImgStitcher.h"
#include "CroppImg.h"

using namespace std;
using namespace cv;
int stitchImg(vector<Mat> &imagesArg, Mat &result, vector<string> params);

int cropp(Mat &result);
#define MAKE_RGBA(r, g, b, a) (((a) << 24) | ((r) << 16) | ((g) << 8) | (b))
#define RGBA_A(p) (((p) & 0xFF000000) >> 24)
#define  LOG_TAG    "native-lib"

#define  LOGD(...)  __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

Mat *gAddress = NULL;
/*
 * This method uses the openCV Stitcher class to create panorama image from given pictures list
 * Additionally if the stitching was successful it crops the image to rectangular shape
 */
extern "C"
JNIEXPORT jint JNICALL
Java_com_allwinner_camera_panoramic_NativePanoramic_processPanorama(JNIEnv *env, jclass clazz, jlongArray imageAddressArray, jlong outputAddress,
         jobjectArray stringArray) {

    bool isCropped = false;
    int status;
    int size = env->GetArrayLength(stringArray);
    vector<string> params;
    bool useDefault = false;
    for (int i = 0; i < size; ++i) {
        auto args = (jstring) env->GetObjectArrayElement(stringArray, i);
        const char *value = env->GetStringUTFChars(args, nullptr);
        if (string(value) == "cropp") {
            isCropped = true;
        } else if (string(value) == "open_cv_default") {
            useDefault = true;
        } else {
            params.emplace_back(value);
        }
        env->ReleaseStringUTFChars(args, value);
        env->DeleteLocalRef(args);
    }

    // Get the length of the long array
    jsize a_len = env->GetArrayLength(imageAddressArray);
    // Convert the jlongArray to an array of jlong
    jlong *imgAddressArr = env->GetLongArrayElements(imageAddressArray, 0);
    // Create a vector to store all the image
    vector<Mat> imgVec;
    for (int k = 0; k < a_len; k++) {
        // Get the image
        LOGE("  code = %d", a_len);
        Mat &curimage = *(Mat *) imgAddressArr[k];
        Mat newimage;
        // Convert to a 3 channel Mat
        cvtColor(curimage, newimage, CV_BGRA2RGB);
        curimage.release();
        imgVec.push_back(newimage);
        newimage.release();
    }
//    if (useDefault) {
//        int64 app_start_time = getTickCount();
//        Mat &result = *(Mat *) outputAddress;
//        Stitcher::Mode mode = Stitcher::PANORAMA;
//        Ptr<Stitcher> stitcher = Stitcher::create(mode, false);
//        Stitcher::Status status = stitcher->stitch(imgVec, result);
//        LOGP("OpenCV Stitcher, total time: %f",
//             ((getTickCount() - app_start_time) / getTickFrequency()));
//        if (status != Stitcher::OK) {
//            LOGE("Can't1 stitch images, error code = %d", int(status));
//        } else {
//            LOGD("Stitch SUCCESS");
//            if (isCropped) {
//                LOGD("cropping...");
//                if (cropp(result) != 0) {
//                    LOGE("cropping FAILED");
//                } else {
//                    LOGD("cropping SUCCESS");
//                }
//            }
       // }
//    } else {
    Mat &result = *(Mat *) outputAddress;
    gAddress = (Mat *) outputAddress;
    status = stitchImg(imgVec, result, params);
    LOGD("cropping2... %d",result.type());
    if (status != 0) {
        LOGE("Can't2 stitch images, error code = %d", status);
    } else {
        LOGD("Stitch SUCCESS");
        if (isCropped) {
            LOGD("cropping... %d",result.type());
            if (cropp(result) != 0) {
                LOGE("cropping FAILED");
            } else {
                LOGD("cropping SUCCESS");
            }
        }
    }
//    }
    return status;
    // Release the jlong array
    env->ReleaseLongArrayElements(imageAddressArray, imgAddressArr, 0);
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_allwinner_camera_panoramic_NativePanoramic_getProgress
        (JNIEnv *env, jclass clazz) {
    return getProgress();
}

extern "C"
JNIEXPORT void JNICALL
Java_com_allwinner_camera_panoramic_NativePanoramic_cropPanorama
        (JNIEnv *env, jclass clazz, jlong imageAddress, jlong outputAddress) {

    Mat &curimage = *(Mat *) imageAddress;
    Mat &result = *(Mat *) outputAddress;

    cvtColor(curimage, result, CV_RGB2BGRA);

    LOGD("cropping...");
    if (cropp(result) != 0) {
        LOGE("cropping FAILED");
    } else {
        LOGD("cropping SUCCESS");
    }
}

void MatToBitmap(JNIEnv *env, Mat &mat, jobject &bitmap, jboolean needPremultiplyAlpha) {
    AndroidBitmapInfo info;
    void *pixels = 0;
    Mat &src = mat;


    try {

        LOGD("nMatToBitmap");
        CV_Assert(AndroidBitmap_getInfo(env, bitmap, &info) >= 0);
        LOGD("nMatToBitmap1");

        CV_Assert(info.format == ANDROID_BITMAP_FORMAT_RGBA_8888 ||
                  info.format == ANDROID_BITMAP_FORMAT_RGB_565);
        LOGD("nMatToBitmap2 :%d  : %d  :%d", src.dims, src.rows, src.cols);

        CV_Assert(src.dims == 2 && info.height == (uint32_t) src.rows &&
                  info.width == (uint32_t) src.cols);
        LOGD("nMatToBitmap3 %d %d %d %d %d",src.type(),CV_16SC3,CV_8UC2,CV_8UC3,CV_8UC4);
       //CV_Assert(src.type() == CV_8UC1 || src.type() == CV_8UC3 || src.type() == CV_8UC4);
        LOGD("nMatToBitmap4");
        CV_Assert(AndroidBitmap_lockPixels(env, bitmap, &pixels) >= 0);
        LOGD("nMatToBitmap5");
        CV_Assert(pixels);
        LOGD("nMatToBitmap6");


        if (info.format == ANDROID_BITMAP_FORMAT_RGBA_8888) {
            Mat tmp(info.height, info.width, CV_8UC4, pixels);
//            Mat tmp(info.height, info.width, CV_8UC3, pixels);
            if (src.type() == CV_8UC1) {
                LOGD("nMatToBitmap: CV_8UC1 -> RGBA_8888");
                cvtColor(src, tmp, COLOR_GRAY2RGBA);
            } else if (src.type() == CV_8UC3) {
                LOGD("nMatToBitmap: CV_8UC3 -> RGBA_8888");
//                cvtColor(src, tmp, COLOR_RGB2RGBA);
//                cvtColor(src, tmp, COLOR_RGB2RGBA);
                cvtColor(src, tmp, COLOR_BGR2RGBA);
//                src.copyTo(tmp);
            } else if (src.type() == CV_8UC4) {
                LOGD("nMatToBitmap: CV_8UC4 -> RGBA_8888");
                if (needPremultiplyAlpha)
                    cvtColor(src, tmp, COLOR_RGBA2mRGBA);
                else
                    src.copyTo(tmp);
            }
        } else {
            // info.format == ANDROID_BITMAP_FORMAT_RGB_565
            Mat tmp(info.height, info.width, CV_8UC2, pixels);
            if (src.type() == CV_8UC1) {
                LOGD("nMatToBitmap: CV_8UC1 -> RGB_565");
                cvtColor(src, tmp, COLOR_GRAY2BGR565);
            } else if (src.type() == CV_8UC3) {
                LOGD("nMatToBitmap: CV_8UC3 -> RGB_565");
//                src.copyTo(tmp);
                cvtColor(src, tmp, COLOR_RGB2BGR565);
            } else if (src.type() == CV_8UC4) {
                LOGD("nMatToBitmap: CV_8UC4 -> RGB_565");
                cvtColor(src, tmp, COLOR_RGBA2BGR565);
            }
        }
        AndroidBitmap_unlockPixels(env, bitmap);
        return;
    } catch (const cv::Exception &e) {
        AndroidBitmap_unlockPixels(env, bitmap);
        LOGE("nMatToBitmap catched cv::Exception: %s", e.what());
        jclass je = env->FindClass("org/opencv/core/CvException");
        if (!je) je = env->FindClass("java/lang/Exception");
        env->ThrowNew(je, e.what());
        return;
    } catch (...) {
        AndroidBitmap_unlockPixels(env, bitmap);
        LOGE("nMatToBitmap catched unknown exception (...)");
        jclass je = env->FindClass("java/lang/Exception");
        env->ThrowNew(je, "Unknown exception in JNI code {nMatToBitmap}");
        return;
    }
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_allwinner_camera_panoramic_NativePanoramic_getBitmap(JNIEnv *env, jclass type, jobject bitmap) {

    Mat &result = *(Mat *) gAddress;
    if (result.dims != 2){
        return -1;
    }

    MatToBitmap(env,result,bitmap,false);

    return 0;

}
