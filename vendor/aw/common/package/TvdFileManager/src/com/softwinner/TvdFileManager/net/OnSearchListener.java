package com.softwinner.TvdFileManager.net;

public interface OnSearchListener {
    void onReceiver(String path);

    boolean onFinish(boolean success);
}
