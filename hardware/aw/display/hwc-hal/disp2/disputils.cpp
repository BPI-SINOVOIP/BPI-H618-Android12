

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

#include "Debug.h"
#include "disputils.h"
#include "uniquefd.h"

// Wrapper class to maintain dispdev fd
class dispdev {
public:
    static dispdev& getInstance() {
        static dispdev _instance;
        return _instance;
    }
    inline int getHandleFd() { return mDispfd.get(); }
    static int fd() { return getInstance().getHandleFd(); }

private:
    dispdev();
   ~dispdev() = default;
    dispdev(const dispdev&) = delete;
    dispdev& operator=(const dispdev&) = delete;
    sunxi::uniquefd mDispfd;
};

dispdev::dispdev() {
    int fd = open("/dev/disp", O_RDWR);
    if (fd < 0) {
        DLOGE("Open '/dev/disp' failed, %s", strerror(errno));
    }
    mDispfd = sunxi::uniquefd(fd);
}

// minimum DE frequency 100 MHz
static int minimumDeFrequency = 100000000;
// default DE frequency 432 MHz
static int __deFrequency      = 432000000;


// dev_composer defines -->
enum {
    // create a fence timeline
    HWC_NEW_CLIENT = 1,
    HWC_DESTROY_CLIENT,
    HWC_ACQUIRE_FENCE,
    HWC_SUBMIT_FENCE,
};
// dev_composer defines <--

// disp sync opt
int createSyncTimeline(int disp)
{
    unsigned long deFreq;
    unsigned long args[4] = {0};

    args[0] = disp;
    args[1] = HWC_NEW_CLIENT;
    args[2] = (unsigned long)&deFreq;

    if (ioctl(dispdev::fd(), DISP_HWC_COMMIT, (unsigned long)args)) {
        DLOGE("createSyncTimeline failed, %s", strerror(errno));
        return -1;
    }

    if (deFreq > minimumDeFrequency)
        __deFrequency = deFreq;

    DLOGI("DisplayEngine Frequency: %d Hz", deFreq);
    return 0;
}

int createSyncpt(int disp, syncinfo *info)
{
    unsigned long args[4] = {0};

    args[0] = disp;
    args[1] = HWC_ACQUIRE_FENCE;
    args[2] = (unsigned long)info;
    args[3] = 0;

    if (ioctl(dispdev::fd(), DISP_HWC_COMMIT, (unsigned long)args)) {
        DLOGE("createSyncpt failed!");
        return -ENODEV;
    }
    return 0;
}

int destroySyncTimeline(int disp)
{
    unsigned long args[4] = {0};

    args[0] = disp;
    args[1] = HWC_DESTROY_CLIENT;

    if (ioctl(dispdev::fd(), DISP_HWC_COMMIT, (unsigned long)args)) {
        DLOGE("destroySyncTimeline failed!");
        return -1;
    }
    return 0;
}

// disp device opt
int submitLayer(int disp, unsigned int syncnum,
        disp_layer_config2 *configs, int configCount)
{
    unsigned long args[4] = {0};

    args[0] = disp;
    args[1] = 1;
    if (ioctl(dispdev::fd(), DISP_SHADOW_PROTECT, (unsigned long)args)) {
        DLOGE("ioctl DISP_SHADOW_PROTECT error");
        return -1;
    }

    args[0] = disp;
    args[1] = (unsigned long)(configs);
    args[2] = configCount;
    args[3] = 0;

    if (ioctl(dispdev::fd(), DISP_LAYER_SET_CONFIG2, (unsigned long)args)) {
        DLOGE("ioctl DISP_LAYER_SET_CONFIG2 error");
    }

    args[0] = disp;
    args[1] = HWC_SUBMIT_FENCE;
    args[2] = syncnum;
    args[3] = 0;
    if (ioctl(dispdev::fd(), DISP_HWC_COMMIT, (unsigned long)args)) {
        DLOGE("ioctl DISP_HWC_COMMIT error");
    }

    args[0] = disp;
    args[1] = 0;
    args[2] = 0;
    args[3] = 0;
    if (ioctl(dispdev::fd(), DISP_SHADOW_PROTECT, (unsigned long)args)) {
        DLOGE("ioctl DISP_SHADOW_PROTECT error");
        return -1;
    }
    return 0;
}

int vsyncCtrl(int disp, int enable)
{
    unsigned long args[4] = {0};
    args[0] = disp;
    args[1] = enable;
    if (ioctl(dispdev::fd(), DISP_VSYNC_EVENT_EN, (unsigned long)args)) {
        DLOGE("ioctl DISP_VSYNC_EVENT_EN error");
        return -1;
    }
    return 0;
}

int blankCtrl(int disp, int enable)
{
    unsigned long args[4] = {0};
    args[0] = disp;
    args[1] = enable;
    if (ioctl(dispdev::fd(), DISP_BLANK, (unsigned long)args)) {
        DLOGE("ioctl DISP_BLANK error");
        return -1;
    }
    return 0;
}

int switchDisplay(int disp, int type, int mode)
{
    struct disp_device_config config;

    switch (type) {
        case DISP_OUTPUT_TYPE_LCD:
        case DISP_OUTPUT_TYPE_TV:
            config.type     = (enum disp_output_type)(type);
            config.mode     = (enum disp_tv_mode)(mode);
            config.format   = DISP_CSC_TYPE_RGB;
            config.cs       = DISP_BT709;
            config.bits     = DISP_DATA_8BITS;
            config.eotf     = DISP_EOTF_GAMMA22;
            config.dvi_hdmi = DISP_DVI_HDMI_UNDEFINED;
            config.range    = DISP_COLOR_RANGE_16_255;
            config.scan     = DISP_SCANINFO_NO_DATA;
            break;
        case DISP_OUTPUT_TYPE_HDMI:
            config.type     = (enum disp_output_type)(type);
            config.mode     = (enum disp_tv_mode)(mode);
            config.format   = DISP_CSC_TYPE_YUV444;
            config.cs       = DISP_BT709;
            config.bits     = DISP_DATA_8BITS;
            config.eotf     = DISP_EOTF_GAMMA22;
            config.dvi_hdmi = DISP_HDMI;
            config.range    = DISP_COLOR_RANGE_16_255;
            config.scan     = DISP_SCANINFO_NO_DATA;
            break;
    }

    unsigned long args[4] = {0};
    args[0] = disp;
    args[1] = (unsigned long)(&config);
    return ioctl(dispdev::fd(), DISP_DEVICE_SET_CONFIG, (unsigned long)args);
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
    struct disp_output info;
    unsigned long args[4] = {0};
    args[0] = disp;;
    args[1] = (unsigned long)&info;
    if (ioctl(dispdev::fd(), DISP_GET_OUTPUT, (unsigned long)args) == 0) {
        return info.type;
    }
    DLOGE("get display output info failed, display=%d", disp);
    return -1;
}

int getDisplayOutputSize(int disp, int* width, int* height)
{
    int size[2] = {0};
    unsigned long args[4] = {0};
    args[0] = disp;;
    size[0] = ioctl(dispdev::fd(), DISP_GET_SCN_WIDTH,  (unsigned long)args);
    size[1] = ioctl(dispdev::fd(), DISP_GET_SCN_HEIGHT, (unsigned long)args);

    if (size[0] <= 0 || size[1] <= 0) {
        DLOGE("get display output size failed, display=%d, size:%dx%d",
                disp, size[0], size[1]);
        *width  = 720;
        *height = 480;
        return -1;
    }

    *width  = size[0];
    *height = size[1];
    return 0;
}

int getDisplayVsyncTimestamp(int32_t *disp, int64_t* timestamp)
{
    struct disp_vsync_timestame ts;
    unsigned long args[4] = {0};

    args[0] = (unsigned long)&ts;
    int ret = ioctl(dispdev::fd(), DISP_GET_VSYNC_TIMESTAMP, (unsigned long)args);

    if (ret < 0) {
        DLOGE("get display vsync timestamp failed, ret=%d", ret);
        return ret;
    }
    *disp      = ts.disp;
    *timestamp = ts.timestamp;
    return 0;
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

bool checkNeedLimitPolicy()
{
    bool ret = true;
    char* buf = nullptr;
    char* flag = nullptr;
    int fd = open("/sys/class/sunxi_info/sys_info", O_RDONLY, 0);
    if (fd == -1) {
        DLOGE("Could not open sys_info, %s(%d)\n", strerror(errno), errno);
        return ret;
    }
    int len = lseek(fd, 0, SEEK_END);
    if(len == -1) {
        DLOGE("Could not seek sys_info, %s(%d)\n", strerror(errno), errno);
        goto OUT;
    }

    buf =(char*)malloc(len);
    if(buf == nullptr) {
        DLOGE("malloc failed ");
        goto OUT;
    }

    lseek(fd, 0, SEEK_SET);

    read(fd, buf,len);//may be not enough

    flag = strstr(buf,"sunxi_batchno");
    if(flag == nullptr) {
        DLOGE("check sunxi_batchno failed ");
        goto OUT;
    }

    if(!(strstr(flag,"0x18550000")
                || strstr(flag,"0x18550003")
                || strstr(flag,"0x18550004"))) {
        ret = false;
        DLOGI("No need to limit bandwidth of HW composition");
    }

OUT:
    close(fd);
    if(buf != nullptr) {
        free(buf);
        buf = nullptr;
    }
    return ret;
}


