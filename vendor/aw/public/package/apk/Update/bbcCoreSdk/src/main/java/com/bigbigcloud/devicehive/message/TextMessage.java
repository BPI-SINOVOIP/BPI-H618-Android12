package com.bigbigcloud.devicehive.message;

import com.bigbigcloud.devicehive.message.BaseMessage;

/**
 * Created by Administrator on 2015/12/31.
 */
public class TextMessage extends BaseMessage {

    private String content;
    public TextMessage(){
        super(Type.TXT);
    }

    public String getContent() {
        return content;
    }

    public void setContent(String content) {
        this.content = content;
    }
}
