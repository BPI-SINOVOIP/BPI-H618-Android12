/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.softwinner.firelauncher.utils;

import java.util.Locale;

/**
 *
 * Constants used by multiple classes in this package
 */
public final class Constants {

    // Set to true to turn on verbose logging
    public static final boolean LOGV = false;
    
    // Set to true to turn on debug logging
    public static final boolean LOGD = true;

    // Custom actions
    
    public static final String ACTION_VIEW_VIDEO_DETAIL =
            "com.softwinner.fireplayer.ACTION_VIEW_VIDEO_DETAIL";

    public static final String ACTION_ZOOM_IMAGE =
            "com.softwinner.fireplayer.ACTION_ZOOM_IMAGE";
    
    public static final String ACTION_MEDIASCAN = "com.softwinner.fireplayer.ACTION_MEDIASCAN";
    
    // Defines a custom Intent action
    public static final String BROADCAST_ACTION_MEDIASCAN_START = "com.softwinner.fireplayer.BROADCAST.mediascan_start";
    public static final String BROADCAST_ACTION_MEDIASCAN_FINISHED = "com.softwinner.fireplayer.BROADCAST.mediascan_finish";
    public static final String BROADCAST_ACTION_BOOKMARK = "com.softwinner.fireplayer.BROADCAST.bookmark";
    public static final String EXTENDED_BOOKMARK_PATH = "com.softwinner.fireplayer.BROADCAST.bookmarkpath";
    public static final String EXTENDED_BOOKMARK_DURATION = "com.softwinner.fireplayer.BROADCAST.bookmarkduration";
    // Fragment tags
    public static final String VIDEO_DETAIL_FRAGMENT_TAG ="com.softwinner.fireplayer.VIDEO_DETAIL_FRAGMENT_TAG";
    public static final String VIDEO_THUMBS_FRAGMENT_TAG ="com.softwinner.fireplayer.VIDEO_THUMBS_FRAGMENT_TAG";
    public static final String VIDEO_FOLDER_FRAGMENT_TAG ="com.softwinner.fireplayer.VIDEO_FOLDER_FRAGMENT_TAG";
    public static final String NETWORKVIDEO_MAIN_FRAGMENT_TAG ="com.softwinner.fireplayer.NETWORKVIDEO_MAIN_FRAGMENT_TAG";
    
//    public static final String THUMBNAIL_FRAGMENT_TAG =
//            "com.softwinner.fireplayer.THUMBNAIL_FRAGMENT_TAG";
    
    

    // Defines the key for the status "extra" in an Intent
    public static final String EXTENDED_DATA_STATUS = "com.softwinner.fireplayer.STATUS";

    // Defines the key for the log "extra" in an Intent
    public static final String EXTENDED_STATUS_LOG = "com.softwinner.fireplayer.LOG";
    
    // Defines the key for storing fullscreen state
    public static final String EXTENDED_FULLSCREEN =
            "com.softwinner.fireplayer.EXTENDED_FULLSCREEN";

    /*
     * A user-agent string that's sent to the HTTP site. It includes information about the device
     * and the build that the device is running.
     */
    public static final String USER_AGENT = "Mozilla/5.0 (Linux; U; Android "
            + android.os.Build.VERSION.RELEASE + ";"
            + Locale.getDefault().toString() + "; " + android.os.Build.DEVICE
            + "/" + android.os.Build.ID + ")";
    
    public static final String USERAGENT_IPAD = "Mozilla/5.0 (iPad;U;" +
	        "CPU OS 5_1_1 like Mac OS X; zh-cn)AppleWebKit/534.46.0" +
	        "(KHTML, like Gecko)CriOS/19.0.1084.60 Mobile/9B206 Safari/7534.48.3";

    // Status values to broadcast to the Activity

    // The download is starting
    public static final int STATE_ACTION_STARTED = 0;

    // The background thread is connecting to the RSS feed
    public static final int STATE_ACTION_CONNECTING = 1;

    // The background thread is parsing the RSS feed
    public static final int STATE_ACTION_PARSING = 2;

    // The background thread is writing data to the content provider
    public static final int STATE_ACTION_WRITING = 3;

    public static final int STATE_ACTION_PART_COMPLETE = 4;
    
    // The background thread is done
    public static final int STATE_ACTION_COMPLETE = 5;

    // The background thread is doing logging
    public static final int STATE_LOG = -1;

    public static final CharSequence BLANK = " ";
    
    public static final String MEDIA_ROOT_PATH = "/mnt";
    
    public static final String PREDEF_CATEGORY_ROOT_PATH = MEDIA_ROOT_PATH;
    public static final String PREDEF_CATEGORY_RECENT = "RecentPlayed";
    public static final String PREDEF_CATEGORY_RECORD = "RecordVides";
    public static final String PREDEF_CATEGORY_FAVORITES = "Favorites";
    public static final int PREDEF_CATEGORY_SIZE = 4;
    
    public static final int TYPE_TODAY = 0;
    public static final int TYPE_MOVIE = 1;
    public static final int TYPE_TVSERIAL = 2; 
    public static final int  TYPE_CARTOON= 3; 
    public static final int TYPE_VARIETY = 4; 
    public static final int TYPE_DOCFILM = 5; 
    public static final int TYPE_CHANNEL = 6; 

    public static final boolean PLAY_HTML5 = false;

	public static final String BROADCAST_ACTION ="broadcast_action";
    
	
    public static final int HTTP_LOAD_PAGE_SIZE = 15*2;
    
    //OLD http://apk.softwinners.com/4kplayercms/
    
    public static final String SOFT_WINNER_API_VERSION = "&v=2";
    //public static final String SOFT_WINNER_BASE_URL = "http://v.softwinners.com/api/";
    //public static final String SOFT_WINNER_BASE_URL = "http://172.16.11.189:8888/";
    public static final String SOFT_WINNER_BASE_URL0 = "http://182.92.128.98";
    public static final String SOFT_WINNER_BASE_URL = SOFT_WINNER_BASE_URL0 + "/";
    //public static final String SOFT_WINNER_BASE_URL = "http://192.168.1.100:8888/";
    public static final String WEATHER_URL = "http://www.weather.com.cn/data/cityinfo/";
    
    public static final boolean DEBUG_LOG = false;

    public static final String SOFT_WINNER_RECOMMAND_URL =  SOFT_WINNER_BASE_URL + "recomm?";
    public static final String SOFT_WINNER_VIDEOS_URL = SOFT_WINNER_BASE_URL + "query?";
    public static final String SOFT_WINNER_DETAIL_URL = SOFT_WINNER_BASE_URL + "detail?";
    public static final String SOFT_WINNER_SEARCH_URL = SOFT_WINNER_BASE_URL + "search?";
    public static final String SOFT_WINNER_CLASSFIY_URL = SOFT_WINNER_BASE_URL + "classfy";
    public static final String SOFT_WINNER_FEEDBACK_URL = SOFT_WINNER_BASE_URL + "feedback";
    public static final String SOFT_WINNER_ONLINECONFIG_URL = SOFT_WINNER_BASE_URL + "onlineconfig";
    public static final String SOFT_WINNER_CRACK_JS = SOFT_WINNER_BASE_URL + "static/xplay.html";
    public static final String SOFT_WINNER_FORUM = SOFT_WINNER_BASE_URL0 + ":8083/forum.php";
    public static final String VST_CRACK_JS = "http://play.91vst.com/play.html";

    public static final String VST_LIVE_CRACK_JS = "proxy.myvst.net/liveconfig.html?ver=v2";
    public static final String SOFT_WINNER_CHANNEL_URL = SOFT_WINNER_BASE_URL + "live";

    public static final String SOFT_WINNER_WELCOME = "http://182.92.128.98/static/vcms_images/20140606/xlrs.jpg";
    
    public static final String SOFT_WINNER_UPDATE_URL = SOFT_WINNER_BASE_URL + "update"; 
    
    public static final String SOFT_WINNER_REPORT_ERROR = SOFT_WINNER_BASE_URL + "error";
    
    //mango tv
    public static final String MANGO_BASE_URL = "http://cs.interface.hifuntv.com/mgtv/";
    public static final String MANGO_RECOMMEND = MANGO_BASE_URL + "FactoryIndex?";
    public static final String MANGO_RECOMMEND_TYPE = "GetRecommendHome";
    public static final int MANGO_RECOMMEND_PAGE_SIZE = 10;
    public static final String MANGO_VERSION = "3.2.100.15.2.QZ.17.0_Release";
    public static final int MANGO_RECOMMEND_VERSION = 2;
    
    public static final int MANGO_RECOMMEND_LEAST_COUNT = 8;
    public static final int XXX_CHANNEL_LEAST_COUNT = 5;
    
    public static final int START_LOAD_RECOMMEND = 11110;
    public static final int FINISH_LOAD_RECOMMEND = 11111;
    public static final int RELOAD_RECOMMEND = 11112;
    public static final int START_LOAD_CHANNEL = 11113;
    public static final int FINISH_LOAD_CHANNLE = 11114;
    public static final int RELOAD_CHANNLE = 11115;
    public static final int START_LOAD_RECOMMEND_ROLL = 11116;
    public static final int FINISH_LOAD_RECOMMEND_ROLL = 11117;
    public static final int FILTER_CLICK_INTERVAL = 1000;
    
    public static final String APP_DB_CHANGE = "com.softwinner.firelauncher.app_db_change";
    
    //ATET Game market
    public static final String ATET_BASE_URL ="http://182.92.128.98/atet";
    
    //DB
    public static final String DBNAME = "mangotv";
    public static final String TVTRACKER_TABLE_NAME = "user_tvtracker";
    public static final String TVCOLLECTION_TABLE_NAME = "user_collection";
    public static final String TVHISTORY_TABLE_NAME = "user_history";
    
    //Mango intent field
    public static final String INTENT_SRC_APP = "srcApp";
	public static final String INTENT_VIDEO_ID = "videoId";
	public static final String INTENT_VIDEO_ID2 = "video_id";  //*
	public static final String INTENT_VIDEO_TYPE = "video_type"; //*
	public static final String INTENT_VIDEO_NAME = "videoName";
	public static final String INTENT_VIDEO_NAME2 = "video_name"; //*
	public static final String INTENT_VIDEO_IMG_URL = "videoImgUrl";
	public static final String INTENT_VIDEO_IMG_URL2 = "video_image_url"; //*
	public static final String INTENT_VIDEO_INDEX = "video_index"; // *
	public static final String INTENT_VIDEO_INDEX_NAME = "video_index_name"; // *
	public static final String INTENT_VIDEO_INDEX_COUNT = "video_index_count"; // *
	public static final String INTENT_UI_STYLE = "ui_style"; //*
	public static final String INTENT_EPISODE_ID = "episodeId";
	public static final String INTENT_EPISODE_NAME = "episodeName";
	public static final String INTENT_EPISODE_COUNT = "episodeCount";
	public static final String INTENT_CURRENT_POSITION = "currentPosition";
	public static final String INTENT_CURRENT_POSITION2 = "current_position"; //*
	public static final String INTENT_DURATION = "duration";
	public static final String INTENT_DURATION2 = "video_duration"; //*
	public static final String INTENT_PACKAGE_NAME = "package_name"; //*
	public static final String XXXXXX_ADDED_DATE = "added_date";
	
	public static final String ID_TYPE = "identifierType";
	public static final String ID_TYPE_RECENT = "0";
	public static final String ID_TYPE_COLLECT = "1";
	public static final String ID_TYPE_TVTRACKER = "2";
	
	public static final String ID_CMD = "cmd_ex";
	public static final String ID_CMD_ADD_PLAY = "add_play_record";
	
	
//	public static final String TVTRACKER_DURATION = "videoId";
}



