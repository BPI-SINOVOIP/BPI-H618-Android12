#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "edid.h"
#include "resolution.h"

#define ARRAY_SIZE(x) (sizeof(x) / sizeof(*(x)))

static const char *edid_cea_modes[] = {
    "640x480@60Hz",
    "720x480@60Hz",
    "720x480@60Hz",
    "1280x720@60Hz",
    "1920x1080i@60Hz",
    "1440x480i@60Hz",
    "1440x480i@60Hz",
    "1440x240@60Hz",
    "1440x240@60Hz",
    "2880x480i@60Hz",
    "2880x480i@60Hz",
    "2880x240@60Hz",
    "2880x240@60Hz",
    "1440x480@60Hz",
    "1440x480@60Hz",
    "1920x1080@60Hz",
    "720x576@50Hz",
    "720x576@50Hz",
    "1280x720@50Hz",
    "1920x1080i@50Hz",
    "1440x576i@50Hz",
    "1440x576i@50Hz",
    "1440x288@50Hz",
    "1440x288@50Hz",
    "2880x576i@50Hz",
    "2880x576i@50Hz",
    "2880x288@50Hz",
    "2880x288@50Hz",
    "1440x576@50Hz",
    "1440x576@50Hz",
    "1920x1080@50Hz",
    "1920x1080@24Hz",
    "1920x1080@25Hz",
    "1920x1080@30Hz",
    "2880x480@60Hz",
    "2880x480@60Hz",
    "2880x576@50Hz",
    "2880x576@50Hz",
    "1920x1080i@50Hz",
    "1920x1080i@100Hz",
    "1280x720@100Hz",
    "720x576@100Hz",
    "720x576@100Hz",
    "1440x576@100Hz",
    "1440x576@100Hz",
    "1920x1080i@120Hz",
    "1280x720@120Hz",
    "720x480@120Hz",
    "720x480@120Hz",
    "1440x480i@120Hz",
    "1440x480i@120Hz",
    "720x576@200Hz",
    "720x576@200Hz",
    "1440x576i@200Hz",
    "1440x576i@200Hz",
    "720x480@240Hz",
    "720x480@240Hz",
    "1440x480i@240Hz",
    "1440x480i@240Hz",
    "1280x720@24Hz",
    "1280x720@25Hz",
    "1280x720@30Hz",
    "1920x1080@120Hz",
    "1920x1080@100Hz",
    "1280x720@24Hz",
    "1280x720@25Hz",
    "1280x720@30Hz",
    "1280x720@50Hz",
    "1280x720@60Hz",
    "1280x720@100Hz",
    "1280x720@120Hz",
    "1920x1080@24Hz",
    "1920x1080@25Hz",
    "1920x1080@30Hz",
    "1920x1080@50Hz",
    "1920x1080@60Hz",
    "1920x1080@100Hz",
    "1920x1080@120Hz",
    "1680x720@24Hz",
    "1680x720@25Hz",
    "1680x720@30Hz",
    "1680x720@50Hz",
    "1680x720@60Hz",
    "1680x720@100Hz",
    "1680x720@120Hz",
    "2560x1080@24Hz",
    "2560x1080@25Hz",
    "2560x1080@30Hz",
    "2560x1080@50Hz",
    "2560x1080@60Hz",
    "2560x1080@100Hz",
    "2560x1080@120Hz",
    "3840x2160@24Hz",
    "3840x2160@25Hz",
    "3840x2160@30Hz",
    "3840x2160@50Hz",
    "3840x2160@60Hz",
    "4096x2160@24Hz",
    "4096x2160@25Hz",
    "4096x2160@30Hz",
    "4096x2160@50Hz",
    "4096x2160@60Hz",
    "3840x2160@24Hz",
    "3840x2160@25Hz",
    "3840x2160@30Hz",
    "3840x2160@50Hz",
    "3840x2160@60Hz",
};

int toSunxiOutputResolution(int vic)
{
    for (size_t i = 0; i < sizeof(vic2resolution) / sizeof(vic2resolution[0]); i++) {
        if (vic == vic2resolution[i].vic) {
            return vic2resolution[i].sunxi_disp_mode;
        }
    }
    return -1;
}

static void
cea_svd(const unsigned char *x, int n, struct vic_info *info, int *count, int regular_sampling)
{
    int i;

    for (i = 0; i < n; i++)  {
        unsigned char svd = x[i];
        unsigned char native;
        unsigned char vic;

        if ((svd & 0x7f) == 0)
            continue;

        if ((svd - 1) & 0x40) {
            vic = svd;
            native = 0;
        } else {
            vic = svd & 0x7f;
            native = svd & 0x80;
        }

        int idx = *count;
        if (vic > 0 && vic <= ARRAY_SIZE(edid_cea_modes)) {
            info[idx].vic = vic;
			info[idx].sunxi_disp_mode = toSunxiOutputResolution(vic);
            info[idx].native = native;
            info[idx].support_regular_sampling_mode = regular_sampling;
            info[idx].name = edid_cea_modes[vic - 1];
            idx++;
        }
        *count = idx;
    }
}

int
cea_video_block(const unsigned char *x, struct edid_info *info)
{
    int length = x[0] & 0x1f;
    cea_svd(x + 1, length, info->supported_vic, &info->total_vic, 1);
    return 0;
}

int cea_y420vdb(const unsigned char *x, struct edid_info *info)
{
    int length = x[0] & 0x1f;
    cea_svd(x + 2, length - 1, info->ycbcr420_vic, &info->total_ycbcr420_vic, 0);

    info->fmt[FMT_YUV420].supported = 1;
    info->fmt[FMT_YUV420].bits |= DEEP_COLOR_24BIT;

    return 0;
}

int cea_y420_capability_map_data_block(const unsigned char *x, struct edid_info *info)
{
    int length = (x[0] & 0x1f) - 1;
    if (length == 0) {
        /* All vic support yuv420 sampling */
        info->y420_sampling_all = 1;

        info->fmt[FMT_YUV420].supported = 1;
        info->fmt[FMT_YUV420].bits |= DEEP_COLOR_24BIT;
        return 0;
    }

    for (int i = 0; i < length; i++) {
        int base = i * 8;
        int maps = x[2 + i];
        for (int j = 0; j < 8; j++) {
            if (maps & (1 << j)) {
                memcpy(&info->ycbcr420_vic[info->total_ycbcr420_vic],
                        &info->supported_vic[base+j], sizeof(struct vic_info));
                info->ycbcr420_vic[info->total_ycbcr420_vic].support_regular_sampling_mode = 1;
                info->total_ycbcr420_vic++;
            }
        }
    }
    info->fmt[FMT_YUV420].supported = 1;
    info->fmt[FMT_YUV420].bits |= DEEP_COLOR_24BIT;
    return 0;
}
