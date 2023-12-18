package com.allwinner.camera.program;

import android.content.Context;
import android.opengl.GLES20;

import com.allwinner.camera.data.CameraData;
import com.allwinner.camera.data.Contants;
import com.allwinner.camera.utils.CameraUtils;
import com.allwinner.camera.utils.EGLUtils;

import static android.opengl.GLES20.glGetUniformLocation;

public class SmoothProgram extends BaseProgram {

    public static final String SINGLESTEPOFFSET = "singleStepOffset";
    public static final String PARAMS = "params";
    public static final String LEVEL = "level";

    private int mSingleLocation;
    private int mParamsLocation;
    private int mLevelLocation;

    public SmoothProgram(EGLUtils.TEXTURETYPE inputType, EGLUtils.TEXTURETYPE outputType) {
        super(inputType, outputType);
    }


    private String fragmentShaderFor2DInput2DOut = "precision mediump float;\n" +
            "\n" +
            "varying mediump vec2 vTextureCoord;\n" +
            "\n" +
            "uniform sampler2D texture;\n" +
            "uniform vec2 singleStepOffset;\n" +
            "uniform mediump float params;\n" +
            "uniform float level;\n" +
            "\n" +
            "const mediump vec3 W = vec3(0.299,0.587,0.114);\n" +
            "vec2 blurCoordinates[20];\n" +
            "\n" +
            "float hardLight(float color)\n" +
            "{\n" +
            "    if(color <= 0.5)\n" +
            "        color = color * color * 2.0;\n" +
            "    else\n" +
            "        color = 1.0 - ((1.0 - color)*(1.0 - color) * 2.0);\n" +
            "    return color;\n" +
            "}\n" +
            "\n" +
            "void modifyColor(vec4 color){\n" +
            "color.r=max(min(color.r,1.0),0.0);\n" +
            "color.g=max(min(color.g,1.0),0.0);\n" +
            "color.b=max(min(color.b,1.0),0.0);\n" +
            "color.a=max(min(color.a,1.0),0.0);\n" +
            " }\n" +
            "\n " +
            "void main(){\n" +
            "\n" +
            "    vec3 centralColor = texture2D(texture, vTextureCoord).rgb;\n" +
            "    blurCoordinates[0] = vTextureCoord.xy + singleStepOffset * vec2(0.0, -10.0);\n" +
            "    blurCoordinates[1] = vTextureCoord.xy + singleStepOffset * vec2(0.0, 10.0);\n" +
            "    blurCoordinates[2] = vTextureCoord.xy + singleStepOffset * vec2(-10.0, 0.0);\n" +
            "    blurCoordinates[3] = vTextureCoord.xy + singleStepOffset * vec2(10.0, 0.0);\n" +
            "    blurCoordinates[4] = vTextureCoord.xy + singleStepOffset * vec2(5.0, -8.0);\n" +
            "    blurCoordinates[5] = vTextureCoord.xy + singleStepOffset * vec2(5.0, 8.0);\n" +
            "    blurCoordinates[6] = vTextureCoord.xy + singleStepOffset * vec2(-5.0, 8.0);\n" +
            "    blurCoordinates[7] = vTextureCoord.xy + singleStepOffset * vec2(-5.0, -8.0);\n" +
            "    blurCoordinates[8] = vTextureCoord.xy + singleStepOffset * vec2(8.0, -5.0);\n" +
            "    blurCoordinates[9] = vTextureCoord.xy + singleStepOffset * vec2(8.0, 5.0);\n" +
            "    blurCoordinates[10] = vTextureCoord.xy + singleStepOffset * vec2(-8.0, 5.0);\n" +
            "    blurCoordinates[11] = vTextureCoord.xy + singleStepOffset * vec2(-8.0, -5.0);\n" +
            "    blurCoordinates[12] = vTextureCoord.xy + singleStepOffset * vec2(0.0, -6.0);\n" +
            "    blurCoordinates[13] = vTextureCoord.xy + singleStepOffset * vec2(0.0, 6.0);\n" +
            "    blurCoordinates[14] = vTextureCoord.xy + singleStepOffset * vec2(6.0, 0.0);\n" +
            "    blurCoordinates[15] = vTextureCoord.xy + singleStepOffset * vec2(-6.0, 0.0);\n" +
            "    blurCoordinates[16] = vTextureCoord.xy + singleStepOffset * vec2(-4.0, -4.0);\n" +
            "    blurCoordinates[17] = vTextureCoord.xy + singleStepOffset * vec2(-4.0, 4.0);\n" +
            "    blurCoordinates[18] = vTextureCoord.xy + singleStepOffset * vec2(4.0, -4.0);\n" +
            "    blurCoordinates[19] = vTextureCoord.xy + singleStepOffset * vec2(4.0, 4.0);\n" +
            "\n" +
            "    float sampleColor = centralColor.g * 20.0;\n" +
            "    sampleColor += texture2D(texture, blurCoordinates[0]).g;\n" +
            "    sampleColor += texture2D(texture, blurCoordinates[1]).g;\n" +
            "    sampleColor += texture2D(texture, blurCoordinates[2]).g;\n" +
            "    sampleColor += texture2D(texture, blurCoordinates[3]).g;\n" +
            "    sampleColor += texture2D(texture, blurCoordinates[4]).g;\n" +
            "    sampleColor += texture2D(texture, blurCoordinates[5]).g;\n" +
            "    sampleColor += texture2D(texture, blurCoordinates[6]).g;\n" +
            "    sampleColor += texture2D(texture, blurCoordinates[7]).g;\n" +
            "    sampleColor += texture2D(texture, blurCoordinates[8]).g;\n" +
            "    sampleColor += texture2D(texture, blurCoordinates[9]).g;\n" +
            "    sampleColor += texture2D(texture, blurCoordinates[10]).g;\n" +
            "    sampleColor += texture2D(texture, blurCoordinates[11]).g;\n" +
            "    sampleColor += texture2D(texture, blurCoordinates[12]).g * 2.0;\n" +
            "    sampleColor += texture2D(texture, blurCoordinates[13]).g * 2.0;\n" +
            "    sampleColor += texture2D(texture, blurCoordinates[14]).g * 2.0;\n" +
            "    sampleColor += texture2D(texture, blurCoordinates[15]).g * 2.0;\n" +
            "    sampleColor += texture2D(texture, blurCoordinates[16]).g * 2.0;\n" +
            "    sampleColor += texture2D(texture, blurCoordinates[17]).g * 2.0;\n" +
            "    sampleColor += texture2D(texture, blurCoordinates[18]).g * 2.0;\n" +
            "    sampleColor += texture2D(texture, blurCoordinates[19]).g * 2.0;\n" +
            "\n" +
            "    sampleColor = sampleColor / 48.0;\n" +
            "\n" +
            "    float highPass = centralColor.g - sampleColor + 0.5;\n" +
            "\n" +
            "    for(int i = 0; i < 5;i++)\n" +
            "    {\n" +
            "        highPass = hardLight(highPass);\n" +
            "    }\n" +
            "    float luminance = dot(centralColor, W);\n" +
            "\n" +
            "    float alpha = pow(luminance, params);\n" +
            "\n" +
            "    vec3 smoothColor = centralColor + (centralColor-vec3(highPass))*alpha*0.1;\n" +
            "\n" +
            "    vec4 nColor = vec4(mix(smoothColor.rgb, max(smoothColor, centralColor), alpha), 1.0);\n" +
            "vec4 deltaColor = nColor+vec4(vec3(level * 0.25),0.0);\n" +
            "modifyColor(deltaColor);\n" +
            "gl_FragColor = deltaColor;\n" +
            "}\n";

    private String fragmentShaderForOESInput2DOut =
            "#extension GL_OES_EGL_image_external : require\n" +
                    "precision mediump float;\n" +
                    "\n" +
                    "varying mediump vec2 vTextureCoord;\n" +
                    "\n" +
                    "uniform samplerExternalOES texture;\n" +
                    "uniform vec2 singleStepOffset;\n" +
                    "uniform mediump float params;\n" +
                    "uniform float level;\n" +
                    "\n" +
                    "const mediump vec3 W = vec3(0.299,0.587,0.114);\n" +
                    "vec2 blurCoordinates[20];\n" +
                    "\n" +
                    "float hardLight(float color)\n" +
                    "{\n" +
                    "    if(color <= 0.5)\n" +
                    "        color = color * color * 2.0;\n" +
                    "    else\n" +
                    "        color = 1.0 - ((1.0 - color)*(1.0 - color) * 2.0);\n" +
                    "    return color;\n" +
                    "}\n" +
                    "\n" +
                    "void modifyColor(vec4 color){\n" +
                    "color.r=max(min(color.r,1.0),0.0);\n" +
                    "color.g=max(min(color.g,1.0),0.0);\n" +
                    "color.b=max(min(color.b,1.0),0.0);\n" +
                    "color.a=max(min(color.a,1.0),0.0);\n" +
                    " }\n" +
                    "\n " +
                    "void main(){\n" +
                    "\n" +
                    "    vec3 centralColor = texture2D(texture, vTextureCoord).rgb;\n" +
                    "    blurCoordinates[0] = vTextureCoord.xy + singleStepOffset * vec2(0.0, -10.0);\n" +
                    "    blurCoordinates[1] = vTextureCoord.xy + singleStepOffset * vec2(0.0, 10.0);\n" +
                    "    blurCoordinates[2] = vTextureCoord.xy + singleStepOffset * vec2(-10.0, 0.0);\n" +
                    "    blurCoordinates[3] = vTextureCoord.xy + singleStepOffset * vec2(10.0, 0.0);\n" +
                    "    blurCoordinates[4] = vTextureCoord.xy + singleStepOffset * vec2(5.0, -8.0);\n" +
                    "    blurCoordinates[5] = vTextureCoord.xy + singleStepOffset * vec2(5.0, 8.0);\n" +
                    "    blurCoordinates[6] = vTextureCoord.xy + singleStepOffset * vec2(-5.0, 8.0);\n" +
                    "    blurCoordinates[7] = vTextureCoord.xy + singleStepOffset * vec2(-5.0, -8.0);\n" +
                    "    blurCoordinates[8] = vTextureCoord.xy + singleStepOffset * vec2(8.0, -5.0);\n" +
                    "    blurCoordinates[9] = vTextureCoord.xy + singleStepOffset * vec2(8.0, 5.0);\n" +
                    "    blurCoordinates[10] = vTextureCoord.xy + singleStepOffset * vec2(-8.0, 5.0);\n" +
                    "    blurCoordinates[11] = vTextureCoord.xy + singleStepOffset * vec2(-8.0, -5.0);\n" +
                    "    blurCoordinates[12] = vTextureCoord.xy + singleStepOffset * vec2(0.0, -6.0);\n" +
                    "    blurCoordinates[13] = vTextureCoord.xy + singleStepOffset * vec2(0.0, 6.0);\n" +
                    "    blurCoordinates[14] = vTextureCoord.xy + singleStepOffset * vec2(6.0, 0.0);\n" +
                    "    blurCoordinates[15] = vTextureCoord.xy + singleStepOffset * vec2(-6.0, 0.0);\n" +
                    "    blurCoordinates[16] = vTextureCoord.xy + singleStepOffset * vec2(-4.0, -4.0);\n" +
                    "    blurCoordinates[17] = vTextureCoord.xy + singleStepOffset * vec2(-4.0, 4.0);\n" +
                    "    blurCoordinates[18] = vTextureCoord.xy + singleStepOffset * vec2(4.0, -4.0);\n" +
                    "    blurCoordinates[19] = vTextureCoord.xy + singleStepOffset * vec2(4.0, 4.0);\n" +
                    "\n" +
                    "    float sampleColor = centralColor.g * 20.0;\n" +
                    "    sampleColor += texture2D(texture, blurCoordinates[0]).g;\n" +
                    "    sampleColor += texture2D(texture, blurCoordinates[1]).g;\n" +
                    "    sampleColor += texture2D(texture, blurCoordinates[2]).g;\n" +
                    "    sampleColor += texture2D(texture, blurCoordinates[3]).g;\n" +
                    "    sampleColor += texture2D(texture, blurCoordinates[4]).g;\n" +
                    "    sampleColor += texture2D(texture, blurCoordinates[5]).g;\n" +
                    "    sampleColor += texture2D(texture, blurCoordinates[6]).g;\n" +
                    "    sampleColor += texture2D(texture, blurCoordinates[7]).g;\n" +
                    "    sampleColor += texture2D(texture, blurCoordinates[8]).g;\n" +
                    "    sampleColor += texture2D(texture, blurCoordinates[9]).g;\n" +
                    "    sampleColor += texture2D(texture, blurCoordinates[10]).g;\n" +
                    "    sampleColor += texture2D(texture, blurCoordinates[11]).g;\n" +
                    "    sampleColor += texture2D(texture, blurCoordinates[12]).g * 2.0;\n" +
                    "    sampleColor += texture2D(texture, blurCoordinates[13]).g * 2.0;\n" +
                    "    sampleColor += texture2D(texture, blurCoordinates[14]).g * 2.0;\n" +
                    "    sampleColor += texture2D(texture, blurCoordinates[15]).g * 2.0;\n" +
                    "    sampleColor += texture2D(texture, blurCoordinates[16]).g * 2.0;\n" +
                    "    sampleColor += texture2D(texture, blurCoordinates[17]).g * 2.0;\n" +
                    "    sampleColor += texture2D(texture, blurCoordinates[18]).g * 2.0;\n" +
                    "    sampleColor += texture2D(texture, blurCoordinates[19]).g * 2.0;\n" +
                    "\n" +
                    "    sampleColor = sampleColor / 48.0;\n" +
                    "\n" +
                    "    float highPass = centralColor.g - sampleColor + 0.5;\n" +
                    "\n" +
                    "    for(int i = 0; i < 5;i++)\n" +
                    "    {\n" +
                    "        highPass = hardLight(highPass);\n" +
                    "    }\n" +
                    "    float luminance = dot(centralColor, W);\n" +
                    "\n" +
                    "    float alpha = pow(luminance, params);\n" +
                    "\n" +
                    "    vec3 smoothColor = centralColor + (centralColor-vec3(highPass))*alpha*0.1;\n" +
                    "\n" +
                    "    vec4 nColor = vec4(mix(smoothColor.rgb, max(smoothColor, centralColor), alpha), 1.0);\n" +
                    "vec4 deltaColor = nColor+vec4(vec3(level * 0.25),0.0);\n" +
                    "modifyColor(deltaColor);\n" +
                    "gl_FragColor = deltaColor;\n" +
                    "}\n";

    private String fragmentShaderFor2DInputYuvOut = "precision mediump float;\n" +
            "\n" +
            "varying mediump vec2 vTextureCoord;\n" +
            "\n" +
            "uniform sampler2D texture;\n" +
            "uniform vec2 singleStepOffset;\n" +
            "uniform mediump float params;\n" +
            "uniform float level;\n" +
            "\n" +
            "const mediump vec3 W = vec3(0.299,0.587,0.114);\n" +
            "vec2 blurCoordinates[20];\n" +
            "\n" +
            "float hardLight(float color)\n" +
            "{\n" +
            "    if(color <= 0.5)\n" +
            "        color = color * color * 2.0;\n" +
            "    else\n" +
            "        color = 1.0 - ((1.0 - color)*(1.0 - color) * 2.0);\n" +
            "    return color;\n" +
            "}\n" +
            "\n" +
            "void modifyColor(vec4 color){\n" +
            "color.r=max(min(color.r,1.0),0.0);\n" +
            "color.g=max(min(color.g,1.0),0.0);\n" +
            "color.b=max(min(color.b,1.0),0.0);\n" +
            "color.a=max(min(color.a,1.0),0.0);\n" +
            " }\n" +
            "\n " +
            "void main(){\n" +
            "\n" +
            "  float r, g, b, y, u, v;\n" +
            "  float y2, u2, v2;\n" +
            "    vec3 centralColor = texture2D(texture, vTextureCoord).rgb;\n" +
            "    blurCoordinates[0] = vTextureCoord.xy + singleStepOffset * vec2(0.0, -10.0);\n" +
            "    blurCoordinates[1] = vTextureCoord.xy + singleStepOffset * vec2(0.0, 10.0);\n" +
            "    blurCoordinates[2] = vTextureCoord.xy + singleStepOffset * vec2(-10.0, 0.0);\n" +
            "    blurCoordinates[3] = vTextureCoord.xy + singleStepOffset * vec2(10.0, 0.0);\n" +
            "    blurCoordinates[4] = vTextureCoord.xy + singleStepOffset * vec2(5.0, -8.0);\n" +
            "    blurCoordinates[5] = vTextureCoord.xy + singleStepOffset * vec2(5.0, 8.0);\n" +
            "    blurCoordinates[6] = vTextureCoord.xy + singleStepOffset * vec2(-5.0, 8.0);\n" +
            "    blurCoordinates[7] = vTextureCoord.xy + singleStepOffset * vec2(-5.0, -8.0);\n" +
            "    blurCoordinates[8] = vTextureCoord.xy + singleStepOffset * vec2(8.0, -5.0);\n" +
            "    blurCoordinates[9] = vTextureCoord.xy + singleStepOffset * vec2(8.0, 5.0);\n" +
            "    blurCoordinates[10] = vTextureCoord.xy + singleStepOffset * vec2(-8.0, 5.0);\n" +
            "    blurCoordinates[11] = vTextureCoord.xy + singleStepOffset * vec2(-8.0, -5.0);\n" +
            "    blurCoordinates[12] = vTextureCoord.xy + singleStepOffset * vec2(0.0, -6.0);\n" +
            "    blurCoordinates[13] = vTextureCoord.xy + singleStepOffset * vec2(0.0, 6.0);\n" +
            "    blurCoordinates[14] = vTextureCoord.xy + singleStepOffset * vec2(6.0, 0.0);\n" +
            "    blurCoordinates[15] = vTextureCoord.xy + singleStepOffset * vec2(-6.0, 0.0);\n" +
            "    blurCoordinates[16] = vTextureCoord.xy + singleStepOffset * vec2(-4.0, -4.0);\n" +
            "    blurCoordinates[17] = vTextureCoord.xy + singleStepOffset * vec2(-4.0, 4.0);\n" +
            "    blurCoordinates[18] = vTextureCoord.xy + singleStepOffset * vec2(4.0, -4.0);\n" +
            "    blurCoordinates[19] = vTextureCoord.xy + singleStepOffset * vec2(4.0, 4.0);\n" +
            "\n" +
            "    float sampleColor = centralColor.g * 20.0;\n" +
            "    sampleColor += texture2D(texture, blurCoordinates[0]).g;\n" +
            "    sampleColor += texture2D(texture, blurCoordinates[1]).g;\n" +
            "    sampleColor += texture2D(texture, blurCoordinates[2]).g;\n" +
            "    sampleColor += texture2D(texture, blurCoordinates[3]).g;\n" +
            "    sampleColor += texture2D(texture, blurCoordinates[4]).g;\n" +
            "    sampleColor += texture2D(texture, blurCoordinates[5]).g;\n" +
            "    sampleColor += texture2D(texture, blurCoordinates[6]).g;\n" +
            "    sampleColor += texture2D(texture, blurCoordinates[7]).g;\n" +
            "    sampleColor += texture2D(texture, blurCoordinates[8]).g;\n" +
            "    sampleColor += texture2D(texture, blurCoordinates[9]).g;\n" +
            "    sampleColor += texture2D(texture, blurCoordinates[10]).g;\n" +
            "    sampleColor += texture2D(texture, blurCoordinates[11]).g;\n" +
            "    sampleColor += texture2D(texture, blurCoordinates[12]).g * 2.0;\n" +
            "    sampleColor += texture2D(texture, blurCoordinates[13]).g * 2.0;\n" +
            "    sampleColor += texture2D(texture, blurCoordinates[14]).g * 2.0;\n" +
            "    sampleColor += texture2D(texture, blurCoordinates[15]).g * 2.0;\n" +
            "    sampleColor += texture2D(texture, blurCoordinates[16]).g * 2.0;\n" +
            "    sampleColor += texture2D(texture, blurCoordinates[17]).g * 2.0;\n" +
            "    sampleColor += texture2D(texture, blurCoordinates[18]).g * 2.0;\n" +
            "    sampleColor += texture2D(texture, blurCoordinates[19]).g * 2.0;\n" +
            "\n" +
            "    sampleColor = sampleColor / 48.0;\n" +
            "\n" +
            "    float highPass = centralColor.g - sampleColor + 0.5;\n" +
            "\n" +
            "    for(int i = 0; i < 5;i++)\n" +
            "    {\n" +
            "        highPass = hardLight(highPass);\n" +
            "    }\n" +
            "    float luminance = dot(centralColor, W);\n" +
            "\n" +
            "    float alpha = pow(luminance, params);\n" +
            "\n" +
            "    vec3 smoothColor = centralColor + (centralColor-vec3(highPass))*alpha*0.1;\n" +
            "\n" +
            "    vec4 nColor = vec4(mix(smoothColor.rgb, max(smoothColor, centralColor), alpha), 1.0);\n" +
            "vec4 deltaColor = nColor+vec4(vec3(level * 0.25),0.0);\n" +
            "modifyColor(deltaColor);\n" +

            "  r = clamp(deltaColor.r, 0.0, 1.0);\n"
            + "  g = clamp(deltaColor.g, 0.0, 1.0);\n"
            + "  b = clamp(deltaColor.b, 0.0, 1.0);\n"
            + "  y2 = 0.299*r + 0.587*g + 0.114*b;\n"
            + "  u2 = - 0.1687*r - 0.3313*g + 0.5*b+0.5;\n"
            + "  v2 = 0.5*r - 0.4187*g - 0.0813*b+0.5;\n"
            + "  gl_FragColor =vec4(y2,u2,v2,1.0);\n" +
            // "gl_FragColor = deltaColor;\n" +
            "}\n";

    @Override
    protected String getFragmentShader(Context context) {
        if (mOutputType == EGLUtils.TEXTURETYPE.YUV) {
            return fragmentShaderFor2DInputYuvOut;
        } else {
            if (mInputType == EGLUtils.TEXTURETYPE.OESTEXTURE) {
                return fragmentShaderForOESInput2DOut;
            } else  {
                return fragmentShaderFor2DInput2DOut;
            }
        }

    }

    @Override
    protected void getProgramParms() {
        super.getProgramParms();
        mSingleLocation = glGetUniformLocation(getProgram(), SINGLESTEPOFFSET);
        mParamsLocation = glGetUniformLocation(getProgram(), PARAMS);
//        mWidthLocation = glGetUniformLocation(getProgram(), WIDTH);
//        mHeightLocation = glGetUniformLocation(getProgram(), HEIGHT);
//        mOpacityLocation = glGetUniformLocation(getProgram(), OPACITY);
        mLevelLocation = glGetUniformLocation(getProgram(), LEVEL);
    }

    @Override
    protected void configParameter(int program) {
        super.configParameter(program);
        GLES20.glUniform2fv(mSingleLocation, 1, new float[]{2.0f / mViewWidth, 2.0f / mViewHeight}, 0);
        GLES20.glUniform1f(mParamsLocation, CameraUtils.range(CameraData.getInstance().getSmoothProgress(), Contants.SMOOTH_START,Contants.SMOOTH_END));
        GLES20.glUniform1f(mLevelLocation, CameraUtils.range(CameraData.getInstance().getWhiteProgress(),Contants.WHITE_START,Contants.WHITE_END));
    }

}
