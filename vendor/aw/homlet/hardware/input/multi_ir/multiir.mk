
# utils, add multi_ir to recovery
PRODUCT_PACKAGES += \
	multi_ir \
	multi_ir.recovery \
	libmultiir_jni \
	libmultiirservice \

BASE_KL_COPY_LIST := virtual-remote.kl \
	sunxi-ir-uinput.kl \
	customer_rc5_ir_04.kl \

BASE_KL_COPY_LIST += customer_ir_9f00.kl \
	customer_ir_dd22.kl \
	customer_ir_fb04.kl \
	customer_ir_ff00.kl \
	customer_ir_4cb3.kl \
	customer_ir_bc00.kl \
	customer_ir_fc00.kl \
	customer_ir_2992.kl \
	customer_ir_4040.kl \
	customer_ir_7f00.kl \
	customer_ir_bf00.kl \

SYSTEM_KL_COPY_LIST := $(BASE_KL_COPY_LIST) \
	Vendor_000d_Product_3838.kl \
	sunxi-ir.kl \

RECOVERY_KL_COPY_LIST := $(BASE_KL_COPY_LIST) \
	customer_ir_fe01.kl

SUNXI_VENDOR_KL_DIR := vendor/aw/homlet/hardware/input/multi_ir/keylayout
SYSTEM_KL_DIR := system/usr/keylayout
RECOVERY_KL_DIR := /root/system/usr/keylayout

PRODUCT_COPY_FILES += $(foreach f,$(SYSTEM_KL_COPY_LIST),$(SUNXI_VENDOR_KL_DIR)/$(f):$(SYSTEM_KL_DIR)/$(f))

# H6 use different ir keyboard
ifeq ($(TARGET_BOARD_PLATFORM),petrel)
PRODUCT_COPY_FILES += \
	$(SUNXI_VENDOR_KL_DIR)/customer_ir_fe01_petrel.kl:$(SYSTEM_KL_DIR)/customer_ir_fe01.kl

else
PRODUCT_COPY_FILES += \
	$(SUNXI_VENDOR_KL_DIR)/customer_ir_fe01.kl:$(SYSTEM_KL_DIR)/customer_ir_fe01.kl

endif

# recovery support multi_ir
PRODUCT_COPY_FILES += $(foreach f,$(RECOVERY_KL_COPY_LIST),$(SUNXI_VENDOR_KL_DIR)/$(f):$(RECOVERY_KL_DIR)/$(f))
PRODUCT_COPY_FILES += \
	$(SUNXI_VENDOR_KL_DIR)/sunxi-ir-recovery.kl:$(RECOVERY_KL_DIR)/sunxi-ir.kl


