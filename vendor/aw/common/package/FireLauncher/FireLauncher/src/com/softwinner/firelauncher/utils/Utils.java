package com.softwinner.firelauncher.utils;

import java.io.BufferedOutputStream;
import java.io.BufferedReader;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.nio.charset.Charset;
import java.util.Calendar;

import android.content.Context;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.net.NetworkInfo.State;
import android.util.Log;
import android.view.View;

public class Utils {
	static public final String TAG = "Utils";
	
	public static void printCallBackStack() {
		Throwable ex = new Throwable();
		StackTraceElement[] stackElements = ex.getStackTrace();
		if (stackElements != null) {
			System.out.println("-----------------printCallBackStack------------------");
			for (int i = 0; i < stackElements.length; i++) {
				System.out.print(">>>> " + stackElements[i].getClassName() + "/");
				System.out.print(stackElements[i].getFileName() + "/line:");
				System.out.print(stackElements[i].getLineNumber() + "/");
				System.out.println(stackElements[i].getMethodName());
			}
		}
	}
	
	/**
	 * 根据手机的分辨率从 dp 的单位 转成为 px(像素)
	 */
	static public int dip2px(Context context, float dpValue) {
		float scale = context.getResources().getDisplayMetrics().density;
		return (int) (dpValue * scale + 0.5f);
	}

	/**
	 * 根据手机的分辨率从 px(像素) 的单位 转成为 dp
	 */
	static public int px2dip(Context context,float pxValue) {
		float scale = context.getResources().getDisplayMetrics().density;
		return (int) (pxValue / scale + 0.5f);
	}
	
	
	public static String convertStreamToString(InputStream is) {
	    BufferedReader reader = new BufferedReader(new InputStreamReader(is,Charset.forName("UTF-8")));
	    StringBuilder sb = new StringBuilder();

	    String line = null;
	    try {
	        while ((line = reader.readLine()) != null) {
	            sb.append(line + "\n");
	        }
	    } catch (IOException e) {
	        e.printStackTrace();
	    } finally {
	        try {
	            is.close();
	        } catch (IOException e) {
	            e.printStackTrace();
	        }
	    }
	    return sb.toString();
	}
	 
	public static String appendCustomUrl(String url) {
		url = Constants.SOFT_WINNER_VIDEOS_URL + "category="+Constants.TYPE_TVSERIAL
				+"&pageNo=" + (0+1) + "&pageSize=" + Constants.HTTP_LOAD_PAGE_SIZE ;
		
		String suffix = "version=" + 10004 + "&os=android" + "_pad_" + android.os.Build.VERSION.RELEASE 
				+ "&device=" + android.os.Build.DEVICE;
		String url2 = (url.contains("?") ? url + "&" + suffix : url + "?" + suffix) + Constants.SOFT_WINNER_API_VERSION ;
		Log.v(TAG, ">>>> appendUrl=" + url2);
		return url2;
	}
	
//	public static String appendRecommendUrl() {
//		String url = Constants.MANGO_RECOMMEND + "Func="+Constants.MANGO_RECOMMEND_TYPE 
//				+ "&PageSize="+Constants.MANGO_RECOMMEND_PAGE_SIZE
//				+ "&Version="+Constants.MANGO_VERSION; 
//		Log.d(TAG, ">>>>>>url="+url);
//		return url;
//	}
	
	public static String appendRecommendUrl() {
		String url = Constants.SOFT_WINNER_BASE_URL + "recommendconfig" + "?"+"version=" +Constants.MANGO_RECOMMEND_VERSION;		
		return url;
	}

	
	public static String appendRecommendRollUrl() {
		String url = Constants.SOFT_WINNER_BASE_URL + "recommendroll";
		return url;
	}
	
	public static String appendGameUrl() {
		return Constants.ATET_BASE_URL;
	}
	
	public static String appendChannelsUrl() {
		String url = Constants.SOFT_WINNER_BASE_URL + "tvchannelconfig";
		return url;
	}
	
	public static String appendWeatherUrl(String cityCode){
		String url = Constants.WEATHER_URL + cityCode + ".html";;
		return url;
	}
	
	public static String getCurrTime(Context context){
		String time;
		Calendar calendar = Calendar.getInstance();
		int hour = calendar.get(android.text.format.DateFormat.is24HourFormat(context)? Calendar.HOUR_OF_DAY : Calendar.HOUR);
		if(hour == 0 && 0 == calendar.get(Calendar.AM_PM)){
			hour = 12;
		}
		int minute = calendar.get(Calendar.MINUTE);	
		if(minute < 10)
			time = hour+":0"+minute;
		else
			time = hour+":"+minute;
		return time;
	}
	
	public static void printViewLocation(View view){
		int[] location = new  int[2] ;
	    view.getLocationInWindow(location); //获取在当前窗口内的绝对坐标
	    Log.d(TAG, "窗口内的绝对坐标 view --->x坐标:"+location [0]+"view--->y坐标:"+location [1]);
	    view.getLocationOnScreen(location);//获取在整个屏幕内的绝对坐标
	    Log.d(TAG, "屏幕内的绝对坐标 view --->x坐标:"+location [0]+"view--->y坐标:"+location [1]);
	}
	
	public static boolean isNetworkConnected(Context context) {
		if (context != null) {
			ConnectivityManager mConnectivityManager = (ConnectivityManager) context.getSystemService(Context.CONNECTIVITY_SERVICE);
			NetworkInfo mNetworkInfo = mConnectivityManager.getActiveNetworkInfo();
			if (mNetworkInfo != null && mNetworkInfo.isAvailable()) {
				NetworkInfo networkInfoWifi = mConnectivityManager.getNetworkInfo(ConnectivityManager.TYPE_WIFI);
				NetworkInfo networkInfoEth = mConnectivityManager.getNetworkInfo(ConnectivityManager.TYPE_ETHERNET);
				if (networkInfoEth.getState() == State.CONNECTED) {
					return true;
				} else if (networkInfoWifi.getState() == State.CONNECTED) {
					return true;
				} else {
					return false;
				}
			}
		}
		return false;
	}
	
	public static void writeFile(String path, String content)throws IOException {
		File file = new File(path);
		if(!file.exists()) file.mkdir();
		BufferedOutputStream fos = new BufferedOutputStream(new FileOutputStream(path));
		fos.write(content.getBytes(Charset.forName("UTF-8")));//content.getBytes(Charset.forName("UTF-8"))
		fos.flush();
		fos.close(); 
	}
	
	public static void writeFile(String path, InputStream is)throws IOException {
		byte[] buf = new byte[30720]; // 30k
		File file = new File(path);
		if(!file.exists()) file.mkdir();
		FileOutputStream os = new FileOutputStream(file);
		int count = -1;
		while ((count = is.read(buf)) != -1) {
			os.write(buf, 0, count);
		}
		is.close();
		os.close();
	}
}
