package com.bigbigcloud.devicehive.api;

import com.bigbigcloud.devicehive.entity.DeviceCommand;

/**
 * Created by Thomas.yang on 2016/7/7.
 * BbcClient 端的接口类，主要封装了client app 端的api接口
 */
public interface BbcClient {
    /*
    *获取短信验证码
    * @param phoneNum 手机号码
    * @action 1注册：register，2快捷登录：login，3绑定手机号：bindPhone,4重置密码：resetpwd
     */
    public void getSmsCode(String phoneNum, String action, IUiCallback uiCallback);

    /*
    *获取邮件验证码
    * @param email 邮箱地址
    * @param action 1注册：register，2 绑定邮箱：bindEmail,3 重置密码：resetpwd
     */
    public void getEmailCode(String email,String action,IUiCallback uiCallback);

    /*
    *获取用户信息
    * @param id  用户账号user的Id
     */
    public void getUser(long id, IUiCallback uiCallback);

    /*
    *手机号码注册
    * @param phone 手机号码
    * @param password  密码的MD5值
    * @param verifyCode 手机验证码
     */
    public void phoneRegister(String phone, String password, String verifyCode, IUiCallback uiCallback);

    /*
    * 邮箱注册
    * @param email 邮箱地址
    * @param password 密码的MD5值
    * @param verifyCode 邮箱验证码
     */
    public void emailRegister(String email, String password, String verifyCode, IUiCallback uiCallback);

    /*
    * @param nickName 用户昵称
    * @param headImgurl 用户头像地址
    * @param thirdLogin 第三方平台的用户唯一标识Id
    * @accessToken 第三方平台登录的accessToken
    * @thirdType 第三方平台类型 　　微信公众号：wechat，微信：weixin，QQ：qq，微博：weibo
     */
    public void thirdTypeRegister(String nickName,String headImgurl,
                                  String thirdLogin, String accessToken,
                                  String thirdType, IUiCallback uiCallback);

    /*
    * 用户登录接口
    * @param userName 手机号/账号/邮箱
    * @param password  密码的MD5值
     */
    public void userLogin(String userName, String password, IUiCallback uiCallback);

    /*
    * 手机验证码快速登录
    * @param phone 手机号码
    * @param verifyCode 手机验证码
     */
    public void verifiedCodeLogin(String phone, String verifyCode, IUiCallback uiCallback);

    /*
    *更新用户信息接口
    * @param userId  用户账号user的Id
    * @param nickName 用户昵称
    * @param headImgurl 用户头像地址
    * @param sex  0 ：位置，　１：男　２：女
     */
    public void updateUser(long userId,String nickName, String headImgurl, int sex, IUiCallback uiCallback);

    /*
    *绑定手机
    *@param userId 用户账号user的Id
    * @param verifyCode 手机验证码
    * @param password  密码的MD5值
     */
    public void bindToPhone(long userId,String phone, String verifyCode, String password,IUiCallback uiCallback);

    /*
    * 绑定邮箱
    * @param userId 用户账号user的Id
    * @param verifyCode 邮箱验证码
    * @param password  密码的MD5值
     */
    public void bindToEmail(long userId, String email, String verifyCode, String password, IUiCallback uiCallback);

    /*
    * 修改密码
    * @param userId 用户账号user的Id
    * @param oldPass 旧密码的MD5值
    * @param newPass 新密码的MD5值
     */
    public void changePassWord(long userId,String oldPass, String newPass,IUiCallback uiCallback);

    /*
    * 重置密码
    * @param phone 手机号
    * @param verifyCode 手机验证码
    * @param password  密码的MD5值
     */
    public void  resetPasswordByPhone(String phone, String verifyCode,String password,IUiCallback uiCallback);

    /*
   * 重置密码
   * @param email 邮箱
   * @param verifyCode 邮箱验证码
   * @param password  密码的MD5值
    */
    public void resetPasswordByEmail(String email, String verifyCode,String password,IUiCallback uiCallback);

    /*
    * 绑定设备
    *  @param userId 用户账号user的Id
    *  @param deviceGuid 设备的Guid值
     */
    public void bindDevice(long userId, String deviceGuid, IUiCallback uiCallback);

    /*
     *  @param userId 用户账号user的Id
    *  @param deviceId 设备的自身id值
    *  @param vendor 厂商名
    *  @param deviceClassName 设备类型名
     */
    public void bindDevice(long userId, String deviceId, String vendor, String deviceClassName, IUiCallback uiCallback);

    /*
    * 解除用户与设备的绑定关系
    * @param userId 用户账号user的Id
    *  @param deviceGuid 设备的Guid值
     */
    public void unBindDevice(long userId, String deviceGuid, IUiCallback uiCallback);

    /*
    * 获取用户的绑定设备列表信息
    * @param userId 用户账号user的Id
     */
    public void getBindDevices(long userId, IUiCallback uiCallback);

    /*
    * 向设备发送command 信息
    * @param deviceGuid 设备的Guid
    * @param deviceComand DeviceCommand实例，具体参考DeviceCommand类
     */
    public void sendCommand(String deviceGuid, DeviceCommand deviceCommand, IUiCallback uiCallback);


}
