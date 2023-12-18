#define LOG_TAG "HWINFO"

#include <stdio.h>
#include <utils/Log.h>
#include "libhwinfo.h"

int main(void)
{
	const char *vendor  = get_wifi_vendor_name();
	const char *module  = get_wifi_module_name();
	const char *wifihal = get_wifi_hal_name();
	const char *libbt   = get_bluetooth_libbt_name();
	int bt_support      = get_bluetooth_is_support();

	ALOGD("********** Wireless information **********");
	ALOGD("module vendor: %s", vendor);
	ALOGD("module name  : %s", module);
	ALOGD("wifi hal name: %s", wifihal);
	ALOGD("libbt-vendor : %s", libbt);
	ALOGD("is bt support: %d", bt_support);
	ALOGD("******************************************");

	return 0;
}
