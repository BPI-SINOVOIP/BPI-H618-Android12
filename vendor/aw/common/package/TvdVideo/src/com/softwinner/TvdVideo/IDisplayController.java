package com.softwinner.TvdVideo;

import android.app.Dialog;
import android.content.Context;

public class IDisplayController extends Dialog {

    protected int mVideoWidth;
    protected int mVideoHeight;
    protected Context mContext;

    public IDisplayController(Context context,int videoWidth, int videoHeight) {
        super(context, R.style.menu);
        mContext = context;
        mVideoWidth = videoWidth;
        mVideoHeight = videoHeight;
    }

}