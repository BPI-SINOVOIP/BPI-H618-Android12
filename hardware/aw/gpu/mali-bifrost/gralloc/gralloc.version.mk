#
# Copyright (C) 2016-2019 ARM Limited. All rights reserved.
#
# Copyright (C) 2008 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# List of all valid Gralloc versions
GRALLOC_VALID_VERSIONS := 0.x 1.x 2.x

# Set default Gralloc version
PLATFORM_SDK_GREATER_THAN_24 := $(shell expr $(PLATFORM_SDK_VERSION) \> 24)
ifeq ($(PLATFORM_SDK_GREATER_THAN_24), 1)
    GRALLOC_API_VERSION?=1.x
else
    GRALLOC_API_VERSION?=0.x
endif

ifdef GRALLOC_USE_GRALLOC1_API
    ifeq ($(GRALLOC_USE_GRALLOC1_API), 1)
        GRALLOC_API_VERSION := 1.x
    else
        ifeq ($(GRALLOC_USE_GRALLOC1_API), 0)
            GRALLOC_API_VERSION := 0.x
        endif
    endif
endif

# Fail build if GRALLOC_API_VERSION is not one of the following.
ifeq ($(filter $(GRALLOC_API_VERSION),$(GRALLOC_VALID_VERSIONS)),)
    $(error Gralloc version $(GRALLOC_API_VERSION) is not valid. Valid versions are $(GRALLOC_VALID_VERSIONS))
endif

# Derive namespaces and associated (internal and scaled) versions for Gralloc 2.x
# Note: On a given SDK platform, versions of Allocator / Mapper / Common can be
#       changed (somewhat) independently of each other. Scaled internal versions, encapsulating
#       their major and minor versions, provide for building specific combinations
ifeq ($(GRALLOC_API_VERSION), 2.x)

    ifeq ($(shell expr $(PLATFORM_SDK_VERSION) \> 27), 1)
        HIDL_IMAPPER_NAMESPACE := V2_1
        HIDL_IALLOCATOR_NAMESPACE := V2_0
        HIDL_COMMON_NAMESPACE := V1_1

        #Allocator = 2.0, Mapper = 2.1 and Common = 1.1
        HIDL_ALLOCATOR_VERSION_SCALED := 200
        HIDL_MAPPER_VERSION_SCALED := 210
        HIDL_COMMON_VERSION_SCALED := 110

    else ifeq ($(shell expr $(PLATFORM_SDK_VERSION) \> 25), 1)
        HIDL_IMAPPER_NAMESPACE := V2_0
        HIDL_IALLOCATOR_NAMESPACE := V2_0
        HIDL_COMMON_NAMESPACE := V1_0

        #Allocator = 2.0, Mapper = 2.0 and Common = 1.0
        HIDL_ALLOCATOR_VERSION_SCALED := 200
        HIDL_MAPPER_VERSION_SCALED := 200
        HIDL_COMMON_VERSION_SCALED := 100
    else
        $(error Gralloc 2.x is not supported on platform SDK version $(PLATFORM_SDK_VERSION))
    endif

endif

GRALLOC_VERSION_MAJOR := $(shell echo $(GRALLOC_API_VERSION) | cut -d. -f1)
