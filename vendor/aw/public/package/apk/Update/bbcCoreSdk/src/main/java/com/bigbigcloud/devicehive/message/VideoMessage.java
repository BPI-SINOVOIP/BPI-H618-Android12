package com.bigbigcloud.devicehive.message;

import com.bigbigcloud.devicehive.message.BaseMessage;

/**
 * Created by Administrator on 2016/1/2.
 */
public class VideoMessage extends BaseMessage {

    private String videourl;
    private String thumburl;

    public VideoMessage(){
        super(Type.VIDEO);
    }

    public String getVideourl() {
        return videourl;
    }

    public void setVideourl(String videourl) {
        this.videourl = videourl;
    }

    public String getThumburl() {
        return thumburl;
    }

    public void setThumburl(String thumburl) {
        this.thumburl = thumburl;
    }
}
