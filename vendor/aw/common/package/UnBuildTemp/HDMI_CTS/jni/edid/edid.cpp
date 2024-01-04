
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <android/log.h>
#include <cutils/log.h>
#include "edid.h"
#include "resolution.h"

#define debug(fmt, args...) ALOGD(fmt, ##args)

int cea_y420_capability_map_data_block(const unsigned char *x, struct edid_info *info);
extern int cea_y420vdb(const unsigned char *x, struct edid_info *info);
extern int cea_video_block(const unsigned char *x, struct edid_info *info);

static char logbuff[4096];
static char *pbuff;

static void print_subsection(const char *name, unsigned char *edid, int start,
                 int end)
{
    int i;

    pbuff += sprintf(pbuff, "%s:", name);
    for (i = strlen(name); i < 15; i++)
    pbuff += sprintf(pbuff, " ");
    for (i = start; i <= end; i++)
    pbuff += sprintf(pbuff, " %02x", edid[i]);
    pbuff += sprintf(pbuff, "\n");
}

static void dump_breakdown(unsigned char *edid)
{
    pbuff += sprintf(pbuff, "Extracted contents:\n");
    print_subsection("header", edid, 0, 7);
    print_subsection("serial number", edid, 8, 17);
    print_subsection("version", edid,18, 19);
    print_subsection("basic params", edid, 20, 24);
    print_subsection("chroma info", edid, 25, 34);
    print_subsection("established", edid, 35, 37);
    print_subsection("standard", edid, 38, 53);
    print_subsection("descriptor 1", edid, 54, 71);
    print_subsection("descriptor 2", edid, 72, 89);
    print_subsection("descriptor 3", edid, 90, 107);
    print_subsection("descriptor 4", edid, 108, 125);
    print_subsection("extensions", edid, 126, 126);
    print_subsection("checksum", edid, 127, 127);
    pbuff += sprintf(pbuff, "\n");
}

static void dump_supported_vic(struct edid_info *info)
{
    pbuff += sprintf(pbuff, "\n");
    pbuff += sprintf(pbuff, "Supported VICs:\n");
    for (int i = 0; i < info->total_vic; i++)
        pbuff += sprintf(pbuff, "    VIC %3d %s %s\n",
            info->supported_vic[i].vic,
            info->supported_vic[i].name,
            info->supported_vic[i].native ? "(native)" : "");
}

static void dump_ycbcr420_vic(struct edid_info *info)
{
    pbuff += sprintf(pbuff, "\n");
    pbuff += sprintf(pbuff, "Supported YCbCr420 VICs:\n");
    for (int i = 0; i < info->total_ycbcr420_vic; i++)
        pbuff += sprintf(pbuff, "    VIC %3d %s %s %s\n",
            info->ycbcr420_vic[i].vic,
            info->ycbcr420_vic[i].name,
            info->ycbcr420_vic[i].native ? "(native)" : "",
            info->ycbcr420_vic[i].support_regular_sampling_mode ? "(support Y444/Y422/RGB sampling)" : "");
}

static void dump_video_format(struct edid_info *info)
{
    const char *video_format_name[] = {
        "RGB   8:8:8",
        "YCbCr 4:4:4",
        "YCbCr 4:2:2",
        "YCbCr 4:2:0",
    };
    pbuff += sprintf(pbuff, "Supported video format:\n");
    for (size_t i = 0;
            i < sizeof(video_format_name) / sizeof(video_format_name[0]);
            i++) {
        if (!info->fmt[i].supported)
            continue;
        pbuff += sprintf(pbuff, "    %s: %s %s %s %s\n", video_format_name[i],
            info->fmt[i].bits & DEEP_COLOR_48BIT ? "48bit" : "     ",
            info->fmt[i].bits & DEEP_COLOR_36BIT ? "36bit" : "     ",
            info->fmt[i].bits & DEEP_COLOR_30BIT ? "30bit" : "     ",
            info->fmt[i].bits & DEEP_COLOR_24BIT ? "24bit" : "     ");
    }
}

static int dump_bitfields(char *p, const char *prefix, const char** name,
                          int size, int bitfields) {
    char *start = p;
    p += sprintf(p, "%s", prefix);
    for (int i = 0; i < size; i++)
        if (bitfields & (1 << i))
            p += sprintf(p, " %s", name[i]);
    p += sprintf(p, "\n");
    return (p - start);
}

static void dump_colorimetry(struct edid_info *info)
{
    char buf[1024] = {0};
    char *p = buf;
    const char *ck_name[] = {
        "XVYCC601",
        "XVYCC709",
        "SYCC601",
        "ADOBE_YCC601",
        "ADOBE_RGB",
        "BT2020_CYCC",
        "BT2020_YCC",
        "BT2020_RGB"
    };
    dump_bitfields(p, "Supported colorimetry:", ck_name,
            sizeof(ck_name) / sizeof(ck_name[0]), info->colorimetry);
    pbuff += sprintf(pbuff, "\n%s\n", buf);
}

static void dump_hdr_metadata(struct edid_info *info)
{
    const char *eo_name[] = {
        "SDR_LUMINANCE_RANGE",
        "HDR_LUMINANCE_RANGE",
        "SMPTE_ST_2048",
        "FUTURE_EOTF"
    };
    char buffer[2048] = {0};
    char *p = buffer;

    p += sprintf(p, "        metadata_descriptor : %04x\n", info->metadata_descriptor);
    p += sprintf(p, "              max_luminance : %04x\n", info->max_luminance);
    p += sprintf(p, "max_frame_average_luminance : %04x\n", info->max_frame_average_luminance);
    p += sprintf(p, "              min_luminance : %04x\n", info->max_luminance);
    p += dump_bitfields(p, "                       eotf :", eo_name, sizeof(eo_name) / sizeof(eo_name[0]), info->eotf);

    pbuff += sprintf(pbuff, "HDR static metadata:\n%s\n", buffer);
}

static void dump_edidinfo(struct edid_info *info)
{
    pbuff += sprintf(pbuff, "\n");
    pbuff += sprintf(pbuff, "Basic display parameters:\n");
    pbuff += sprintf(pbuff, "    Supported color formats: %s, %s, %s\n",
        info->fmt[FMT_RGB].name, info->fmt[FMT_YUV444].name, info->fmt[FMT_YUV422].name);

    pbuff += sprintf(pbuff, "    sink type: %d\n", info->sink_type);

    pbuff += sprintf(pbuff, "max tmds rate(1.4): %d MHz\n", info->hdmi1p4_max_tmds_clock);
    pbuff += sprintf(pbuff, "max tmds rate(2.0): %d MHz\n", info->hdmi2p0_max_tmds_clock);

    pbuff += sprintf(pbuff, "\n");
    pbuff += sprintf(pbuff, "y420 sampling all  : %s\n", info->y420_sampling_all ? "Yes" : "No");
    pbuff += sprintf(pbuff, "RGB only  : %s\n", info->rgb_only ? "Yes" : "No");
    pbuff += sprintf(pbuff, "3D present: %s\n", info->present_3d ? "Yes" : "No");

    ALOGD("\n%s\n", logbuff);
    pbuff = &logbuff[0];
    memset(pbuff, 0, sizeof(logbuff));

    dump_supported_vic(info);

    ALOGD("\n%s\n", logbuff);
    pbuff = &logbuff[0];
    memset(pbuff, 0, sizeof(logbuff));

    dump_ycbcr420_vic(info);
    dump_video_format(info);
    dump_colorimetry(info);
    dump_hdr_metadata(info);

    ALOGD("\n%s\n", logbuff);
}



static void parse_basic_display_parameters(
                const unsigned char *edid, struct edid_info *out)
{
    /* 1. basic display parameters from block0 */
    if ((edid[0x14] & 0x80) == 0)
        return;

    struct video_format *fmt = &out->fmt[FMT_RGB];
    fmt->name = "RGB 8:8:8";
    fmt->supported = 1;
    fmt->bits |= DEEP_COLOR_24BIT;

#if 0
    if (edid[0x18] & 0x08) {
        fmt = &out->fmt[FMT_YUV444];
        fmt->name = "YCbCr 4:4:4";
        fmt->supported = 1;
        fmt->bits |= DEEP_COLOR_24BIT;
    }
    if (edid[0x18] & 0x10) {
        fmt = &out->fmt[FMT_YUV422];
        fmt->name = "YCbCr 4:2:2";
        fmt->supported = 1;
        fmt->bits |= DEEP_COLOR_24BIT;
    }
#endif
}

static void do_checksum(unsigned char *x)
{
    debug("Checksum: 0x%hhx", x[0x7f]);
    unsigned char sum = 0;
    int i;
    for (i = 0; i < 128; i++)
        sum += x[i];
    if (sum) {
        debug(" (should be 0x%hhx)", (unsigned char)(x[0x7f] - sum));
    } else
        debug(" (valid)");
}

static void cea_hdmi_block_1p4(const unsigned char *x, struct edid_info *info)
{
    static const char *edid_cea_hdmi_modes[] = {
        "3840x2160@30Hz",
        "3840x2160@25Hz",
        "3840x2160@24Hz",
        "4096x2160@24Hz",
    };

    static const int disp_modes[] = {
        DISP_TV_MOD_3840_2160P_30HZ,
        DISP_TV_MOD_3840_2160P_25HZ,
        DISP_TV_MOD_3840_2160P_24HZ,
        0xff,   /* Invalid format */
    };

    static const int standard_vics[] = {
        HDMI3840_2160P_30,
        HDMI3840_2160P_25,
        HDMI3840_2160P_24,
        HDMI4096_2160P_24,
    };

    int length = x[0] & 0x1f;
    if (length > 5) {
        if (x[6] & 0x40) info->fmt[FMT_RGB].bits |= DEEP_COLOR_48BIT;
        if (x[6] & 0x20) info->fmt[FMT_RGB].bits |= DEEP_COLOR_36BIT;
        if (x[6] & 0x10) info->fmt[FMT_RGB].bits |= DEEP_COLOR_30BIT;
        if (x[6] & 0x08) {
            info->fmt[FMT_YUV444].bits = info->fmt[FMT_RGB].bits;
        }
    }
    if (length > 6)
        info->hdmi1p4_max_tmds_clock = x[7] * 5;

    /* parse hdmi vic */
    if ((length > 7) && (x[8] & 0x20)) {
        int skip = 0;
        if (x[8] & 0x80) skip += 2;
        if (x[8] & 0x40) skip += 2;

        if (x[9 + skip] & 0x80) {
            info->present_3d = 1;
        }

        skip += 2;
        int len_vic = (x[8 + skip] & 0xe0) >> 5;

        debug("+++ len 3d %d", (x[8 + skip] & 0x1f) >> 0);
        if (len_vic) {
            skip += 1;
            for (int i = 0; i < len_vic; i++) {
                unsigned char vic = x[8 + skip + i];
                vic--;
                /* TODO: convert to standard vic */
                struct vic_info *saved = info->supported_vic;
                size_t idx = info->total_vic;
                info->total_vic++;
                saved[idx].vic = standard_vics[vic];
                saved[idx].sunxi_disp_mode = disp_modes[vic];
                saved[idx].native = 0;
                saved[idx].support_regular_sampling_mode = 1;
                saved[idx].name = edid_cea_hdmi_modes[vic];

                debug("  > HDMI VIC %02d %02d %s",
                    vic, disp_modes[vic], edid_cea_hdmi_modes[vic]);
            }
        }
    }
}

static void cea_hdmi_block_2p0(const unsigned char *x, struct edid_info *info)
{
    unsigned char encoding = 0;
    info->hdmi2p0_max_tmds_clock = x[5] * 5;
    encoding = x[7] & 0x07;

    info->fmt[FMT_YUV420].name = "YCbCr 4:2:2";
    info->fmt[FMT_YUV420].supported = 1;
    if (encoding & 0x01) info->fmt[FMT_YUV420].bits |= DEEP_COLOR_30BIT;
    if (encoding & 0x02) info->fmt[FMT_YUV420].bits |= DEEP_COLOR_36BIT;
    if (encoding & 0x04) info->fmt[FMT_YUV420].bits |= DEEP_COLOR_48BIT;

    return;
}

static void cea_colorimetry_block(const unsigned char *x, struct edid_info *info)
{
    int length = x[0] & 0x1f;
    if (length >= 3) info->colorimetry = x[2];
}

static void cea_hdr_metadata_block(const unsigned char *x, struct edid_info *info)
{
    int length = x[0] & 0x1f;

    if (length >= 3) {
        info->eotf = x[2];
        info->metadata_descriptor = x[3];
    }
    if (length >= 4) info->max_luminance = x[4];
    if (length >= 5) info->max_frame_average_luminance = x[5];
    if (length >= 6) info->min_luminance = x[6];
}

static void cea_block(const unsigned char *x, struct edid_info *info)
{
    unsigned int oui;
    unsigned char tag_code = (x[0] & 0xe0) >> 5;
    debug("+ CEA tag: %02x", tag_code);

    switch (tag_code) {
    case 0x01:
        /* audio data block */
        break;
    case 0x02:
        /* video data block */
        cea_video_block(x, info);
        break;
    case 0x03:
        /* hdmi vendor-specific data block */
        info->sink_type = 0x02;
        oui = (x[3] << 16) + (x[2] << 8) + x[1];
        if (oui == 0x000c03) cea_hdmi_block_1p4(x, info);
        else if (oui == 0xc45dd8) cea_hdmi_block_2p0(x, info);
        else debug("Unknown IEEE Registration Identifier (0x%08x)", oui);

        debug("HDMI Vendor specific: 0x%08x", oui);
        break;
    case 0x07:
        /* Extended tag */
        switch(x[1]) {
        case 0x00:
            /* video capability data block */
            break;
        case 0x01:
            /* vendor specific video data block */
            break;
        case 0x02:
            /* VESA video display device information data block */
            break;
        case 0x03:
            /* VESA video data block */
            break;
        case 0x04:
            /* HDMI video data block */
            break;
        case 0x05:
            /* colorimetry data block */
            cea_colorimetry_block(x, info);
            break;
        case 0x06:
            /* HDR static metadata data block */
            cea_hdr_metadata_block(x, info);
            break;
        case 0x0e:
            /* video format only allow YCbCr4:2:0 sampling mode */
            cea_y420vdb(x, info);
            break;
        case 0x0f:
            /* video format only allow YCbCr4:2:0 and regular sampling mode */
            cea_y420_capability_map_data_block(x, info);
            break;
        }
        break;
    default:
        break;
    }
}

static int parse_cea(const unsigned char *x, struct edid_info *info)
{
    int version = x[1];
    int offset  = x[2];
    int supported_yuv = 0;

    if (offset < 4) {
        debug("No DTDs present and no non-DTD data");
        return 0;
    }

    if (version == 3) {
        int i;
        debug("%d bytes of CEA data", offset - 4);
        for (i = 4; i < offset; i += (x[i] & 0x1f) + 1) {
            cea_block(x + i, info);
        }
    }

    if (version >= 2) {
        if (x[3] & 0x20) {
            info->fmt[FMT_YUV444].name = "YCbCr 4:4:4";
            info->fmt[FMT_YUV444].supported = 1;
            info->fmt[FMT_YUV444].bits |= DEEP_COLOR_24BIT;
            supported_yuv++;
        }
        if (x[3] & 0x10) {
            info->fmt[FMT_YUV422].name = "YCbCr 4:2:2";
            info->fmt[FMT_YUV422].supported = 1;
            info->fmt[FMT_YUV422].bits |= (DEEP_COLOR_24BIT | DEEP_COLOR_36BIT);
            supported_yuv++;
        }

        if (supported_yuv)
            info->rgb_only = 0;
        else
            info->rgb_only = 1;
    }
    return 0;
}

static int parse_extension(const unsigned char *x, struct edid_info *info)
{
    switch (x[0]) {
    case 0x02:
        debug("CEA extension block: version(%d)", x[1]);
        parse_cea(x, info);
        break;
    default:
        debug("Unknown extension block");
        break;
    }
    return 0;
}

#define EDID_SRC_PATH   "/sys/class/hdmi/hdmi/attr/edid"
int read_edid_info(struct edid_info *info) {
    FILE *fd;
    if ((fd = fopen(EDID_SRC_PATH, "r")) == 0) {
        debug("Open file error, %s", strerror(errno));
        exit(1);
    }
    pbuff = &logbuff[0];

    unsigned char edid[256] = {0};
    int length = fread(edid, 1, 256, fd);
    if (length > 0) {
        debug("length: %d", length);
        dump_breakdown(edid);
    }
    fclose(fd);

    do_checksum(edid);
    if (memcmp(edid, "\x00\xFF\xFF\xFF\xFF\xFF\xFF\x00", 8)) {
        debug("Invalid header, exit");
        return -1;
    }

    memset(info, 0, sizeof(*info));
    info->sink_type = 0x01;
    info->rgb_only  = 1;
    parse_basic_display_parameters(edid, info);

    int num_extensions = edid[0x7e];
    if (num_extensions == 1) {
        debug("Number of extensions to follow: %d", num_extensions);
        parse_extension(edid + 128, info);
    }

    dump_edidinfo(info);
    return 0;
}
