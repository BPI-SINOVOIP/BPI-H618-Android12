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

 #ifndef _H_PLATFORM_SUN50IW6P1_
 #define _H_PLATFORM_SUN50IW6P1_

const int _hdmi_supported_modes[] = {
    DISP_TV_MOD_480P           ,
    DISP_TV_MOD_576P           ,
    DISP_TV_MOD_720P_50HZ      ,
    DISP_TV_MOD_720P_60HZ      ,

    DISP_TV_MOD_1080P_24HZ     ,
    DISP_TV_MOD_1080P_50HZ     ,
    DISP_TV_MOD_1080P_60HZ     ,
    DISP_TV_MOD_1080I_50HZ     ,
    DISP_TV_MOD_1080I_60HZ     ,

    DISP_TV_MOD_3840_2160P_24HZ,
    DISP_TV_MOD_3840_2160P_25HZ,
    DISP_TV_MOD_3840_2160P_30HZ,
    DISP_TV_MOD_3840_2160P_60HZ,

    DISP_TV_MOD_4096_2160P_50HZ,
    DISP_TV_MOD_4096_2160P_60HZ,
};

const int _hdmi_supported_3d_modes[] = {
    DISP_TV_MOD_720P_50HZ_3D_FP,
    DISP_TV_MOD_720P_60HZ_3D_FP,
    DISP_TV_MOD_1080P_24HZ_3D_FP,
};

const int _hdmi_perfect_modes[] = {
    DISP_TV_MOD_4096_2160P_60HZ,
    DISP_TV_MOD_4096_2160P_50HZ,
    DISP_TV_MOD_3840_2160P_60HZ,
    DISP_TV_MOD_3840_2160P_50HZ,
    DISP_TV_MOD_1080P_60HZ,
    DISP_TV_MOD_1080P_50HZ,
    DISP_TV_MOD_720P_60HZ,
    DISP_TV_MOD_720P_50HZ,
};

const int _cvbs_supported_modes[] = {
    DISP_TV_MOD_NTSC,
    DISP_TV_MOD_PAL ,
};

#endif

