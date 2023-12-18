package com.softwinner.firelauncher.subui;

public class MangoBean {  // * 表示播放记录独有
	public String videoId;
	public int videoType;   // *
	public String videoName;
	public String uiStyle; //*
	public String videoImgUrl;
	public String episodeId;
	public String episodeName;
	public int episodeCount;
	public int videoIndex;  // *
	public String videoIndexName;  // *
	public int videoIndexCount; //*
	public int currentPosition;
	public int duration;
	public String addedDate;
	public String packageName; //*
	
	public MangoBean() {
		videoId = "unknown";
		videoName = "unknown";
		videoImgUrl = "unknown";
		currentPosition = 0;
		duration = 0;
		videoIndex = 0;
		videoIndexName = "unknown";
		videoIndexCount = 0;
		episodeId = "unknown";
		episodeName = "unknown";
		episodeCount = 0;
	}
	
	/*
	 
	 public String videoId;
	 public String videoType;
	 public String videoName;
	 public int videoIndex;
	 public String videoIndexName;
	 public int videoIndexCount;
	 public String uiStyle;
	 public int currentPosition;
	 public int videoDuration;
	 public String packageName;
	 public String videoImgUrl;
	  
	 */
}

