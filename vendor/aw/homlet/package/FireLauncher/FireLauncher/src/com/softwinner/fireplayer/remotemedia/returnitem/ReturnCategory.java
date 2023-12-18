package com.softwinner.fireplayer.remotemedia.returnitem;

//public class ReturnCategory {
//	public String[] category_list;
//	public String[] year_list;
//	public String[] area_list;
//}


public class ReturnCategory {
	public CategoryInfo[] category;
	
	public static class CategoryInfo{
		public String[] area;
		public int name;
		public String []type;
		public String[]year;
	}
} 
