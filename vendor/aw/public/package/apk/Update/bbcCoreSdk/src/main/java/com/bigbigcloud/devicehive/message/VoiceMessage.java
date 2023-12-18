package com.bigbigcloud.devicehive.message;

import com.bigbigcloud.devicehive.message.BaseMessage;

/**
 * Created by Administrator on 2016/1/2.
 */
public class VoiceMessage extends BaseMessage {
    private String voiceurl;
    private String format;
    private transient boolean isListened;

    public VoiceMessage(){
        super(Type.VOICE);
    }

    public String getVoiceurl() {
        return voiceurl;
    }

    public void setVoiceurl(String voiceurl) {
        this.voiceurl = voiceurl;
    }

    public String getFormat() {
        return format;
    }

    public void setFormat(String format) {
        this.format = format;
    }

    public boolean isListened() {
        return isListened;
    }

    public void setIsListened(boolean isListened) {
        this.isListened = isListened;
    }
}
