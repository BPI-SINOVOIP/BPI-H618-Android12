package com.softwinner.firelauncher.utils;

import java.lang.reflect.Method;

import android.content.Context;

public class ReflectionCall {
	//SystemProperties
    private static ReflectionCall mInstance;
//    private final Context mContext;
    
	private ReflectionCall(Context context) {
//		mContext = context.getApplicationContext();
	}
    
	public static ReflectionCall getInstance(Context context) {
		if(mInstance == null) {
			mInstance = new ReflectionCall(context);
		}
		return mInstance;
	}
	
	public String getString(String className, String methodName, String key){
        String value = null;  
        try { 
        	Class<?> mClassType = Class.forName(className);
        	Method mMethod = mClassType.getDeclaredMethod(methodName, String.class);
            value = (String) mMethod.invoke(mClassType, key);  
        } catch (Exception e) {  
            e.printStackTrace();  
        }  
        return value;  
	}
	
	public String[] getStringArray(String className, String methodName){
		String[] value = null;  
        try { 
        	Class<?> mClassType = Class.forName(className);
        	Method mMethod = mClassType.getDeclaredMethod(methodName);
        	mMethod.setAccessible(true);
        	Object result = mMethod.invoke(mClassType);
          if (result != null && result instanceof String[]) 
        	  value = (String[]) result;
        } catch (Exception e) {  
            e.printStackTrace();  
        }  
        return value;  
	}
	
	public int getInt(String className, String methodName, String key, int def){
    	int value = def;  
        try {  
        	Class<?> mClassType = Class.forName(className);
        	Method mMethod = mClassType.getDeclaredMethod(methodName, String.class, int.class);  
            Integer v = (Integer) mMethod.invoke(mClassType, key, def);  
            value = v.intValue();  
        } catch (Exception e) {  
            e.printStackTrace();  
        }  
        return value;   
	}
	
	public boolean getBoolean(String className, String methodName, String key, boolean def){
		boolean value = def;  
        try {  
        	Class<?> mClassType = Class.forName(className);
        	Method mMethod = mClassType.getDeclaredMethod(methodName, String.class, int.class);  
        	value = (Boolean) mMethod.invoke(mClassType, key, def);  
        } catch (Exception e) {  
            e.printStackTrace();  
        }  
        return value;   
	}
	
	public void callMethod(String className, String methodName, Object receiver){
		try {
			Class<?> mClassType = Class.forName(className);
			Method mMethod = mClassType.getMethod(methodName);
			mMethod.invoke(receiver);
		}catch(Exception e) {
			e.printStackTrace();
		}
	}
    

}
