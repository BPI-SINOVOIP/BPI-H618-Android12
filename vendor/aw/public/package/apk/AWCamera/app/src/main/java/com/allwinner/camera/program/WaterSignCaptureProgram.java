package com.allwinner.camera.program;

import android.content.Context;

public class WaterSignCaptureProgram extends BaseProgram {
    public static final String TEXTURE_S = "sTexture";
    @Override
    protected String getFragmentShader(Context context) {
        String fragmentShader =
                "precision mediump float;\n" +
                        "varying vec2 vTextureCoord;\n" +
                        "uniform sampler2D sTexture;\n" +
                        "void main() {\n" +
                        "  float r, g, b,a, y, u, v;\n"
                        + "  r = texture2D(sTexture, vTextureCoord).r;\n"
                        + "  g = texture2D(sTexture, vTextureCoord).g ;\n"
                        + "  b = texture2D(sTexture, vTextureCoord).b ;\n"
                        + "  a = texture2D(sTexture, vTextureCoord).a ;\n"
                        + "  y = 0.299*r + 0.587*g + 0.114*b;\n"
                        + "  u = - 0.1687*r - 0.3313*g + 0.5*b+0.5;\n"
                        + "  v = 0.5*r - 0.4187*g - 0.0813*b+0.5;\n"
                        //+ "  gl_FragColor = rgb2yuv * temprgb; \n"
                        + "  gl_FragColor =vec4(y,u,v,a);\n"
                        + "}\n";
        return fragmentShader;
    }
}
