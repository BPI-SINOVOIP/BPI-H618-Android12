package com.bigbigcloud.devicehive.utils;

/**
 * Created by Administrator on 2015/12/24.
 */
public class Config {
    public static final String DEFAULT_WEBSOCKET_URI = "ws://ws.bigbigcloud.cn/dh/v2/websocket";
    public static final String DEFAULT_RESTFUL_URI = "http://api.bigbigcloud.cn/dh/v2/rest";
    public static final String SDK_VERSION = "0.8.3";
    public static final String PLATFORM = "android";

    private String restUrl;
    private String websocketUrl;

    private Config(){
        restUrl = DEFAULT_RESTFUL_URI;
        websocketUrl = DEFAULT_WEBSOCKET_URI;
    }

    private static class SingletonHoder{
        public static Config INSTANCE = new Config();
    }

    public static Config getInstance(){
        return SingletonHoder.INSTANCE;
    }

    public void setUrl(String restUrl, String websocketUrl){
        this.restUrl = restUrl;
        this.websocketUrl = websocketUrl;
    }

    public String getWebsocketUri(){
        return websocketUrl;
    }

    public String getRestfulUri(){
        return restUrl;
    }

}
