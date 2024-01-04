package com.softwinner.dragonbox.entity;

import com.softwinner.dragonbox.testcase.CaseWifi;

public class MES_RequestBean {

    public String passStationNum = "";//过站序号，配置文件获取
    public String user = "";//用户名，配置文件获取
    public String stationKey = "";//站点key值，配置文件获取
    public String defectiveProduct = "";//不良品，配置文件获取
    public String scpContent = "";//SPC字段及存储内容，需要从测试结果中获取（非配置文件）
    public String step = "";//分步，需要从测试结果中获取（非配置文件）
    public String remark = "";//备注，配置文件获取

    public static MES_RequestBean getQuestBeanFromCaseWifi(){
        MES_RequestBean ret = new MES_RequestBean();
        ret.user = CaseWifi.user;
        ret.stationKey = CaseWifi.stationKey;
        ret.defectiveProduct = CaseWifi.defectiveProduct;
        ret.remark = CaseWifi.remark;
        return ret;
    }

}