package com.softwinner.fireplayer.remotemedia.returnitem;

import com.fasterxml.jackson.annotation.JsonIgnoreProperties;

public class RetrunHomeRecommandItem {
/**
 * "电影":
 *      [
 *       {
 *        "img_url":"http://www.qiyipic.com/thumb/20130918/a629882.jpg",
 *        "id":11692,
 *        "episode_count":1,
 *        "episode_update":1,
 *        "name":"《一座城池》特辑"
 *        },
 *        {
 *         "img_url":"http://pic0.qiyipic.com/thumb/20130918/a629861.jpg",
 *         "id":11693,
 *         "episode_count":1,
 *         "episode_update":1,
 *         "name":"超级大导演之金刚王"
 *         },
 *         {
 *          "img_url":"http://pic5.qiyipic.com/thumb/20130916/a627699.jpg",
 *          "id":11696,
 *          "episode_count":1,
 *          "episode_update":1,
 *          "name":"艳遇丽江 上集"
 *          },
 *          {
 *           "img_url":"http://pic6.qiyipic.com/thumb/20130904/a617767.jpg",
 *           "id":11718,
 *           "episode_count":1,
 *           "episode_update":1,
 *           "name":"拉古莱特的夏天"
 *           },
 *           {
 *            "img_url":"http://pic9.qiyipic.com/thumb/20130829/a610235.jpg",
 *            "id":11727,
 *            "episode_count":1,
 *            "episode_update":1,
 *            "name":"风筝"
 *            },
 *            {
 *             "img_url":"http://pic4.qiyipic.com/thumb/20130812/a584853.jpg",
 *             "id":13509,
 *             "episode_count":1,
 *             "episode_update":1,
 *             "name":"青木瓜之味"
 *             },{"img_url":"http://pic1.qiyipic.com/thumb/20130808/a579724.jpg","id":13518,"episode_count":1,"episode_update":1,"name":"公民黑帮"},{"img_url":"http://pic1.qiyipic.com/thumb/20130808/a579559.jpg","id":13519,"episode_count":1,"episode_update":1,"name":"随风而逝"},{"img_url":"http://www.qiyipic.com/thumb/20130808/a579084.jpg","id":13520,"episode_count":1,"episode_update":1,"name":"蓝色情人节"},{"img_url":"http://pic0.qiyipic.com/thumb/20130807/a577831.jpg","id":13524,"episode_count":1,"episode_update":1,"name":"樱桃的滋味"}]
 */
	public Slider[] sliders;
	public CategoryItem[] list;

	@JsonIgnoreProperties(ignoreUnknown = true)
	public static class Slider{
		public String id;
		public String name;
		public String img;  
		public int ttl;
		public String desc;
	}
	@JsonIgnoreProperties(ignoreUnknown = true)
	public static class Video{
		public String img;
		public String id;
		public String name;
		public String scov;
		public String qxd;
	}
	@JsonIgnoreProperties(ignoreUnknown = true)
	public static class CategoryItem{
		public int type;
		public Video[] shows;
	}
}
