package com.softwinner.dragonbox.utils;

import com.softwinner.dragonbox.entity.MES_RequestBean;

public class MES_Utils {

    public static String TAG = "DragonBox-MESUtils";

    public static String getSuffix(MES_RequestBean requestBean){
        if (requestBean == null) return null;
        String body = "/rest/wip/bool/RouteSetpStation/string/" + requestBean.passStationNum + "/string/"
            + requestBean.user + "/int64/" + requestBean.stationKey + "/string/"
            + requestBean.defectiveProduct + "/string/" + requestBean.scpContent + "/string/" + requestBean.step
            + "/string/" + requestBean.remark + "/rest";
        return body;
    }

}