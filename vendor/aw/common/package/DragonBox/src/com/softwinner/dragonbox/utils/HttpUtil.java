package com.softwinner.dragonbox.utils;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.UnsupportedEncodingException;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import org.apache.http.HttpEntity;
import org.apache.http.HttpResponse;
import org.apache.http.NameValuePair;
import org.apache.http.client.ClientProtocolException;
import org.apache.http.client.HttpClient;
import org.apache.http.client.entity.UrlEncodedFormEntity;
import org.apache.http.client.methods.HttpPost;
import org.apache.http.impl.client.DefaultHttpClient;
import org.apache.http.protocol.HTTP;

import android.util.Log;

public class HttpUtil {

    public static final String requestRouter(String url,
                                             List<NameValuePair> pairList) {
        HttpClient httpClient = new DefaultHttpClient();
        HttpPost httpPost = new HttpPost(url);
        HttpResponse response;
        try {
            if (pairList != null) {
                httpPost.setEntity(new UrlEncodedFormEntity(pairList, HTTP.UTF_8));
            }
            response = httpClient.execute(httpPost);

            if (response.getStatusLine().getStatusCode() == 200) {
                // 取出回应字串
                return parseEntity(response.getEntity());
            } else {
                return response.getStatusLine().toString();
            }

        } catch (ClientProtocolException e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
        } catch (Exception e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
        }
        return "";
    }

    private static final String parseEntity(HttpEntity httpEntity) {
        String result = "";
        try {
            InputStream inputStream = httpEntity.getContent();
            BufferedReader reader = new BufferedReader(new InputStreamReader(
                    inputStream));

            String line = "";
            while (null != (line = reader.readLine())) {
                result += line;
            }

        } catch (Exception e) {
            e.printStackTrace();
        }
        Log.d("HttpUtil", "parseEntity=" + result);
        return result;
    }

    public static final boolean parseBooleanResult(String body){
        return body.contains("successful");
    }

    public static final Map<String, String> parseMapResult(String body){
        String sub = body.substring("<body>".length(), body.length()-"</body>".length());
        String[] split = sub.split(",");
        Map<String,String> result = new HashMap<String,String>();
        for (String str : split) {
            String[] strs = str.split("=", 2);
            if (strs.length == 2) {
                result.put(strs[0], strs[1]);
            }
        }
        return result;
    }


    public static final boolean ping(String ipAddr) {
        String result = null;
        try {
            Process p = Runtime.getRuntime().exec("ping -c 3 -w 100 " + ipAddr);// ping5次
            InputStream input = p.getInputStream();
            BufferedReader in = new BufferedReader(new InputStreamReader(input));
            StringBuffer stringBuffer = new StringBuffer();
            String content = "";
            while ((content = in.readLine()) != null) {
                stringBuffer.append(content);
            }
            Log.i("HttpUtil", "result content : " + stringBuffer.toString());
            // PING的状态
            int status = p.waitFor();
            if (status == 0) {
                result = "successful~";
                return true;
            } else {
                result = "failed~ cannot reach the IP address";
            }

        } catch (IOException e) {
            e.printStackTrace();
            result = "failed~ IOException";
        } catch (InterruptedException e) {
            e.printStackTrace();
            result = "failed~ InterruptedException";
        } finally {
            Log.i("HttpUtil", "result = " + result);
        }
        return false;
    }


}
