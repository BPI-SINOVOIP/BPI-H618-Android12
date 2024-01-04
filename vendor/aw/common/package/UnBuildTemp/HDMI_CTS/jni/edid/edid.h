
#define MAX_VIC_COUNT   (128)

enum video_fmt_type {
    FMT_RGB    = 0,
    FMT_YUV444 = 1,
    FMT_YUV422 = 2,
    FMT_YUV420 = 3,
    FMT_MAX
};

#define DEEP_COLOR_48BIT    (1 << 3)
#define DEEP_COLOR_36BIT    (1 << 2)
#define DEEP_COLOR_30BIT    (1 << 1)
#define DEEP_COLOR_24BIT    (1 << 0)

struct video_format {
    const char *name;
    int supported;
    int bits;
};

struct vic_info {
    unsigned char vic;
    unsigned char sunxi_disp_mode;
    unsigned char native;
    /*
     * for ycbcr420 vic
     * to detect RGB/Y444/Y422 sampling mode support
     */
    unsigned char support_regular_sampling_mode;
    const char *name;
};

struct edid_info {
    /*
     *  dvi(0x01), hdmi(0x02)
     */
    int sink_type;
    struct video_format fmt[4];
    struct vic_info supported_vic[MAX_VIC_COUNT];
    struct vic_info ycbcr420_vic[MAX_VIC_COUNT];
    int total_vic;
    int total_ycbcr420_vic;

    int y420_sampling_all;
    int rgb_only;
    int present_3d;
    int hdmi1p4_max_tmds_clock;
    int hdmi2p0_max_tmds_clock;

    char colorimetry;

    char eotf;
    char metadata_descriptor;
    char max_luminance;
    char min_luminance;
    char max_frame_average_luminance;
};

int read_edid_info(struct edid_info *info);
