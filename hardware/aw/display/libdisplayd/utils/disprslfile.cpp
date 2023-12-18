
#include "debug.h"
#include "utils.h"
#include "hardware/sunxi_display2.h"

#define MAX_MODE_STRING_LEN (128)
#define MODE_FILE_NAME      "/Reserve0/disp_rsl.fex"

extern int write_to_file(const char *path, const char *buffer, int i);
extern int read_from_file(const char *path, char *buf, size_t size);

static inline int typeToLineNumbers(int type) {
    switch (type) {
        case DISP_OUTPUT_TYPE_TV:   return 0;
        case DISP_OUTPUT_TYPE_HDMI: return 1;
        case DISP_OUTPUT_TYPE_VGA:  return 2;
        default: return -1;
    }
}

// save device config to /Resolve0::disp_rsl.fex
void saveDeviceConfigToFile(const disp_device_config& config)
{
    int type = config.type;
    int mode = config.mode;
    int pack = ((type & 0xFF) << 8) | (mode & 0xFF);
    int lineNumbers = typeToLineNumbers(type);

    if (lineNumbers < 0) {
        // unknow output type
        return;
    }

    char valueString[MAX_MODE_STRING_LEN] = {0};
    int length = read_from_file(MODE_FILE_NAME, valueString, MAX_MODE_STRING_LEN);
    if (length <= 0) {
        dd_error("getFileData failed with I/O error");
    }

    // pack fromat decode from file
    int values[3] = {0, 0, 0};
    char *hdmiconfigStr = nullptr;
    if (length > 0) {
        char *begin = valueString;
        char *iter  = valueString;
        char *end   = valueString + length;
        for(int i = 0; (i < 4) && (iter != end); iter++) {
            if ((*iter == '\n') || (*iter == '\0')) {
                *iter = '\0';
                if (strstr(begin, "hdmi_config=")) {
                    asprintf(&hdmiconfigStr, "%s", begin);
                    begin = iter + 1;
                    continue;
                }
                values[i] = (int)strtoul(begin, NULL, 16);
                begin = iter + 1;
                i++;
            }
        }
    }

    values[lineNumbers] = pack;
    memset(valueString, 0, MAX_MODE_STRING_LEN);
    int slen = sprintf(valueString, "%x\n%x\n%x\n", values[0], values[1], values[2]);

    // append hdmi device config at the end of file
    if (type == DISP_OUTPUT_TYPE_HDMI) {
        slen += sprintf(valueString + slen, "hdmi_config=%d,%d,%d,%d,%d\n",
                config.format, config.bits, config.eotf, config.cs, config.dvi_hdmi);
    } else if (hdmiconfigStr != nullptr) {
        slen += sprintf(valueString + slen, "%s\n", hdmiconfigStr);
        free(hdmiconfigStr);
    }

    write_to_file(MODE_FILE_NAME, valueString, slen);
}
