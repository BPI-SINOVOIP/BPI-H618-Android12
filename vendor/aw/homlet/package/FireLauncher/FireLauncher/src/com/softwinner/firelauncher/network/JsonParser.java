package com.softwinner.firelauncher.network;

import java.io.IOException;
import java.io.InputStream;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import android.util.Log;
import com.fasterxml.jackson.core.JsonParseException;
import com.fasterxml.jackson.databind.JsonMappingException;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.softwinner.firelauncher.utils.Constants;
import com.softwinner.firelauncher.utils.Utils;
import com.softwinner.fireplayer.remotemedia.returnitem.ChannelImages;
import com.softwinner.fireplayer.remotemedia.returnitem.NetworkVideoDetailInfo;
import com.softwinner.fireplayer.remotemedia.returnitem.PushContent;
import com.softwinner.fireplayer.remotemedia.returnitem.RecommendRoll;
import com.softwinner.fireplayer.remotemedia.returnitem.RetrunHomeRecommandItem;
import com.softwinner.fireplayer.remotemedia.returnitem.ReturnCategory;
import com.softwinner.fireplayer.remotemedia.returnitem.ReturnCityInfo;
import com.softwinner.fireplayer.remotemedia.returnitem.ReturnCityInfo.CityInfo;
import com.softwinner.fireplayer.remotemedia.returnitem.ReturnGameItem;
import com.softwinner.fireplayer.remotemedia.returnitem.ReturnOnlineConfig;
import com.softwinner.fireplayer.remotemedia.returnitem.ReturnRecommandVideoItem;
import com.softwinner.fireplayer.remotemedia.returnitem.ReturnVideoDetail;
import com.softwinner.fireplayer.remotemedia.returnitem.ReturnWeatherInfo;
import com.softwinner.fireplayer.remotemedia.returnitem.ReturnWeatherInfo.WeatherInfo;
import com.softwinner.fireplayer.remotemedia.returnitem.SearchItem;

public class JsonParser {
	private static final String TAG = "JsonParser";
//	public int mBegin, mEnd, mTotalCount;
	private List<ReturnRecommandVideoItem.VideoInfo> mRecommendVideoList;
	private List<ChannelImages.Channel> mChannelImageList;
	private List<ReturnGameItem.GameInfo> mGameItemList;
	private List<WeatherInfo> mWeatherList;
	private List<CityInfo> mCityList;
	private RecommendRoll mRecommendRoll;
	private ObjectMapper mapper = new ObjectMapper();
//	public int pageNo,pageSize;
	
//	public void readVideoListJsonStream(InputStream in) throws JsonParseException, JsonMappingException, IOException{
//		mVideoList = new ArrayList<VideoItem>();
//		String items = Utils.convertStreamToString(in);
//		NetworkVideoItem result = mapper.readValue(items,
//				NetworkVideoItem.class);
//		mBegin = Integer.valueOf(result.beg);
//		mEnd = Integer.valueOf(result.end);
//		mVideoList.addAll(Arrays.asList(result.videolist));
//	}
		
	public void parserVideoListJsonString(String data) throws JsonParseException, JsonMappingException, IOException {
		
		if(mRecommendVideoList==null) {
			mRecommendVideoList = new ArrayList<ReturnRecommandVideoItem.VideoInfo>();
		}
		else {
			mRecommendVideoList.clear();
		}
		String items = data;
		ReturnRecommandVideoItem result = mapper.readValue(items, ReturnRecommandVideoItem.class);
		if(result.error.code.equals("0") || result.video_total_count < Constants.MANGO_RECOMMEND_LEAST_COUNT) {
			if (null != result.video_infos)
				mRecommendVideoList.addAll(Arrays.asList(result.video_infos));
		} else {
			Log.e(TAG, "parserVideoListJsonString, parser error");
		}
	}
	
   	public List<ReturnRecommandVideoItem.VideoInfo> getRecommamdVideoItemList(){
   		return mRecommendVideoList;
   	}
	
//	public List<VideoItem> getVideoList() {
//		return mVideoList;
//	}
//
//	public int getListSize() {
//		return mEnd - mBegin;
//	}
//	
//	public int getTotalCount() {
//		return mTotalCount;
//	}

	//
	public NetworkVideoDetailInfo parserVideoDetailInfoObject(InputStream in) throws JsonParseException, JsonMappingException, IOException{
		String videoDetail = Utils.convertStreamToString(in);
		NetworkVideoDetailInfo result = mapper.readValue(videoDetail,
				NetworkVideoDetailInfo.class);
		return result;
	}
	
	public ReturnVideoDetail parserReturnVideoDetail(String videoDetail) throws JsonParseException, JsonMappingException, IOException{
		ReturnVideoDetail result = mapper.readValue(videoDetail,
				ReturnVideoDetail.class);
		return result;
	}

	public RetrunHomeRecommandItem parserHomePageInfoHomeRecommandItem (String str) throws JsonParseException, JsonMappingException, IOException{
		//String str = Utils.convertStreamToString(in);
		//Log.d(TAG, str);
		RetrunHomeRecommandItem item = mapper.readValue(str, RetrunHomeRecommandItem.class);
		return item;
	}
	
	public ReturnOnlineConfig parserOnlineConfig (String str) throws JsonParseException, JsonMappingException, IOException{
		//String str = Utils.convertStreamToString(in);
		//Log.d(TAG, str);
		ReturnOnlineConfig item = mapper.readValue(str, ReturnOnlineConfig.class);
		return item;
	}
	
	public ReturnCategory parserCategoryInfo(String jsonString) {
		//Log.v(TAG, "jsonString is = " + jsonString);
		try {
			return mapper.readValue(jsonString,
					ReturnCategory.class);
		} catch (JsonParseException e) {
			e.printStackTrace();
		} catch (JsonMappingException e) {
			e.printStackTrace();
		} catch (IOException e) {
			e.printStackTrace();
		}
		return null;
	}
		
	public boolean parserSearchItems(InputStream in ,List<SearchItem.VideoItem> list) throws JsonParseException, JsonMappingException, IOException{
		String jsonString = Utils.convertStreamToString(in);
		SearchItem result = mapper.readValue(jsonString, SearchItem.class);
		list.addAll(Arrays.asList(result.videolist));
		return true;
	}
	
	public PushContent parserPushContent (String str) throws JsonParseException, JsonMappingException, IOException{
		//String str = Utils.convertStreamToString(in);
		//Log.d(TAG, str);
		PushContent item = mapper.readValue(str, PushContent.class);
		return item;
	}
	
	public void parserChannels(String jsonString) throws JsonParseException, JsonMappingException, IOException{
		if(mChannelImageList==null) {
			mChannelImageList = new ArrayList<ChannelImages.Channel>();
		}
		else {
			mChannelImageList.clear();
		}
	
		ChannelImages result = mapper.readValue(jsonString, ChannelImages.class);
		if(result.channels != null) {
			mChannelImageList.addAll(Arrays.asList(result.channels));
		} else {
			Log.e(TAG, "parserChannels error: "+jsonString);
		}
	}

	public List<ChannelImages.Channel> getChannels() {
		return mChannelImageList;
	}
	
	public void parserRecommendRoll(String jsonString) throws JsonParseException, JsonMappingException, IOException{
		if(mRecommendRoll==null) {
			mRecommendRoll = new RecommendRoll();
		}
	
		RecommendRoll result = mapper.readValue(jsonString, RecommendRoll.class);
		if(result.ImageUrl != null) {
			mRecommendRoll.ImageUrl = result.ImageUrl;
		} else {
			Log.e(TAG, "parserChannels error: "+jsonString);
		}
		
	}
	
	public String getRecommendRollImageUrl() {
		return mRecommendRoll.ImageUrl;
	}
	
	public void parserGames(String jsonString) throws JsonParseException, JsonMappingException, IOException{
		if(mGameItemList == null) {
			mGameItemList = new ArrayList<ReturnGameItem.GameInfo>();
		}else {
			mGameItemList.clear();
		}
	
		ReturnGameItem result = mapper.readValue(jsonString, ReturnGameItem.class);
		if(result.data != null) {
			mGameItemList.addAll(Arrays.asList(result.data));
		} 
	}
	
	public List<ReturnGameItem.GameInfo> getGameInfo() {
		return mGameItemList;
	}
	
	public void parserWeather(String jsonString) throws JsonParseException, JsonMappingException, IOException{
		// TODO Auto-generated method stub
		if(mWeatherList == null) {
			mWeatherList = new ArrayList<WeatherInfo>();
		}else {
			mWeatherList.clear();
		}
	
		ReturnWeatherInfo result = mapper.readValue(jsonString, ReturnWeatherInfo.class);
		if(result.weatherinfo != null) {
			mWeatherList.addAll(Arrays.asList(result.weatherinfo));
		} 
	}
	
	public List<WeatherInfo> getWeatherInfo() {
		return mWeatherList;
	}

	public void parserCity(String jsonString) throws JsonParseException, JsonMappingException, IOException{
		// TODO Auto-generated method stub
		String jasonCity = null;
		if(mCityList == null) {
			mCityList = new ArrayList<CityInfo>();
		}else {
			mCityList.clear();
		}

		Log.d("parserCity","jsonString = " + jsonString);
		ReturnCityInfo.CityInfo result = mapper.readValue(jsonString, ReturnCityInfo.CityInfo.class);
		if(result != null) {
			mCityList.addAll(Arrays.asList(result));
		} 
	}
	
	public List<CityInfo> getCityInfo() {
		return mCityList;
	}
}
