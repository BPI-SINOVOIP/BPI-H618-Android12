package com.allwinnertech.dragonsn;

import android.content.Context;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.content.pm.PackageManager.NameNotFoundException;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.net.EthernetManager;
import android.util.Log;

import java.net.NetworkInterface;
import java.net.SocketException;
import java.util.Enumeration;

public class Utils {

	private static final String MAC_SPLIT = ":";

	private static String hexByte(byte b) {
		String s = "000000" + Integer.toHexString(b);
		return s.substring(s.length() - 2);
	}

	public static boolean isNetworkConn(Context context) {
		ConnectivityManager connectivity = (ConnectivityManager) context.getSystemService(Context.CONNECTIVITY_SERVICE);
		if (connectivity != null) {
			NetworkInfo info = connectivity.getActiveNetworkInfo();
			if (info != null && info.isConnected()) {
				if (info.getState() == NetworkInfo.State.CONNECTED) {
					return true;
				}
			}
		}
		return false;
	}

	public static String getAppVersionName(Context context) {
		PackageManager packageManager = context.getPackageManager();
		PackageInfo packInfo;
		String version = "";
		try {
			packInfo = packageManager.getPackageInfo(context.getPackageName(), PackageManager.GET_ACTIVITIES);
			version = packInfo.versionName;
		} catch (NameNotFoundException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
		return version;
	}

	// public static void openEthernet() {
	// EthernetManager eManager = EthernetManager.getInstance();
	// if (eManager.getState() == EthernetManager.ETHERNET_STATE_DISABLED) {
	// Log.d(DragonSNActivity.TAG, "Disable state.");
	// /*List<EthernetDevInfo> devList = eManager.getDeviceNameList();
	// if (devList.size() > 0) {
	// EthernetDevInfo devInfo = devList.get(0);
	// devInfo.setIfName("eth0");
	// eManager.updateDevInfo(devInfo);
	// eManager.setEnabled(true);
	// } else {
	// Log.d(DragonSNActivity.TAG, "###No Ethernet support.");
	// }*/
	// eManager.setEnabled(true);
	// } else {
	// Log.d(DragonSNActivity.TAG, "Enabled state.");
	// }
	// }

	public static String getMAC(String ifaceName) {
		Enumeration<NetworkInterface> el;
		String macString = "";
		try {
			el = NetworkInterface.getNetworkInterfaces();
			while (el.hasMoreElements()) {
				NetworkInterface iface = el.nextElement();
				if (iface.getName().compareToIgnoreCase(ifaceName) == 0) {
					byte[] mac = iface.getHardwareAddress();
					if (mac == null) {
						continue;
					}
					macString = hexByte(mac[0]) + MAC_SPLIT + hexByte(mac[1]) + MAC_SPLIT + hexByte(mac[2]) + MAC_SPLIT
							+ hexByte(mac[3]) + MAC_SPLIT + hexByte(mac[4]) + MAC_SPLIT + hexByte(mac[5]);
				}
			}
		} catch (SocketException e1) {
			e1.printStackTrace();
		}
		return macString;
	}

}
