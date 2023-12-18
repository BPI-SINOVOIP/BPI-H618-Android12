package com.allwinner.camera.data;

import java.util.ArrayList;
import java.util.List;

/**
 * 动态贴纸
 */
public class DynamicSticker {
    // 贴纸的文件夹路径
    public String folderPath;
    // 贴纸列表
    public List<DynamicStickerData> dataList;

    public DynamicSticker() {
        folderPath = null;
        dataList = new ArrayList<>();
    }

    @Override
    public String toString() {
        return "DynamicSticker{" +
                "folderPath='" + folderPath + '\'' +
                ", dataList=" + dataList +
                '}';
    }
}
