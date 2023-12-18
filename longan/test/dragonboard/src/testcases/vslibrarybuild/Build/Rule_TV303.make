# We add some make rule here to faciliate our project
# 1)Please add the system define only related to this project
# 2)Please add the project related library to LIBS_RULE

#Only used inside display module
HIDTV_CHIP=HIDTVPROFUSION

#Specified system information storeage location
DEF_CFG = -DCFG_PATH=\"/data/tv/save/\" -DMISC_AUDIO_PATH=\"/system/misc/audio/\" -DMISC_PATH=\"/system/misc/\"
DEF_INTERFACE = -DWLAN0_INTERFACE=\"wlan0\"
DEF_IFACE = -DWLAN_IFACE=\"wlan\"
DEF_CTRL_IFACE_PATH = -DCONFIG_CTRL_IFACE_DIR=\"/var/run/wpa_supplicant\"
DEF_PROJECT_RULE = -DTV303

DEF_PROJECT_RULE += $(DEF_INTERFACE)
DEF_PROJECT_RULE += $(DEF_IFACE)
DEF_PROJECT_RULE += $(DEF_CTRL_IFACE_PATH)

#Specify kernel arch32, required by HAL_SX6/demux
DEF_PROJECT_RULE += -DCONFIG_ARM

#Specify kernel arch64, required by HAL_SX6/demux
#DEF_PROJECT_RULE += -DCONFIG_ARM64

VS_ROOT:= $(shell \
			if [ -d $(LICHEE_TOP_DIR)/../vendor/vs ]; then \
				readlink -f $(LICHEE_TOP_DIR)/../vendor/vs; \
			elif [ -d $(LICHEE_PLATFORM_DIR)/vendor/vs ]; then \
				readlink -f $(LICHEE_PLATFORM_DIR)/vendor/vs; \
			fi)
