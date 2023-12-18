package com.bigbigcloud.devicehive.message;

import java.util.UUID;

/**
 * Created by Administrator on 2015/12/31.
 */
public class BaseMessage {
    protected transient String id;
    protected transient Type type;
    protected long userId;
    protected long createTime;
    protected String nickName;
    protected String headImgurl;
    protected transient String filePath;
    protected transient Status status;
    protected transient MessageCallBack messageCallBack;

    public BaseMessage(Type type){
        id = UUID.randomUUID().toString();
        this.type = type;
    }

    public long getUserId() {
        return userId;
    }

    public void setUserId(long userId) {
        this.userId = userId;
    }

    public Type getType() {
        return type;
    }

    public String getId() {
        return id;
    }

    public String getFilePath() {
        return filePath;
    }

    public void setFilePath(String filePath) {
        this.filePath = filePath;
    }

    public long getCreateTime() {
        return createTime;
    }

    public void setCreateTime(long createTime) {
        this.createTime = createTime;
    }

    public String getNickName() {
        return nickName;
    }

    public void setNickName(String nickName) {
        this.nickName = nickName;
    }

    public String getHeadImgurl() {
        return headImgurl;
    }

    public void setHeadImgurl(String headImgurl) {
        this.headImgurl = headImgurl;
    }

    public Status getStatus() {
        return status;
    }

    public void setStatus(Status status) {
        this.status = status;
    }

    public MessageCallBack getMessageCallBack() {
        return messageCallBack;
    }

    public void setMessageCallBack(MessageCallBack messageCallBack) {
        this.messageCallBack = messageCallBack;
    }

    public static enum Status {
        SUCCESS,
        FAIL,
        INPROGRESS;
    }

    public static enum Type {
        UNKNOWN,
        TXT,
        IMAGE,
        VIDEO,
        VOICE,
    }

    @Override
    public boolean equals(Object o) {
        if (this == o) return true;
        if (o == null || getClass() != o.getClass()) return false;

        BaseMessage that = (BaseMessage) o;

        return !(id != null ? !id.equals(that.id) : that.id != null);

    }

    @Override
    public int hashCode() {
        return id != null ? id.hashCode() : 0;
    }
}
