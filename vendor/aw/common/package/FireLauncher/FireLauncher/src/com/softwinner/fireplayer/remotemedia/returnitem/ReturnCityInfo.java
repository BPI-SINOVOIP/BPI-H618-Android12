package com.softwinner.fireplayer.remotemedia.returnitem;


public class ReturnCityInfo {
	
	/*
	 * var remote_ip_info = 
	 * {"ret":1,
	 *  "start":-1,
	 *  "end":-1,
	 *  "country":"中国",
	 *  "province":"广东",
	 *  "city":"珠海",
	 *  "district":"",
	 *  "isp":"",
	 *  "type":"",
	 *  "desc":""};
	 * 
	 * */
	
	public CityInfo cityInfo;
	
	public static class CityInfo{
		public String ret;
		public String start;
		public String end;
		public String country;
		public String province;
		public String city;
		public String district;
		public String isp;
		public String type;
		public String desc;
	}
}
