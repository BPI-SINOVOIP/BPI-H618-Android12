package com.softwinner.agingdragonbox;
import com.softwinner.agingdragonbox.engine.Utils;
import com.softwinner.agingdragonbox.platform.IAgingFinish;

import android.content.Context;
import android.os.SystemProperties;
import android.util.Log;

public class AgingFinish implements IAgingFinish {
	public static final String TAG = "DragonAging-AgingFinish";
	@Override
	public void agingTestOverCallback() {
        Utils.setPropertySecure(Utils.PROPERTY_DRAGON_USB0DEVICE,"1");
        Utils.setPropertySecure("persist.sys.usb.config","mtp,adb");
	}

}
