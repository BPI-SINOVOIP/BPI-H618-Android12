/*
 * Copyright (c) 2016-2017, The Linux Foundation. All rights reserved.
 * Not a Contribution.
 *
 * Copyright 2015 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#include "EGLImageBuffer.h"
#include <cutils/native_handle.h>
#include <ui/GraphicBuffer.h>
#include <map>
#include "EGLImageWrapper.h"
#include "glengine.h"
#include <drm/drm_fourcc.h>

#define HWC_ALIGN(x,a) (((x) + (a) - 1L) & ~((a) - 1L))

static void checkGlError(const char* op) {
    for (GLint error = glGetError(); error; error
         = glGetError()) {
        ALOGD("after %s() glError (0x%x)\n", op, error);
    }
}

EGLImageKHR create_eglImage(android::sp<android::GraphicBuffer> graphicBuffer)
{
    bool isProtected = (graphicBuffer->getUsage() & GRALLOC_USAGE_PROTECTED);
    EGLint attrs[] = {EGL_IMAGE_PRESERVED_KHR, EGL_TRUE,
        isProtected ? EGL_PROTECTED_CONTENT_EXT : EGL_NONE,
        isProtected ? EGL_TRUE : EGL_NONE, EGL_NONE};

    EGLImageKHR eglImage = eglCreateImageKHR(eglGetCurrentDisplay(),
            (EGLContext)EGL_NO_CONTEXT,
            EGL_NATIVE_BUFFER_ANDROID,
            (EGLClientBuffer)(graphicBuffer->getNativeBuffer()),
            attrs);

    checkGlError("eglCreateImageKHR");

    ALOGD("%s error0x%x, eglImage=%p\n", __func__, glGetError(), eglImage);

    return eglImage;
}

EGLImageBuffer::EGLImageBuffer(private_handle_t *handle) {
    EGLint attribs[30];
    int atti = 0;
    int p1Ofst, p2Ofst;
    int format;
    if (handle->format == HAL_PIXEL_FORMAT_YV12) {
        int height = 0;
        ALOGD("%s get yuv\n", __func__);
        format = DRM_FORMAT_YVU420;
        height = HWC_ALIGN(handle->height, handle->aw_byte_align[1]);
        p1Ofst = handle->stride * height;
        p2Ofst = handle->stride * height
            + handle->stride * height * 1 / 4;

        attribs[atti++] = EGL_WIDTH;
        attribs[atti++] = handle->width;
        attribs[atti++] = EGL_HEIGHT;
        attribs[atti++] = handle->height;
        attribs[atti++] = EGL_LINUX_DRM_FOURCC_EXT;
        attribs[atti++] = DRM_FORMAT_YVU420;

        attribs[atti++] = EGL_DMA_BUF_PLANE0_FD_EXT;
        attribs[atti++] = handle->share_fd;
        attribs[atti++] = EGL_DMA_BUF_PLANE0_OFFSET_EXT;
        attribs[atti++] = 0;
        attribs[atti++] = EGL_DMA_BUF_PLANE0_PITCH_EXT;
        attribs[atti++] = handle->width;

        attribs[atti++] = EGL_DMA_BUF_PLANE1_FD_EXT;
        attribs[atti++] = handle->share_fd;
        attribs[atti++] = EGL_DMA_BUF_PLANE1_OFFSET_EXT;
        attribs[atti++] = p1Ofst;
        attribs[atti++] = EGL_DMA_BUF_PLANE1_PITCH_EXT;
        attribs[atti++] = handle->width / 2;

        attribs[atti++] = EGL_DMA_BUF_PLANE2_FD_EXT;
        attribs[atti++] = handle->share_fd;
        attribs[atti++] = EGL_DMA_BUF_PLANE2_OFFSET_EXT;
        attribs[atti++] = p2Ofst;
        attribs[atti++] = EGL_DMA_BUF_PLANE2_PITCH_EXT;
        attribs[atti++] = handle->width / 2;

        attribs[atti++] = EGL_YUV_COLOR_SPACE_HINT_EXT;
        attribs[atti++] = EGL_ITU_REC709_EXT;
        attribs[atti++] = EGL_SAMPLE_RANGE_HINT_EXT;
        attribs[atti++] = EGL_YUV_FULL_RANGE_EXT;

        attribs[atti++] = EGL_NONE;

    } else {
        ALOGD("%s get rgb\n", __func__);
        format = DRM_FORMAT_RGBA8888;
        p1Ofst = 0;
        p2Ofst = 0;
        attribs[atti++] = EGL_WIDTH;
        attribs[atti++] = handle->width;
        attribs[atti++] = EGL_HEIGHT;
        attribs[atti++] = handle->height;
        attribs[atti++] = EGL_LINUX_DRM_FOURCC_EXT;
        attribs[atti++] = DRM_FORMAT_ABGR8888;
        // plane 0
        attribs[atti++] = EGL_DMA_BUF_PLANE0_FD_EXT;
        attribs[atti++] = handle->share_fd;
        attribs[atti++] = EGL_DMA_BUF_PLANE0_OFFSET_EXT;
        attribs[atti++] = 0;
        attribs[atti++] = EGL_DMA_BUF_PLANE0_PITCH_EXT;
        attribs[atti++] = handle->width * 4;
        attribs[atti++] = EGL_NONE;
    }

    this->eglImageID = eglCreateImageKHR(eglGetCurrentDisplay(),
                                         EGL_NO_CONTEXT, EGL_LINUX_DMA_BUF_EXT, 0, attribs);
    checkGlError("eglCreateImageKHR");

    if (this->eglImageID == EGL_NO_IMAGE_KHR)
        ALOGD("Error:0000 failed: 0x%08X\n",glGetError());

    textureID = 0;
    renderbufferID = 0;
    framebufferID = 0;
    this->width = handle->width;
    this->height = handle->height;
    ALOGD("%s w,h = %d,%d, eglImageID=%p\n", __func__, this->width,
          this->height, this->eglImageID);
}

EGLImageBuffer::EGLImageBuffer(android::sp<android::GraphicBuffer> graphicBuffer)
{
    //this->graphicBuffer = graphicBuffer;
    this->eglImageID = create_eglImage(graphicBuffer);
    checkGlError("eglCreateImageKHR");
    this->width = graphicBuffer->getWidth();
    this->height = graphicBuffer->getHeight();
    ALOGD("%s w,h = %d,%d, ntb=%p\n", __func__, this->width, this->height,
          graphicBuffer->getNativeBuffer());

    textureID = 0;
    renderbufferID = 0;
    framebufferID = 0;
}

EGLImageBuffer::~EGLImageBuffer()
{
    if (textureID != 0) {
        GL(glDeleteTextures(1, &textureID));
        textureID = 0;
    }

#ifdef USE_GRAPHICBUFFER
    if (renderbufferID != 0) {
        GL(glDeleteRenderbuffers(1, &renderbufferID));
        renderbufferID = 0;
    }
#else
    if (renderbufferID != 0) {
        GL(glDeleteTextures(1, &renderbufferID));
        renderbufferID = 0;
    }
#endif

    if (framebufferID != 0) {
        GL(glDeleteFramebuffers(1, &framebufferID));
        framebufferID = 0;
    }

    // Delete the eglImage
    if (eglImageID != 0)
    {
        eglDestroyImageKHR(eglGetCurrentDisplay(), eglImageID);
        eglImageID = 0;
    }
}

int EGLImageBuffer::getWidth()
{
    return width;
}

int EGLImageBuffer::getHeight()
{
    return height;
}

unsigned int EGLImageBuffer::getTexture()
{
    if (textureID == 0) {
        bindAsTexture();
    }

    return textureID;
}

unsigned int EGLImageBuffer::getOutTexture()
{
    if (renderbufferID == 0) {
        bindAsFramebuffer();
    }

    return renderbufferID;
}

unsigned int EGLImageBuffer::getFramebuffer()
{
    if (framebufferID == 0) {
        bindAsFramebuffer();
    }

    return framebufferID;
}

void EGLImageBuffer::bindAsTexture()
{
#ifdef USE_GRAPHICBUFFER
    if (textureID == 0) {
        GL(glGenTextures(1, &textureID));
        int target = GL_TEXTURE_2D;
        GL(glBindTexture(target, textureID));
        GL(glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
        GL(glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
        GL(glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
        GL(glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));

        GL(glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, eglImageID));
    }

    GL(glBindTexture(GL_TEXTURE_2D, textureID));
#else
    if (textureID == 0) {
        GL(glGenTextures(1, &textureID));
        glActiveTexture(GL_TEXTURE0);
        int target = GL_TEXTURE_EXTERNAL_OES;
        GL(glBindTexture(target, textureID));
        GL(glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
        GL(glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
        GL(glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
        GL(glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));

        GL(glEGLImageTargetTexture2DOES(target, (GLeglImageOES)eglImageID));
    }

    GL(glBindTexture(GL_TEXTURE_EXTERNAL_OES, textureID));

#endif
}

void EGLImageBuffer::bindAsFramebuffer()
{
#ifdef USE_GRAPHICBUFFER
    if (renderbufferID == 0) {
        GL(glGenRenderbuffers(1, &renderbufferID));

        GL(glBindRenderbuffer(GL_RENDERBUFFER, renderbufferID));
        GL(glEGLImageTargetRenderbufferStorageOES(GL_RENDERBUFFER, eglImageID));

        GL(glGenFramebuffers(1, &framebufferID));
        GL(glBindFramebuffer(GL_FRAMEBUFFER, framebufferID));
        GL(glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER,
                                     renderbufferID));

        GLenum result = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (result != GL_FRAMEBUFFER_COMPLETE) {
            ALOGD("%s Framebuffer Invalid*************** %p", __FUNCTION__, eglImageID);
        }
    }

    GL(glBindFramebuffer(GL_FRAMEBUFFER, framebufferID));
#else
    if (renderbufferID == 0) {
        GL(glGenTextures(1, &renderbufferID));

        GL(glActiveTexture(GL_TEXTURE1));
        GL(glBindTexture(GL_TEXTURE_EXTERNAL_OES, renderbufferID));
        GL(glEGLImageTargetTexture2DOES(GL_TEXTURE_EXTERNAL_OES, (GLeglImageOES)eglImageID));

        GL(glGenFramebuffers(1, &framebufferID));
        GL(glBindFramebuffer(GL_FRAMEBUFFER, framebufferID));
        /*GL(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
          GL_TEXTURE_EXTERNAL_OES, renderbufferID, 0));*/
        GL(glFramebufferTexture2DMultisampleEXT(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                                GL_TEXTURE_EXTERNAL_OES, renderbufferID, 0, 4));

        GLenum result = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (result != GL_FRAMEBUFFER_COMPLETE) {
            ALOGD("%s Framebuffer Invalid*************** %p", __FUNCTION__, eglImageID);
        }
    }

    GL(glBindFramebuffer(GL_FRAMEBUFFER, framebufferID));

#endif
}
