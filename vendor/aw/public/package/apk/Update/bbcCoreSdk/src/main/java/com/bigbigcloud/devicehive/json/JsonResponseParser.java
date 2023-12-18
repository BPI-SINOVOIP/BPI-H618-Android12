package com.bigbigcloud.devicehive.json;

import com.google.gson.Gson;
import com.google.gson.JsonElement;
import com.google.gson.JsonParser;

import org.xutils.http.app.ResponseParser;
import org.xutils.http.request.UriRequest;

import java.lang.reflect.Type;

/**
 * Created by Administrator on 2016/3/24.
 */
public class JsonResponseParser implements ResponseParser {
    public JsonResponseParser() {
        super();
    }

    @Override
    public void checkResponse(UriRequest request) throws Throwable {

    }

    @Override
    public Object parse(Type resultType, Class<?> resultClass, String result) throws Throwable {
        Gson gson = GsonFactory.createGson();
        JsonElement jsonElement = new JsonParser().parse(result);
        return gson.fromJson(jsonElement, resultType);
    }
}
