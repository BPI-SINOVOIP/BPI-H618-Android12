package com.bigbigcloud.devicehive.message;

import java.io.Serializable;

/**
 * Created by Administrator on 2015/12/31.
 */
public class BbcMessage implements Serializable {
    protected transient String messageType;
    protected transient String parameters;

    public BbcMessage(String messageType, String parameters){
        this.messageType = messageType;
        this.parameters = parameters;
    }

    public String getMessageType() {
        return messageType;
    }

    public String getParameters() {
        return parameters;
    }
}
