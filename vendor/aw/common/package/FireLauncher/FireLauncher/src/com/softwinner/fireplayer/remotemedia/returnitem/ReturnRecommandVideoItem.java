package com.softwinner.fireplayer.remotemedia.returnitem;

import com.fasterxml.jackson.annotation.JsonIgnoreProperties;

/*
{
    "src_identification": "com.mgtv.tv",
    "error": {
        "code": "0",
        "info": ""
	},
	"video_total_count": 7,
	"video_count": 7
	"version_time": 20130726T110022Z,
    "video_infos": [
        {
            "video_id": "8e4dc96e7345992612ef560a01b8ebcb",
            "video_name":"分手合约",
			"video_desc":"分手合约2013爱情大戏",
            "video_kind": [
                "爱情",
                "爱情喜剧",
                "最新收录",
                "院线速递",
                "高清影院"
            ],
            "video_ui_style": "1",
            "video_index_infos": [
                {
                    "video_index_name": "分手合约 第1集",
                    "video_index": "0"
                },
                {
                    "video_index_name": "分手合约 第2集",
                    "video_index": "1"
                }
            ],
            "video_img_list": {
                "video_img_url_1": "",
                "video_img_url_2": "http://1img.imgo.tv/preview/intertv/prev/StarCor/epgimg/416f5e211501e252a070d6d2cfd7fbec1.jpg",
                "video_img_url_3": "http://1img.imgo.tv/preview/intertv/prev/KsImg/c2ddef6e-2d66-42a3-adbe-fd95204b4f75.jpg"
            },
            "video_actors": [
                "白百何",
                "彭于晏",
                "吴佩慈",
                "蒋劲夫"
            ],
            "video_duration": "3111",
            "video_director": [
                "吴基焕"
            ],
            "video_description": "父母亡故的何俏俏得到同班同学李行加倍关怀，毕业前夕，李行向俏俏求婚，不料被拒。双方立下“5年后若单身就结婚”的合约。5年之后，俏俏满心以为两人可以再续前缘，却得知李行要结婚的消息。倍受打击又不甘心的俏俏只有求助于朋友毛毛，准备联手把李行再次夺回。 "
        },
       
    ]
}
*/

public class ReturnRecommandVideoItem {
	@JsonIgnoreProperties(ignoreUnknown = true)
	public String src_identification;
	public RecommandError error;
	public int video_total_count;
	public int video_count;
	public int version_time;
	public String update_time;
	public VideoInfo[] video_infos;
	
	public class RecommandError {
		public String code;
		public String info;
	}
	
	public static class VideoInfo{
		public String video_id;
		public String video_name;
		public String video_category_name;
		public String video_category_id;
		
		
		public String[] video_kind;
		public String video_ui_style;
		public VideoImgList video_img_list;
		public String[] video_actors;
		public String video_duration;
		public String[] video_director;
		public String video_description;
		public String video_desc;
		
		public class VideoIndexInfo {
			public String video_index_name;
			public String video_index;
		}
		
		public class VideoImgList {
			public String video_img_url_1;
			public String video_img_url_2;
			public String video_img_url_3;
			public String video_img_url_4;
			public String video_img_url_5;
			public String video_img_url_6;
		}
		
		
	}
}
