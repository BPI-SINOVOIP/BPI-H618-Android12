/*
 * Copyright (C) 2016 The Android Open Source Project
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

#ifndef _USECASE_H_
#define _USECASE_H_

typedef enum {
    UC_NODE = 0x0,
    UC_DUAL_SPK = 0x1,  /* use double speaker  */
    UC_DMIC = 0x2,      /* use Dmic */
    UC_EAR = 0x4,       /* use earpiece */
    UC_DPHONE = 0x8,    /* use digital phone(modem) */

} use_case_t;

use_case_t get_use_case();

#endif
