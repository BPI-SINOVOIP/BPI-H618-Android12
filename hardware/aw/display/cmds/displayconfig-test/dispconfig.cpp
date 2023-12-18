
#include <stdio.h>
#include <vendor/display/config/1.0/IDisplayConfig.h>

#include "modes.h"

using ::vendor::display::config::V1_0::IDisplayConfig;
using ::vendor::display::config::V1_0::LayerMode;
using ::vendor::display::config::V1_0::Dataspace;
using ::vendor::display::config::V1_0::ScreenMargin;
using android::hardware::hidl_string;
using android::hardware::hidl_vec;

class Client {
public:
    Client() : mDisplayConfig(IDisplayConfig::getService()) { }

    std::vector<uint32_t> getSupportedModes(int display);
    void switchMode(int display, int mode)     { mDisplayConfig->setActiveMode(display, mode); }
    void set3DLayerMode(int display, int mode) { mDisplayConfig->set3DLayerMode(display, static_cast<LayerMode>(mode)); };
    bool support3D(int display)                       { return mDisplayConfig->supported3D(display); }
    Dataspace getDataspaceMode(int display)           { return mDisplayConfig->getDataspaceMode(display); }
    int setDataspaceMode(int display, Dataspace mode) { return mDisplayConfig->setDataspaceMode(display, mode); }
    PixelFormat getPixelFormat(int display)           { return mDisplayConfig->getPixelFormat(display); }
    int setPixelFormat(int display, PixelFormat f)    { return mDisplayConfig->setPixelFormat(display, f); }
    int setOutputMargin(int display, ScreenMargin margin) { return mDisplayConfig->setScreenMargin(display, margin); }
    void dump();

private:
    android::sp<IDisplayConfig> mDisplayConfig = IDisplayConfig::getService();
};

std::vector<uint32_t> Client::getSupportedModes(int display)
{
    std::vector<uint32_t> outmodes;
    mDisplayConfig->getSupportedModes(display,
        [&](const hidl_vec<uint32_t>& modes) {
            std::vector<uint32_t> v = modes;
            outmodes.clear();
            outmodes.swap(v);
        });
    return outmodes;
}

void Client::dump() {
    hidl_string info;
    mDisplayConfig->dumpDebugInfo(
        [&](const hidl_string& debugInfo){
            info = debugInfo;
        });
    printf("%s\n", info.c_str());
}

struct usropt {
    int printmodes;
    int dumpinfo;
    int display;
    int setmode;
    int newmode;
    int layermode;
    Dataspace dataspace;

    int setpixelformat;
    PixelFormat pixelformat;

    int margin[2];
};

static struct usropt inputopt;

static void usage(char *name)
{
    fprintf(stderr, "Usage: %s [-p] [-o device] [-s mode] [-l mode] [-c hdr] [-d] [-h]\n", name);
    fprintf(stderr, "   -p: Print supported hdmi modes\n");
    fprintf(stderr, "   -s: Swith mode\n");
    fprintf(stderr, "   -l: Set 3D output layer mode\n");
    fprintf(stderr, "   -o: Which display\n");
    fprintf(stderr, "   -c: Config dataspace: 'hdr|sdr|auto'\n");
    fprintf(stderr, "   -f: Config pixel format: 'rgb8|yuv444-8|yuv422-10|yuv420-10'\n");
    fprintf(stderr, "   -m: Config output content margin in percentage: 80,90\n");
    fprintf(stderr, "   -d: Dump displayd info\n");
    fprintf(stderr, "   -h: Print this message\n");
}

static void parseOpt(int argc, char **argv)
{
    memset(&inputopt, 0, sizeof(inputopt));
    inputopt.layermode = -1;
    inputopt.dataspace = Dataspace::DATASPACE_MODE_OTHER;

    int c;

    while (1) {
        c = getopt(argc, argv, "pdo:s:l:c:f:m:h");
        if (c == EOF)
            break;
        switch (c) {
            case 'p':
                inputopt.printmodes = 1;
                break;
            case 'd':
                inputopt.dumpinfo = 1;
                break;
            case 's':
                if (optarg) {
                    inputopt.newmode = strtoul(optarg, NULL, 0);
                    inputopt.setmode = 1;
                }
                break;
            case 'l':
                if (optarg) {
                    inputopt.layermode = strtoul(optarg, NULL, 0);
                }
                break;

            case 'o':
                if (optarg) {
                    inputopt.display = strtoul(optarg, NULL, 0);
                }
                break;
            case 'c':
                if (optarg) {
                    if (strstr(optarg, "sdr") != nullptr)
                        inputopt.dataspace = Dataspace::DATASPACE_MODE_SDR;
                    else if (strstr(optarg, "hdr") != nullptr)
                        inputopt.dataspace = Dataspace::DATASPACE_MODE_HDR;
                    else if (strstr(optarg, "wcg") != nullptr)
                        inputopt.dataspace = Dataspace::DATASPACE_MODE_WCG;
                    else if (strstr(optarg, "auto") != nullptr)
                        inputopt.dataspace = Dataspace::DATASPACE_MODE_AUTO;
                }
                break;
            case 'f':
                if (optarg) {
                    inputopt.setpixelformat = 1;
                    if (strstr(optarg, "rgb-8") != nullptr)
                        inputopt.pixelformat = PixelFormat::PIXEL_FORMAT_RGB888_8bit;
                    else if (strstr(optarg, "yuv444-8") != nullptr)
                        inputopt.pixelformat = PixelFormat::PIXEL_FORMAT_YUV444_8bit;
                    else if (strstr(optarg, "yuv422-10") != nullptr)
                        inputopt.pixelformat = PixelFormat::PIXEL_FORMAT_YUV422_12bit;
                    else if (strstr(optarg, "yuv420-10") != nullptr)
                        inputopt.pixelformat = PixelFormat::PIXEL_FORMAT_YUV420_10bit;
                    else
                        inputopt.setpixelformat = 0;
                }
                break;
            case 'm':
                if (optarg) {
                    if (sscanf(optarg, "%d,%d", &inputopt.margin[0], &inputopt.margin[1]) != 2) {
                        usage(argv[0]);
                        exit(1);
                    }
                }
                break;
            case 'h':
            default:
                usage(argv[0]);
                exit(1);
        }
    }

    if (optind != argc) {
        usage(argv[0]);
        exit(1);
    }
}

int main(int argc, char** argv) {
    parseOpt(argc, argv);
    Client hwclient;

    if (inputopt.printmodes) {
        std::vector<uint32_t> supported = hwclient.getSupportedModes(inputopt.display);
        printf("Display.%d Total supported modes %zu:\n", inputopt.display, supported.size());
        for (unsigned int i = 0; i < supported.size(); i++) {
            modename n = getModenameByVendorId(supported[i]);
            printf("  - [%02d] %s\n", n.vendorid, n.name);
        }
    }

    if (inputopt.dumpinfo) {
        hwclient.dump();
    }

    if (inputopt.setmode) {
        modename n = getModenameByVendorId(inputopt.newmode);
        printf("Display.%d Switch to mode: %s [%d]\n", inputopt.display, n.name, n.vendorid);
        hwclient.switchMode(inputopt.display, inputopt.newmode);
    }

    if (inputopt.layermode >= 0) {
        const char* name = layerModeName(inputopt.layermode);
        if (!hwclient.support3D(inputopt.display)) {
            printf("Display.%d not support 3D output\n", inputopt.display);
            return 0;
        }
        printf("Display.%d Switch to 3D: %s [%d]\n", inputopt.display, name, inputopt.layermode);
        hwclient.set3DLayerMode(inputopt.display, inputopt.layermode);
    }

    if (inputopt.dataspace != Dataspace::DATASPACE_MODE_OTHER) {
        Dataspace dataspace  = hwclient.getDataspaceMode(inputopt.display);
        printf("Display.%d current dataspace [%s]\n", inputopt.display, dataspaceName(dataspace));
        int ret = hwclient.setDataspaceMode(inputopt.display, inputopt.dataspace);
        printf("Display.%d Switch to dataspace [%s] %s\n",
                inputopt.display, dataspaceName(inputopt.dataspace), ret==0 ? "success" : "failed");
    }

    if (inputopt.setpixelformat) {
        PixelFormat format = hwclient.getPixelFormat(inputopt.display);
        printf("Display.%d current pixelformat [%s]\n", inputopt.display, pixelformtName(format));
        int ret = hwclient.setPixelFormat(inputopt.display, inputopt.pixelformat);
        printf("Display.%d Switch to pixelformat[%s] %s\n",
                inputopt.display, pixelformtName(inputopt.pixelformat), ret==0 ? "success" : "failed");
    }

    if (inputopt.margin[0] != 0) {
        ScreenMargin margin;
        margin.left   = inputopt.margin[0];
        margin.right  = inputopt.margin[0];
        margin.top    = inputopt.margin[1];
        margin.bottom = inputopt.margin[1];
        printf("Display.%d output margin [%d %d]\n", inputopt.display, margin.left, margin.top);
        hwclient.setOutputMargin(inputopt.display, margin);
    }

    return 0;
}
