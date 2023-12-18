package com.bigbigcloud.devicehive.json;


import com.google.gson.FieldNamingPolicy;
import com.google.gson.Gson;
import com.google.gson.GsonBuilder;

/**
 * Created by Administrator on 2015/10/22.
 */

public class GsonFactory {
    public static Gson createGson() {
        return gson;
    }
    private final static Gson gson;

    static {
        GsonBuilder builder = new GsonBuilder();
        builder.setFieldNamingPolicy(FieldNamingPolicy.IDENTITY);
        builder.registerTypeAdapterFactory(new JsonStringWrapperAdapterFactory());
        gson = builder.create();
    }
}
