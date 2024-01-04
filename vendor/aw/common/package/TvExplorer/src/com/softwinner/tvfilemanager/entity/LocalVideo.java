package com.softwinner.tvfilemanager.entity;

import android.graphics.Bitmap;
import android.os.Parcel;
import android.os.Parcelable;

/**
 * Created by ningxiangzhong on 15-2-28.
 */
public class LocalVideo implements IFile, Parcelable {

    private String video_path;
    private String video_name;
    private Bitmap video_thumbnail;

    public LocalVideo() {}

    @Override
    public String getFile_path() {
        return video_path;
    }

    @Override
    public String getFile_name() {
        return video_name;
    }

    @Override
    public Bitmap getFile_thumbnail () {return video_thumbnail;}

    public void setFile_path(String path) {
        video_path = path;
    }

    public void setFile_name(String name) {
        video_name = name;
    }

    public void setVideo_thumbnail(Bitmap drawable) {video_thumbnail = drawable;}

    @Override
    public int describeContents() {
        return 0;
    }

    @Override
    public void writeToParcel(Parcel dest, int flags) {
        dest.writeString(video_path);
        dest.writeString(video_name);
        dest.writeParcelable(video_thumbnail,flags);
    }

    public static final Creator<LocalVideo> CREATOR = new Creator<LocalVideo>(){

        @Override
        public LocalVideo createFromParcel(Parcel source) {
            return new LocalVideo(source);
        }

        @Override
        public LocalVideo[] newArray(int size) {
            return new LocalVideo[size];
        }
    };

    public LocalVideo(Parcel in) {
        video_path = in.readString();
        video_name = in.readString();
        video_thumbnail = in.readParcelable(Bitmap.class.getClassLoader());
    }

    @Override
    public String toString() {
        return "LocalVideo{" +
                "video_path='" + video_path + '\'' +
                ", video_name='" + video_name + '\'' +
                ", video_thumbnail=" + video_thumbnail +
                '}';
    }
}
