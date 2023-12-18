package com.softwinner.protocol.net;

import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

public abstract class AsyncHttpSessionManager {

	private ExecutorService mThreadPool = null;

	public AsyncHttpSessionManager(int nMaxConnectCount) {

		mThreadPool = Executors.newFixedThreadPool(nMaxConnectCount);
	}

	public void submit(AsyncHttpSession session) {
		mThreadPool.submit(session);

	}
}
