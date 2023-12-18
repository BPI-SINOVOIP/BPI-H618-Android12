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
#include <utils/Log.h>

#include "EGLImageWrapper.h"
#include "ImageMapper.h"
#include "engine.h"
#include "KeystoneRender.h"

ImageMapper::ImageMapper()
{
    eglImageWrapper = new EGLImageWrapper();
}

ImageMapper::~ImageMapper()
{
    void* caller_context = engine_backup();
    engine_bind(engineContext);

    // clear EGLImage mappings
    if (eglImageWrapper != 0) {
        delete eglImageWrapper;
        eglImageWrapper = 0;
    }

    engine_shutdown(engineContext);
    // restore the caller context
    engine_bind(caller_context);
    engine_free_backup(caller_context);
}

void ImageMapper::setXRad(float rad) {
    if (mXrad != rad) {
        isChanged = true;
        mUpdateCnt = 0;
    }
    mXrad = rad;
}

void ImageMapper::setYRad(float rad) {
    if (mYrad != rad) {
        isChanged = true;
        mUpdateCnt = 0;
    }
    mYrad = rad;
}

ImageMapper *ImageMapper::build(bool isSecure)
{
    // build new imageMapper
    ImageMapper *imageMapper = new ImageMapper();

    imageMapper->engineContext = NULL;
    ALOGD("In %s context = %p", __FUNCTION__, (void *)(imageMapper->engineContext));

    //#define PI 3.14159265
    imageMapper->mXrad = 0.02;//30 * PI / 180;
    imageMapper->mYrad = 0.01;//10 * PI / 180;
    imageMapper->mWidth = 0;
    imageMapper->mHeight = 0;
    imageMapper->isSecure = isSecure;
    imageMapper->isChanged = true;
    imageMapper->mUpdateCnt = 0;

    return imageMapper;
}

void ImageMapper::initialize(int width, int height) {
    if (engineContext == NULL) {
        engineContext = engine_initialize(isSecure, width, height);
    }
}

int ImageMapper::doRender(const void *src, const void *dst, int srcFenceFd)
{
    private_handle_t *handle = (private_handle_t *)dst;
    void* caller_context = NULL;
    unsigned int fbo = 0;
    unsigned int tex = 0;
    int fenceFD = -1;
    if (engineContext == NULL) {
        engineContext = engine_initialize(isSecure, handle->width, handle->height);
    }
    caller_context = engine_backup();

    // make current
    engine_bind(engineContext);

    // create eglimages if required
    EGLImageBuffer *dst_buffer = eglImageWrapper->wrap(dst);
    EGLImageBuffer *src_buffer = eglImageWrapper->wrap(src);

    if (mWidth != dst_buffer->getWidth() || mHeight != dst_buffer->getHeight()) {
        mWidth = dst_buffer->getWidth();
        mHeight = dst_buffer->getHeight();
        setupGraphics(0, 0, mWidth, mHeight);
        /*ALOGD("%s graphic width=%d, height=%d",__func__, mWidth, mHeight);
          GLint mMaxTextureSize;
          GLint mMaxViewportDims[2];
          glGetIntegerv(GL_MAX_TEXTURE_SIZE, &mMaxTextureSize);
          ALOGD("%s max=%d",__func__, mMaxTextureSize);
          glGetIntegerv(GL_MAX_VIEWPORT_DIMS, mMaxViewportDims);
          ALOGD("%s max=%d,%d",__func__, mMaxViewportDims[0], mMaxViewportDims[1]);*/
    }


    if (isChanged) {
        mUpdateCnt++;
        if (mUpdateCnt > 3)
            isChanged = false;
        glClearColor(0, 0, 0, 1.0);
        glClear(GL_COLOR_BUFFER_BIT);
    }
    use_program();
    fbo = dst_buffer->getFramebuffer();

    // set destination
    engine_setDestination(fbo, 0, 0, dst_buffer->getWidth(),
                          dst_buffer->getHeight());

    tex = src_buffer->getTexture();
    // set source
    engine_set2DInputBuffer(0, tex);

    fenceFD = render_frame(tex, mXrad, mYrad, srcFenceFd);

    // restore the caller context
    engine_bind(caller_context);
    engine_free_backup(caller_context);

    return fenceFD;
}
