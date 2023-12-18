package com.softwinner.fireplayer.remotemedia.returnitem;

import com.fasterxml.jackson.annotation.JsonIgnoreProperties;

@JsonIgnoreProperties(ignoreUnknown = true)
public class PushContent {
	public String cmd;
	public String vid;
	public String p0;
	public String p1;
	public int silent;
	public int nd; //notify default
	public String msgtt;//msg title
	public String cnttt;//content title
	public String cnttx;//content text
}
