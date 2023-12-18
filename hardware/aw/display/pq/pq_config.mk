PQ_ROOT := $(shell dirname $(lastword $(MAKEFILE_LIST)))
PRODUCT_COPY_FILES += \
					$(PQ_ROOT)/disp_cfg/disp_firmware:$(TARGET_COPY_OUT_VENDOR)/etc/firmware/disp_firmware\
					$(PQ_ROOT)/disp_cfg/sunxi_pq.cfg:$(TARGET_COPY_OUT_VENDOR)/etc/sunxi_pq.cfg

