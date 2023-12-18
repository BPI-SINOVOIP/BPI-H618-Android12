package com.allwinner.camera.program;

import android.content.Context;
import android.opengl.GLES20;
import android.util.Log;

import com.allwinner.camera.utils.EGLUtils;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.FloatBuffer;
import java.nio.ShortBuffer;

import static android.opengl.GLES20.GL_COMPILE_STATUS;
import static android.opengl.GLES20.GL_FRAGMENT_SHADER;
import static android.opengl.GLES20.GL_INFO_LOG_LENGTH;
import static android.opengl.GLES20.GL_LINK_STATUS;
import static android.opengl.GLES20.GL_TEXTURE0;
import static android.opengl.GLES20.GL_TEXTURE1;
import static android.opengl.GLES20.GL_TEXTURE_2D;
import static android.opengl.GLES20.GL_VERTEX_SHADER;
import static android.opengl.GLES20.glActiveTexture;
import static android.opengl.GLES20.glAttachShader;
import static android.opengl.GLES20.glBindTexture;
import static android.opengl.GLES20.glCompileShader;
import static android.opengl.GLES20.glCreateProgram;
import static android.opengl.GLES20.glCreateShader;
import static android.opengl.GLES20.glDeleteProgram;
import static android.opengl.GLES20.glDeleteShader;
import static android.opengl.GLES20.glGetAttribLocation;
import static android.opengl.GLES20.glGetError;
import static android.opengl.GLES20.glGetProgramInfoLog;
import static android.opengl.GLES20.glGetProgramiv;
import static android.opengl.GLES20.glGetShaderInfoLog;
import static android.opengl.GLES20.glGetShaderiv;
import static android.opengl.GLES20.glGetUniformLocation;
import static android.opengl.GLES20.glLinkProgram;
import static android.opengl.GLES20.glShaderSource;
import static android.opengl.GLES20.glUniform1i;

public class BaseProgram {
    protected FloatBuffer mTriangleVertices;
    protected FloatBuffer mTriangleVerticesNotRotate;
    private FloatBuffer mTriangleUV;
    private ShortBuffer mDrawListBuffer;
    private int mVertexShader = -1;
    private int mFragmentShader = -1;
    private int mProgram = -1;
    private int mVertexEffectShader = -1;
    private int mFragmenEffectShader = -1;
    private static final String TAG = "BasePreviewProgram";
    private boolean mInit = false;

    private int mUTextureYLocation = -1;
    private int mUTextureUVLocation = -1;
    private int mPositionLocation = -1;
    private int mTextureCoordLocation = -1;
    private int mTextureLocation = -1;
    private int mMVPMatrixLocation = -1;
    private int mTexMatrixLocation = -1;

    protected int mViewWidth;
    protected int mViewHeight;

    protected FloatBuffer mFinalTriangleVertices;
    protected FloatBuffer mFinalTriangleUV;


    protected EGLUtils.TEXTURETYPE mInputType = EGLUtils.TEXTURETYPE.OESTEXTURE;

    protected EGLUtils.TEXTURETYPE mOutputType = EGLUtils.TEXTURETYPE.OESTEXTURE;

    public BaseProgram() {
        createBuffer();
        mInit = false;
    }

    public BaseProgram(EGLUtils.TEXTURETYPE inputType, EGLUtils.TEXTURETYPE outputType) {
        createBuffer();
        mInit = false;
        mInputType = inputType;
        mOutputType = outputType;
    }

    public EGLUtils.TEXTURETYPE getInputType() {
        return mInputType;
    }

    public EGLUtils.TEXTURETYPE getOutputType() {
        return mOutputType;
    }

    public void initProgram(Context context) {
        if (!mInit) {
            Log.i(TAG, "initProgram: " + this.getClass().getSimpleName());
            mVertexShader = loadShader(GL_VERTEX_SHADER, getVertexShader());
            Log.i(TAG, "loadShader mFragmentShader");
            mFragmentShader = loadShader(GL_FRAGMENT_SHADER, getFragmentShader(context));
            mProgram = linkProgram(mVertexShader, mFragmentShader);
            mInit = true;
        }
    }

    private short mDrawOrder[] = {0, 1, 2, 2, 1, 3}; // order to draw vertices


    private final float mTriangleVerticesData[] = {
            // X, Y
            -1.0f, 1.0f,
            1.0f, 1.0f,
            -1.0f, -1.0f,
            1.0f, -1.0f,
    };
    private final float mTriangleUVData[] = {
            // U, V
            0.0f, 1.0f,
            1.0f, 1.0f,
            0.0f, 0.0f,
            1.0f, 0.0f
    };

    private final float mTriangleVerticesDataNotRotate[] = {
            // X, Y, Z,
            -1.0f, -1.0f,
            1.0f, -1.0f,
            -1.0f, 1.0f,
            1.0f, 1.0f,
    };


    public static final String POSITION_ATTRIBUTE = "aPosition";
    public static final String TEXTURE_COORD_ATTRIBUTE = "aTextureCoordinate";
    public static final String MVP_MATRIX = "uMVPMatrix";
    public static final String TEX_MATRIX = "uTexMatrix";
    public static final String TEXTURE_ATTRIBUTE = "texture";
    public static final String TEXTURE_Y = "textureY";
    public static final String TEXTURE_UV = "textureUV";


    protected String getVertexShader() {
        String vertexShader =
                "attribute vec4 aPosition;\n" +
                        "attribute vec2 aTextureCoordinate;\n" +
                        "varying vec2 vTextureCoord;\n" +
                        "uniform mat4 uTexMatrix;\n"+
                        "uniform mat4 uMVPMatrix;\n"+

                        "void main()\n" +
                        "{\n" +
                        "  gl_Position = aPosition * uMVPMatrix;\n" +
                        "  vTextureCoord = (uTexMatrix * vec4(aTextureCoordinate, 0.0, 1.0)).st;\n" +
                       // "  vTextureCoord =aTextureCoordinate;\n" +
                        "}\n";
        return vertexShader;
    }

    private String fragmentShaderForYUVInput2DOut =
            "precision mediump float;\n"
                    + "uniform sampler2D textureY;\n"
                    + "uniform sampler2D textureUV;\n"
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
                    //+ "  gl_FragColor = rgb2yuv * temprgb; \n"
                    + "  gl_FragColor =vec4(r,g,b,1.0);\n"
                    // + "  gl_FragColor =vec4(1.0,0.0,1.0,1.0);\n"
                    + "}\n";



    private String fragmentShaderForOESInput2DOut =
            "#extension GL_OES_EGL_image_external : require\n" +
                    "precision mediump float ;"+
                    "uniform samplerExternalOES texture;\n" +
                    "varying vec2 vTextureCoord;\n" +
                    "void main() \n" +
                    "{\n" +
                    "  gl_FragColor = texture2D(texture, vTextureCoord);\n" +
                    "}\n";

    private String fragmentShaderFor2DInput2DOut =
                    "precision mediump float ;"+
                    "uniform sampler2D texture;\n" +
                    "varying vec2 vTextureCoord;\n" +
                    "void main() \n" +
                    "{\n" +
                    "  gl_FragColor = texture2D(texture, vTextureCoord);\n" +
                    "}\n";

    private String fragmentShaderForOESInputYuvOut =
            "#extension GL_OES_EGL_image_external : require\n" +
                    "precision mediump float ;" +
                    "uniform samplerExternalOES texture;\n" +
                    "varying vec2 vTextureCoord;\n" +
                    "void main() \n" +
                    "{\n"
                    + "  float r, g, b, a, y, u, v;\n"
                    + "  float y2, u2, v2;\n"
                    + "  vec4 color = texture2D(texture, vTextureCoord);\n"
                    + "r = color.r;\n"
                    + "g = color.g;\n"
                    + "b = color.b;\n"
                    + "a = color.a;\n"
                    + "r = clamp(r, 0.0, 1.0);\n"
                    + "g = clamp(g, 0.0, 1.0);\n"
                    + "b = clamp(b, 0.0, 1.0);\n"
                    + "y2 = 0.299*r + 0.587*g + 0.114*b;\n"
                    + "u2 = - 0.1687*r - 0.3313*g + 0.5*b+0.5;\n"
                    + "v2 = 0.5*r - 0.4187*g - 0.0813*b+0.5;\n"
                    + "gl_FragColor =vec4(y2,u2,v2,a);\n" +
                    // "gl_FragColor = deltaColor;\n" +
                    "}\n";

    private String fragmentShaderFor2DInputYuvOut =
            "#extension GL_OES_EGL_image_external : require\n" +
                    "precision mediump float ;" +
                    "uniform sampler2D texture;\n" +
                    "varying vec2 vTextureCoord;\n" +
                    "void main() \n" +
                    "{\n"
                    + "  float r, g, b, a, y, u, v;\n"
                    + "  float y2, u2, v2;\n"
                    + "  vec4 color = texture2D(texture, vTextureCoord);\n"
                    + "r = color.r;\n"
                    + "g = color.g;\n"
                    + "b = color.b;\n"
                    + "a = color.a;\n"
                    + "r = clamp(r, 0.0, 1.0);\n"
                    + "g = clamp(g, 0.0, 1.0);\n"
                    + "b = clamp(b, 0.0, 1.0);\n"
                    + "y2 = 0.299*r + 0.587*g + 0.114*b;\n"
                    + "u2 = - 0.1687*r - 0.3313*g + 0.5*b+0.5;\n"
                    + "v2 = 0.5*r - 0.4187*g - 0.0813*b+0.5;\n"
                    + "gl_FragColor =vec4(y2,u2,v2,a);\n" +
                    // "gl_FragColor = deltaColor;\n" +
                    "}\n";

    private String fragmentShaderForYuvInputYuvOut =
            "#extension GL_OES_texture_3D : enable\n"
                    + "precision mediump float;\n"
                    + "uniform sampler2D textureY;\n"
                    + "uniform sampler2D textureUV;\n"
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
                    + "  y2 = 0.299*r + 0.587*g + 0.114*b;\n"
                    + "  u2 = - 0.1687*r - 0.3313*g + 0.5*b+0.5;\n"
                    + "  v2 = 0.5*r - 0.4187*g - 0.0813*b+0.5;\n"
                    //+ "  gl_FragColor = rgb2yuv * temprgb; \n"
                    + "  gl_FragColor =vec4(y2,u2,v2,1.0);\n"
                    + "}\n";


    protected String getFragmentShader(Context context) {
        if (mOutputType == EGLUtils.TEXTURETYPE.YUV) {
            if (mInputType == EGLUtils.TEXTURETYPE.OESTEXTURE) {
                return fragmentShaderForOESInputYuvOut;
            } else if (mInputType == EGLUtils.TEXTURETYPE.YUV) {
                return fragmentShaderForYuvInputYuvOut;
            } else {
                return fragmentShaderFor2DInputYuvOut;
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


    public void createBuffer() {
        mTriangleVertices = ByteBuffer.allocateDirect(mTriangleVerticesData
                .length * 4)
                .order(ByteOrder.nativeOrder()).asFloatBuffer();
        mTriangleVertices.put(mTriangleVerticesData).position(0);


        mTriangleVerticesNotRotate = ByteBuffer.allocateDirect(mTriangleVerticesDataNotRotate
                .length * 4)
                .order(ByteOrder.nativeOrder()).asFloatBuffer();
        mTriangleVerticesNotRotate.put(mTriangleVerticesDataNotRotate).position(0);


        mTriangleUV = ByteBuffer.allocateDirect(
                mTriangleUVData.length * 4)
                .order(ByteOrder.nativeOrder()).asFloatBuffer();
        mTriangleUV.put(mTriangleUVData).position(0);
        // initialize byte buffer for the draw list
        ByteBuffer dlb = ByteBuffer.allocateDirect(mDrawOrder.length * 2);
        dlb.order(ByteOrder.nativeOrder());
        mDrawListBuffer = dlb.asShortBuffer();
        mDrawListBuffer.put(mDrawOrder);
        mDrawListBuffer.position(0);
    }

    public void updateVeticesBuffer(float[] triangleVertices){
        mTriangleVertices.put(triangleVertices).position(0);
        mTriangleVerticesNotRotate.put(triangleVertices).position(0);
    }

    public void updateUVBuffer(float[] triangleUV){
        mTriangleUV.put(triangleUV).position(0);
    }

    public int loadShader(int type, String shaderSource) {
        int shader = glCreateShader(type);
        if (shader == 0) {
            throw new RuntimeException("Create Shader Failed!" + glGetError());
        }
        glShaderSource(shader, shaderSource);
        glCompileShader(shader);

        int[] compiled = new int[1];
        glGetShaderiv(shader, GL_COMPILE_STATUS, compiled, 0);
        if (compiled[0] == 0) {
            int[] infoLen = new int[1];
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, infoLen, 0);
            if (infoLen[0] > 1) {
                String s = glGetShaderInfoLog(shader);
                Log.e(TAG, "Error Compiling shader" + s);
                throw new RuntimeException("Error Compiling shader" + s);
            }
            glDeleteShader(shader);
            return 0;
        }
        return shader;
    }

    public int linkProgram(int verShader, int fragShader) {
        int program = glCreateProgram();
        if (program == 0) {
            throw new RuntimeException("Create Program Failed: " + glGetError());
        }
        glAttachShader(program, verShader);
        glAttachShader(program, fragShader);
        glLinkProgram(program);

        int[] linked = new int[1];
        glGetProgramiv(program, GL_LINK_STATUS, linked, 0);
        if (linked[0] == 0) {
            int[] infoLen = new int[1];
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, infoLen, 0);
            if (infoLen[0] > 1) {
                String log = glGetProgramInfoLog(program);
                Log.e(TAG, "Error linking program: " + log);
            }
            glDeleteProgram(program);
            return 0;
        }
        //glUseProgram(program);
        return program;
    }


    public int getProgram() {
        return mProgram;
    }

    public FloatBuffer getPostionBuffer() {
        if (mInputType == EGLUtils.TEXTURETYPE.OESTEXTURE) {
            return mTriangleVertices;
        } else {
            return mTriangleVerticesNotRotate;
        }
    }

    public FloatBuffer getCoordBuffer() {
        return mTriangleUV;
    }

    public ShortBuffer getDrawlistBuffer() {
        return mDrawListBuffer;
    }

    public int getDrawOrderLength(){
        return  mDrawOrder.length;
    }

    public boolean isReady() {
        return mInit;
    }

    public void  release() {
        Log.i(TAG, "release");
        if(mTriangleVertices != null) {
            mTriangleVertices.clear();
        }
        if(mDrawListBuffer !=null) {
            mDrawListBuffer.clear();
        }
        if(mTriangleUV != null) {
            mTriangleUV.clear();
        }
        mTriangleVertices = null;
        mDrawListBuffer = null;
        mTriangleUV = null;
        if (mInit) {
            Log.i(TAG, "glDeleteProgram");
            glDeleteProgram(mProgram);
            EGLUtils.checkEglError("glDeleteProgram");
            mInit = false;
        }
    }

    protected void getProgramParms() {
        mFinalTriangleVertices = getPostionBuffer();
        mFinalTriangleUV = getCoordBuffer();
        mPositionLocation = glGetAttribLocation(getProgram(), BaseProgram.POSITION_ATTRIBUTE);
        mTextureCoordLocation = glGetAttribLocation(getProgram(), BaseProgram.TEXTURE_COORD_ATTRIBUTE);
        mTextureLocation = glGetUniformLocation(getProgram(), BaseProgram.TEXTURE_ATTRIBUTE);
        mUTextureYLocation = glGetUniformLocation(getProgram(), BigEyesProgram.TEXTURE_Y);
        mUTextureUVLocation = glGetUniformLocation(getProgram(), BigEyesProgram.TEXTURE_UV);
        mTexMatrixLocation = glGetUniformLocation(getProgram(), BaseProgram.TEX_MATRIX);
        mMVPMatrixLocation = glGetUniformLocation(getProgram(), BaseProgram.MVP_MATRIX);

    }

    public void drawTexture(int target, int texture, int offsetX, int offsetY, int viewWidth, int viewHeight, float[] texMatrix, float[] mvpMartix) {
        mViewWidth = viewWidth;
        mViewHeight = viewHeight;
        GLES20.glViewport(offsetX, offsetY, viewWidth, viewHeight);
        getProgramParms();
        if (!isReady()) {
            Log.i(TAG, "mSmooth2DRenderProgram not ready");
            return;
        }
        GLES20.glUseProgram(mProgram);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(target, texture);
        glUniform1i(mTextureLocation, 0);//把选定的纹理单元传递给片段着色器中的u_Texture
        draw(texMatrix, mvpMartix);

    }

    public void drawYuv(int yTextureId, int uvTextureId, int viewWidth, int viewHeight, float[] texMatrix, float[] mvpMartix) {
        mViewWidth = viewWidth;
        mViewHeight = viewHeight;
        GLES20.glViewport(0, 0, viewWidth, viewHeight);
        getProgramParms();
        if (!isReady()) {
            Log.i(TAG, "mSmooth2DRenderProgram not ready");
            return;
        }
        GLES20.glUseProgram(mProgram);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, yTextureId);
        glUniform1i(mUTextureYLocation, 0);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, uvTextureId);
        glUniform1i(mUTextureUVLocation, 1);
        draw(texMatrix,mvpMartix);
    }

    private void draw(float[] texMatrix, float[] mvpMartix){
        configParameter(getProgram());
        mTriangleVertices.position(0);
        GLES20.glBindBuffer(GLES20.GL_ARRAY_BUFFER, 0);
        GLES20.glBindBuffer(GLES20.GL_ELEMENT_ARRAY_BUFFER, 0);
        GLES20.glVertexAttribPointer(mPositionLocation, 2, GLES20.GL_FLOAT, false, 0, mFinalTriangleVertices);
        EGLUtils.checkEglError("glVertexAttribPointer PositionLocation");
        GLES20.glEnableVertexAttribArray(mPositionLocation);
        EGLUtils.checkEglError("glEnableVertexAttribArray PositionLocation");
        mTriangleUV.position(0);
        GLES20.glVertexAttribPointer(mTextureCoordLocation, 2, GLES20.GL_FLOAT, false, 0, mFinalTriangleUV);
        GLES20.glEnableVertexAttribArray(mTextureCoordLocation);
        EGLUtils.checkEglError("glEnableVertexAttribArray TextureCoordLocation");
        GLES20.glUniformMatrix4fv(mTexMatrixLocation, 1, false, texMatrix, 0);
        EGLUtils.checkEglError("glUniformMatrix4fv texMatrix");
        GLES20.glUniformMatrix4fv(mMVPMatrixLocation, 1, false, mvpMartix, 0);
        EGLUtils.checkEglError("glUniformMatrix4fv mMVPMatrixLocation");
        GLES20.glDrawArrays(GLES20.GL_TRIANGLE_STRIP, 0, 4);
    }

    protected void configParameter(int program){
        //config other parameter
    }

}

