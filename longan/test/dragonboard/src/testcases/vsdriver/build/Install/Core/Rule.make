# We add some make rule here to faciliate our project
# 1)Please add the system define only related to this project
# 2)Please add the project related library to LIBS_RULE

#Only used inside display module
HIDTV_CHIP=HIDTVPROFUSION

#Specified system information storeage location
DEF_CFG = -DCFG_PATH=\"/etc/Save/\" -DMISC_PATH=\"/opt/Misc/\"
DEF_INTERFACE = -DWLAN0_INTERFACE=\"wlan0\"
DEF_IFACE = -DWLAN_IFACE=\"wlan\"
DEF_CTRL_IFACE_PATH = -DCONFIG_CTRL_IFACE_DIR=\"/var/run/wpa_supplicant\"
DEF_PROJECT_RULE = -DTV303
DEF_PROJECT_RULE += -DHAS_DEMUX_DMA_BUF_SUPPORT=0

DEF_PROJECT_RULE += $(DEF_INTERFACE)
DEF_PROJECT_RULE += $(DEF_IFACE)
DEF_PROJECT_RULE += $(DEF_CTRL_IFACE_PATH)
DEF_PROJECT_RULE += -DCONFIG_ARM
