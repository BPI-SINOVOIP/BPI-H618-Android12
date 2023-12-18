package com.softwinner.fireplayer.remotemedia.returnitem;

import com.fasterxml.jackson.annotation.JsonIgnoreProperties;
@JsonIgnoreProperties(ignoreUnknown = true)
public class ReturnVideoDetail {
/**
 * {
 *  "comment_count":0,
 *  "img_url":"http://www.qiyipic.com/thumb/20130918/a629882.jpg",
 *  "favourite_count":0,
 *  "episode_count":1,
 *  "publish_date":1379433600000,
 *  "alias":null,
 *  "vendor":"奇艺",
 *  "videos":[
 *           {
 *            "img_url":"http://pic6.qiyipic.com/thumb/20130918/v771129.jpg",
 *            "duration":2378,
 *            "play_url":"http://m.iqiyi.com/play.html?tvid=771129&vid=4ffab2a662534614a273be7eebfc8896&msrc=3_80_160",
 *            "update_time":1379506864000,
 *            "video_name":"《一座城池》特辑",
 *            "description":"从学校肄业的“我（林夕）”（房祖名 饰）因为一次群架事件，和朋友“健叔”（王太利 饰）从上海逃到了一个城镇健叔是高我一年的同学，我们住在长江旅馆里，整日在这个城市里闲晃。后来我们认识了朋友王超（杨地 饰），从此，王超和他的桑塔纳就和我们混在了一起。我们跟着王超去他的学校看姑娘无意中参与了一次行为艺术，这让我忆起了自杀身亡的同桌和他短暂的爱情。 我们仍然在这个城市里闲晃着。我不时做着我的奇...",
 *            "source_url":"http://www.iqiyi.com/v_19rrifv8et.html",
 *            "sub_order":1,
 *            "show_id":11692,
 *            "vid":"4ffab2a662534614a273be7eebfc8896",
 *            "size":0}],
 *  "episode_update":1,
 *  "score":8.3,
 *  "director":"",
 *  "type":"喜剧|剧情|青春|",
 *  "id":11692,
 *  "down_count":0,
 *  "performers":"房祖名,王太利,黄瀞怡,谢芳,",
 *  "category":"电影",
 *  "area":"华语",
 *  "description":"从学校肄业的“我（林夕）”（房祖名 饰）因为一次群架事件，和朋友“健叔”（王太利 饰）从上海逃到了一个城镇健叔是高我一年的同学，我们住在长江旅馆里，整日在这个城市里闲晃。后来我们认识了朋友王超（杨地 饰），从此，王超和他的桑塔纳就和我们混在了一起。我们跟着王超去他的学校看姑娘无意中参与了一次行为艺术，这让我忆起了自杀身亡的同桌和他短暂的爱情。 我们仍然在这个城市里闲晃着。我不时做着我的奇特的梦，想着我曾经的女朋友…… ",
 *  "name":"《一座城池》特辑",
 *  "up_count":0,
 *  "language":null,
 *  "released_date":null,
 *  "vendors":[{"vendor":"奇艺","show_id":11692}],
 *  "show_url":"http://m.iqiyi.com/play.html?tvid=771129&vid=4ffab2a662534614a273be7eebfc8896&msrc=3_80_160"}
 *
 *
 */

        //public String _id;
        public String name;
        public int cat;//1:m,
        public String src;
        public String []area;
        public String[] type;
        public String lang;
        public String img;

        public String dur;
        public String mtype;
        public String def;
        public String vendor;
        public String tv;
        public String[] tags;
        public RDate rdate;
        public Roles roles;
        //public String cdate;
        public int paid;
        public int recd;
        public int del;
        public int hhot;
        public int sets;
        public int cset;
        public String desc;
        public String wage;
        public String space;
        public String qxd;

        public Video[] videos;
        public Sites[] sites;

        public RelativeVideo[] relat;
        @JsonIgnoreProperties(ignoreUnknown = true)
        public static class Video{
                public String vid;
                public String name;
                public String subtit;
                public String subset;
                public String dur;
                public String img;
                public String desc;
                public int begt;
                public int endt;
                public String url;
                public String purl;
                public int ptype;
        }

        @JsonIgnoreProperties(ignoreUnknown = true)
        public static class Sites{
                public SitesList []list;
                public String site;
        }

//      @JsonIgnoreProperties(ignoreUnknown = true)
//      public static class Vendor{
//              public String vendor;
//              public String show_id;
//      }

        @JsonIgnoreProperties(ignoreUnknown = true)
        public static class RDate{
                public int year;
                public int month;
                public int day;
        }

        @JsonIgnoreProperties(ignoreUnknown = true)
        public static class Roles{
                public String[] dirs;
                public String[] acts;
                public String[] voic;
        }
        
        @JsonIgnoreProperties(ignoreUnknown = true)
        public static class SitesList{
            public String url;
            public String name;
            public SitesList() {
				// TODO Auto-generated constructor stub
            	url = null;
            	name = null;
			}
        }
        @JsonIgnoreProperties(ignoreUnknown = true)
        public static class RelativeVideo{
        	public String qxd;
        	public String img;
        	public String cation;
        	public String score;
        	public String banben;
        	public String id;
        	public String name;
        }
}

