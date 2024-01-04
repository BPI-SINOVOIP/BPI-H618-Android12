package com.softwinner.dragonbox.manager;

import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;

import android.content.Context;
import android.content.res.AssetManager;

public class VideoManager {
    Context mContext;
    private String videoFileName = "case_video_sample";
    public File videoFile;

    public VideoManager(Context context) {
        mContext = context;
        videoFile = new File(mContext.getCacheDir().getPath() + File.separator
                + videoFileName);
        wirteFileToCache(videoFileName);
    }

    private void wirteFileToCache(String fileName) {
        AssetManager am = mContext.getAssets();
        try {
            if (videoFile.exists()) {
                return;
            }

            videoFile.createNewFile();
            videoFile.setReadable(true, false);
            InputStream is = am.open(fileName);
            BufferedInputStream bis = new BufferedInputStream(is);
            BufferedOutputStream bos = new BufferedOutputStream(
                    new FileOutputStream(videoFile));
            byte[] buf = new byte[1024 * 1024];
            int length = 0;
            while ((length = bis.read(buf)) > 0) {
                bos.write(buf, 0, length);
            }
            bos.flush();
            bis.close();
            is.close();
            bos.close();
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

}
