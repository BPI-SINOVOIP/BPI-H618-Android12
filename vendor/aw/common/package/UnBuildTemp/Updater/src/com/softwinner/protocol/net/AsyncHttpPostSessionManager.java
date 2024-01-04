package com.softwinner.protocol.net;

public class AsyncHttpPostSessionManager extends AsyncHttpSessionManager {

	private static AsyncHttpSessionManager instance = null;

	private AsyncHttpPostSessionManager(int nMaxConnectCount) {
		super(nMaxConnectCount);
	}

	public static AsyncHttpSessionManager getInstance() {
		if (instance == null) {
			instance = new AsyncHttpPostSessionManager(HttpSessionConstant.MAX_POST_CONNECTIONS);
		}
		return instance;
	}
}
