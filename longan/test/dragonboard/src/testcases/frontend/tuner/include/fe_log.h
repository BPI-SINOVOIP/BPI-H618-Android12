#ifndef _FE_LOG_H_
#define _FE_LOG_H_

#ifndef FE_LOG_TAG
#define FE_LOG_TAG "TV_Frontend_Hal"
#endif

#ifndef BUILD_WITH_LINUX
	#include <log/log.h>
	#include <string.h>

	#define FE_LOG(level, fmt, arg...)  \
		LOG_PRI(level, FE_LOG_TAG, "<%s:%u>: " fmt, strrchr(__FILE__, '/') + 1, __LINE__, ##arg)

	#define FE_LOGD(fmt, arg...) FE_LOG(ANDROID_LOG_DEBUG, fmt, ##arg)

	#define FE_LOGE(fmt, arg...) FE_LOG(ANDROID_LOG_ERROR, "\033[40;31m" fmt "\033[0m", ##arg)
	#define FE_LOGW(fmt, arg...) FE_LOG(ANDROID_LOG_WARN, fmt, ##arg)
	#define FE_LOGI(fmt, arg...) FE_LOG(ANDROID_LOG_INFO, fmt, ##arg)
	#define FE_LOGV(fmt, arg...) FE_LOG(ANDROID_LOG_VERBOSE, fmt, ##arg)

	#define FE_ASSERT(e) 									\
			LOG_ALWAYS_FATAL_IF(                        	\
					!(e),                               	\
					"<%s:%d> assert(%s) failed.",    		\
					strrchr(__FILE__, '/') + 1, __LINE__, #e)

	#define FE_LOG_ASSERT(e, fmt, arg...)               \
		do {											\
			if (!(e)) { 										\
				FE_LOGE("assert:" fmt, ##arg);					\
			} 													\
			LOG_ALWAYS_FATAL_IF(								\
					!(e),										\
					"<%s:%d> assert(%s) failed.",				\
					strrchr(__FILE__, '/') + 1, __LINE__, #e); 	\
		} while(0)

	#define FE_CHECK_INTERFACE(hdr, func) \
		FE_ASSERT(hdr); \
		FE_ASSERT(hdr->ops); \
		FE_ASSERT(hdr->ops->func)
#else
	#include <string.h>
	#include <stdio.h>

	#define FE_LOG(fmt, arg...)  printf(fmt, ##arg)

	#define FE_LOGD(fmt, arg...) FE_LOG(fmt, ##arg)

	#define FE_LOGE(fmt, arg...) FE_LOG(fmt, ##arg)
	#define FE_LOGW(fmt, arg...) FE_LOG(fmt, ##arg)
	#define FE_LOGI(fmt, arg...) FE_LOG(fmt, ##arg)
	#define FE_LOGV(fmt, arg...) FE_LOG(fmt, ##arg)
#endif

#endif

