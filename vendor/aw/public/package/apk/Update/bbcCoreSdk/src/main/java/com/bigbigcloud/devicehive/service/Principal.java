package com.bigbigcloud.devicehive.service;

import android.util.Pair;

/**
 * Created by Administrator on 2016/2/22.
 */
public class Principal {
    private final Pair<String, String> principal;
    private final Type type;

    private enum Type {
        USER, DEVICE, ACCESS_TOKEN, APP_Client
    }

    private Principal(Pair<String, String> principal, Type type) {
        this.principal = principal;
        this.type = type;
    }

    /**
     * Create new hive principal with user credentials.
     *
     * @param login    login
     * @param password password
     * @return new hive principal with user credentials
     */
    public static Principal createUser(String login, String password) {
        return new Principal(Pair.create(login, password), Type.USER);
    }

    /**
     * Create new hive principal with device credentials.
     *
     * @param deviceGuid  device identifier
     * @param licence device key
     * @return new hive principal with device credentials.
     */
    public static Principal createDevice(String deviceGuid, String licence) {
        return new Principal(Pair.create(deviceGuid, licence), Type.DEVICE);
    }

    /**
     * Create new hive principal with access key credentials.
     *
     * @param token access token
     * @return new hive principal with access key credentials
     */
    public static Principal createAccessToken(String appKey,String token) {
        return new Principal(Pair.create(appKey, token), Type.ACCESS_TOKEN);
    }

    public static Principal createAppClient(String appKey, String appSecrect){
        return new Principal(Pair.create(appKey, appSecrect), Type.APP_Client);
    }

    public Pair<String, String> getPrincipal() {
        return principal;
    }

    public boolean isUser() {
        return Type.USER.equals(this.type);
    }

    public boolean isDevice() {
        return Type.DEVICE.equals(this.type);
    }

    public boolean isAccessToken() {
        return Type.ACCESS_TOKEN.equals(this.type);
    }

    public boolean isAppClient(){
        return Type.APP_Client.equals(this.type);
    }

    @Override
    public boolean equals(Object o) {
        if (this == o) return true;
        if (o == null || getClass() != o.getClass()) return false;

        Principal that = (Principal) o;

        if (principal != null ? !principal.equals(that.principal) : that.principal != null) return false;
        if (type != that.type) return false;

        return true;
    }

    @Override
    public int hashCode() {
        int result = principal != null ? principal.hashCode() : 0;
        result = 31 * result + (type != null ? type.hashCode() : 0);
        return result;
    }

}
