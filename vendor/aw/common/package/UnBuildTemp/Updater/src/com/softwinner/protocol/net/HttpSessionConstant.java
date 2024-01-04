package com.softwinner.protocol.net;

import com.softwinner.update.App;
import com.softwinner.update.utils.Utils;

import android.os.Build;


public final class HttpSessionConstant {
	public static final int MAX_FILE_DOWNLOAD_CONNECTIONS = 2;
	public static final int MAX_IMAGE_DOWNLOAD_CONNECTIONS = 5;
	public static final int MAX_POST_CONNECTIONS = 5;
	public static final int MAX_GET_CONNECTIONS = 3;
	public static final int CONNECTION_TIMEOUT = 30 * 1000; // 30 seconds
	public static final int SO_TIMEOUT = 30 * 1000;
	public static final int SOCKET_BUFFER_SIZE = 4 * 1024;

	public static final String getUserAgent() {
		StringBuilder sbuilder = new StringBuilder();
		sbuilder.append("PgcV:");
		sbuilder.append(Utils.VersionName(App.getAppContext()));
		sbuilder.append(";DevV:");
		sbuilder.append(Build.MODEL);
		sbuilder.append(";AdrV:");
		sbuilder.append(Build.VERSION.RELEASE);
		return sbuilder.toString();
	}

	public static final class MAX_RETRY {
		public static final int GET = 3;
		public static final int POST_DATA = 3;
		public static final int POST_FILE = 3;
	}

	public static final class ERROR_CODE {
		public static final int ERR_NETWORK_DISABLE = 100;
		public static final int ERR_UNKNOWN_HOST = 101;
		public static final int ERR_CONNECT_REFUSE = 102;
		public static final int ERR_PROTOCOL_ERROR = 103;
		public static final int ERR_CONNECT_TIMEOUT = 104;

	}
}
