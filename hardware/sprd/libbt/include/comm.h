/******************************************************************************
 *
 *  Copyright (C) 2016 Spreadtrum Corporation
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at:
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 ******************************************************************************/

#ifndef BT_LIBBT_INCLUDE_COMM_H_
#define BT_LIBBT_INCLUDE_COMM_H_

#include "vnd_buildcfg.h"

#ifdef SPRD_WCNBT_SR2351
#include "sr2351.h"
#elif defined (SPRD_WCNBT_MARLIN)
#include "marlin.h"
#elif defined (SPRD_WCNBT_MARLIN2)
#include "marlin2.h"
#elif defined (SPRD_WCNBT_SHARKLE)
#include "sharkle.h"
#elif defined (SPRD_WCNBT_MARLIN3)
#include "marlin3.h"
#elif defined (SPRD_WCNBT_MARLIN3_LITE)
#include "marlin3_lite.h"
#elif defined (SPRD_WCNBT_PIKE2)
#include "pike2.h"
#elif defined (SPRD_WCNBT_SHARKL3)
#include "sharkl3.h"
#elif defined (SPRD_WCNBT_SHARKLEP)
#include "sharklep.h"
#endif




#endif  // BT_LIBBT_INCLUDE_COMM_H_
