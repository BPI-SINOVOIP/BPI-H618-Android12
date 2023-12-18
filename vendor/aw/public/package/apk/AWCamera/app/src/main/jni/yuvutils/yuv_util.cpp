

#include <jni.h>
#include <android/log.h>
#include <string.h>
#include <android/bitmap.h>
#include <malloc.h>
#include "YUVLIB/include/libyuv/basic_types.h"
#include "YUVLIB/include/libyuv/convert.h"
#include "YUVLIB/include/libyuv.h"




/*（YUV1YUV1YUV1YUV1） 转NV21 (YYYYUVUV)*/
extern "C"
JNIEXPORT void JNICALL
Java_com_allwinner_camera_utils_YuvUtils_changeYuvToNv21
        (JNIEnv *env, jobject instance, jbyteArray inputdata, jbyteArray outyuvBuf, jint width,
         jint height, jint bits) {
  //  __android_log_print(ANDROID_LOG_DEBUG, "yuvUtils", "changeYuvToNv21 start");
    unsigned char *yuv_image = (unsigned char *) env->GetByteArrayElements(inputdata, NULL);
    unsigned char *nv21_image = (unsigned char *) env->GetByteArrayElements(outyuvBuf, NULL);
    unsigned char *nv21_y_image = nv21_image;
    unsigned char *nv21_uv_image = nv21_image + width * height;
    int line1 = 0, line2 = 0, y_line1 = 0, y_line2 = 0;
    int stride = width * bits;
    int u1 = 0, v1 = 0, u2 = 0, v2 = 0, u3 = 0, v3 = 0, u4 = 0, v4 = 0;

    for (int i = 0; i < height; i += 2) {
        line1 = i * stride;
        line2 = line1 + stride;

        y_line1 = i * width;
        y_line2 = y_line1 + width;
        for (int j = 0; j < width; j += 2) {
            *(nv21_y_image + y_line1++) = *(yuv_image + line1++);
            u1 = *(yuv_image + line1++);
            v1 = *(yuv_image + line1++);
            line1++;
            *(nv21_y_image + y_line1++) = *(yuv_image + line1++);
            u2 = *(yuv_image + line1++);
            v2 = *(yuv_image + line1++);
            line1++;

            *(nv21_y_image + y_line2++) = *(yuv_image + line2++);
            u3 = *(yuv_image + line2++);
            v3 = *(yuv_image + line2++);
            line2++;
            *(nv21_y_image + y_line2++) = *(yuv_image + line2++);
            u4 = *(yuv_image + line2++);
            v4 = *(yuv_image + line2++);
            line2++;

            *(nv21_uv_image++) = (v1 + v2 + v3 + v4) >> 2;
            *(nv21_uv_image++) = (u1 + u2 + u3 + u4) >> 2;
        }
    }
    if (NULL != yuv_image) {
        env->ReleaseByteArrayElements(inputdata, (jbyte *) yuv_image, 0);
    }
    if (NULL != nv21_image) {
        env->ReleaseByteArrayElements(outyuvBuf, (jbyte *) nv21_image, 0);
    }
   // __android_log_print(ANDROID_LOG_DEBUG, "yuvUtils", "changeYuvToNv21 end");
}

void seperate_vu(unsigned char *vu_buf, unsigned char *u_buf, unsigned char *v_buf, int size) {
    while (size--) {
        *(u_buf++) = *(vu_buf++);
        *(v_buf++) = *(vu_buf++);
    }
}


extern "C"
JNIEXPORT void JNICALL
Java_com_allwinner_camera_utils_YuvUtils_changeNv21torealyuv
        (JNIEnv *env, jobject instance, jbyteArray Nv21Buf, jint width,
         jint height, jint stride, jboolean isBurst) {
   // __android_log_print(ANDROID_LOG_DEBUG, "yuvUtils", "changeNv21torealyuv start");
    unsigned char *pNv21 = (unsigned char *) env->GetByteArrayElements(Nv21Buf, NULL);
    if (pNv21 == NULL) {
        return;
    }
    long srcDataSize = env->GetArrayLength(Nv21Buf);
    //unsigned char *pRelYuv = (unsigned char *) env->GetByteArrayElements(realNv21Buf, NULL);
    unsigned char *pRelYuv = (unsigned char *) malloc(srcDataSize + 1);
    int frame_size = width * height;
    // copy y channel
    if (stride == width) {
        memcpy(pRelYuv, pNv21, frame_size);
    } else {
        unsigned char *p_pass1_y = pRelYuv;
        unsigned char *p_gy = pNv21;
        for (int i = 0; i < height; ++i) {
            memcpy(p_pass1_y, p_gy, width);
            p_pass1_y += width;
            p_gy += stride;
        }
    }
    // copy uv channel
    unsigned char *p_gvu = NULL;
    unsigned char *p_pass1_vu = pRelYuv + frame_size;
    int strideH = height / 64 + ((height % 64 > 0) ? 1 : 0);
    int height_stride = strideH * 64; //for height stride
    if (stride == width) {
        if (isBurst) {
            p_gvu = pNv21 + stride * height;
        } else {
            p_gvu = pNv21 + stride * (height - 1) +
                    width;  //frame_size; //note: if height stride change maybe bug
        }
        memcpy(p_pass1_vu, p_gvu, (frame_size >> 2));
    } else {
        if (isBurst) {
            p_gvu = pNv21 + stride * height;
        } else {
            p_gvu = pNv21 + stride * (height - 1) +
                    width;  //frame_size; //note: if height stride change maybe bug
        }
        const int vu_h = height >> 1;
        const int vu_w = width;
        for (int i = 0; i < vu_h; ++i) {
            memcpy(p_pass1_vu, p_gvu, vu_w);
            p_gvu += stride;
            p_pass1_vu += vu_w;
        }
    }
    int yv12length = (frame_size >> 1) + frame_size;
    memcpy(pNv21, pRelYuv, yv12length);

    free(pRelYuv);

    env->ReleaseByteArrayElements(Nv21Buf, (jbyte *) pNv21, 0);
/*    if (NULL != pRelYuv) {
        env->ReleaseByteArrayElements(realNv21Buf, (jbyte *) pRelYuv, 0);
    }*/
   // __android_log_print(ANDROID_LOG_DEBUG, "yuvUtils", "changeNv21torealyuv end");
}



void rotateMirror(jint width, jint height, jint yStride, jint vuStride, jboolean mirror,
                  jint orientation, int size, int i420Stride, int rotatedStride, int rWidth,
                  int rHeight, const jbyte *y, const jbyte *vu) {
    uint8 *src_y = static_cast<uint8 *>(malloc(static_cast<size_t>(size)));
    uint8 *src_u = static_cast<uint8 *>(malloc(static_cast<size_t>(size / 4)));
    uint8 *src_v = static_cast<uint8 *>(malloc(static_cast<size_t>(size / 4)));
    uint8 *dst_y = static_cast<uint8 *>(malloc(static_cast<size_t>(size)));
    uint8 *dst_u = static_cast<uint8 *>(malloc(static_cast<size_t>(size / 4)));
    uint8 *dst_v = static_cast<uint8 *>(malloc(static_cast<size_t>(size / 4)));
    uint8 *mirror_y = static_cast<uint8 *>(malloc(static_cast<size_t>(size)));
    uint8 *mirror_u = static_cast<uint8 *>(malloc(static_cast<size_t>(size / 4)));
    uint8 *mirror_v = static_cast<uint8 *>(malloc(static_cast<size_t>(size / 4)));

    libyuv::NV21ToI420((unsigned char *) y, yStride, (unsigned char *) vu, vuStride, src_y,
                       i420Stride, src_u, i420Stride >> 1,
                       src_v, i420Stride >> 1, width, height);
    if (mirror) {
        I420Rotate(src_y, i420Stride, src_u, i420Stride >> 1, src_v, i420Stride >> 1, mirror_y,
                   rotatedStride,
                   mirror_u, rotatedStride >> 1, mirror_v, rotatedStride >> 1, width, height,
                   static_cast<libyuv::RotationMode>(orientation));
        libyuv::I420Mirror(mirror_y, rotatedStride, mirror_u, rotatedStride >> 1, mirror_v,
                           rotatedStride >> 1, dst_y, rotatedStride,
                           dst_u, rotatedStride >> 1, dst_v, rotatedStride >> 1, rWidth, rHeight);
    } else {
        I420Rotate(src_y, i420Stride, src_u, i420Stride >> 1, src_v, i420Stride >> 1, dst_y,
                   rotatedStride,
                   dst_u, rotatedStride >> 1, dst_v, rotatedStride >> 1, width, height,
                   static_cast<libyuv::RotationMode>(orientation));
    }
    libyuv::I420ToNV21(dst_y, rotatedStride, dst_u, rotatedStride >> 1, dst_v, rotatedStride >> 1,
                       (uint8 *) (y), rotatedStride,
                       (uint8 *) (vu), rotatedStride, rWidth, rHeight);

    free(src_y);
    free(src_u);
    free(src_v);
    free(dst_y);
    free(dst_u);
    free(dst_v);
    free(mirror_y);
    free(mirror_u);
    free(mirror_v);
}

extern "C"
JNIEXPORT jbyteArray JNICALL
Java_com_allwinner_camera_utils_YuvUtils_rotateNV21(JNIEnv *env, jclass type,
                                                                jbyteArray input_,
                                                                jint width, jint height,
                                                                jint rotation,
                                                                jboolean mirror) {
    //__android_log_print(ANDROID_LOG_ERROR, "yuvUtils", "rotateNV21 start %d", rotation);
    int i420Stride = width;
    int rotatedStride = i420Stride;
    int rWidth = width, rHeight = height;
    if (rotation == 0 && !mirror) {
        return input_;
    } else if (rotation == 90 || rotation == 270) {
        rotatedStride = height;
        rWidth = height;
        rHeight = width;
    }
    jbyte *input = env->GetByteArrayElements(input_, NULL);

    int size = width * height;
    jbyte *y = input;
    jbyte *vu = input + size;


    rotateMirror(width, height, i420Stride, i420Stride, mirror, rotation, size, i420Stride,
                 rotatedStride, rWidth, rHeight, y, vu);

    env->ReleaseByteArrayElements(input_, input, 0);
    //__android_log_print(ANDROID_LOG_ERROR, "yuvUtils", "rotateNV21 end");
    return input_;
}


void i420ToNv21(jbyte *src_i420_data, jint width, jint height, jbyte *src_nv21_data){
    jint src_y_size = width * height;
    jint src_u_size = (width >> 1) * (height >> 1);

    jbyte *src_nv21_y_data = src_nv21_data;
    jbyte *src_nv21_vu_data = src_nv21_data + src_y_size;

    jbyte *src_i420_y_data = src_i420_data;
    jbyte *src_i420_u_data = src_i420_data + src_y_size;
    jbyte *src_i420_v_data = src_i420_data + src_y_size + src_u_size;
 //__android_log_print(ANDROID_LOG_DEBUG, "yuvUtil", "i420ToNv21   222");
    libyuv::I420ToNV21((const uint8 *)src_i420_y_data, width, (const uint8 *)src_i420_u_data, width >> 1, (const uint8 *)src_i420_v_data, width >> 1,
                       (uint8 *) (src_nv21_y_data), width,
                       (uint8 *) (src_nv21_vu_data), width, width, height);
   // __android_log_print(ANDROID_LOG_DEBUG, "yuvUtil", "i420ToNv21 end");

}



void nv21ToI420(jbyte *src_nv21_data, jint width, jint height, jbyte *src_i420_data) {
    jint src_y_size = width * height;
    jint src_u_size = (width >> 1) * (height >> 1);

    jbyte *src_nv21_y_data = src_nv21_data;
    jbyte *src_nv21_vu_data = src_nv21_data + src_y_size;

    jbyte *src_i420_y_data = src_i420_data;
    jbyte *src_i420_u_data = src_i420_data + src_y_size;
    jbyte *src_i420_v_data = src_i420_data + src_y_size + src_u_size;

    libyuv::NV21ToI420((const uint8 *) src_nv21_y_data, width,
                       (const uint8 *) src_nv21_vu_data, width,
                       (uint8 *) src_i420_y_data, width,
                       (uint8 *) src_i420_u_data, width >> 1,
                       (uint8 *) src_i420_v_data, width >> 1,
                       width, height);
}

void scaleI420(jbyte *src_i420_data, jint width, jint height, jbyte *dst_i420_data, jint dst_width,
               jint dst_height, jint mode) {
    jint src_i420_y_size = width * height;
    jint src_i420_u_size = (width >> 1) * (height >> 1);
    jbyte *src_i420_y_data = src_i420_data;
    jbyte *src_i420_u_data = src_i420_data + src_i420_y_size;
    jbyte *src_i420_v_data = src_i420_data + src_i420_y_size + src_i420_u_size;

    jint dst_i420_y_size = dst_width * dst_height;
    jint dst_i420_u_size = (dst_width >> 1) * (dst_height >> 1);
    jbyte *dst_i420_y_data = dst_i420_data;
    jbyte *dst_i420_u_data = dst_i420_data + dst_i420_y_size;
    jbyte *dst_i420_v_data = dst_i420_data + dst_i420_y_size + dst_i420_u_size;

    libyuv::I420Scale((const uint8 *) src_i420_y_data, width,
                      (const uint8 *) src_i420_u_data, width >> 1,
                      (const uint8 *) src_i420_v_data, width >> 1,
                      width, height,
                      (uint8 *) dst_i420_y_data, dst_width,
                      (uint8 *) dst_i420_u_data, dst_width >> 1,
                      (uint8 *) dst_i420_v_data, dst_width >> 1,
                      dst_width, dst_height,
                      (libyuv::FilterMode) mode);
}


extern "C"
JNIEXPORT void JNICALL
Java_com_allwinner_camera_utils_YuvUtils_scaleNV21Data
        (JNIEnv *env, jobject instance, jbyteArray inputData, jint width, jint height,
         jbyteArray outputData, jint out_width, jint out_height) {
    unsigned char *input_array = (unsigned char *) env->GetByteArrayElements(inputData, NULL);
    unsigned char *output_array = (unsigned char *) env->GetByteArrayElements(outputData, NULL);
  //  __android_log_print(ANDROID_LOG_DEBUG, "yuvUtil", "scaleNV21Data start");
    jbyte *i420_data = (jbyte *) malloc(sizeof(jbyte) * width * height * 3 / 2);
    jbyte *scalei420_data = (jbyte *) malloc(sizeof(jbyte) * out_width * out_height * 3 / 2);
    nv21ToI420((jbyte *) input_array, width, height, i420_data);
//__android_log_print(ANDROID_LOG_DEBUG, "yuvUtil", "scaleNV21Data start1");
    scaleI420(i420_data, width, height, scalei420_data, out_width, out_height,
              0);
  //  i420ToNv21(scalei420_data, width, height, (jbyte *)output_array);
    i420ToNv21(scalei420_data, out_width, out_height, (jbyte *)output_array);
    if (NULL != input_array) {
        env->ReleaseByteArrayElements(inputData, (jbyte *) input_array, 0);
    }
    if (NULL != output_array) {
        env->ReleaseByteArrayElements(outputData, (jbyte *) output_array, 0);
    }

    free(i420_data);
    free(scalei420_data);
}


