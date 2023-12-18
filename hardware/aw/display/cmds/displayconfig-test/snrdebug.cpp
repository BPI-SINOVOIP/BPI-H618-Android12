
#include <stdio.h>
#include <vendor/display/config/1.0/IDisplayConfig.h>
#include <vendor/display/config/1.0/types.h>

using ::vendor::display::config::V1_0::IDisplayConfig;
using ::vendor::display::config::V1_0::SNRInfo;
using ::vendor::display::config::V1_0::SNRFeatureMode;
using android::hardware::hidl_string;
using android::hardware::hidl_vec;

struct usropt {
    int enable;
    int demostyle;
    int strength[3];
    int dump;
};

static struct usropt inputopt;

static void usage(char *name)
{
    fprintf(stderr, "Usage: %s [-p] [-h] [-e enable] [-d] [-c y u v]\n", name);
    fprintf(stderr, "   -p: print snr info\n");
    fprintf(stderr, "   -e: enable or disable snr: [-e 1] or [-e 0]\n");
    fprintf(stderr, "   -d: enable demo style\n");
    fprintf(stderr, "   -c: config yuv snr strength\n");
}

static void parseOpt(int argc, char **argv)
{
    memset(&inputopt, 0, sizeof(inputopt));
    inputopt.strength[0] = -1;
    inputopt.enable = -1;

    int c;

    while (1) {
        c = getopt(argc, argv, "phde:c:");
        if (c == EOF)
            break;
        switch (c) {
            case 'p':
                inputopt.dump= 1;
                break;
            case 'e':
                if (optarg) {
                    inputopt.enable = strtoul(optarg, NULL, 0);
                }
                break;
            case 'd':
                inputopt.demostyle = 1;
                break;
            case 'c':
                inputopt.strength[0]= strtoul(optarg,   NULL, 0);
                inputopt.strength[1]= strtoul(argv[optind+0], NULL, 0);
                inputopt.strength[2]= strtoul(argv[optind+1], NULL, 0);
                optind+=2;
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
    android::sp<IDisplayConfig> mDisplayConfig = IDisplayConfig::getService();

    if (inputopt.dump) {
        bool supported = mDisplayConfig->supportedSNRSetting(0);
        int error;
        SNRInfo info;
        mDisplayConfig->getSNRInfo(0, [&](int32_t ret, const SNRInfo& input){
                    error = ret;
                    info = input;
                });
        if (error!= 0) {
            printf("getSNRInfo return: %d\n", error);
        }
        printf("support snr: %d\n", supported);
        printf("mode: %s strength: %d %d %d\n", toString(info.mode).c_str(), info.y, info.u, info.v);
    }

    if (inputopt.enable != -1) {
        SNRInfo info;
        info.mode = inputopt.enable != 0 ?
            (inputopt.demostyle ? SNRFeatureMode::SNR_DEMO : SNRFeatureMode::SNR_CUSTOM) : SNRFeatureMode::SNR_DISABLE;
        info.y = inputopt.strength[0];
        info.u = inputopt.strength[1];
        info.v = inputopt.strength[2]; 
        printf("mode: %s strength: %d %d %d\n", toString(info.mode).c_str(), info.y, info.u, info.v);
        int ret = mDisplayConfig->setSNRInfo(0, info);
        printf("setSNRInfo %s\n", ret == 0 ? "success" : "failed");
    }

    return 0;
}

