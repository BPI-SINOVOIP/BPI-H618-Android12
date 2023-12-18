package com.bigbigcloud.devicehive.entity;

import android.os.Parcel;
import android.os.Parcelable;

/**
 * Created by Administrator on 2016/1/6.
 */
public class UpdateInfoResponse implements Parcelable {
    private int packType;//0 未确定； 1 差分包；2 全量包
    private String pubTime;
    private String newVersion;
    private long packSize;
    private String packMD5;
    private String packUrl;
    private String updatePrompt;
    private String feature;
    private String detailDesc;
    private String curVersion;
    private PubSetting pubSetting;//Ota 升级的一些配置参数

    public UpdateInfoResponse(int packType, String pubTime, String newVersion, long packSize, String packMD5, String packUrl,
                               String updatePrompt, String feature, String detailDesc, String curVersion, PubSetting pubSetting){
        this.packType = packType;
        this.pubTime = pubTime;
        this.newVersion = newVersion;
        this.packSize = packSize;
        this.packMD5 = packMD5;
        this.packUrl = packUrl;
        this.updatePrompt = updatePrompt;
        this.feature = feature;
        this.detailDesc = detailDesc;
        this.curVersion = curVersion;
    }

    public int getPackType() {
        return packType;
    }

    public String getPubTime() {
        return pubTime;
    }

    public String getNewVersion() {
        return newVersion;
    }

    public long getPackSize() {
        return packSize;
    }

    public String getPackMD5() {
        return packMD5;
    }

    public String getUpdatePrompt() {
        return updatePrompt;
    }

    public String getFeature() {
        return feature;
    }

    public String getDetailDesc() {
        return detailDesc;
    }


    public String getCurVersion() {
        return curVersion;
    }

    public String getPackUrl() {
        return packUrl;
    }

    public PubSetting getPubSetting() {
        return pubSetting;
    }

    public UpdateInfoResponse() {
        super();
    }

    @Override
    public int describeContents() {
        return 0;
    }

    @Override
    public void writeToParcel(Parcel dest, int flags) {
        dest.writeInt(packType);
        dest.writeString(pubTime);
        dest.writeString(newVersion);
        dest.writeLong(packSize);
        dest.writeString(packMD5);
        dest.writeString(packUrl);
        dest.writeString(updatePrompt);
        dest.writeString(feature);
        dest.writeString(detailDesc);
        dest.writeString(curVersion);
        dest.writeSerializable(pubSetting);
    }

    public static Creator<UpdateInfoResponse> CREATOR = new Creator<UpdateInfoResponse>() {
        @Override
        public UpdateInfoResponse createFromParcel(Parcel source) {
            return new UpdateInfoResponse(source.readInt(), source.readString(),source.readString(),
                    source.readLong(), source.readString(), source.readString(), source.readString(),
                    source.readString(), source.readString(), source.readString(), (PubSetting)source.readSerializable());
        }

        @Override
        public UpdateInfoResponse[] newArray(int size) {
            return new UpdateInfoResponse[size];
        }
    };

    public String toString(){
        return "packType" + packType + " \npacksize " + packSize
                + " \npackUrl " + packUrl  + " \nupdate prompt " + updatePrompt
                + " \nfeature " + feature + " \ndetail " + detailDesc
                + " \nnew version " + newVersion;
    }
}
