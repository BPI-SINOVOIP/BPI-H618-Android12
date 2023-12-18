/*
 * Copyright 2013 The Android Open Source Project
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


#ifndef KEYSTONE_RENDER_H_
#define KEYSTONE_RENDER_H_

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <sched.h>
#include <sys/resource.h>

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <utils/Timers.h>

#include <ui/Rect.h>

#include <utils/String8.h>
#include <utils/Trace.h>

#include <cutils/compiler.h>
#include <math.h>

using namespace android;

int render_frame(GLuint texture, float x_rad, float y_rad, int srcFence);
void use_program();
int setupGraphics(int left, int top, int right, int bottom);

#endif /* KEYSTONE_RENDER_H_ */
