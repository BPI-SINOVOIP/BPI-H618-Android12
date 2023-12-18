package com.softwinner.dragonbox.utils;

import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.OutputStreamWriter;
import java.net.HttpURLConnection;
import java.net.MalformedURLException;
import java.net.URL;
import java.net.URLConnection;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.List;
import java.util.Locale;
import java.lang.Exception;
import com.softwinner.dragonbox.entity.NetReceivedResult;

import android.content.Context;
import android.net.ConnectivityManager;
import android.net.wifi.ScanResult;
import android.net.wifi.SupplicantState;
import android.net.wifi.WifiConfiguration;
import android.net.wifi.WifiInfo;
import android.net.wifi.WifiConfiguration.KeyMgmt;
import android.net.wifi.WifiManager;
import android.util.Log;
import com.softwinner.dragonbox.entity.MES_RequestBean;
import com.softwinner.dragonbox.utils.MES_Utils;
import java.io.OutputStream;
import java.util.HashMap;
import java.io.ByteArrayOutputStream;
import java.util.HashMap;
import java.lang.StringBuilder;
import java.io.FileReader;

public class NetUtil {

    public static final String TAG="DragonBox-NetUtil";

    public static NetReceivedResult getURLContentByPost(String urlStr, String sCommand,String[] arrSParams) {
        String msg = "";
        NetReceivedResult netReceivedResult = new NetReceivedResult();
        try {
            URL url = new URL(urlStr);
            HttpURLConnection conn = (HttpURLConnection) url.openConnection();
            conn.setDoOutput(true);//设置是否向conn输出，因为这个是post请求，参数需要放在http正文内，需要设为TRUE，默认false
            conn.setDoInput(true);//设置是否从conn读取，默认true。
            conn.setUseCaches(false);//post请求不能使用缓存
            conn.setConnectTimeout(5*1000);
            conn.setReadTimeout(5*1000);
            conn.setInstanceFollowRedirects(true);//设置是否自动执行http重定向。
            conn.setRequestProperty("Content-Type", "text/xml; charset=utf-8");
            conn.setRequestProperty("SOAPAction", "http://tempuri.org/"+sCommand);
            conn.setRequestMethod("POST");
            conn.connect();//连接

            BufferedWriter bw = new BufferedWriter(new OutputStreamWriter(conn.getOutputStream()));
            bw.write(SOAPUtil.getXml(sCommand, arrSParams));
            bw.flush();
            bw.close();

            int code = conn.getResponseCode();
            Log.e(TAG,"code = "+code);
            netReceivedResult.iCode=code;
            netReceivedResult.sResult = code == 200 ? "PASS" : "FAIL";

            if(code==200) {
                InputStream inputStream = conn.getInputStream();
                XmlUtil.parseXML(inputStream,netReceivedResult);
    /*BufferedReader br = new BufferedReader(new InputStreamReader(conn.getInputStream()));
    String line = "";
    while ((line = br.readLine())!=null) {
     msg += line+"\n";
    }
    br.close();*/
                inputStream.close();
            }else{
                Log.e(TAG,"error code = "+code);
                BufferedReader br = new BufferedReader(new InputStreamReader(conn.getErrorStream()));
                String line = "";
                Log.e(TAG,"msg = " + br.readLine());
                while ((line = br.readLine())!=null) {
                    msg += line+"\n";
                }
                Log.e(TAG,"msg = "+msg);
                br.close();
            }

            conn.disconnect();
            //Log.e(TAG,"msg = "+msg);
            return netReceivedResult;

        } catch (MalformedURLException e) {
            e.printStackTrace();
        } catch (IOException e) {
            e.printStackTrace();
        }
        return netReceivedResult;
    }

    public static NetReceivedResult getURLContentByPost(String urlStr, String sCommand,
                                                        String[] arrSParams,MES_RequestBean requestBean) {
        String user = "";
        if (requestBean != null) {
            user = requestBean.user;
        }
        String msg = "";
        String body = MES_Utils.getSuffix(requestBean);
        Log.d(TAG,"-------request body:" + body);
        NetReceivedResult netReceivedResult = new NetReceivedResult();
        try {
            URL url = new URL(urlStr);
            HttpURLConnection conn = (HttpURLConnection) url.openConnection();
            conn.setDoOutput(true);//设置是否向conn输出，因为这个是post请求，参数需要放在http正文内，需要设为TRUE，默认false
            conn.setDoInput(true);//设置是否从conn读取，默认true。
            conn.setUseCaches(false);//post请求不能使用缓存
            conn.setConnectTimeout(5*1000);
            conn.setReadTimeout(5*1000);
            conn.setInstanceFollowRedirects(true);//设置是否自动执行http重定向。

            //设置请求头
            conn.setRequestMethod("POST");
            conn.setRequestProperty("Schema", "MES");//固定格式
            conn.setRequestProperty("Language", "chs");
            conn.setRequestProperty("User", user);
            conn.setRequestProperty("clientType", "ClientAk.1");//固定格式
            conn.setRequestProperty("Customer", "Efficient");
            conn.setRequestProperty("Content-Type", "application/x-www-form-urlencoded");
            conn.setRequestProperty("Content-Length", String.valueOf(body.length()));

            //连接
            conn.connect();

            OutputStream outputStream = conn.getOutputStream();
            outputStream.write(body.getBytes());
            outputStream.close();

            int code = conn.getResponseCode();
            netReceivedResult.iCode = code;
            netReceivedResult.sResult = code == 200 ? "PASS" : "FAIL";
            Log.d(TAG,"code = " + code+"  sCommand:" + sCommand);
            if(code==200) {
                InputStream inputStream = conn.getInputStream();
                byte[] data = readStream(inputStream);
                inputStream.close();
                String jsonStr = new String(data);
                Log.d(TAG,"success return jsonStr = " + jsonStr);
                if(jsonStr!=null&&jsonStr.contains("error")){
                    netReceivedResult.sResult="FAIL";
                }
            } else {
                BufferedReader br = new BufferedReader(new InputStreamReader(conn.getErrorStream()));
                String line = "";
                while ((line = br.readLine())!=null) {
                    msg += line+"\n";
                }
                br.close();
            }
            Log.d(TAG,"msg = "+msg);
            netReceivedResult.sReason = msg;
            conn.disconnect();
        } catch (Exception e) {
            e.printStackTrace();
        }
        return netReceivedResult;
    }

    public static NetReceivedResult getURLContentByGet(String urlStr,MES_RequestBean requestBean) {
        String user = "";
        if (requestBean != null) {
            user = requestBean.user;
        }
        String msg = "";
        String body = MES_Utils.getSuffix(requestBean);
        String urlGet  = urlStr + body;
        Log.d(TAG,"-------------urlGet :" + urlGet);
        NetReceivedResult netReceivedResult = new NetReceivedResult();
        try {
            URL url = new URL(urlGet);
            HttpURLConnection conn = (HttpURLConnection) url.openConnection();
            conn.setDoOutput(false);
            conn.setDoInput(true);
            conn.setConnectTimeout(5*1000);
            conn.setReadTimeout(5*1000);
            conn.setInstanceFollowRedirects(true);
            conn.setRequestMethod("GET");
            conn.setRequestProperty("Schema", "MES");
            conn.setRequestProperty("Language", "chs");
            conn.setRequestProperty("User", user);
            conn.setRequestProperty("clientType", "ClientAk.1");
            conn.setRequestProperty("Customer", "Efficient");
            conn.connect();
            int code = conn.getResponseCode();
            netReceivedResult.iCode = code;
            netReceivedResult.sResult = code == 200 ? "PASS" : "FAIL";
            Log.d(TAG,"-------------reques code:" + code);
            if(code==200) {
                InputStream inputStream = conn.getInputStream();
                byte[] data = readStream(inputStream);
                inputStream.close();
                String jsonStr = new String(data);
                Log.d(TAG,"http return = " + jsonStr);
                if(jsonStr!=null&&jsonStr.contains("error")){
                    netReceivedResult.sResult="FAIL";
                }
            } else {
                BufferedReader br = new BufferedReader(new InputStreamReader(conn.getErrorStream()));
                String line = "";
                while ((line = br.readLine())!=null) {
                    msg += line+"\n";
                }
                br.close();
            }
            Log.d(TAG,"msg = "+msg);
            netReceivedResult.sReason = msg;
            conn.disconnect();
        } catch (Exception e) {
            e.printStackTrace();
        }
        Log.d(TAG,"-------------msg :" + msg);
        return netReceivedResult;
    }

    public static byte[] readStream(InputStream inputStream) throws Exception {
        ByteArrayOutputStream bout = new ByteArrayOutputStream();
        byte[] buffer = new byte[1024];
        int len = 0;
        while ((len = inputStream.read(buffer)) != -1) {
            bout.write(buffer, 0, len);
        }
        bout.close();
        return bout.toByteArray();
    }

    public static void connectWifi(Context context,String ssid,String password) {
        WifiManager wifiManager = (WifiManager) context.getSystemService(Context.WIFI_SERVICE);
        if(!wifiManager.isWifiEnabled())
            wifiManager.setWifiEnabled(true);
        WifiInfo wifiInfo = wifiManager.getConnectionInfo();
        Log.e(TAG,"check ssid whether already connected: "+wifiInfo.getSSID());
        if(wifiInfo!=null&&wifiInfo.getSSID().equals("\""+ssid+"\"")&&wifiInfo.getSupplicantState()==SupplicantState.COMPLETED) {
            Log.e(TAG,"Wifi has connected the target ssid:"+ssid);
            wifiManager.disconnect();
            //return ;
        }
        List<WifiConfiguration> lstWifiConfiguration = wifiManager.getConfiguredNetworks();
        if (lstWifiConfiguration != null && lstWifiConfiguration.size() > 0) {
            for (WifiConfiguration wifiConfig : lstWifiConfiguration) {
                wifiManager.removeNetwork(wifiConfig.networkId);
            }
        }
        WifiConfiguration wifiConfiguration = new WifiConfiguration();
        wifiConfiguration.SSID="\""+ssid+"\"";//小写的L
        wifiConfiguration.preSharedKey="\""+password+"\"";
        int netId = wifiManager.addNetwork(wifiConfiguration);
        wifiManager.enableNetwork(netId, true);
    }

    public static void forgetWifi(Context context) {
        WifiManager wifiManager = (WifiManager) context.getSystemService(Context.WIFI_SERVICE);
        List<WifiConfiguration> lstWifiConfiguration = wifiManager.getConfiguredNetworks();
        if (lstWifiConfiguration != null && lstWifiConfiguration.size() > 0) {
            for (WifiConfiguration wifiConfig : lstWifiConfiguration) {
                wifiManager.forget(wifiConfig.networkId,null);
            }
        }
    }

    public static String getWebsiteDatetime(String timeUrl) {
        try {
            URL url = new URL(timeUrl);
            URLConnection uc = url.openConnection();
            uc.connect();
            long ld = uc.getDate();
            Date date = new Date(ld);
            //上传年月日时分秒的格式不能被服务器解析，但年月日能被服务器解析。
            SimpleDateFormat sdf = new SimpleDateFormat("yyyy-MM-dd", Locale.CHINA);
            return sdf.format(date);
        } catch (Exception e) {
            // TODO: handle exception
        }
        return "1970-01-01 00:00:00";
    }

    public static String getMacAddr() {
        String mac = "";
        try {
            mac = getLocalMac().toUpperCase().substring(0, 17);
            return mac;
        } catch (IOException e) {
            e.printStackTrace();
        }
        return "02:00:00:00:00:00";
    }

    public static String getWifiMac(Context context){
        if(context == null) return null;
        String mac = "";
        WifiManager wm = (WifiManager) context.getSystemService(Context.WIFI_SERVICE);
        if (wm != null) {
            WifiInfo winfo = wm.getConnectionInfo();
            if (winfo != null) {
                mac = winfo.getBSSID();
            }
        }
        return mac;
    }

    private static String getLocalMac() throws java.io.IOException {
        FileReader reader = new FileReader("/sys/class/net/eth0/address");
        BufferedReader br = new BufferedReader(reader);
        String mac = br.readLine();
        br.close();
        return mac;
    }

}
