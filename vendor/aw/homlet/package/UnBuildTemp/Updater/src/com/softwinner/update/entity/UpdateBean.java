package com.softwinner.update.entity;

/**
 * OTA信息数据类
 * @author Nurmuhammad
 *
 */
public class UpdateBean
{
    int rescode; //返回码，0=成功，非0=失败
    String resmsg; //返回消息，失败时返回错误信息
    int updateType; //更新结果（0=无更新，1=存在更新）
    String newRomName; //Rom名称
    String oldRomType; //oldRom类型
    String oldRomVersion; //oldRom版本号
    String newRomType; //新Rom类型
    String newRomVersion; //新Rom版本号
    int packId; //包编号，用以标识、区分安装包
    int packType; //包类型（1=整包，2=增量包）

    public String getOldRomType()
    {
	return oldRomType;
    }

    public void setOldRomType( String oldRomType )
    {
	this.oldRomType = oldRomType;
    }

    public String getOldRomVersion()
    {
	return oldRomVersion;
    }

    public void setOldRomVersion( String oldRomVersion )
    {
	this.oldRomVersion = oldRomVersion;
    }

    int packSize; //包大小，单位：bytes
    String packMD5; //包MD5值
    String packUrl; //安装包地址
    String pubTime; //安装包发布时间，格式：yyyy-MM-dd HH:mm:ss。如2013-12-14 16:38
    String updatePrompt; //更新说明，用以在通知栏中显示
    String updateDesc; //更新描述。详细的版本说明，用以点击更新说明中的「更多」后显示。支持HTML格式内容

    public UpdateBean()
    {}

    public int getRescode()
    {
	return rescode;
    }

    public void setRescode( int rescode )
    {
	this.rescode = rescode;
    }

    public String getResmsg()
    {
	return resmsg;
    }

    public void setResmsg( String resmsg )
    {
	this.resmsg = resmsg;
    }

    public int getUpdateType()
    {
	return updateType;
    }

    public void setUpdateType( int updateType )
    {
	this.updateType = updateType;
    }

    public int getPackId()
    {
	return packId;
    }

    public void setPackId( int packId )
    {
	this.packId = packId;
    }

    public int getPackType()
    {
	return packType;
    }

    public void setPackType( int packType )
    {
	this.packType = packType;
    }

    public int getPackSize()
    {
	return packSize;
    }

    public void setPackSize( int packSize )
    {
	this.packSize = packSize;
    }

    public String getPackMD5()
    {
	return packMD5;
    }

    public void setPackMD5( String packMD5 )
    {
	this.packMD5 = packMD5;
    }

    public String getPackUrl()
    {
	return packUrl;
    }

    public void setPackUrl( String packUrl )
    {
	this.packUrl = packUrl;
    }

    public String getPubTime()
    {
	return pubTime;
    }

    public void setPubTime( String pubTime )
    {
	this.pubTime = pubTime;
    }

    public String getUpdatePrompt()
    {
	return updatePrompt;
    }

    public void setUpdatePrompt( String updatePrompt )
    {
	this.updatePrompt = updatePrompt;
    }

    public String getUpdateDesc()
    {
	return updateDesc;
    }

    public void setUpdateDesc( String updateDesc )
    {
	this.updateDesc = updateDesc;
    }

    public String getNewRomName()
    {
	return newRomName;
    }

    public void setNewRomName( String newRomName )
    {
	this.newRomName = newRomName;
    }

    public String getNewRomType()
    {
	return newRomType;
    }

    public void setNewRomType( String newRomType )
    {
	this.newRomType = newRomType;
    }

    public String getNewRomVersion()
    {
	return newRomVersion;
    }

    public void setNewRomVersion( String newRomVersion )
    {
	this.newRomVersion = newRomVersion;
    }

    @Override
    public String toString()
    {
	return "UpdateBean [rescode=" + rescode + ", resmsg=" + resmsg + ", updateType=" + updateType + ", newRomName="
		+ newRomName + ", oldRomType=" + oldRomType + ", oldRomVersion=" + oldRomVersion + ", newRomType="
		+ newRomType + ", newRomVersion=" + newRomVersion + ", packId=" + packId + ", packType=" + packType
		+ ", packSize=" + packSize + ", packMD5=" + packMD5 + ", packUrl=" + packUrl + ", pubTime=" + pubTime
		+ ", updatePrompt=" + updatePrompt + ", updateDesc=" + updateDesc + "]";
    }

}
