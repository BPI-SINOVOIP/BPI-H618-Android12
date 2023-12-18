package com.softwinner.tvfilemanager.entity;

import android.graphics.Bitmap;

import java.io.Serializable;

/**
 * Created by ningxiangzhong on 15-2-28.
 */
public class LocalPicture implements IFile, Serializable {
    private String picture_path;
    private String picture_name;
    private Bitmap picture_thumbanil;

    @Override
    public String getFile_path() {
        return picture_path;
    }

    @Override
    public String getFile_name() {
        return picture_name;
    }

    @Override
    public Bitmap getFile_thumbnail() {return picture_thumbanil;}

    public void setPicture_path(String path) {
        picture_path = path;
    }

    public void setPicture_name(String name) {
        this.picture_name = name;
    }

    public void setPicture_thumbanil(Bitmap picture_thumbanil) {
        this.picture_thumbanil = picture_thumbanil;
    }
}
