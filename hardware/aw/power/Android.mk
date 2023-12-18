# Copyright (C) 2011 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#	   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

LOCAL_PATH := $(call my-dir)

# When platform use default implement, no need to define it

SEARCH_TABLES := \
	astar    SUN8IW5P1_A33.c    SUN8IW5P1_A33.h   \
	tulip    SUN50IW1P1_A64.c   SUN50IW1P1_A64.h  \
	kylin    SUN9IW1P1_A80.c    SUN9IW1P1_A80.h   \
	octopus  SUN8IW6P1_A83T.c   SUN8IW6P1_A83T.h  \
	eagle    SUN8IW6P1_A83T.c   SUN8IW6P1_A83T.h  \
	neptune  SUN50IW3P1_VR9.c   SUN50IW3P1_VR9.h  \
	uranus   SUN50IW3P1_A63.c   SUN50IW3P1_A63.h  \
	venus    SUN8IW15P1_A50.c   SUN8IW15P1_A50.h  \
	t3       Default.c          SUN8IW11P1_T3.h   \
	t7       Default.c          SUN8IW17P1_T7.h   \
	t8       Default.c          SUN8IW6P1_T8.h    \
	petrel   Default.c          SUN50IW6P1_H6.h   \
	cupid    Default.c          SUN50IW9P1_H616.h \
	apollo   Default.c          SUN50IW9P1_H616.h \
	ceres    Default.c          SUN50IW10P1_A100.h

POS_X     = $(if $(findstring $1,$2),$(call POS_X,$1,$(wordlist 2,$(words $2),$2),x $3),$3)
POS_N     = $(words $(call POS_X,$1,$2))

PLAT_IDX := $(call POS_N,$(TARGET_BOARD_PLATFORM),$(SEARCH_TABLES))
CSRC_VAL := Default.c
HINC_VAL := Default.h

ifneq ($(PLAT_IDX),0)
CSRC_IDX := $(shell expr $(PLAT_IDX) + 1)
HINC_IDX := $(shell expr $(PLAT_IDX) + 2)
CSRC_VAL := $(word $(CSRC_IDX),$(SEARCH_TABLES))
HINC_VAL := $(word $(HINC_IDX),$(SEARCH_TABLES))
endif

include $(CLEAR_VARS)
LOCAL_MODULE := power.$(TARGET_BOARD_PLATFORM)
LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_PROPRIETARY_MODULE := true
LOCAL_SRC_FILES := power.c

LOCAL_C_INCLUDES += \
	system/core/libutils/include \
	hardware/libhardware/include \
	system/core/libsystem/include

LOCAL_CFLAGS += \
	-include platform/$(HINC_VAL) \
	-Wno-unused-parameter \
	-Wno-unused-function

ifeq ($(BOARD_POWER_SUPPLY_TYPE), pmu_plus_bmu)
LOCAL_CFLAGS   += -DPMU_PLUS_BMU
endif

LOCAL_SRC_FILES  += platform/$(CSRC_VAL)
LOCAL_SHARED_LIBRARIES := liblog libcutils
LOCAL_MODULE_TAGS := optional
include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_SRC_FILES:= powertest.c
LOCAL_MODULE:= powertest
LOCAL_SHARED_LIBRARIES := libcutils liblog

LOCAL_CFLAGS += \
	-include platform/$(HINC_VAL) \
	-Wno-unused-parameter \
	-Wno-unused-function

LOCAL_SRC_FILES  += platform/$(CSRC_VAL)
include $(BUILD_EXECUTABLE)

