package com.softwinner.fireplayer.remotemedia.returnitem;

public class ReturnWeatherInfo {
/*
 * 
 * {
	    "weatherinfo": {
	        "city": "上海",
	        "cityid": "101020100",
	        "temp1": "13℃",
	        "temp2": "18℃",
	        "weather": "多云",
	        "img1": "n1.gif",
	        "img2": "d1.gif",
	        "ptime": "18:00"
	    }
   }
*/
	public WeatherInfo weatherinfo;
	
	public static class WeatherInfo{
		public String city;
		public String cityid;
		public String temp1;
		public String temp2;
		public String weather;
		public String img1;
		public String img2;
		public String ptime;
	}
	
}
