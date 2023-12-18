package com.softwinner.fireplayer.remotemedia.returnitem;

import com.fasterxml.jackson.annotation.JsonIgnoreProperties;

@JsonIgnoreProperties(ignoreUnknown = true)
public class NetworkVideoDetailInfo{
    public String[] play_url;
	public String title;
	public String img_url;
	public String url;
	public String duration;
	public String description;
	public String movie_type;
	public String directors;
	public String actors;
	public String label;
	public String area;
	public String release_date;
	public String vendor;
	//public int series_count;
}
