/*
 * tvd_interface.h
 */

typedef enum
{
    TVD_CVBS                = 0,
    TVD_YPBPR_I             = 1,
    TVD_YPBPR_P             = 2,
}tvd_interface_t;

typedef enum
{
    TVD_NTSC                = 0,
    TVD_PAL                 = 1,
    TVD_SECAM               = 2,
    TVD_YPBPR               = 3,
}tvd_system_t;

typedef enum
{
    TVD_UV_SWAP             = 0,
    TVD_COLOR_SET           = 1,
}tvd_param_t;

typedef enum
{
    TVD_PL_YUV420           = 0,
    TVD_MB_YUV420           = 1,
    TVD_PL_YUV422           = 2,
}tvd_fmt_t;
typedef enum
{
    TVD_CHANNEL_ONLY_1      = 0,
    TVD_CHANNEL_ONLY_2      = 1,
    TVD_CHANNEL_ONLY_3      = 2,
    TVD_CHANNEL_ONLY_4      = 3,
    TVD_CHANNEL_ALL_2x2     = 4,
    TVD_CHANNEL_ALL_1x4     = 5,
    TVD_CHANNEL_ALL_4x1     = 6,
}tvd_channel_t;