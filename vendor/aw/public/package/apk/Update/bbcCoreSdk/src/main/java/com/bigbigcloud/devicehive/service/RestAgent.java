package com.bigbigcloud.devicehive.service;

import android.util.Base64;
import android.util.Log;

import com.bigbigcloud.devicehive.json.GsonFactory;
import com.bigbigcloud.devicehive.utils.Config;
import com.bigbigcloud.devicehive.utils.Utils;
import com.google.gson.JsonObject;

import org.xutils.common.Callback;
import org.xutils.http.HttpMethod;
import org.xutils.http.RequestParams;
import org.xutils.x;

import java.nio.charset.Charset;
import java.util.Map;

/**
 * Created by Administrator on 2016/2/22.
 */
public class RestAgent {
    private static final String TAG = "RestAgent";
    private static final String USER_AUTH_SCHEMA = "Basic";
    private static final String KEY_AUTH_SCHEMA = "Bearer";
    public static final String AUTHORIZATION = "Authorization";
    private static final String AUTH_APPID_HEADER = "Auth-AppId";
    public static final String AUTH_APPTOKEN_HEADER = "AW-APP-User-Token";
    public static final String AUTH_TIMESTAMP_HEADER = "Auth-Timestamp";
    public static final String AUTH_SIGNATURE_HEADER = "Auth-Signature";
    public static final String AUTH_SIGNATURE_PLATFORM = "Platform-Type";

    private static final Charset UTF8_CHARSET = Charset.forName("UTF-8");

    private  String restUri;

    protected Principal principal;

    public RestAgent(){}

    public RestAgent(String restUri){
        this.restUri = restUri;
    }

    public void setPrincipal(final Principal principal){
        this.principal = principal;
    }

    private void addAuthHeaders(RequestParams requestParams){
        long timestamp = System.currentTimeMillis()/1000;
        if(principal != null){
            if(principal.isUser()){
                String decodeStr = principal.getPrincipal().first + ":" + principal.getPrincipal().second;
                String encodeStr = Base64.encodeToString(decodeStr.getBytes(UTF8_CHARSET),0);
                requestParams.addHeader(AUTHORIZATION, USER_AUTH_SCHEMA + " " + encodeStr);
            } else if (principal.isDevice()){
                String signature = Utils.SHA1(principal.getPrincipal().first + timestamp + principal.getPrincipal().second);
                requestParams.addHeader(AUTH_TIMESTAMP_HEADER, String.valueOf(timestamp));
                requestParams.addHeader(AUTH_SIGNATURE_HEADER, signature);
                Log.d(TAG, " timestamp " + timestamp + " signature " + signature);
            }else if (principal.isAccessToken()){
                requestParams.addHeader(AUTH_APPID_HEADER, principal.getPrincipal().first);
                requestParams.addHeader(AUTH_APPTOKEN_HEADER, principal.getPrincipal().second);
                requestParams.addHeader(AUTH_SIGNATURE_PLATFORM, Config.PLATFORM);
                Log.d(TAG, "use token to authenticate !!! appid " + principal.getPrincipal().first + " token " + principal.getPrincipal().second);
            }else if(principal.isAppClient()){
                String decodeStr = principal.getPrincipal().first + timestamp + principal.getPrincipal().second;
                Log.d(TAG, "original signature " + decodeStr);
                String signature = Utils.SHA1(decodeStr);
                requestParams.addHeader(AUTH_SIGNATURE_HEADER, signature);
                requestParams.addHeader(AUTH_APPID_HEADER, principal.getPrincipal().first);
                requestParams.addHeader(AUTH_TIMESTAMP_HEADER, String.valueOf(timestamp));
                requestParams.addHeader(AUTH_SIGNATURE_PLATFORM, Config.PLATFORM);
                Log.d(TAG, " add client auth " + signature);
            }
        }
    }

    private void addCommonHeaders(RequestParams requestParams){
        requestParams.setAsJsonContent(true);
        requestParams.setHeader("Content-Type", "application/json; charset=utf-8");
        Log.d("YANG", " add content- type ");
    }


    public <T> void execute(String path, HttpMethod method, Callback.CommonCallback callback){
        RequestParams requestParams = new RequestParams(restUri + path);//("http://httpbin.org/headers");
        requestParams.setMethod(method);
        addAuthHeaders(requestParams);
        addCommonHeaders(requestParams);
        Log.d("yang", requestParams.toString());
        x.http().request(method, requestParams, callback);
    }

    public <T> void execute(String path, HttpMethod method, Map<String, String> headers,
                            Map<String, String> queryParams,  Callback.CommonCallback callback){
        RequestParams requestParams = new RequestParams(restUri + path);
        requestParams.setMethod(method);
        addAuthHeaders(requestParams);
        addCommonHeaders(requestParams);
        if(headers != null) {
            for (Map.Entry<String, String> entry : headers.entrySet()) {
                requestParams.addHeader(entry.getKey(), entry.getValue());
            }
        }
        if (queryParams != null){
            if(method == HttpMethod.GET){
                for (Map.Entry<String, String> entry : queryParams.entrySet()){
                    requestParams.addQueryStringParameter(entry.getKey(), entry.getValue());
                }
            }else {
                for (Map.Entry<String, String> entry : queryParams.entrySet()){
                    requestParams.addBodyParameter(entry.getKey(), entry.getValue());
                }
            }
        }
        x.http().request(method, requestParams, callback);
    }

    public <S,T> void execute(String path, HttpMethod method, Map<String, String> headers,S objectToSend, Callback.CommonCallback callback){
        RequestParams requestParams = new RequestParams(restUri + path);
        if(headers != null) {
            for (Map.Entry<String, String> entry : headers.entrySet()) {
                requestParams.addHeader(entry.getKey(), entry.getValue());
            }
        }
        requestParams.setMethod(method);
        addAuthHeaders(requestParams);
        addCommonHeaders(requestParams);
        String body ;
        if(objectToSend instanceof  String){
            body = (String)objectToSend;
        }else if (objectToSend instanceof JsonObject){
            body = ((JsonObject)objectToSend).toString();
        }else{
            body = GsonFactory.createGson().toJson(objectToSend);
        }
        Log.d("YANG", " body " + body);
        requestParams.setBodyContent(body);
        x.http().request(method, requestParams, callback);
    }

    public <T> T executeSync(String path, HttpMethod method, Map<String, String> headers,
                            Map<String, String> queryParams,Class<T> resultType) throws Throwable{
        RequestParams requestParams = new RequestParams(restUri + path);
        requestParams.setMethod(method);
        addAuthHeaders(requestParams);
        addCommonHeaders(requestParams);
        if(headers != null) {
            for (Map.Entry<String, String> entry : headers.entrySet()) {
                requestParams.addHeader(entry.getKey(), entry.getValue());
            }
        }
        if (queryParams != null){
            for (Map.Entry<String, String> entry : queryParams.entrySet()){
                requestParams.addQueryStringParameter(entry.getKey(), entry.getValue());
            }
        }
        return x.http().requestSync(method, requestParams, resultType);
    }

    public <S,T> T executeSync(String path, HttpMethod method, S objectToSend,Class<T> resultType) throws Throwable{
        RequestParams requestParams = new RequestParams(restUri + path);
        requestParams.setMethod(method);
        addAuthHeaders(requestParams);
        addCommonHeaders(requestParams);
        String body;
        if(objectToSend instanceof  String){
            body = (String)objectToSend;
        }else if (objectToSend instanceof JsonObject){
            body = ((JsonObject)objectToSend).toString();
        }else{
            body = GsonFactory.createGson().toJson(objectToSend);
        }
        Log.d("YANG", " body " + body);
        requestParams.setBodyContent(body);
        return x.http().requestSync(method, requestParams, resultType);
    }

}
