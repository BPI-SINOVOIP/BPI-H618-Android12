#ifndef WRITEBACK_DEF_H_
#define WRITEBACK_DEF_H_
#define SRC 0 //source de hardware index
#define DST 1 //dest de hardware index
#define WBID 0xff //fake logcal id for wb
#define FBWD 1920 //default ui width
#define FBHG 1080 //default ui height
#define SCWD 1280 //default screen width
#define SCHG 720 //default screen height
#define SCRATE 60 //default screen height
#define HPER 100 //default horizontal persent
#define VPER 100 //default vertical present
#define BUFSZ 6 //default buffer size
#define TIMEOUT 3000 //default fence timeout
#define MINCACHE 3 //default cache size
#define FRMT DISP_FORMAT_ABGR_8888 // default format
#define DEFAULT_TYPE DISP_OUTPUT_TYPE_HDMI
#define DEFAULT_MODE DISP_TV_MOD_720P_60HZ
#define SELF_WB_TYPE DISP_OUTPUT_TYPE_RTWB
#define SELF_WB_MODE DISP_TV_MOD_1080P_60HZ
#endif
