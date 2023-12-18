package com.allwinner.camera.program;

import android.content.Context;
import android.util.Log;

import com.allwinner.camera.utils.EGLUtils;

import static android.opengl.GLES20.GL_TEXTURE1;
import static android.opengl.GLES20.GL_TEXTURE2;
import static android.opengl.GLES20.GL_TEXTURE_2D;
import static android.opengl.GLES20.glActiveTexture;
import static android.opengl.GLES20.glBindTexture;
import static android.opengl.GLES20.glGetUniformLocation;
import static android.opengl.GLES20.glUniform1i;

public class LutProgram extends BaseProgram {

    public static final String TEXTURE_LUT = "lut_tab";
    private int mLuttabLocation = -1;
    private int mLutTextureId = -1;

    private String fragmentShaderFor2DInput2DOut = "precision mediump float;\n" +
            "uniform sampler2D texture;\n" +
            "uniform sampler2D lut_tab;\n" +
            "varying vec2 vTextureCoord;\n" +
            "void main() {\n" +
            "  vec4 color = texture2D(texture, vTextureCoord);\n" +
            "  highp float blueColor = color.b * 63.0; \n" +//蓝色部分[0, 63] 共64种
            "  highp vec2 quad1; \n" + // 第一个正方形的位置, 假如blueColor=22.5，则y=22/8=2，x=22-8*2=6，
            "  quad1.y = floor(floor(blueColor) / 8.0); \n" +//y=22/8=2
            "  quad1.x = floor(blueColor) - (quad1.y * 8.0); \n" +//x=22-8*2=6 即是第3行，第6个正方形
            "  highp vec2 quad2; \n" +// 第一个正方形的位置, 假如blueColor=22.5，则y=23/8=2，x=23-8*2=7，
            "  quad2.y = floor(ceil(blueColor) / 8.0); \n" +
            "  quad2.x = ceil(blueColor) - (quad2.y * 8.0); \n" +
            "  highp vec2 texPos1; \n" +//计算颜色(r,b,g)在第一个正方形中对应位置
            "  texPos1.x = (quad1.x * 0.125) + 0.5/512.0 + ((0.125 - 1.0/512.0) * color.r); \n" +
            "  texPos1.y = (quad1.y * 0.125) + 0.5/512.0 + ((0.125 - 1.0/512.0) * color.g); \n" +
            "  highp vec2 texPos2; \n" +
            "  texPos2.x = (quad2.x * 0.125) + 0.5/512.0 + ((0.125 - 1.0/512.0) * color.r); \n" +
            "  texPos2.y = (quad2.y * 0.125) + 0.5/512.0 + ((0.125 - 1.0/512.0) * color.g); \n" +
            "  lowp vec4 newColor1 = texture2D(lut_tab, texPos1); \n" +// 正方形1的颜色值
            "  lowp vec4 newColor2 = texture2D(lut_tab, texPos2); \n" +// 正方形2的颜色值
            "  lowp vec4 newColor = mix(newColor1, newColor2, fract(blueColor)); \n" +// 根据小数点的部分进行mix
            "  newColor.r = clamp(newColor.r, 0.0, 1.0);\n" +
            "  newColor.g = clamp(newColor.g, 0.0, 1.0);\n" +
            "  newColor.b= clamp(newColor.b, 0.0, 1.0);\n" +
            "  gl_FragColor =newColor; \n" +
            "}\n";

    private String fragmentShaderForOESInput2DOut = "#extension GL_OES_EGL_image_external : require\n" +
            "precision mediump float;\n" +
            "uniform samplerExternalOES texture;\n" +
            "uniform sampler2D lut_tab;\n" +
            "varying vec2 vTextureCoord;\n" +
            "void main() {\n" +
            "  vec4 color = texture2D(texture, vTextureCoord);\n" +
            "  highp float blueColor = color.b * 63.0; \n" +//蓝色部分[0, 63] 共64种
            "  highp vec2 quad1; \n" + // 第一个正方形的位置, 假如blueColor=22.5，则y=22/8=2，x=22-8*2=6，
            "  quad1.y = floor(floor(blueColor) / 8.0); \n" +//y=22/8=2
            "  quad1.x = floor(blueColor) - (quad1.y * 8.0); \n" +//x=22-8*2=6 即是第3行，第6个正方形
            "  highp vec2 quad2; \n" +// 第一个正方形的位置, 假如blueColor=22.5，则y=23/8=2，x=23-8*2=7，
            "  quad2.y = floor(ceil(blueColor) / 8.0); \n" +
            "  quad2.x = ceil(blueColor) - (quad2.y * 8.0); \n" +
            "  highp vec2 texPos1; \n" +//计算颜色(r,b,g)在第一个正方形中对应位置
            "  texPos1.x = (quad1.x * 0.125) + 0.5/512.0 + ((0.125 - 1.0/512.0) * color.r); \n" +
            "  texPos1.y = (quad1.y * 0.125) + 0.5/512.0 + ((0.125 - 1.0/512.0) * color.g); \n" +
            "  highp vec2 texPos2; \n" +
            "  texPos2.x = (quad2.x * 0.125) + 0.5/512.0 + ((0.125 - 1.0/512.0) * color.r); \n" +
            "  texPos2.y = (quad2.y * 0.125) + 0.5/512.0 + ((0.125 - 1.0/512.0) * color.g); \n" +
            "  lowp vec4 newColor1 = texture2D(lut_tab, texPos1); \n" +// 正方形1的颜色值
            "  lowp vec4 newColor2 = texture2D(lut_tab, texPos2); \n" +// 正方形2的颜色值
            "  lowp vec4 newColor = mix(newColor1, newColor2, fract(blueColor)); \n" +// 根据小数点的部分进行mix
            "  newColor.r = clamp(newColor.r, 0.0, 1.0);\n" +
            "  newColor.g = clamp(newColor.g, 0.0, 1.0);\n" +
            "  newColor.b= clamp(newColor.b, 0.0, 1.0);\n" +
            "  gl_FragColor =newColor; \n" +
            "}\n";

    private String fragmentShaderForYUVInput2DOut =
            "#extension GL_OES_texture_3D : enable\n"
                    + "precision mediump float;\n"
                    + "uniform sampler2D textureY;\n"
                    + "uniform sampler2D textureUV;\n"
                    + "uniform sampler2D lut_tab;\n"
                    + "varying vec2 vTextureCoord;\n"
                    + "void main() \n"
                    + "{\n"
                    + "  float r, g, b, y, u, v;\n"
                    + "  float y2, u2, v2;\n"
                    + "  y = texture2D(textureY, vTextureCoord).r;\n"
                    + "  u = texture2D(textureUV, vTextureCoord).a - 0.5;\n"
                    + "  v = texture2D(textureUV, vTextureCoord).r - 0.5;\n"
                    + "  r = y + 1.402*v;\n"
                    + "  g = y - 0.34414*u - 0.71414*v;\n"
                    + "  b = y + 1.772*u;\n"
                    + "  r = clamp(r, 0.0, 1.0);\n"
                    + "  g = clamp(g, 0.0, 1.0);\n"
                    + "  b = clamp(b, 0.0, 1.0);\n"
                    + "  vec4 color = vec4(r, g, b, 1.0);\n"
                    + "  highp float blueColor = color.b * 63.0; \n"
                    + "  highp vec2 quad1; \n"
                    + "  quad1.y = floor(floor(blueColor) / 8.0); \n"
                    + "  quad1.x = floor(blueColor) - (quad1.y * 8.0); \n"
                    + "  highp vec2 quad2; \n"
                    + "  quad2.y = floor(ceil(blueColor) / 8.0); \n"
                    + "  quad2.x = ceil(blueColor) - (quad2.y * 8.0); \n"
                    + "  highp vec2 texPos1; \n"
                    + "  texPos1.x = (quad1.x * 0.125) + 0.5/512.0 + ((0.125 - 1.0/512.0) * color.r); \n"
                    + "  texPos1.y = (quad1.y * 0.125) + 0.5/512.0 + ((0.125 - 1.0/512.0) * color.g); \n"
                    + "  highp vec2 texPos2; \n"
                    + "  texPos2.x = (quad2.x * 0.125) + 0.5/512.0 + ((0.125 - 1.0/512.0) * color.r); \n"
                    + "  texPos2.y = (quad2.y * 0.125) + 0.5/512.0 + ((0.125 - 1.0/512.0) * color.g); \n"
                    + "  lowp vec4 newColor1 = texture2D(lut_tab, texPos1); \n"
                    + "  lowp vec4 newColor2 = texture2D(lut_tab, texPos2); \n"
                    + "  lowp vec4 newColor = mix(newColor1, newColor2, fract(blueColor)); \n"
                        /*            + "  gl_FragColor = mix(color, vec4(newColor.rgb, color.w), 1.0); \n"
                        + "  vec4 mixcolor = mix(color, vec4(newColor.rgb, color.w), 1.0); \n"*/
                    + "  newColor.r = clamp(newColor.r, 0.0, 1.0);\n"
                    + "  newColor.g = clamp(newColor.g, 0.0, 1.0);\n"
                    + "  newColor.b= clamp(newColor.b, 0.0, 1.0);\n"
                    + "  gl_FragColor =newColor; \n"
                    + "}\n";

    private String fragmentShaderForYUVInputYuvOut =
            "#extension GL_OES_texture_3D : enable\n"
                    + "precision mediump float;\n"
                    + "uniform sampler2D textureY;\n"
                    + "uniform sampler2D textureUV;\n"
                    + "uniform sampler2D lut_tab;\n"
                    + "varying vec2 vTextureCoord;\n"
                    + "void main() \n"
                    + "{\n"
                    + "  float r, g, b, y, u, v;\n"
                    + "  float y2, u2, v2;\n"
                    + "  y = texture2D(textureY, vTextureCoord).r;\n"
                    + "  u = texture2D(textureUV, vTextureCoord).a - 0.5;\n"
                    + "  v = texture2D(textureUV, vTextureCoord).r - 0.5;\n"
                    + "  r = y + 1.402*v;\n"
                    + "  g = y - 0.34414*u - 0.71414*v;\n"
                    + "  b = y + 1.772*u;\n"
                    + "  r = clamp(r, 0.0, 1.0);\n"
                    + "  g = clamp(g, 0.0, 1.0);\n"
                    + "  b = clamp(b, 0.0, 1.0);\n"
                    + "  vec4 color = vec4(r, g, b, 1.0);\n"
                    + "  highp float blueColor = color.b * 63.0; \n"
                    + "  highp vec2 quad1; \n"
                    + "  quad1.y = floor(floor(blueColor) / 8.0); \n"
                    + "  quad1.x = floor(blueColor) - (quad1.y * 8.0); \n"
                    + "  highp vec2 quad2; \n"
                    + "  quad2.y = floor(ceil(blueColor) / 8.0); \n"
                    + "  quad2.x = ceil(blueColor) - (quad2.y * 8.0); \n"
                    + "  highp vec2 texPos1; \n"
                    + "  texPos1.x = (quad1.x * 0.125) + 0.5/512.0 + ((0.125 - 1.0/512.0) * color.r); \n"
                    + "  texPos1.y = (quad1.y * 0.125) + 0.5/512.0 + ((0.125 - 1.0/512.0) * color.g); \n"
                    + "  highp vec2 texPos2; \n"
                    + "  texPos2.x = (quad2.x * 0.125) + 0.5/512.0 + ((0.125 - 1.0/512.0) * color.r); \n"
                    + "  texPos2.y = (quad2.y * 0.125) + 0.5/512.0 + ((0.125 - 1.0/512.0) * color.g); \n"
                    + "  lowp vec4 newColor1 = texture2D(lut_tab, texPos1); \n"
                    + "  lowp vec4 newColor2 = texture2D(lut_tab, texPos2); \n"
                    + "  lowp vec4 newColor = mix(newColor1, newColor2, fract(blueColor)); \n"
                        /*            + "  gl_FragColor = mix(color, vec4(newColor.rgb, color.w), 1.0); \n"
                        + "  vec4 mixcolor = mix(color, vec4(newColor.rgb, color.w), 1.0); \n"*/
                    + "  newColor.r = clamp(newColor.r, 0.0, 1.0);\n"
                    + "  newColor.g = clamp(newColor.g, 0.0, 1.0);\n"
                    + "  newColor.b= clamp(newColor.b, 0.0, 1.0);\n"
                    + "  y2 = 0.299*newColor.r + 0.587*newColor.g + 0.114*newColor.b;\n"
                    + "  u2 = - 0.1687*newColor.r - 0.3313*newColor.g + 0.5*newColor.b+0.5;\n"
                    + "  v2 = 0.5*newColor.r - 0.4187*newColor.g - 0.0813*newColor.b+0.5;\n"
                    + "  gl_FragColor =vec4(y2,u2,v2,1.0);\n"
                    + "}\n";

    private String fragmentShaderFor2DInputYuvOut =
            "#extension GL_OES_texture_3D : enable\n"
                    + "precision mediump float;\n"
                    +"uniform sampler2D texture;\n"
                    + "uniform sampler2D lut_tab;\n"
                    + "varying vec2 vTextureCoord;\n"
                    + "void main() \n"
                    + "{\n"
                    + "  float r, g, b, y, u, v;\n"
                    + "  float y2, u2, v2;\n"
                    + "  vec4 color = texture2D(texture, vTextureCoord);\n"
                    + "  highp float blueColor = color.b * 63.0; \n"
                    + "  highp vec2 quad1; \n"
                    + "  quad1.y = floor(floor(blueColor) / 8.0); \n"
                    + "  quad1.x = floor(blueColor) - (quad1.y * 8.0); \n"
                    + "  highp vec2 quad2; \n"
                    + "  quad2.y = floor(ceil(blueColor) / 8.0); \n"
                    + "  quad2.x = ceil(blueColor) - (quad2.y * 8.0); \n"
                    + "  highp vec2 texPos1; \n"
                    + "  texPos1.x = (quad1.x * 0.125) + 0.5/512.0 + ((0.125 - 1.0/512.0) * color.r); \n"
                    + "  texPos1.y = (quad1.y * 0.125) + 0.5/512.0 + ((0.125 - 1.0/512.0) * color.g); \n"
                    + "  highp vec2 texPos2; \n"
                    + "  texPos2.x = (quad2.x * 0.125) + 0.5/512.0 + ((0.125 - 1.0/512.0) * color.r); \n"
                    + "  texPos2.y = (quad2.y * 0.125) + 0.5/512.0 + ((0.125 - 1.0/512.0) * color.g); \n"
                    + "  lowp vec4 newColor1 = texture2D(lut_tab, texPos1); \n"
                    + "  lowp vec4 newColor2 = texture2D(lut_tab, texPos2); \n"
                    + "  lowp vec4 newColor = mix(newColor1, newColor2, fract(blueColor)); \n"
                        /*            + "  gl_FragColor = mix(color, vec4(newColor.rgb, color.w), 1.0); \n"
                        + "  vec4 mixcolor = mix(color, vec4(newColor.rgb, color.w), 1.0); \n"*/
                    + "  newColor.r = clamp(newColor.r, 0.0, 1.0);\n"
                    + "  newColor.g = clamp(newColor.g, 0.0, 1.0);\n"
                    + "  newColor.b= clamp(newColor.b, 0.0, 1.0);\n"
                    + "  y2 = 0.299*newColor.r + 0.587*newColor.g + 0.114*newColor.b;\n"
                    + "  u2 = - 0.1687*newColor.r - 0.3313*newColor.g + 0.5*newColor.b+0.5;\n"
                    + "  v2 = 0.5*newColor.r - 0.4187*newColor.g - 0.0813*newColor.b+0.5;\n"
                    + "  gl_FragColor =vec4(y2,u2,v2,1.0);\n"
                    + "}\n";

    private String fragmentShaderForOesInputYuvOut =
            "#extension GL_OES_EGL_image_external : require\n" +
            "#extension GL_OES_texture_3D : enable\n"
                    + "precision mediump float;\n"
                    +"uniform samplerExternalOES texture;\n"
                    + "uniform sampler2D lut_tab;\n"
                    + "varying vec2 vTextureCoord;\n"
                    + "void main() \n"
                    + "{\n"
                    + "  float r, g, b, y, u, v;\n"
                    + "  float y2, u2, v2;\n"
                    + "  vec4 color = texture2D(texture, vTextureCoord);\n"
                    + "  highp float blueColor = color.b * 63.0; \n"
                    + "  highp vec2 quad1; \n"
                    + "  quad1.y = floor(floor(blueColor) / 8.0); \n"
                    + "  quad1.x = floor(blueColor) - (quad1.y * 8.0); \n"
                    + "  highp vec2 quad2; \n"
                    + "  quad2.y = floor(ceil(blueColor) / 8.0); \n"
                    + "  quad2.x = ceil(blueColor) - (quad2.y * 8.0); \n"
                    + "  highp vec2 texPos1; \n"
                    + "  texPos1.x = (quad1.x * 0.125) + 0.5/512.0 + ((0.125 - 1.0/512.0) * color.r); \n"
                    + "  texPos1.y = (quad1.y * 0.125) + 0.5/512.0 + ((0.125 - 1.0/512.0) * color.g); \n"
                    + "  highp vec2 texPos2; \n"
                    + "  texPos2.x = (quad2.x * 0.125) + 0.5/512.0 + ((0.125 - 1.0/512.0) * color.r); \n"
                    + "  texPos2.y = (quad2.y * 0.125) + 0.5/512.0 + ((0.125 - 1.0/512.0) * color.g); \n"
                    + "  lowp vec4 newColor1 = texture2D(lut_tab, texPos1); \n"
                    + "  lowp vec4 newColor2 = texture2D(lut_tab, texPos2); \n"
                    + "  lowp vec4 newColor = mix(newColor1, newColor2, fract(blueColor)); \n"
                        /*            + "  gl_FragColor = mix(color, vec4(newColor.rgb, color.w), 1.0); \n"
                        + "  vec4 mixcolor = mix(color, vec4(newColor.rgb, color.w), 1.0); \n"*/
                    + "  newColor.r = clamp(newColor.r, 0.0, 1.0);\n"
                    + "  newColor.g = clamp(newColor.g, 0.0, 1.0);\n"
                    + "  newColor.b= clamp(newColor.b, 0.0, 1.0);\n"
                    + "  y2 = 0.299*newColor.r + 0.587*newColor.g + 0.114*newColor.b;\n"
                    + "  u2 = - 0.1687*newColor.r - 0.3313*newColor.g + 0.5*newColor.b+0.5;\n"
                    + "  v2 = 0.5*newColor.r - 0.4187*newColor.g - 0.0813*newColor.b+0.5;\n"
                    + "  gl_FragColor =vec4(y2,u2,v2,1.0);\n"
                    + "}\n";


    public LutProgram(EGLUtils.TEXTURETYPE inputType, EGLUtils.TEXTURETYPE outputType) {
        super(inputType, outputType);
        Log.i("LutPreviewProgram", "inputType: " + inputType);
    }

    @Override
    protected String getFragmentShader(Context context) {
        if (mOutputType == EGLUtils.TEXTURETYPE.YUV) {
            if (mInputType == EGLUtils.TEXTURETYPE.OESTEXTURE) {
                return fragmentShaderForOesInputYuvOut;
            } else if (mInputType == EGLUtils.TEXTURETYPE.TEXTURE2D) {
                return fragmentShaderFor2DInputYuvOut;
            } else {
                return fragmentShaderForYUVInputYuvOut;
            }
        } else {
            if (mInputType == EGLUtils.TEXTURETYPE.OESTEXTURE) {
                return fragmentShaderForOESInput2DOut;
            } else if (mInputType == EGLUtils.TEXTURETYPE.TEXTURE2D) {
                return fragmentShaderFor2DInput2DOut;
            } else {
                return fragmentShaderForYUVInput2DOut;
            }
        }
    }

    @Override
    protected void configParameter(int program) {
        super.configParameter(program);
        if (mInputType == EGLUtils.TEXTURETYPE.OESTEXTURE) {
            if (mLutTextureId != -1) {
                glActiveTexture(GL_TEXTURE1);
                glBindTexture(GL_TEXTURE_2D, mLutTextureId);
                glUniform1i(mLuttabLocation, 1);
            }
        } else if (mInputType == EGLUtils.TEXTURETYPE.TEXTURE2D) {
            if (mLutTextureId != -1) {
                glActiveTexture(GL_TEXTURE1);
                glBindTexture(GL_TEXTURE_2D, mLutTextureId);
                glUniform1i(mLuttabLocation, 1);
            }
        } else {
            if (mLutTextureId != -1) {
                glActiveTexture(GL_TEXTURE2);
                glBindTexture(GL_TEXTURE_2D, mLutTextureId);
                glUniform1i(mLuttabLocation, 2);
            }
        }
    }

    public void setLutTextureId(int textureId) {
        mLutTextureId = textureId;
    }

    @Override
    protected void getProgramParms() {
        super.getProgramParms();
        mLuttabLocation = glGetUniformLocation(getProgram(),TEXTURE_LUT);
    }
}
