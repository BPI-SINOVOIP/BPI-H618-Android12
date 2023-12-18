

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <poll.h>

#include "Debug.h"
#include "disputils.h"
#include "uniquefd.h"
#include "sunxi_eink.h"
#include "syncfence.h"

// Wrapper class to maintain dispdev fd
class dispdev {
public:
    static dispdev& getInstance() {
        static dispdev _instance;
        return _instance;
    }
    inline int getHandleFd() { return mDispfd.get(); }
    static int fd() { return getInstance().getHandleFd(); }
	int createSyncpt(syncinfo* info);
	int destroySyncTimeline(void);
	int OnWriteBackFinish(unsigned int syncnum);

private:
    dispdev();
   ~dispdev() = default;
    dispdev(const dispdev&) = delete;
    void initSyncTimeline();
    dispdev& operator=(const dispdev&) = delete;
    sunxi::uniquefd mDispfd;
    sunxi::uniquefd mSyncTimelineHandle;
    unsigned int mTimelineValue;
    unsigned int mSignaledPtValue;
};

void dispdev::initSyncTimeline()
{
    int fd = fence_timeline_create();
    if (fd < 0) {
        DLOGE("sw sync timeline create failed: %s", strerror(errno));
        return;
    }
    mTimelineValue = 0;
    mSignaledPtValue = 0;
    mSyncTimelineHandle = sunxi::uniquefd(fd);
}


dispdev::dispdev() {
    int fd = open("/dev/sunxi-eink", O_RDWR);
    if (fd < 0) {
        DLOGE("Open '/dev/sunxi-eink' failed, %s", strerror(errno));
    }
    mDispfd = sunxi::uniquefd(fd);
    initSyncTimeline();
}

// default DE frequency 254 MHz
static int __deFrequency = 300000000;


// dev_composer defines -->
enum {
    // create a fence timeline
    HWC_NEW_CLIENT = 1,
    HWC_DESTROY_CLIENT,
    HWC_ACQUIRE_FENCE,
    HWC_SUBMIT_FENCE,
};
// dev_composer defines <--
int dispdev::createSyncpt(syncinfo* info)
{
	int ret = -1;
	if (info) {
		info->count = ++mTimelineValue;
		info->fd = fence_create(mSyncTimelineHandle.get(), "de_rtwb.fence",
							info->count);
		if (info->fd < 0) {
			DLOGE("fence create failed: %s", strerror(errno));
		} else
			ret = 0;
	}
    return ret;
}

int createSyncpt(syncinfo* info)
{
    return dispdev::getInstance().createSyncpt(info);
}

int dispdev::destroySyncTimeline(void)
{
	mTimelineValue = 0;
	mSignaledPtValue = 0;
    return 0;
}
int destroySyncTimeline(void)
{
	return dispdev::getInstance().destroySyncTimeline();
}

int dispdev::OnWriteBackFinish(unsigned int syncnum)
{
    ++mSignaledPtValue;
	if (mSignaledPtValue != syncnum) {
		DLOGW("fence timeline corruption, mTimelineValue=%d mSignaledPtValue=%d syncnum=%d",
			  mTimelineValue, mSignaledPtValue, syncnum);
	}

	return fence_timeline_inc(mSyncTimelineHandle.get(), 1);
}


int eink_update_image(struct eink_upd_cfg *config)
{
	int ret = -1;
	unsigned long upd_arr[8] = {0};

	if (!config) {
		DLOGE("Null pointer!");
		goto OUT;
	}

	upd_arr[0] = (unsigned long)config;

	//flush framebuffer to eink panel
	ret = ioctl(dispdev::fd(), EINK_UPDATE_IMG, (void *)upd_arr);
	if (ret < 0) {
		DLOGE("%s: fail to ioctl EINK_UPD_IMG, ret=%d\n", __func__, ret);
		ret = -2;
		goto OUT;
	}

OUT:
	return ret;
}


// disp device opt
int submitLayer(unsigned int syncnum, disp_layer_config2* configs,
		int configCount, struct eink_img* cur_img,
		struct eink_img* last_img)
{
	int ret = 0;
    unsigned long args[4] = {0};

	if (!configs || !configCount || !cur_img || !last_img) {
        DLOGE("NULL argument!");
		goto OUT;
	}

	args[0] = (unsigned long)configs;
	args[1] = configCount;/* layer num */
	args[2] = (unsigned long)last_img;
	args[3] = (unsigned long)cur_img;

	ret = ioctl(dispdev::fd(), EINK_WRITE_BACK_IMG, (unsigned long)args);

    if (ret) {
        DLOGE("ioctl EINK_WRITE_BACK_IMG error:%d", ret);
    }


OUT:
    return ret;
}
int FenceSignal(unsigned int syncnum)
{
	return dispdev::getInstance().OnWriteBackFinish(syncnum);
}


int vsyncCtrl(int disp, int enable)
{
    return 0;
}

int blankCtrl(int disp, int enable)
{
    unsigned long args[4] = {0};
    args[0] = enable;

    if (ioctl(dispdev::fd(), EINK_SELF_STANDBY, (unsigned long)args)) {
        DLOGE("ioctl DISP_BLANK error");
        return -1;
    }
    return 0;
}

int switchDisplay(int disp, int type, int mode)
{
    return 0;
}

int regal_process(struct eink_img* cur_img,
               struct eink_img* last_img)
{
       int ret = 0;
    unsigned long args[4] = {0};

       if (!cur_img || !last_img) {
        DLOGE("NULL argument!");
               goto OUT;
       }
       args[0] = (unsigned long)last_img;
       args[1] = (unsigned long)cur_img;

       ret = ioctl(dispdev::fd(), EINK_REGAL_PROCESS, (unsigned long)args);

    if (ret) {
        DLOGE("ioctl EINK_REGAL_PROCESS error:%d", ret);
    }
OUT:
    return ret;
}

int getDeFrequency()
{
    return __deFrequency;
}

int getFramebufferVarScreenInfo(struct fb_var_screeninfo* info)
{
    int fd = open("/dev/graphics/fb0", O_RDWR);
    if (fd < 0) {
        DLOGE("Open '/dev/graphics/fb0' failed, %s", strerror(errno));
        return -1;
    }
    int ret = ioctl(fd, FBIOGET_VSCREENINFO, info);
    close(fd);
    return ret;
}

int getDisplayOutputType(int disp)
{
	if (disp == 0) {
		return DISP_OUTPUT_TYPE_EINK;
	} else
		return -1;
}

int eink_handwrite_dma_map(bool map, int fd, u32 *paddr)
{
	int ret = -1;
	unsigned long args[4] = {0};

	if (!map) {
		args[0] = map;
		args[1] = fd;
		ret = ioctl(dispdev::fd(), EINK_HANDWRITE_DMA_MAP, (unsigned long)args);
		if (ret) {
			DLOGE("EINK_HANDWRITE_DMA_MAP fail!");
		}
	} else {
		if (fd >= 0 && paddr) {
			args[0] = map;
			args[1] = fd;
			args[2] = (unsigned long)paddr;/* layer num */
			ret = ioctl(dispdev::fd(), EINK_HANDWRITE_DMA_MAP, (unsigned long)args);
			if (ret) {
				DLOGE("EINK_HANDWRITE_DMA_MAP fail!");
			}
		}
	}


	return ret;
}


int getDisplayOutputSize(int disp, int* width, int* height)
{
	struct fb_var_screeninfo info;
	int ret = -1;

    int fd = open("/dev/graphics/fb0", O_RDWR);
    if (fd < 0) {
        DLOGE("Open '/dev/graphics/fb0' failed, %s", strerror(errno));
        return -1;
    }
    ret = ioctl(fd, FBIOGET_VSCREENINFO, &info);
    close(fd);

    *width  = info.xres;
    *height = info.yres;
    return ret;
}

int SetGCCnt(int gc_cnt)
{
	unsigned long arg[4] = {0};
	int ret = 0;

	arg[0] = gc_cnt;
	ret = ioctl(dispdev::fd(), EINK_SET_GC_CNT, (void*)arg);
	if (ret < 0) {
		DLOGE("%s: fail to SET_GC_CNT, ret=%d\n", __func__, ret);
		ret = -2;
		goto err;
	}
	return 0;

err:
	return ret;
}

int GetFreeBufSlot(struct buf_slot *slot)
{
	unsigned long args[4] = {0};
	int ret = -1;
	int timeout = 20;
	struct pollfd fds;

	fds.fd = dispdev::fd();
	fds.events = POLLIN;



	ret = poll(&fds, 1, timeout);
	if (ret > 0) {
		if (fds.revents &(POLLERR | POLLNVAL)) {
			DLOGE("[%s]state is err!\n", __func__);
			goto OUT;
		} else {
			args[0] = (unsigned long)slot;

			ret = ioctl(dispdev::fd(), EINK_GET_FREE_BUF, (void *)args);
			if (ret < 0) {
				DLOGE("%s: fail to ioctl EINK_GET_FREE_BUF\n", __func__);
			}
		}
	} else if (ret == 0) {
		ret = 1; //timeout
	}


OUT:
	return ret;
}

const char *outputType2Name(int type)
{
    switch (type) {
        case DISP_OUTPUT_TYPE_HDMI: return "hdmi";
        case DISP_OUTPUT_TYPE_TV:   return "cvbs";
        case DISP_OUTPUT_TYPE_LCD:  return "lcd";
        case DISP_OUTPUT_TYPE_VGA:  return "vga";
        default:                    return "unknow";
    }
}
