package com.softwinner.firelauncher.network;

import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

import android.content.Context;
import android.net.Uri;
import android.util.Log;

import com.softwinner.firelauncher.network.AsyncHttplLoader.onLoadFinishListener;
import com.softwinner.fireplayer.remotemedia.returnitem.ChannelImages;
import com.softwinner.fireplayer.remotemedia.returnitem.ReturnCategory;
import com.softwinner.fireplayer.remotemedia.returnitem.ReturnCityInfo.CityInfo;
import com.softwinner.fireplayer.remotemedia.returnitem.ReturnGameItem;
import com.softwinner.fireplayer.remotemedia.returnitem.ReturnRecommandVideoItem;
import com.softwinner.fireplayer.remotemedia.returnitem.ReturnGameItem.GameInfo;
import com.softwinner.fireplayer.remotemedia.returnitem.ReturnWeatherInfo.WeatherInfo;

public class HttpJsonLoader implements onLoadFinishListener {
	private static final String TAG = "HttpJsonLoader";
	
	private Context mContext;
	private Uri mUrl;
	private int mType;
	public List<ReturnRecommandVideoItem.VideoInfo> mRecommamdVideoItemList = new ArrayList<ReturnRecommandVideoItem.VideoInfo>();
	public List<ChannelImages.Channel> mChannelImageList = new ArrayList<ChannelImages.Channel>();
	public List<GameInfo> mGameItemList = new ArrayList<ReturnGameItem.GameInfo>();
	public List<WeatherInfo> mWeatherList = new ArrayList<WeatherInfo>();
	public List<CityInfo> mCityInfoList = new ArrayList<CityInfo>();
	public ReturnCategory mReturnCategory;
	private onJsonLoaderFinishListener mListener;
	AsyncHttplLoader mAsyncHttplLoader;
	private JsonParser mJsonParser;
	public static final int JSON_RECOMMEND = 0;
	public static final int JSON_TYPE_CATEGORY_LIST = 1;
	public static final int JSON_CHANNEL = 2;
	public static final int JSON_RECOMMEND_ROLL = 3;
	public static final int JSON_GAME = 4;
	public static final int JSON_WEATHER = 5;
	public static final int JSON_GET_CITY = 6;
	
	
	public interface onJsonLoaderFinishListener{
		void onJsonLoaderFinish(int type, int addedSize);
		void onNetworkError(String message);
	}
	
	public HttpJsonLoader(Context context, int type) {
		mContext = context;
		mType = type;
		mAsyncHttplLoader = new AsyncHttplLoader(mContext);
		mAsyncHttplLoader.setLoadFinishListener(this);
	}
	
	public void clearVideoList() {
		mRecommamdVideoItemList.clear();
	}
	
	public void loadUrl(String url)  {
		//Log.v(TAG, "loadUrl: " + url);
		mUrl = Uri.parse(url);
		if(null == mJsonParser)
			mJsonParser = new JsonParser();
		mAsyncHttplLoader.load(mUrl.toString());
	}
	/**
	public VideoItem getItem(int pos){
		return mVideoList.get(pos);
	}*/
	
	public ReturnRecommandVideoItem.VideoInfo getRecommendVideoItem(int location){
		return mRecommamdVideoItemList.get(location);
	}
	
	public String getRecommendRollImageUrl(){
		return mJsonParser.getRecommendRollImageUrl();
	}
	
	public ChannelImages.Channel getChannelImageItem(int location) {
		return mChannelImageList.get(location);
	}	
	
	public List<GameInfo> getGameItemList(){
		return mGameItemList;
	}
	
	public WeatherInfo getWeatherInfo(){
		return mWeatherList.get(0);
	}
	
	public CityInfo getCityInfo(){
		return mCityInfoList.get(0);
	}

	public void setJsonLoaderFinishListener(onJsonLoaderFinishListener listener) {
		mListener = listener;
	}
	
	public int getSize() {
		if (mType == JSON_RECOMMEND) {
			return mRecommamdVideoItemList.size();
		} else if(mType == JSON_CHANNEL){
			return mChannelImageList.size();
		} else if(mType == JSON_GAME){
			return mGameItemList.size();
		} else if(mType == JSON_WEATHER){
			return mWeatherList.size();
		} else if(mType == JSON_GET_CITY){
			return mCityInfoList.size();
		} else {
			return 0;
		}
	}
	
	@Override
	public void onDataLoadFinish(boolean sucess, String data) {
		if(sucess) {
			try {
				if (mType == JSON_RECOMMEND) {
					Log.d(TAG, "onDataLoadFinish JSON_RECOMMEND ");
					mJsonParser.parserVideoListJsonString(data);
					if(mJsonParser.getRecommamdVideoItemList()!=null) {
						mRecommamdVideoItemList.clear();
						mRecommamdVideoItemList.addAll(mJsonParser.getRecommamdVideoItemList());
					}
				}else if(mType == JSON_CHANNEL){
					Log.d(TAG, "onDataLoadFinish JSON_CHANNEL ");
					mJsonParser.parserChannels(data);
					if(mJsonParser.getChannels() != null) {
						mChannelImageList.clear();
						mChannelImageList.addAll(mJsonParser.getChannels()); 
					}
				}else if(mType == JSON_RECOMMEND_ROLL){
					mJsonParser.parserRecommendRoll(data);
				}else if(mType == JSON_GAME){
					mJsonParser.parserGames(data);
					if(mJsonParser.getGameInfo() != null) {
						mGameItemList.clear();
						mGameItemList.addAll(mJsonParser.getGameInfo()); 
					}
					for (GameInfo game : mGameItemList) {
						Log.i(TAG, "JsonParser: game.name="+game.gameName+" icon.url="+game.url);
					}
				}else if(mType == JSON_WEATHER){
					mJsonParser.parserWeather(data);
					if(mJsonParser.getWeatherInfo() != null) {
						mWeatherList.clear();
						mWeatherList.addAll(mJsonParser.getWeatherInfo()); 
					}
				}else if(mType == JSON_GET_CITY){
					Log.d(TAG, "onDataLoadFinish JSON_GET_CITY ");
					mJsonParser.parserCity(data);
					if(mJsonParser.getCityInfo() != null) {
						Log.d(TAG, "city =" + mJsonParser.getCityInfo().get(0).city );
						mCityInfoList.clear();
						mCityInfoList.addAll(mJsonParser.getCityInfo()); 
						Log.d(TAG, "city =" + mCityInfoList.get(0).city );
					}
				}
			} catch (IOException e) {
				e.printStackTrace();
			}
		}
	}

	@Override
	public void onPostExecute(boolean sucess) {
		if(mListener != null) {
			mListener.onJsonLoaderFinish(mType, getSize());
		}
	}

	@Override
	public void onNetworkError(String message) {
		Log.d(TAG,"onNetworkError ");
		mListener.onNetworkError(message);
	}
}