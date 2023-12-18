
#include <stdio.h>
#include <vendor/display/config/1.0/IDisplayConfig.h>
#include <vendor/display/config/1.0/types.h>

using ::vendor::display::config::V1_0::IDisplayConfig;
using ::vendor::display::config::V1_0::HdcpAuthorizedStatus;
using ::vendor::display::config::V1_0::HdcpLevel;
using android::hardware::hidl_string;
using android::hardware::hidl_vec;

struct usropt {
    int config;
    int enable;
    int dump;
};

static struct usropt inputopt;

static void usage(char *name)
{
    fprintf(stderr, "Usage: %s [-p] [-h] [-e enable]\n", name);
    fprintf(stderr, "   -p: print hdcp info\n");
    fprintf(stderr, "   -e: enable or disable hdcp: [-e 1] or [-e 0]\n");
}

static void parseOpt(int argc, char **argv)
{
    memset(&inputopt, 0, sizeof(inputopt));

    int c;

    while (1) {
        c = getopt(argc, argv, "phe:");
        if (c == EOF)
            break;
        switch (c) {
            case 'p':
                inputopt.dump= 1;
                break;
            case 'e':
                if (optarg) {
                    inputopt.config = 1;
                    inputopt.enable = strtoul(optarg, NULL, 0);
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
    android::sp<IDisplayConfig> mDisplayConfig = IDisplayConfig::getService();

    if (inputopt.config) {
        int ret = mDisplayConfig->configHdcp(inputopt.enable ? true : false);
        printf("%s hdcp return %d\n", inputopt.enable ? "enable" : "disable", ret);
    }

    if (inputopt.dump) {
        HdcpAuthorizedStatus status = mDisplayConfig->getAuthorizedStatus();
        printf("hdcp authorized status: %s\n",
                ::vendor::display::config::V1_0::toString(status).c_str());

        HdcpLevel level = mDisplayConfig->getConnectedHdcpLevel();
        printf("hdcp level: %s\n",
                ::vendor::display::config::V1_0::toString(level).c_str());
    }
    return 0;
}

