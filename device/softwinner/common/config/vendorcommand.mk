define soong_config_add
$(strip \
    $(if $(1),,$(error Parameter 1 namespace empty))
    $(if $(2),,$(error Parameter 2 configkey empty))
    $(eval SOONG_CONFIG_NAMESPACES := $(sort $(1) $(SOONG_CONFIG_NAMESPACES)))
    $(eval SOONG_CONFIG_$(1) := $(sort $(2) $(SOONG_CONFIG_$(1))))
    $(eval SOONG_CONFIG_$(1)_$(2) := $(3))
)
endef

# For auto load partition size config
define get_sys_partition_path
$(strip \
    $(eval __exist :=)
    $(if $(BOARD_ADD_PACK_CONFIG),
        $(eval __exist := $(filter %/sys_partition.fex,$(BOARD_ADD_PACK_CONFIG)))
        $(if $(__exist),
            $(eval __sys_partition_path := $(shell dirname $(__exist)))
        ),
    )
    $(if $(__exist),,
        $(if $(wildcard $(TOP_DIR)*/device/config/chips),
            $(eval __chip_path := $(shell cd $(TOP_DIR)*/device/config/chips && pwd))
            $(eval __sys_partition_path := $(__chip_path)/$(TARGET_BOARD_IC)/configs/$(PRODUCT_BOARD)/android),
        )
    )
    $(shell echo $(__sys_partition_path))
)
endef

define get_partition_size
$(strip \
    $(eval __def := $(call get_sys_partition_path)/sys_partition.fex)
    $(if $(strip $(1)),,$(error Parameter 1 partition name empty))
    $(if $(strip $(2)),$(eval __cfg := $(2)),$(eval __cfg := $(__def)))
    $(if $(wildcard $(__cfg)),,$(error Config file $(__cfg) not found))
    $(eval __size := $(shell sed -n '/^\s*name\s*=\s*$(1)\s*$$/,/^\s*user_type\s*=/p' $(__cfg) | awk -F= '/size/{print $$2}'))
    $(if $(__size),,$(eval __size := $(shell sed -n '/^\s*name\s*=\s*$(1)_a\s*$$/,/^\s*user_type\s*=/p' $(__cfg) | awk -F= '/size/{print $$2}')))
    $(if $(__size),,$(error Cannot find size config for partition $(1)))
    $(eval __number := $(shell echo $(__size) | sed 's|[a-z,A-Z]*$$||g'))
    $(eval __dimension :=$(shell echo $(__size) | sed 's|^[0-9,.]\+||g'))
    $(if $(__dimension),
        $(if $(filter B b,$(__dimension)),
            $(eval __m := 1),
            $(if $(filter K k,$(__dimension)),
                $(eval __m := 1024),
                $(if $(filter M m,$(__dimension)),
                    $(eval __m := 1048576),
                    $(if $(filter G g,$(__dimension)),
                        $(eval __m := 1073741824),
                        $(error Error Dimension($(__dimension)))
                    )
                )
            )
        ),
        $(eval __m := 512)
    )
    $(shell echo "$(__m) * $(__number)" | bc | awk -F. '{print $$1}')
)
endef

define get_kernel_arch
$(strip \
    $(eval __prebuilt_kernel := $(PRODUCT_PREBUILT_PATH)/bImage)
    $(if $(wildcard $(__prebuilt_kernel)),
        $(eval __getval := $(shell build/make/tools/extract_kernel.py \
                             --input $(__prebuilt_kernel) --output-configs | grep "^CONFIG_ARM64=y"))
        $(if $(__getval),arm64,arm),$(1)
    )
)
endef

define get_longan_config
$(strip \
    $(eval __longan_cfg := $(PRODUCT_PREBUILT_PATH)/.buildconfig)
    $(eval __longan_key := $(1))
    $(eval __default_val:= $(2))
    $(if $(wildcard $(__longan_cfg)),
        $(eval __getval := $(shell awk -F'=' '/^([[:space:]]*export[[:space:]]+){0,1}*'"$(__longan_key)"'=/{print $$2}' \
                           $(__longan_cfg) | tail -n 1)),
        $(eval __getval := $(__default_val))
    )
    $(if $(__getval),
        $(__getval),
        $(__default_val)
    )
)
endef

define get_boot_offset
$(strip \
    $(if $(filter arm,$(1)),
        $(eval __kernel_offset := 0x8000),
        $(if $(filter arm64,$(1)),
            $(eval __kernel_offset := 0x80000),
            $(error Unsupport arch $(2))
        )
    )
    $(eval __kernel_min    := $(shell printf "%d" 0x3000000))
    $(eval __prebuilt_kernel := $(PRODUCT_PREBUILT_PATH)/bImage)
    $(if $(wildcard $(__prebuilt_kernel)),
        $(eval __kernel_size   := $(shell stat $(__prebuilt_kernel) --format="%s"))
        $(eval __kernel_size   := $(shell [ $(__kernel_size) -lt $(__kernel_min) ] && echo $(__kernel_min) ||  echo $(__kernel_size))),
        $(eval __kernel_size   := $(__kernel_min))
    )
    $(eval __prebuilt_sysmap := $(PRODUCT_PREBUILT_PATH)/System.map)
    $(if $(wildcard $(__prebuilt_sysmap)),
        $(eval __bss_start := $(shell sed -n '/__bss_start/p' $(__prebuilt_sysmap) | awk '{print $$1}' | cut -b8-))
        $(eval __bss_stop  := $(shell sed -n '/__bss_stop/p'  $(__prebuilt_sysmap) | awk '{print $$1}' | cut -b8-))
        $(eval __bss_size  := $(shell printf "%d-%d\n" 0x$(__bss_stop) 0x$(__bss_start) | bc)),
        $(eval __bss_size  := 0)
    )
    $(eval __dtb_offset    := $(shell printf "(%d+%d+%d+%d)/%d*%d\n" $(__kernel_offset) $(__kernel_size) $(__bss_size) 0x1FFFFF 0x100000 0x100000 | bc))
    $(eval __ramdisk_offset:= $(shell printf "%d+%d\n" $(__dtb_offset) 0x100000 | bc))
    $(if $(filter KERNEL,$(2)),
        $(eval __offset := $(__kernel_offset)),
        $(if $(filter DTB,$(2)),
            $(eval __offset := $(__dtb_offset)),
            $(if $(filter RAMDISK,$(2)),
                $(eval __offset := $(__ramdisk_offset)),
                $(error Unsupport args $(2))
            )
        )
    )
    $(shell printf "0x%08X" $(__offset))
)
endef

define copy-user-file
LOCAL_PATH := $$(shell dirname $1)
include $$(CLEAR_VARS)
LOCAL_MODULE := $$(subst /,-,$2)
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $$(shell dirname $2)
ifeq ($$(filter $$(PRODUCT_OUT)%,$$(LOCAL_MODULE_PATH)),)
LOCAL_MODULE_PATH := $$(PRODUCT_OUT)/$$(LOCAL_MODULE_PATH)
endif
LOCAL_MODULE_STEM := $$(shell basename $2)
LOCAL_SRC_FILES := $$(shell basename $1)
include $$(BUILD_PREBUILT)
endef

define get-copy-file-src
$(strip $(shell echo $(1) | awk -F':' '{print $$1}'))
endef

define get-copy-file-dst
$(strip $(shell echo $(1) | awk -F':' '{print $$2}'))
endef

define get-copy-file-name
$(strip $(subst /,-,$(call get-copy-file-dst,$(1))))
endef

define define-copy-target
$(foreach f,$(1),\
    $(eval __internal_current_name := $(call get-copy-file-name,$(f))) \
    $(if $(filter $(__internal_current_name),$(__internal_total_name)), \
        $(warning $(__internal_current_name) allready defined, skip.), \
        $(eval __internal_total_name += $(__internal_current_name)) \
        $(eval $(call copy-user-file,$(call get-copy-file-src,$(f)),$(call get-copy-file-dst,$(f)))) \
    ) \
)
endef

define get-copy-target
$(eval __name :=) \
$(foreach f,$(1),\
    $(eval __name += $(call get-copy-file-name,$(f)))) \
$(__name)
endef
