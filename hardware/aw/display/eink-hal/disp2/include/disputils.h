/*
 * Copyright (C) 2019 The Android Open Source Project
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

#ifndef _disp_utils_
#define _disp_utils_

#include <linux/fb.h>
#include "hardware/sunxi_display2.h"
#include "sunxi_eink.h"

// dev_composer defines -->
struct syncinfo {
    int fd;
    unsigned int count;
};
// dev_composer defines <--

// disp sync opt
int createSyncTimeline(int disp);
int destroySyncTimeline(void);

// disp device opt
int submitLayer(unsigned int syncnum, disp_layer_config2* configs,
		int configCount, struct eink_img* cur_img,
		struct eink_img* last_img);
int vsyncCtrl(int disp, int enable);
int blankCtrl(int disp, int enable);
int switchDisplay(int disp, int type, int mode = 0);

int getDeFrequency();

int getFramebufferVarScreenInfo(struct fb_var_screeninfo* info);
int getDisplayOutputType(int disp);
int getDisplayOutputSize(int disp, int* width, int* height);
const char *outputType2Name(int type);
int eink_update_image(struct eink_upd_cfg *config);
int GetFreeBufSlot(struct buf_slot *slot);
int SetGCCnt(int gc_cnt);
int createSyncpt(syncinfo* info);
int FenceSignal(unsigned int syncnum);
int eink_handwrite_dma_map(bool map, int fd, u32 *paddr);
int regal_process(struct eink_img* cur_img,
               struct eink_img* last_img);
#define EINK_ALIGN(x,a) (((x) + (a) - 1L) & ~((a) - 1L))

#endif
