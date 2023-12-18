package com.softwinner.fireplayer.remotemedia.returnitem;

public class ReturnGameItem {
/*
 * {
    "data": [
        {
            "adId": "20141015140344102301",
            "url": "http://g1.atet.tv:25001/file/atetplatform/GAME_AD/6b6b27ac98c94acc91c94c859a8b58a3.jpg",
            "backgroundUrl": "",
            "videoUrl": "",
            "gameId": "20140705184239047105",
            "packageName": "com.rovio.angrybirds.cub",
            "gameName": "愤怒的小鸟",
            "positionIndex": 1,
            "sizeType": "全志首页竖",
            "type": 1,
            "remark": "愤怒的小鸟",
            "actionUrl": {
                "behavior": "activity",
                "action": ""
            },
            "title": ""
        },
        {
            "adId": "20141015141031913312",
            "url": "http://g1.atet.tv:25001/file/atetplatform/GAME_AD/cbfdc853c66b4689bb15c6f2b611e40c.jpg",
            "backgroundUrl": "",
            "videoUrl": "",
            "gameId": "20140818161514087137",
            "packageName": "com.gdhy800.gdhy.play800",
            "gameName": "格鬥火影",
            "positionIndex": 6,
            "sizeType": "全志首页方",
            "type": 1,
            "remark": "格斗火影",
            "actionUrl": {
                "behavior": "activity",
                "action": ""
            },
            "title": ""
        }
    ],
    "startTime": 1413216000000,
    "endTime": -1,
    "createTime": 1413277269000,
    "code": 0
}*/
	public GameInfo[] data;
	public long startTime;
	public long endTime;
	public long createTime;
	public int code;
	
	public static class GameInfo{
		public String adId;
		public String url;
		public String backgroundUrl;
		public String videoUrl;
		public String gameId;
		public String packageName;
		public String gameName;
		public int positionIndex;
		public String sizeType;
		public int type;
		public String remark;
		public ActionUrl actionUrl;
		public String title;
	}
	
	public static class ActionUrl{
		public String behavior;
		public String action;
	}
	
}
