package com.bigbigcloud.devicehive.message;

/**
 * Created by Administrator on 2015/12/31.
 */
public class ImageMessage extends BaseMessage {
    private String picurl;
    private String mediaId;
    private boolean liked = false;
    private String commments;
    public ImageMessage(){
        super(Type.IMAGE);
    }

    public String getMediaId() {
        return mediaId;
    }

    public String getPicurl() {
        return picurl;
    }

    public void setPicurl(String picurl) {
        this.picurl = picurl;
    }

    public boolean isLiked() {return liked;}
    public void setLiked(boolean liked){
        this.liked = liked;
    }

    public String getCommments() {
        return commments;
    }

    public void setCommments(String commments) {
        this.commments = commments;
    }

    @Override
    public boolean equals(Object o) {
        if (this == o) return true;
        if (o == null || getClass() != o.getClass()) return false;

        ImageMessage that = (ImageMessage) o;

        return !(picurl != null ? !picurl.equals(that.picurl) : that.picurl != null);

    }

    @Override
    public int hashCode() {
        return picurl != null ? picurl.hashCode() : 0;
    }
}
