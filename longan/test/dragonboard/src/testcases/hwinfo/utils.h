#ifndef __UTILS_H
#define __UTILS_H

#ifndef LOG_TAG
#define LOG_TAG            "DRAGONBOARD"
#endif

enum {
	LEVEL_V,
	LEVEL_D,
	LEVEL_I,
	LEVEL_W,
	LEVEL_E,
};

#define LEVEL_DEF          LEVEL_D

#define PROPERTY_VALUE_MAX 64
#define PROPERTY_FILE      "/vendor/etc/prop.ini"
#define PROPERTY_SECTION   "DEF_PROP"


int property_get(const char *name, char *value, const char *defaults);
int property_set(const char *name, char *value);

int log_print(int level, const char *tag, const char *fmt, ...);

#define ALOGE(fmt, arg...) log_print(LEVEL_E, LOG_TAG, fmt, ##arg)
#define ALOGW(fmt, arg...) log_print(LEVEL_W, LOG_TAG, fmt, ##arg)
#define ALOGI(fmt, arg...) log_print(LEVEL_I, LOG_TAG, fmt, ##arg)
#define ALOGD(fmt, arg...) log_print(LEVEL_D, LOG_TAG, fmt, ##arg)
#define ALOGV(fmt, arg...) log_print(LEVEL_V, LOG_TAG, fmt, ##arg)

#endif
