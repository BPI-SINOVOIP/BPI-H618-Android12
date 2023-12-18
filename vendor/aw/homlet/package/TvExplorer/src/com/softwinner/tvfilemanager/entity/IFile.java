package com.softwinner.tvfilemanager.entity;

import android.graphics.Bitmap;

/**
 * Created by ningxiangzhong on 15-2-28.
 */
public interface IFile {

    public String getFile_path();
    public String getFile_name();
    public Bitmap getFile_thumbnail();
}
