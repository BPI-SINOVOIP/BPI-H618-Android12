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

#define ATRACE_TAG ATRACE_TAG_GRAPHICS

#include "KeystoneRender.h"
#include "engine.h"

using namespace android;

GLuint gOnScreenProgram;
GLuint gOnScreenPositionHandle;
GLuint gOnScreenTextureHandle;
GLuint gOnScreenSamplerHandle;
GLuint gOnScreenTexture;
GLuint gOnScreenRotateXHandle;
GLuint gOnScreenRotateYHandle;
GLuint gOnScreenProjectionHandle;

/*
 * shader pair for draw fbo's object to screen
 */
static const char *vs_onscreen_source =
"attribute vec4 aOnScreenPosition;                       \n"
"attribute vec2 aOnScreenTexCoord;                       \n"
"varying vec2 vOnScreenTexCoord;                         \n"
"uniform mat4 uRotateX;                                   \n"
"uniform mat4 uRotateY;                                   \n"
"uniform mat4 uProjection;                               \n"
"void main()                                             \n"
"{                                                       \n"
"       vOnScreenTexCoord = aOnScreenTexCoord;               \n"
"       gl_Position = uProjection * uRotateY * uRotateX * aOnScreenPosition;\n"
"}                                                       \n";
static const char *fs_onscreen_source =
"#extension GL_OES_EGL_image_external : require          \n"
"precision mediump float;                                \n"
"uniform samplerExternalOES uOnScreenTexSampler;         \n"
"varying vec2 vOnScreenTexCoord;                                 \n"
"void main()                                             \n"
"{                                                       \n"
"       gl_FragColor = texture2D(uOnScreenTexSampler, vOnScreenTexCoord);    \n"
"}                                                      \n";

const GLfloat gOnScreenVerticesData[] = {
    // X, Y, Z, U, V
    -1.0f, -1.0f, 0.5,
    1.0f, -1.0f, 0.5,
    -1.0f,  1.0f, 0.5,
    1.0f,  1.0f, 0.5,
};

const GLfloat gOnScreenTextureData[] = {
    0.f, 0.f,
    1.f, 0.f,
    0.f, 1.f,
    1.f, 1.f,
};


static GLfloat vRotateDefault[] = {
    1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f,
};

static GLfloat vProjection[] = {
    0.5f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.5f, 0.0f, 0.0f,
    0.0f, 0.0f,  -3.0f, 1.0f,
    0.0f, 0.0f,  2.0f, 0.0f,
};

GLfloat vRotateX[] = {
    1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f,
};

GLfloat vRotateY[] = {
    1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f,
};

static void checkGlError(const char* op) {
    for (GLint error = glGetError(); error; error
         = glGetError()) {
        ALOGD("after %s() glError (0x%x)\n", op, error);
    }
}


GLuint loadShader(GLenum shaderType, const char* pSource) {
    GLuint shader = glCreateShader(shaderType);
    if (shader) {
        glShaderSource(shader, 1, &pSource, NULL);
        glCompileShader(shader);
        GLint compiled = 0;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
        if (!compiled) {
            GLint infoLen = 0;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
            if (infoLen) {
                char* buf = (char*) malloc(infoLen);
                if (buf) {
                    glGetShaderInfoLog(shader, infoLen, NULL, buf);
                    ALOGD("Could not compile shader %d:\n%s\n",
                          shaderType, buf);
                    free(buf);
                }
            } else {
                ALOGD("Guessing at GL_INFO_LOG_LENGTH size\n");
                char* buf = (char*) malloc(0x1000);
                if (buf) {
                    glGetShaderInfoLog(shader, 0x1000, NULL, buf);
                    ALOGD("Could not compile shader %d:\n%s\n",
                          shaderType, buf);
                    free(buf);
                }
            }
            glDeleteShader(shader);
            shader = 0;
        }
    }
    return shader;
}

GLuint createProgram(const char* pVertexSource, const char* pFragmentSource) {
    GLuint vertexShader = loadShader(GL_VERTEX_SHADER, pVertexSource);
    if (!vertexShader) {
        return 0;
    }

    GLuint pixelShader = loadShader(GL_FRAGMENT_SHADER, pFragmentSource);
    if (!pixelShader) {
        return 0;
    }

    GLuint program = glCreateProgram();
    if (program) {
        glAttachShader(program, vertexShader);
        checkGlError("glAttachShader");
        glAttachShader(program, pixelShader);
        checkGlError("glAttachShader");
        glLinkProgram(program);
        GLint linkStatus = GL_FALSE;
        glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
        if (linkStatus != GL_TRUE) {
            GLint bufLength = 0;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &bufLength);
            if (bufLength) {
                char* buf = (char*) malloc(bufLength);
                if (buf) {
                    glGetProgramInfoLog(program, bufLength, NULL, buf);
                    ALOGD("Could not link program:\n%s\n", buf);
                    free(buf);
                }
            }
            glDeleteProgram(program);
            program = 0;
        }
    }
    return program;
}

int setupGraphics(int left, int top, int right, int bottom)
{
    ALOGD("%s,right=%d,bottom=%d\n", __func__, right, bottom);
    if (gOnScreenProgram) {
        ALOGI("already setup\n");
        return true;
    }
    gOnScreenProgram = createProgram(vs_onscreen_source, fs_onscreen_source);
    if (!gOnScreenProgram) {
        ALOGD("cannot create onscreen program\n");
        return false;
    }

    gOnScreenPositionHandle = glGetAttribLocation(gOnScreenProgram, "aOnScreenPosition");
    checkGlError("glGetAttribLocation");

    gOnScreenTextureHandle = glGetAttribLocation(gOnScreenProgram, "aOnScreenTexCoord");
    checkGlError("glGetAttribLocation");

    gOnScreenRotateXHandle = glGetUniformLocation(gOnScreenProgram, "uRotateX");
    checkGlError("glGetUniformLocation");

    gOnScreenRotateYHandle = glGetUniformLocation(gOnScreenProgram, "uRotateY");
    checkGlError("glGetUniformLocation");

    gOnScreenProjectionHandle = glGetUniformLocation(gOnScreenProgram, "uProjection");
    checkGlError("glGetUniformLocation");

    gOnScreenSamplerHandle = glGetUniformLocation(gOnScreenProgram, "uOnScreenTexSampler");
    checkGlError("glGetUniformLocation");
    ALOGD("glGetUniformLocation(\"uOnScreenTexSampler\") = %d\n", gOnScreenSamplerHandle);

    /*glViewport(left, top, right, bottom);
      checkGlError("glViewport");*/
    ALOGD("setup %d,%d,%d,%d\n", left, top, right, bottom);

    return true;
}

static void genetate_rotate_matrix(GLfloat x_rad, GLfloat y_rad)
{
    unsigned int i;
    /* generate rotate x-axis */
    for (i = 0; i < 16; i++) {
        if (i == 5)
            vRotateX[i] = cos(x_rad);
        else if (i == 6)
            vRotateX[i] = sin(x_rad);
        else if (i == 9)
            vRotateX[i] = -sin(x_rad);
        else if (i == 10)
            vRotateX[i] = cos(x_rad);
        else if (i == 14)
            vRotateX[i] = fabs(sin(x_rad));
        else
            vRotateX[i] = vRotateDefault[i];
    }

    /* generate rotate y-axis */
    for (i = 0; i < 16; i++) {
        if (i == 0)
            vRotateY[i] = cos(y_rad);
        else if (i == 2)
            vRotateY[i] = -sin(y_rad);
        else if (i == 8)
            vRotateY[i] = sin(y_rad);
        else if (i == 10)
            vRotateY[i] = cos(y_rad);
        else if (i == 14)
            vRotateY[i] = fabs(sin(y_rad));
        else
            vRotateY[i] = vRotateDefault[i];
    }
}

void use_program() {
    glUseProgram(gOnScreenProgram);
    checkGlError("glUseProgram");
    glUniform1i(gOnScreenSamplerHandle, 0);
    checkGlError("glUniform1i");
}

int render_frame(GLuint texture, float x_rad, float y_rad, int srcFence)
{

#if 0
    FILE *fp;
    GLubyte *pPixelData= (GLubyte *)malloc(1920*1080*4);

    glReadPixels(0, 0, 1920, 1080, GL_RGBA, GL_UNSIGNED_BYTE, pPixelData);

    fp = fopen("/data/read_from_buffer.bin", "w");
    if (fp != NULL) {
        fwrite(pPixelData, 1, 1920*1080*4, fp);
        sync();
        fclose(fp);
    } else {
        if (pPixelData != NULL)
            free(pPixelData);
        return false;
    }
    if (pPixelData != NULL)
        free(pPixelData);
#endif
    int fd = -1;

    /*setupGraphics();*/
    /*
       glClearColor(0.0, 0.0, 0.0, 1.0);
       checkGlError("glClearColor");

       glBindFramebuffer(GL_FRAMEBUFFER, 0);
       checkGlError("glBindFramebuffer");*/
    //glUseProgram(gOnScreenProgram);
    //checkGlError("glUseProgram");

    /*glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_EXTERNAL_OES, texture);
      checkGlError("glBindTexture");*/

    //glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + 1,
    //GL_TEXTURE_2D, texture, 0);
    //checkGlError("glFramebufferTexture2D");
    if (srcFence >= 0)
        WaitOnNativeFence(srcFence);
    ALOGV("do render texture (0x%x), %d\n", texture, srcFence);
    glVertexAttribPointer(gOnScreenPositionHandle, 3, GL_FLOAT, GL_FALSE,
                          0, &gOnScreenVerticesData[0]);
    checkGlError("glVertexAttribPointer");
    glEnableVertexAttribArray(gOnScreenPositionHandle);

    glVertexAttribPointer(gOnScreenTextureHandle, 2, GL_FLOAT, GL_FALSE,
                          0, &gOnScreenTextureData[0]);
    checkGlError("glVertexAttribPointer");
    glEnableVertexAttribArray(gOnScreenTextureHandle);

    ALOGV("do render texture  xrad=%f,%f", x_rad, y_rad);
    genetate_rotate_matrix(x_rad, y_rad);

    glUniformMatrix4fv(gOnScreenRotateXHandle, 1, GL_FALSE, (GLfloat *)vRotateX);
    glUniformMatrix4fv(gOnScreenRotateYHandle, 1, GL_FALSE, (GLfloat *)vRotateY);
    glUniformMatrix4fv(gOnScreenProjectionHandle, 1, GL_FALSE, (GLfloat *)vProjection);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    //glBindFramebuffer(GL_FRAMEBUFFER, 0);

    /*fd = CreateNativeFence();
      glFlush();
      checkGlError("glFlush");*/
    glFinish();
#if 0
    FILE *fp_1;
    GLubyte *pPixelData_1= (GLubyte *)malloc(1280*800*4);

    glReadPixels(0, 0, 1280, 800, GL_RGBA, GL_UNSIGNED_BYTE, pPixelData_1);

    fp_1 = fopen("/data/read_from_buffer_1.bin", "w");
    if (fp_1 != NULL) {
        fwrite(pPixelData_1, 1, 1280*800*4, fp_1);
        sync();
        fclose(fp_1);
    } else {
        ALOGD(": open read_from_buffer_1.bin failed\n");
        if (pPixelData_1 != NULL)
            free(pPixelData_1);
        return false;
    }
    if (pPixelData_1 != NULL)
        free(pPixelData_1);
#endif

    return fd;
}

