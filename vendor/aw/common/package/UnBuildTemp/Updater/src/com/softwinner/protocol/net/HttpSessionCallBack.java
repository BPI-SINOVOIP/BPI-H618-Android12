package com.softwinner.protocol.net;

public interface HttpSessionCallBack {

	public void onSucceed(byte[] rspData);

	public void onError(int errCode, String strErr);

}
