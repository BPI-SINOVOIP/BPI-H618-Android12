#ifndef DC_LOG_H
#define DC_LOG_H

#ifndef LOG_TAG
#define LOG_TAG "DVBCore"
#endif

#include <string.h>
#include <assert.h>

#define DC_ASSERT(e) assert(e)


#define DC_CHECK_INTERFACE(hdr, func) \
	DC_ASSERT(hdr); \
	DC_ASSERT(hdr->ops); \
	DC_ASSERT(hdr->ops->func)

#endif

