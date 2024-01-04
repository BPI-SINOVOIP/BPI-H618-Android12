package com.softwinner.tvfilemanager.entity;

import android.graphics.Bitmap;

import java.io.Serializable;

/**
 * Created by ningxiangzhong on 15-2-28.
 */
public class LocalMusic implements IFile, Serializable {
    private String music_path;
    private String music_name;
    private Bitmap music_thumbnail;

    @Override
    public String getFile_path() {
        return music_path;
    }

    @Override
    public String getFile_name() {
        return music_name;
    }

    @Override
    public Bitmap getFile_thumbnail() {return music_thumbnail;}

    public void setMusic_path(String path) {
        music_path = path;
    }

    public void setMusic_name(String name) {
        music_name = name;
    }

    public void setMusic_thumbnail(Bitmap bmp) {music_thumbnail = bmp;}
}
