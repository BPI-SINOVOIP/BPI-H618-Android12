package com.softwinner.tvdsetting.weather;

import java.io.IOException;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.Callable;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.FutureTask;
import android.util.Log;
import java.util.Map;
import java.io.InputStreamReader;
import java.net.ConnectException;
import java.net.URL;
import java.net.URLConnection;
import java.io.BufferedReader;

import org.apache.http.HttpResponse;
import org.apache.http.client.HttpClient;
import org.apache.http.client.methods.HttpGet;
import org.apache.http.impl.client.DefaultHttpClient;
import org.apache.http.util.EntityUtils;
import org.json.JSONException;
import org.json.JSONObject;
import org.ksoap2.SoapEnvelope;
import org.ksoap2.serialization.SoapObject;
import org.ksoap2.serialization.SoapSerializationEnvelope;
import org.ksoap2.transport.HttpTransportSE;
import org.xmlpull.v1.XmlPullParserException;

public class WeatherCityHelper {

	static final String SERVICE_NS = "http://WebXml.com.cn/";
	static final String SERVICE_URL = "http://webservice.webxml.com.cn/WebServices/WeatherWS.asmx";

	public static List<String> getProvinceList() {
		String methodName = "getRegionProvince";
		HttpTransportSE ht = new HttpTransportSE(SERVICE_URL);
		ht.debug = true;
		SoapSerializationEnvelope envelope = new SoapSerializationEnvelope(
				SoapEnvelope.VER11);
		SoapObject soapObject = new SoapObject(SERVICE_NS, methodName);
		envelope.bodyOut = soapObject;
		envelope.dotNet = true;
		try {
			ht.call(SERVICE_NS + methodName, envelope);
			if (envelope.getResponse() != null) {
				SoapObject result = (SoapObject) envelope.bodyIn;
				SoapObject detail = (SoapObject) result.getProperty(methodName
						+ "Result");
				return parseProvinceOrCity(detail);
			}
		} catch (IOException e) {
			e.printStackTrace();
		} catch (XmlPullParserException e) {
			e.printStackTrace();
		}
		return null;
	}

	public static List<String> getCityListByProvince(String province) {

		String methodName = "getSupportCityString";
		HttpTransportSE ht = new HttpTransportSE(SERVICE_URL);
		ht.debug = true;
		SoapObject soapObject = new SoapObject(SERVICE_NS, methodName);
		soapObject.addProperty("theRegionCode", province);
		SoapSerializationEnvelope envelope = new SoapSerializationEnvelope(
				SoapEnvelope.VER11);
		envelope.bodyOut = soapObject;
		envelope.dotNet = true;
		try {
			ht.call(SERVICE_NS + methodName, envelope);
			if (envelope.getResponse() != null) {
				SoapObject result = (SoapObject) envelope.bodyIn;
				SoapObject detail = (SoapObject) result.getProperty(methodName
						+ "Result");
				return parseProvinceOrCity(detail);
			}
		} catch (IOException e) {
			e.printStackTrace();
		} catch (XmlPullParserException e) {
			e.printStackTrace();
		}
		return null;
	}

	private static List<String> parseProvinceOrCity(SoapObject detail) {
		List<String> result = new ArrayList<String>();
		for (int i = 0; i < detail.getPropertyCount(); i++) {
			result.add(detail.getProperty(i).toString().split(",")[0]);
		}
		return result;
	}

	private static HttpClient httpClient = new DefaultHttpClient();

	private static String getRequest(String inputStr, boolean isWeacher)
			throws InterruptedException, ExecutionException {
		final String url;
		if (isWeacher) {
			url = "http://www.weather.com.cn/data/cityinfo/" + inputStr + ".html";
		} else {
			url = inputStr;
		}
		FutureTask<String> task = new FutureTask<String>(
				new Callable<String>() {
					@Override
					public String call() throws Exception {
						HttpGet get = new HttpGet(url);
						HttpResponse httpResponse = httpClient.execute(get);
						if (httpResponse.getStatusLine().getStatusCode() == 200) {
							return EntityUtils.toString(httpResponse
									.getEntity());
						}
						return null;
					}
				});
		new Thread(task).start();
		return task.get();
	}

	private static String getRequest(String inputStr) {

		String result = "";
		BufferedReader in = null;
		try {
			String urlName = "http://www.weather.com.cn/data/cityinfo/"
					+ inputStr + ".html";
			URL realUrl = new URL(urlName);
			// 打开和URL之间的连接
			URLConnection conn = realUrl.openConnection();
			// 设置通用的请求属性
			conn.setRequestProperty("accept", "*/*");
			conn.setRequestProperty("connection", "Keep-Alive");
			conn.setRequestProperty("user-agent",
					"Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 5.1; SV1)");
			conn.setConnectTimeout(5000);
			// 建立实际的连接
			conn.connect();
			
			// 获取所有响应头字段
			// Map<String, List<String>> map = conn.getHeaderFields();
			// 遍历所有的响应头字段
			// for (String key : map.keySet()) {
			// System.out.println(key + "--->" + map.get(key));
			// }
			
			// 定义BufferedReader输入流来读取URL的响应
			in = new BufferedReader(
					new InputStreamReader(conn.getInputStream()));
			String line;
			while ((line = in.readLine()) != null) {
				result += "\n" + line;
			}
		} catch (Exception e) {
			System.out.println("发送GET请求出现异常！" + e);
			e.printStackTrace();
		} finally { // 使用finally块来关闭输入流
			try {
				if (in != null) {
					in.close();
				}
			} catch (IOException ex) {
				ex.printStackTrace();
			}
		}
		Log.e("Ningxz", "result = " + result);
		return result;
	}

	public static String[] getRoughlyLocation() {
		String location[] = new String[4];
		String url = "http://int.dpool.sina.com.cn/iplookup/iplookup.php?format=json&ip=";
		String json = null;
		try {
			json = getRequest(url, false);
		} catch (InterruptedException e1) {
			// TODO Auto-generated catch block
			e1.printStackTrace();
		} catch (ExecutionException e1) {
			// TODO Auto-generated catch block
			e1.printStackTrace();
		}
		if (json == null) {
			return location;
		}
		try {
			JSONObject object = new JSONObject(json);
			location[0] = object.getString("country");
			location[1] = object.getString("province");
			location[2] = object.getString("city");
			location[3] = object.getString("district");
		} catch (JSONException e) {
			e.printStackTrace();
			return location;
		} catch (Exception e) {
			e.printStackTrace();
		}
		return location;
	}

	public static String getWeatherByCityCode1(String cityCode) {

		return getRequest(cityCode);
	}

	public static String getWeatherByCityCode(String cityCode) {
		String result = null;
		try {
			result = getRequest(cityCode, true);
		} catch (InterruptedException e) {
			e.printStackTrace();
		} catch (ExecutionException e) {
			e.printStackTrace();
		}
		return result;
	}

	public static SoapObject getWeatherByCity(String cityName) {

		String methodName = "getWeather";
		HttpTransportSE ht = new HttpTransportSE(SERVICE_URL);
		ht.debug = true;
		SoapSerializationEnvelope envelope = new SoapSerializationEnvelope(
				SoapEnvelope.VER11);
		SoapObject soapObject = new SoapObject(SERVICE_NS, methodName);
		soapObject.addProperty("theCityCode", cityName);
		envelope.bodyOut = soapObject;
		envelope.dotNet = true;
		try {
			ht.call(SERVICE_NS + methodName, envelope);
			SoapObject result = (SoapObject) envelope.bodyIn;
			SoapObject detail = (SoapObject) result.getProperty(methodName
					+ "Result");
			return detail;
		} catch (Exception e) {
			e.printStackTrace();
		}
		return null;
	}
}
