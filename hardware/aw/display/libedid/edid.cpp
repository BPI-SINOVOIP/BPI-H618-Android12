/*
 * Copyright (C) 2017 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define _DEBUG_TAG TAG_EDID

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <ctype.h>
#include <unistd.h>

#include "edid/edid.h"
#include "edid/cea_vic.h"
#include "debug.h"

#define DESCRIPTOR1  (0x36)
#define DESCRIPTOR2  (0x48)
#define DESCRIPTOR3  (0x5A)
#define DESCRIPTOR4  (0x6C)

/* CEA Data block tag codes */
#define VIDEO_DATA_BLOCK            (0x02)
#define VENDOR_SPECIFIC_DATA_BLOCK  (0x03)
#define EXTENDED_TAG                (0x07)

/* extended code */
#define COLORIMETRY_DATA_BLOCK              (0x05)
#define HDR_STATIC_METADATA_DATA_BLOCK      (0x06)
#define YCBCR420_VIDEO_DATA_BLOCK           (0x0E)
#define YCBCR420_CAPABILITY_MAP_DATA_BLOCK  (0x0F)
#define VENDOR_SPECIFIC_VIDEO_DATA_BLOCK    (0x01)
#define HF_EEODB_DATA_BLOCK                (0x78)

static int read_from_file(const char *path, unsigned char *buf, size_t size)
{
    int fd = open(path, O_RDONLY, 0);
    if (fd == -1) {
        dd_error("Could not open '%s', %s(%d)", path, strerror(errno), errno);
        return -1;
    }
    ssize_t count = read(fd, buf, size);
    close(fd);
    return count;
}

static int do_checksum(unsigned char *x)
{
    unsigned char sum = 0;
    int i;
    for (i = 0; i < 128; i++)
        sum += x[i];
    if (sum) {
        dd_error("Checksum = %02x (should be 0x%02x)",
                 x[0x7f], (unsigned char)(x[0x7f] - sum));
        return -1;
    }
    return 0;
}

EdidParser::EdidParser(const char *path)
  : mPath(path),
    mStandardTiming(),
    mDetailedTiming(),
    mHdmi4k2kVIC(),
    mSupportedVIC(),
    mY420VIC(),
    mSupportedFormat() {}

EdidParser::~EdidParser()
{
    clear();
}

void EdidParser::clear()
{
    memset(edid, 0, sizeof(edid));
    memset(mMonitorName, 0, sizeof(mMonitorName));
    memset(mVendorName, 0, sizeof(mVendorName));
    mProductId = 0;
    mSerialNumber = 0;
    mManufactureWeek = 0;
    mManufactureYear = 0;
    mVersion = 0;
    mDisplaySizeH = 0;
    mDisplaySizeV = 0;

    mStandardTiming.clear();
    mYCbCr420CapabilityMap.clear();
    mY420SamplingAny = 0;
    mMaxTmdsCharacterRate1p4 = 0;
    mMaxTmdsCharacterRate2p0 = 0;
    mColorimetry = 0;
    m3Dpresent = 0;
    mRGBOnly = 1;
    mSinkType = SINK_TYPE_DVI;
    mExtensionBlock = 0;
    mHDMIVersion = 0;

    mEotf = 0;
    mMetadataDescriptor = 0;
    mMaxLuminance = 0;
    mMaxFrameAverageLuminance = 0;
    mMinLuminance = 0;
    mAppVer = 0;

    for (auto it = mHdmi4k2kVIC.begin(); it != mHdmi4k2kVIC.end();) {
        videoInformationCode *v = *it;
        delete v;
        it = mHdmi4k2kVIC.erase(it);
    }
    for (auto it = mDetailedTiming.begin(); it != mDetailedTiming.end();) {
        char *t = *it;
        delete t;
        it = mDetailedTiming.erase(it);
    }
    for (auto it = mSupportedFormat.begin(); it != mSupportedFormat.end();) {
        videoFormat *f = it->second;
        delete f;
        it = mSupportedFormat.erase(it);
    }
    for (auto it = mSupportedVIC.begin(); it != mSupportedVIC.end();) {
        videoInformationCode *f = *it;
        delete f;
        it = mSupportedVIC.erase(it);
    }
    for (auto it = mY420VIC.begin(); it != mY420VIC.end();) {
        videoInformationCode *f = *it;
        delete f;
        it = mY420VIC.erase(it);
    }
}

int EdidParser::parse(const char *path)
{
    clear();
    if (read_from_file(path, edid, EDID_MAX_LENGTH) < 0) {
        dd_error("read edid from '%s' failed", path);
        return -1;
    }

    /* check block 0 first 8 bytes */
    if(memcmp(edid, "\x00\xFF\xFF\xFF\xFF\xFF\xFF\x00", 8)) {
        dd_error("No header found");
        return -1;
    }

    if (do_checksum(&edid[0]) != 0) {
        dd_error("block0 edid checksum error");
        return -1;
    }

    /* block0 */
    dd_info("Parse block0:");
    parseMonitorInfo();
    parseStandardTiming();

    parseDescriptorBlock(DESCRIPTOR1);
    parseDescriptorBlock(DESCRIPTOR2);
    parseDescriptorBlock(DESCRIPTOR3);
    parseDescriptorBlock(DESCRIPTOR4);

    /* extension block */
    parseCeaExtensionBlock();

    updateYCbCr420CapabilityVIC();

    /*Correcting 4k60/4k50 base on MaxTmds Clock*/
    /*In order to follow the HDMI CTS:HF1-31 iter06*/
    for (auto it = mSupportedVIC.begin(); it != mSupportedVIC.end();) {
        videoInformationCode *f = *it;
        if (f->vic == HDMI_3840x2160p50_16x9 || f->vic == HDMI_3840x2160p60_16x9
              || f->vic == HDMI_4096x2160p50_256x135 || f->vic == HDMI_4096x2160p60_256x135) {
            if (mMaxTmdsCharacterRate1p4 < 340 && mMaxTmdsCharacterRate2p0 < 340) {
               it = mSupportedVIC.erase(it);
               delete f;
               continue;
            }
        }

        it++;
    }

    for (auto it = mY420VIC.begin(); it != mY420VIC.end();) {
        videoInformationCode *f = *it;
        if (f->vic == HDMI_3840x2160p50_16x9 || f->vic == HDMI_3840x2160p60_16x9
              || f->vic == HDMI_4096x2160p50_256x135 || f->vic == HDMI_4096x2160p60_256x135) {
            if (mMaxTmdsCharacterRate1p4 < 297 && mMaxTmdsCharacterRate2p0 < 297) {
                it = mY420VIC.erase(it);
                delete f;
                continue;
            }
        }

        it++;
    }

    return 0;
}

int EdidParser::parseMonitorInfo()
{
    /* Vendor Name */
    mVendorName[0] = ((edid[8] & 0x7C) >> 2) + '@';
    mVendorName[1] = ((edid[8] & 0x03) << 3) + ((edid[9] & 0xE0) >> 5) + '@';
    mVendorName[2] = ( edid[9] & 0x1F) + '@';
    mVendorName[3] = 0;

    /* Product identify id */
    mProductId = ((uint16_t)edid[11] << 8) | edid[10];

    /* Serial number */
    mSerialNumber = (edid[15] << 24) | (edid[14] << 16) |
                    (edid[13] << 0 ) | edid[12];

    mManufactureWeek = edid[16];
    mManufactureYear = (edid[17] <= 0xf) ? 1990 : edid[17] + 1990;

    mVersion = (edid[18] << 8) | edid[19];

    mDisplaySizeH = edid[21];
    mDisplaySizeV = edid[22];

    /* XXX: always support rgb output */
    getVideoFormat(_videoFmtRGB);
    return 0;
}

#define NEMLEN(_array) (sizeof(_array) / sizeof(_array[0]))
int EdidParser::parseStandardTiming()
{
    struct standard_timing {
        uint8_t vic;
        uint8_t ratio;
        uint32_t horizontal_pixel;
        uint32_t frame_rate;
    };
    const struct standard_timing _standard_timing_tab[] = {
        {HDMI_480i60_16x9, 0x03,  720, 30},
        {HDMI_480i60,      0x01,  720, 30},
        {HDMI_576i50_16x9, 0x03,  720, 25},
        {HDMI_576i50,      0x01,  720, 25},
        {HDMI_480p60_16x9, 0x03,  720, 60},
        {HDMI_480p60,      0x01,  720, 60},
        {HDMI_576p50_16x9, 0x03,  720, 50},
        {HDMI_576p50,      0x01,  720, 50},
        {HDMI_720p60,      0x03, 1280, 60},
        {HDMI_720p50,      0x03, 1280, 50},
        {HDMI_1080i60,     0x03, 1920, 30},
        {HDMI_1080i50,     0x03, 1920, 25},
        {HDMI_1080p60,     0x03, 1920, 60},
        {HDMI_1080p50,     0x03, 1920, 50},
        {HDMI_1080p24,     0x03, 1920, 24},
        {HDMI_1080p25,     0x03, 1920, 25},
        {HDMI_1080p30,     0x03, 1920, 30},
    };

    uint8_t *data = &edid[38];
    size_t length = 8;
    uint32_t width, freq;
    uint8_t ratio;

    for (size_t i = 0; i < length; i++) {
        uint8_t byte0 = data[i*2];
        uint8_t byte1 = data[i*2 + 1];
        if ((byte0 == 0x01) || (byte1 == 0x01))
            continue;

        /*  X resolution, divided by 8, less 31 */
        width = (byte0 + 31) * 8;
        freq  = (byte1 & 0x3F) + 60;
        ratio = (byte1 >> 6) & 0x03;

        for (size_t j = 0; j < NEMLEN(_standard_timing_tab); j++) {
            const struct standard_timing *info = &_standard_timing_tab[j];
            if ((info->horizontal_pixel == width) &&
                    (info->frame_rate == freq) && (info->ratio == ratio)) {
                mStandardTiming.push_back(info->vic);
            }
        }
    }
    return 0;
}

int EdidParser::parseDescriptorBlock(size_t begin)
{
    uint8_t* x = &edid[begin];

    /* not a detailed timing descriptor */
    if (x[0] == 0 && x[1] == 0) {
        if (x[2] != 0 || x[3] != 0xFC) {
            /* No a Monitor name (text), skip */
            return 0;
        }
        char tmp[16] = {0};
        strncat((char *)tmp, (char *)x + 5, 13);
        for (int i = 0; i < 13; i++) {
            if (tmp[i] == 0x0a) {
                mMonitorName[i] = 0x00;
                break;
            } else if (((tmp[i] & 0x80) == 0) && isprint(tmp[i])) {
                mMonitorName[i] = tmp[i];
            } else {
                mMonitorName[i] = 0x00;
                break;
            }
        }
        return 0;
    }

    /* detailed timing descriptor */
    int hsync_active = (x[2] + ((x[4] & 0xF0) << 4));
    int vsync_active = (x[5] + ((x[7] & 0xF0) << 4));
    int interlaced = (x[17] & 0x80);
    float pixel_clock = (x[0] + (x[1] << 8)) / 100.0; /* MHz */

    char *str = 0;
    (void)asprintf(&str, "%4dx%4d pixel clock %.3f MHz %s",
        hsync_active, vsync_active, pixel_clock, interlaced ? "interlaced" : "");
    mDetailedTiming.push_back(str);
    return 0;
}

int EdidParser::parseCeaExtensionBlock()
{
    dd_info("Extension block count: %d", edid[0x7E]);
    mExtensionBlock = edid[0x7E];

    /* check Extension flag at block 0 */
    if (edid[0x7E] == 0)
        return 0;
    else if (edid[0x80] != 0x2)
        /* check block 1 extension tag */
        /* 02h for CEA EDID */
        return 0;

    unsigned char *x = &edid[0x80];
    int version = x[1];
    int offset  = x[2];
    dd_info("Parse CEA extension block (version: %d)", version);

    if (version == 3) {
        dd_info("CEA data length: %d bytes", offset - 4);
        for (int i = 4; i < offset; i += (x[i] & 0x1f) + 1) {
            parseCeaDataBlocks(x+i);
        }
    }

    if (version >= 2) {
        int yuvSupported = 0;
        if (x[3] & 0x20) {
            getVideoFormat(_videoFmtYUV444);
            yuvSupported++;
        }
        if (x[3] & 0x10) {
            videoFormat *yuvfmt = getVideoFormat(_videoFmtYUV422);
            yuvfmt->dc_36bit = 1;
            yuvSupported++;
        }
        if (yuvSupported)
            mRGBOnly = 0;
    }

    /* check block 1 checksum */
    if (do_checksum(&edid[0x80])) {
        dd_error("block1 checksum error");
        return -1;
    }

    return 0;
}

void EdidParser::parseCeaDataBlocks(const unsigned char *x)
{
    unsigned char tagcode = (x[0] & 0xE0) >> 5;
    switch (tagcode) {
    case VIDEO_DATA_BLOCK:
        videoDataBlock(x);
        break;
    case VENDOR_SPECIFIC_DATA_BLOCK:
        vendorSpecificDataBlock(x);
        break;
    case EXTENDED_TAG:
        switch (x[1]) {
        case COLORIMETRY_DATA_BLOCK:
            colorimetryDataBlock(x);
            break;
        case HDR_STATIC_METADATA_DATA_BLOCK:
            hdrStaticMetadataBlock(x);
            break;
        case YCBCR420_VIDEO_DATA_BLOCK:
            YCbCr420VideoDataBlock(x);
            break;
        case YCBCR420_CAPABILITY_MAP_DATA_BLOCK:
            YCbCr420CapabilityMapDataBlock(x);
            break;
        case HF_EEODB_DATA_BLOCK:
            HfEeodbDataBlock(x);
            break;
        case VENDOR_SPECIFIC_VIDEO_DATA_BLOCK:
            vendorSpecificVideoDataBlock(x);
            break;
        default:
            dd_error("parseCeaDataBlocks:unhandled EXTENDED_TAG code%d", x[1]);
            break;
        }
        break;
    default:
        break;
    }
}


void EdidParser::YCbCr420CapabilityMapDataBlock(const unsigned char *x)
{
    int length = (x[0] & 0x1F) - 1;
    if (length == 0) {
        mY420SamplingAny = 1;
        return;
    }
    for (int i = 0; i < length; i++) {
        int base = i * 8;
        unsigned maps = x[2 + i];
        for (int j = 0; j < 8; j++) {
            /* Only store the vic pos here */
            if (maps & (1 << j)) {
                int pos = base + j;
                mYCbCr420CapabilityMap.push_back(pos);
            }
        }
    }
}

void EdidParser::updateYCbCr420CapabilityVIC()
{
    int supportedY420 = 0;

    if (mY420SamplingAny == 1) {
        /* Every vic supported Y420 sampling */
        for (auto it = mSupportedVIC.begin(); it != mSupportedVIC.end(); ++it) {
            videoInformationCode *f = *it;
            f->ycbcr420_sampling = 1;
        }
        supportedY420 = 1;
    }
    for (auto it = mYCbCr420CapabilityMap.begin();
            it != mYCbCr420CapabilityMap.end(); ++it) {
        size_t pos = *it;
        if (pos >= mSupportedVIC.size()) {
            dd_error("YCbCr420CapabilityMap index(%zu) error", pos);
            continue;
        }
        mSupportedVIC[pos]->ycbcr420_sampling = 1;
        supportedY420 = 1;
    }

    /* Some sink not has hdmi2.0 vsdb, but support y420 */
    if (supportedY420)
        getVideoFormat(_videoFmtYUV420);

}

void EdidParser::YCbCr420VideoDataBlock(const unsigned char *x)
{
    int length = (x[0] & 0x1F) - 1;
    ceaSvd(x+2, length, 0, 1, mY420VIC);

    getVideoFormat(_videoFmtYUV420);
}

void EdidParser::ceaSvd(const unsigned char *x, int n, int r, int y,
                        std::vector<videoInformationCode *>& out)
{
    for (int i = 0; i < n; i++) {
        unsigned char svd = x[i];
        unsigned char native;
        unsigned char vic;

        if ((svd & 0x7F) == 0)
            continue;
        if ((svd - 1) & 0x40) {
            vic = svd;
            native = 0;
        } else {
            vic = svd & 0x7F;
            native = svd & 0x80;
        }
        const char *name = getCeaVicName(vic);
        if (name)
            out.push_back(new videoInformationCode(vic, native, r, y, name));
        else
            dd_error("unknow cea vic %d", vic);
    }
}

void EdidParser::videoDataBlock(const unsigned char *x)
{
    int length = x[0] & 0x1F;
    ceaSvd(x + 1, length, 1, 0, mSupportedVIC);
}

void EdidParser::vendorSpecificDataBlock(const unsigned char *x)
{
    unsigned int oui = (x[3] << 16) + (x[2] << 8) + x[1];
    switch (oui) {
    case HDMI1X_Version:
        hdmi1p4VSDB(x);
        mSinkType = SINK_TYPE_HDMI;
        mHDMIVersion |= 1 << 0;
        break;
    case HDMI20_Version:
        hdmi2p0VSDB(x);
        mSinkType = SINK_TYPE_HDMI;
        mHDMIVersion |= 1 << 1;
        break;
    default:
        dd_error("Unknow IEEE Registration Identifier (0x%08X)", oui);
        break;
    }
}

void EdidParser::vendorSpecificVideoDataBlock(const unsigned char *x)
{
    unsigned int oui = (x[4] << 16) + (x[3] << 8) + x[2];
    switch (oui) {
    case HDR10PLUS_Ver:
        hdr10plusVSDB(x);
        break;
    default:
        dd_error("Unknow IEEE Registration Identifier (0x%08X)", oui);
        break;
    }
}


EdidParser::videoFormat *EdidParser::getVideoFormat(int format)
{
    if (mSupportedFormat.count(format))
        return mSupportedFormat[format];

    videoFormat *newfmt = new videoFormat();
    newfmt->format = format;
    switch (format) {
        case _videoFmtRGB:    newfmt->name = "RGB        "; break;
        case _videoFmtYUV444: newfmt->name = "YCbCr 4:4:4"; break;
        case _videoFmtYUV422: newfmt->name = "YCbCr 4:2:2"; break;
        case _videoFmtYUV420: newfmt->name = "YCbCr 4:2:0"; break;
        default:              newfmt->name = "unknow     "; break;
    }
    mSupportedFormat.emplace(format, newfmt);
    return newfmt;
}

void EdidParser::hdmi1p4VSDB(const unsigned char *x)
{
    int length = x[0] & 0x1F;

    if (length > 5) {
        /* Deep color support */
        videoFormat *rgbfmt = getVideoFormat(_videoFmtRGB);
        if (x[6] & 0x10)
            rgbfmt->dc_30bit = 1;
        if (x[6] & 0x20)
            rgbfmt->dc_36bit = 1;
        if (x[6] & 0x40)
            rgbfmt->dc_48bit = 1;
        if (x[6] & 0x80) {
            videoFormat *yuvfmt = getVideoFormat(_videoFmtYUV444);
            yuvfmt->dc_30bit = rgbfmt->dc_30bit;
            yuvfmt->dc_36bit = rgbfmt->dc_36bit;
            yuvfmt->dc_48bit = rgbfmt->dc_48bit;
        }
    }

    if (length > 6)
        mMaxTmdsCharacterRate1p4 = x[7] * 5;

    if (length > 7 && (x[8] & 0x20)) {
        int skip = 0;
        if (x[8] & 0x80) skip += 2;
        if (x[8] & 0x40) skip += 2;

        /* HDMI video present */
        if (x[8] & 0x20) {
            skip += 2;
            int viclen = (x[8 + skip] & 0xE0) >> 5;

            if (viclen)
                hdmi4k2kVICParse(x + 8 + skip + 1, viclen);

            /* 3D present and 3D multi present */
            if (x[8 + skip - 1] & 0x80)
                m3Dpresent = 1;
        }
    }
    return;
}

void EdidParser::hdmi4k2kVICParse(const unsigned char *x, int size)
{
    const int __extended_vic[] = {
        0,
        HDMI_3840x2160p30_16x9,
        HDMI_3840x2160p25_16x9,
        HDMI_3840x2160p24_16x9,
        HDMI_4096x2160p24_256x135,
    };
    if (size > 4 || !size) {
        dd_error("4k2k vic out of range, size = %d", size);
        return;
    }
    while (size) {
        if (*x >= 1 && *x <=4) {
            int vic = __extended_vic[*x];
            mHdmi4k2kVIC.push_back(
                new videoInformationCode(vic, 0, 1, 0, getCeaVicName(vic)));
        }
        size--;
        ++x;
    }
}

void EdidParser::hdmi2p0VSDB(const unsigned char *x)
{
    mMaxTmdsCharacterRate2p0 = x[5] * 5;
    videoFormat *newfmt = getVideoFormat(_videoFmtYUV420);
    if (x[7] & 0x01)
        newfmt->dc_30bit = 1;
    if (x[7] & 0x02)
        newfmt->dc_36bit = 1;
    if (x[7] & 0x04)
        newfmt->dc_48bit = 1;
}

void EdidParser::hdr10plusVSDB(const unsigned char *x)
{
    mAppVer = x[5] & 0x01;
}


void EdidParser::colorimetryDataBlock(const unsigned char *x)
{
    int length = x[0] & 0x1F;
    if (length >= 3 )
        mColorimetry = x[2];
}

void EdidParser::hdrStaticMetadataBlock(const unsigned char *x)
{
    int length = x[0] & 0x1F;

    if (length >= 3) {
        mEotf = x[2];
        mMetadataDescriptor = x[3];
    }
    if (length >= 4) mMaxLuminance = x[4];
    if (length >= 5) mMaxFrameAverageLuminance = x[5];
    if (length >= 6) mMinLuminance = x[6];
}

void EdidParser::HfEeodbDataBlock(const unsigned char *x)
{
    mExtensionBlockCount = x[2];
    if (mExtensionBlockCount < 2 || mExtensionBlockCount >= EDID_MAX_BLOCK_COUNT) {
        dd_error("invalid mExtensionBlockCount:%d\n", mExtensionBlockCount);
        return;
    }

    for (int b = 2; b <= mExtensionBlockCount; b++) {
        const unsigned char *blockExt = &edid[0x80 * b];

        int version = blockExt[1];
        int offset  = blockExt[2];


        if (version == 3) {
            for (int i = 4; i < offset; i += (blockExt[i] & 0x1f) + 1) {
                parseCeaDataBlocks(blockExt + i);
            }
        }
    }
}

void dumphex(const unsigned char *data, size_t len, size_t split, android::String8& out)
{
    out.append("       ");
    for (size_t i = 0; i < split; i++)
        out.appendFormat("%02zu ", i);
    out.append("\n=======================================================\n");

    size_t i = 0;
    size_t prefix = 0;
    while(i < len) {
        out.appendFormat("0x%02x |", (unsigned int)(prefix * split));
        while (i < len) {
            out.appendFormat(" %02x", data[i]);
            i++;
            if (i % split == 0)
                break;
        }
        out.append("\n");
        prefix++;
    }
}

void EdidParser::dump(android::String8& out)
{
    out.append("__________________________________________________________________\n\n");
    out.append("Block 0 (EDID Base Block), Bytes 0 - 127,  128  BYTES OF EDID CODE:\n");
    dumphex(edid, 128, 16, out);

    out.appendFormat("\n");
    out.appendFormat("  Monitor Name        : %s\n",   mMonitorName);
    out.appendFormat("  ID Manufacture Name : %s\n",   mVendorName);
    out.appendFormat("  ID Product Code     : %04x\n", mProductId);
    out.appendFormat("  ID Serial Number    : %08x\n", mSerialNumber);
    out.appendFormat("  Week of Manufacture : %d\n",   mManufactureWeek);
    out.appendFormat("  Year of Manufacture : %d\n",   mManufactureYear);

    out.appendFormat("\n");
    out.appendFormat("  EDID Version Number : %d\n",   (mVersion >> 8) & 0xff);
    out.appendFormat("  EDID Revision Number: %d\n",   (mVersion >> 0) & 0xff);

    out.appendFormat("\n");
    out.appendFormat("  Maximum Horizontal Image Size: %3d cm\n", mDisplaySizeH);
    out.appendFormat("  Maximum Vertical   Image Size: %3d cm\n", mDisplaySizeV);

    out.appendFormat("\n");
    out.appendFormat("  Standard Timings:\n");
    for (auto it = mStandardTiming.begin(); it != mStandardTiming.end(); ++it)
        out.appendFormat("    %s\n", getCeaVicName(*it));

    out.appendFormat("\n");
    out.appendFormat("  Detailed Timing Descriptor:\n");
    for (auto it = mDetailedTiming.begin(); it != mDetailedTiming.end(); ++it)
        out.appendFormat("    %s\n", *it);

    out.appendFormat("\n");
    out.appendFormat("  Extension Block(s) : %d\n",     mExtensionBlock);
    out.appendFormat("  Checksum Value     : 0x%02x\n", edid[0x7F]);

    out.append("__________________________________________________________________\n\n");
    out.append("Block 1 (CEA-861 Extension Block), Bytes 128 - 255,  128 BYTES OF EDID CODE:\n");
    dumphex(edid + 128, 128, 16, out);

    out.appendFormat("\n");
    out.appendFormat("  Video Data Block:\n");
    for (auto it = mSupportedVIC.begin(); it != mSupportedVIC.end(); ++it) {
        videoInformationCode *v = *it;
        out.appendFormat("    %3d %-25s %8s %s\n",
                v->vic, v->name,
                v->native ? "[native]" : "        ",
                v->ycbcr420_sampling ? "[YCbCr420 sampling]" : "");
    }

    out.appendFormat("\n");
    out.appendFormat("  YCbCr 4:2:0 Video Data Block:\n");
    for (auto it = mY420VIC.begin(); it != mY420VIC.end(); ++it) {
        videoInformationCode *v = *it;
        out.appendFormat("    %3d %-25s %8s\n",
                v->vic, v->name,
                v->native ? "[native]" : "        ");
    }

    out.appendFormat("\n");
    out.appendFormat("  Supported video format:\n");
    for (auto it = mSupportedFormat.begin(); it != mSupportedFormat.end(); ++it) {
        videoFormat *f = it->second;
        out.appendFormat("    %s: %s %s %s %s\n",
            f->name,
            "24bit",
            f->dc_30bit ? "30bit" : "     ",
            f->dc_36bit ? "36bit" : "     ",
            f->dc_48bit ? "40bit" : "     ");
    }

    out.appendFormat("\n");
    out.appendFormat("  Supported Colorimetry: (0x%02X)\n", mColorimetry);
    for (int i = 0; i < 8; i++) {
        const char* __colorimetry[] = {
            "XVYCC601"    ,
            "XVYCC709"    ,
            "SYCC601"     ,
            "ADOBE_YCC601",
            "ADOBE_RGB"   ,
            "BT2020_CYCC" ,
            "BT2020_YCC"  ,
            "BT2020_RGB"  ,
        };
        if (mColorimetry & (1 << i))
            out.appendFormat("    %s\n", __colorimetry[i]);
    }

    out.appendFormat("\n");
    out.appendFormat("  HDR static metadata:\n");
    out.appendFormat("    Supported EOTF:");
    for (int i = 0; i < 4; i++) {
        const char* __eotf[] = {
            "SDR_LUMINANCE_RANGE",
            "HDR_LUMINANCE_RANGE",
            "SMPTE_ST_2084",
            "FUTURE_EOTF"
        };
        if (mEotf & (1 << i))
            out.appendFormat(" %s", __eotf[i]);
    }
    out.appendFormat("\n");
    out.appendFormat("            metadata descriptor : %02x\n", mMetadataDescriptor);
    out.appendFormat("                  max luminance : %02x\n", mMaxLuminance);
    out.appendFormat("    max frame average luminance : %02x\n", mMaxFrameAverageLuminance);
    out.appendFormat("                  min luminance : %02x\n", mMinLuminance);

    out.appendFormat("\n");
    out.appendFormat("  HDMI vendor specific:\n");
    out.appendFormat("    3D present: %d\n", m3Dpresent);
    out.appendFormat("    RGB Only  : %d\n", mRGBOnly);
    out.appendFormat("    Max tmds character rate (1.x) : %3d MHz\n", mMaxTmdsCharacterRate1p4);
    out.appendFormat("    Max tmds character rate (2.0) : %3d MHz\n", mMaxTmdsCharacterRate2p0);
    out.appendFormat("    HDMI Version: %s\n", (mHDMIVersion & 0x01) == 1 ? "support 1.x VSDB" : "not support 1.x VSDB");
    out.appendFormat("    HDMI Version: %s\n", (mHDMIVersion & 0x02) == 2 ? "support 2.0 VSDB" : "not support 2.0 VSDB");
}

int EdidParser::reload()
{
    return parse(mPath);
}
