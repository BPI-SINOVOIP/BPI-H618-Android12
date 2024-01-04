package com.softwinner.firelauncher.network;

import android.content.Context;
import android.util.Log;

import com.android.volley.DefaultRetryPolicy;
import com.android.volley.RequestQueue;
import com.android.volley.Request.Priority;
import com.android.volley.Response;
import com.android.volley.Response.ErrorListener;
import com.android.volley.Response.Listener;
import com.android.volley.VolleyError;
import com.android.volley.toolbox.StringRequest;
import com.softwinner.firelauncher.LauncherApplication;

public class AsyncHttplLoader {
	private static final String TAG = "AsyncHttplLoader";
	
//	private static AsyncHttplLoader mInstance = null;
//	private Context mContext;
//	private PriorityStringRequest mStringRequest;
//	private boolean isOnLoadFinishExe = false;
	private RequestQueue mRequestQueue;
	public int mSize;
	private onLoadFinishListener mListener;

	
	public interface onLoadFinishListener{
		void onDataLoadFinish(boolean sucess, String content);
		void onPostExecute(boolean sucess);
		void onNetworkError(String message);
	}
	
//	public static AsyncHttplLoader getInstance(Context context) {
//		if( null == mInstance ) {
//			mInstance = new AsyncHttplLoader(context);
//		}
//		return mInstance;
//	}
	
	public AsyncHttplLoader(Context context) {
//		mContext = context;
		mRequestQueue = LauncherApplication.getInstance().getRequestQueue();
	}
	
	public void load(String url)  {
		Log.v(TAG, "load url=" + url);
		PriorityStringRequest stringRequest = new PriorityStringRequest(url,
				new Response.Listener<String>() {
					@Override
					public void onResponse(String response) {
						//Log.d(TAG, response);
						mListener.onDataLoadFinish(true,response);
						mListener.onPostExecute(true);
					}
				}, new Response.ErrorListener() {
					@Override
					public void onErrorResponse(VolleyError error) {
						Log.e(TAG, error.getMessage(), error);
						mListener.onNetworkError(error.getMessage());
					}
				});
		stringRequest.setPriority(Priority.HIGH);
		stringRequest.setRetryPolicy(new DefaultRetryPolicy(2000, 2, 1.4f));
		mRequestQueue.add(stringRequest);  //context to null, as startup initial it
	}

	public void setLoadFinishListener(onLoadFinishListener listener) {
		mListener = listener;
	}
	
	private class PriorityStringRequest extends StringRequest {
		private Priority mPriority = Priority.NORMAL;

		@Override
		public Priority getPriority() {
		    return mPriority;
		}

		public void setPriority(Priority priority) {
		    mPriority = priority;
		}
		
	    public PriorityStringRequest(String url, Listener<String> listener, ErrorListener errorListener) {
	    	super(url, listener, errorListener);
	    }
		
	}
	
	
}