package com.softwinner.protocol.controller;

public class ErrorMsg
{
    /**
     * 上报网络错误
     */
    public static final String ERROR_MSG_NETWORK = "ERROR MSG NETWORK";

    /**
     * Update错误，没有找到升级包
     */
    public static final String ERROR_UPDATE_NO_PACKAGE = "ERROR UPDATE NO PACKAGE";

    /**
     * Update错误，升级包没有下载完成
     */
    public static final String ERROR_UPDATE_NOT_FINISH_LOAD = "ERROR UPDATE NOT FINISH LOAD";

    /**
     * Update错误，MD5值不匹配
     */
    public static final String ERROR_UPDATE_NO_MATE_MD5 = "ERROR UPDATE NO MATE MD5";

    /**
     * Update错误，安装时出错
     */
    public static final String ERROR_UPDATE_INSTALL_FIALD = "ERROR UPDATE INSTALL FIALD";

    /**
     * Update错误，安装，验证失败
     */
    public static final String ERROR_UPDATE_VERIFY_FAILD = "ERROR UPDATE VERIFY FAILD";

    public static final String ERROR_UPDATE_COPY_FAILD = "ERROR UPDATE COPY FAILD";

    /**
     * download错误，升级包有更新版本
     */
    public static final String ERROR_PACK_INFORMATION_MODIFYED = "ERROR PACK INFORMATION MODIFYED";

    /**
     * download错误，下载地址发生改变
     */
    public static final String ERROR_LOAD_URL_CHANGED = "ERROR DOWNLOAD URL CHANGED";
}
