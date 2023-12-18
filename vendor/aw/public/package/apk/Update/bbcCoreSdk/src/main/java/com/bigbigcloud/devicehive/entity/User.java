package com.bigbigcloud.devicehive.entity;

import java.io.Serializable;

/**
 * Represents a user to this API. See <a href="http://www.devicehive.com/restful#Reference/User">User</a>
 */

public class User implements Serializable {

    public static final String WEIXIN = "weixin";
    public static final String WEIBO = "weibo";
    public static final String QQ = "qq";

    private static final long serialVersionUID = -8980491502416082011L;

    private Long userId;

    private String nickName;

    private String headImgurl;

    private String phone;

    private String verifyCode;

    private int sex;//0:未知， 1：男， 2：女

    //第三方登录的用户id
    private String thirdLogin;
    //第三方登录的accessToken
    private String accessToken;

    private String thirdType;

    private String email;

    private String password;


    public User() {
    }

    public Long getUserId() {
        return userId;
    }

    public void setUserId(Long userId) {
        this.userId = userId;
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

    public int getSex() {
        return sex;
    }

    public void setSex(int sex) {
        this.sex = sex;
    }

    public String getPassword() {
        return password;
    }

    public void setPassword(String password) {
        this.password = password;
    }

    public void removePassword() {
        this.password = null;
    }


    public String getAccessToken() {
        return accessToken;
    }

    public void setAccessToken(String accessToken) {
        this.accessToken = accessToken;
    }

    public String getEmail() {
        return email;
    }

    public void setEmail(String email) {
        this.email = email;
    }

    public String getPhone() {
        return phone;
    }

    public void setPhone(String phone) {
        this.phone = phone;
    }

    public String getThirdLogin() {
        return thirdLogin;
    }

    public void setThirdLogin(String thirdLogin) {
        this.thirdLogin = thirdLogin;
    }

    public String getThirdType() {
        return thirdType;
    }

    public void setThirdType(String thirdType) {
        this.thirdType = thirdType;
    }

    public String getVerifyCode() {
        return verifyCode;
    }

    public void setVerifyCode(String verifyCode) {
        this.verifyCode = verifyCode;
    }


    @Override
    public boolean equals(Object o) {
        if (this == o) return true;
        if (o == null || getClass() != o.getClass()) return false;

        User user = (User) o;

        return userId.equals(user.userId);

    }

    @Override
    public int hashCode() {
        return userId.hashCode();
    }

    @Override
    public String toString() {
        return "User{" +
                "accessToken='" + accessToken + '\'' +
                ", userId=" + userId +
                ", nickName='" + nickName + '\'' +
                ", headImgurl='" + headImgurl + '\'' +
                ", phone='" + phone + '\'' +
                ", verifyCode='" + verifyCode + '\'' +
                ", sex=" + sex +
                ", thirdLogin='" + thirdLogin + '\'' +
                ", thirdType='" + thirdType + '\'' +
                ", email='" + email + '\'' +
                ", password='" + password + '\'' +
                '}';
    }
}
