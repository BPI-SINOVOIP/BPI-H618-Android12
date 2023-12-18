package com.softwinner.fireplayer.remotemedia.returnitem;

import com.fasterxml.jackson.annotation.JsonIgnoreProperties;

public class SearchItem {
	public VideoItem[] videolist;
	@JsonIgnoreProperties(ignoreUnknown = true)
	public static class VideoItem{
		public String video_id;
		public String title;
		public String img_url;
		public String vid_url;
	}
}
