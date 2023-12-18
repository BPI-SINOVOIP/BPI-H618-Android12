#define LOG_TAG "HWINFO"
#include "utils.h"
#include <stdio.h>
#include <getopt.h>
#include "libhwinfo.h"

#define ARRAY_SIZE(x)   sizeof(x) / sizeof(x[0])

struct option_t {
	const struct option long_options;
	const char *help;
};

static const struct option_t option_list[] = {
	{{"info",      0, NULL, 'a'}, "get all module info"},
	{{"vendor",    0, NULL, 'v'}, "get module vendor info"},
	{{"module",    0, NULL, 'm'}, "get module name"},
	{{"driver",    0, NULL, 'd'}, "get module driver name"},
	{{"drvmod",    0, NULL, 'n'}, "get module driver module name"},
	{{"drvarg",    0, NULL, 'g'}, "get module driver arg"},
	{{"wifihal",   0, NULL, 'w'}, "get wifi hal name"},
	{{"libbt",     0, NULL, 'b'}, "get libbt name"},
	{{"btsup",     0, NULL, 's'}, "get bt support status"},
	{{"help",      0, NULL, 'h'}, "show this help"},
};

int show_help(const char *name)
{
	int i;
	ALOGD("Usage: %s [OPTION...]\n", name);

	for (i = 0; i < ARRAY_SIZE(option_list); i++)
		ALOGD("\t-%c, --%-10s %s",
					option_list[i].long_options.val,
					option_list[i].long_options.name,
					option_list[i].help);

	ALOGD("\n");
	return 0;
}


int main(int argc, char **argv)
{
	int opt, i;
	const char *vendor  = get_wifi_vendor_name();
	const char *module  = get_wifi_module_name();
	const char *driver  = get_wifi_driver_name();
	const char *drvmod  = get_wifi_driver_module_name();
	const char *drvarg  = get_driver_module_arg();
	const char *wifihal = get_wifi_hal_name();
	const char *libbt   = get_bluetooth_libbt_name();
	int btsup           = get_bluetooth_is_support();

	struct option long_options[ARRAY_SIZE(option_list)];
	for (i = 0; i < ARRAY_SIZE(option_list); i++) {
		long_options[i] = option_list[i].long_options;
	}

	while ((opt = getopt_long(argc, argv, "avmdngwbsh", long_options, NULL)) != -1) {
		switch(opt) {
			case 'a':
				ALOGD("********** Wireless information **********");
				ALOGD("vendor=%s", vendor);
				ALOGD("module=%s", module);
				ALOGD("driver=%s", driver);
				ALOGD("drvmod=%s", drvmod);
				ALOGD("drvarg=%s", drvarg);
				ALOGD("wifihal=%s", wifihal);
				ALOGD("libbt=%s", libbt);
				ALOGD("btsup=%d", btsup);
				ALOGD("******************************************");
				break;
			case 'v':
				ALOGD("vendor=%s", vendor);
				break;
			case 'm':
				ALOGD("module=%s", module);
				break;
			case 'd':
				ALOGD("driver=%s", driver);
				break;
			case 'n':
				ALOGD("drvmod=%s", drvmod);
				break;
			case 'g':
				ALOGD("drvarg=%s", drvarg);
				break;
			case 'w':
				ALOGD("wifihal=%s", wifihal);
				break;
			case 'b':
				ALOGD("libbt=%s", libbt);
				break;
			case 's':
				ALOGD("btsup=%d", btsup);
				break;
			default:
				show_help(argv[0]);
		}
	}

	return 0;
}
