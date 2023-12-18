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

#ifndef __IMAGE_MAPPER_H__
#define __IMAGE_MAPPER_H__

#define KEY_STONE 1

#include "EGLImageWrapper.h"
#include "engine.h"
#include "KeystoneRender.h"

class ImageMapper {
private:
    void* engineContext;
    EGLImageWrapper* eglImageWrapper;
    float mXrad;
    float mYrad;
    int mWidth;
    int mHeight;
    int mUpdateCnt;
    bool isSecure;
    bool isChanged;

    ImageMapper();

public:
    ~ImageMapper();
    static ImageMapper *build(bool isSecure);
    int doRender(const void *dst, const void *src, int srcFenceFd);
    void setXRad(float rad);
    void setYRad(float rad);
    void initialize(int width, int height);
};

/*  int blit(const void *dst, const void *src, int srcFenceFd);
    void setXRad(float rad);
    void setYRad(float rad);*/

#endif  //~__IMAGE_MAPPER_H__
