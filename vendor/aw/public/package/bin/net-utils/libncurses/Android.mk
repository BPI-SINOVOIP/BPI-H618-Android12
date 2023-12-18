LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := $(call all-c-files-under, ncurses/tty) 
LOCAL_SRC_FILES += $(call all-c-files-under, ncurses/base) 
LOCAL_SRC_FILES := $(filter-out ncurses/base/lib_driver.c ncurses/base/sigaction.c, $(LOCAL_SRC_FILES))
LOCAL_SRC_FILES += $(call all-c-files-under, ncurses/tinfo) 
LOCAL_SRC_FILES := $(filter-out ncurses/tinfo/doalloc.c ncurses/tinfo/make_keys.c ncurses/tinfo/tinfo_driver.c, $(LOCAL_SRC_FILES))

LOCAL_SRC_FILES += ncurses/trace/lib_trace.c \
		ncurses/trace/varargs.c \
		ncurses/trace/visbuf.c \
		ncurses/codes.c \
		ncurses/comp_captab.c \
		ncurses/expanded.c \
		ncurses/fallback.c \
		ncurses/lib_gen.c \
		ncurses/lib_keyname.c \
		ncurses/names.c \
		ncurses/unctrl.c \

LOCAL_SRC_FILES := $(sort $(LOCAL_SRC_FILES))
		
LOCAL_CFLAGS := -DHAVE_CONFIG_H -U_XOPEN_SOURCE -D_XOPEN_SOURCE=500 -U_POSIX_C_SOURCE -D_POSIX_C_SOURCE=199506L -DNDEBUG 

LOCAL_C_INCLUDES := $(LOCAL_PATH) \
		$(LOCAL_PATH)/include \
		$(LOCAL_PATH)/ncurses \

LOCAL_MODULE_TAGS := optional
LOCAL_PROPRIETARY_MODULE := true
LOCAL_REQUIRED_MODULES := terminfo-package
LOCAL_MODULE := libncurses

include $(BUILD_SHARED_LIBRARY)

# Copy only basic terminal type definitions as ncurses-base in debian does by default.
# http://anonscm.debian.org/gitweb/?p=collab-maint/ncurses.git;a=blob;f=debian/rules;hb=HEAD#l140
TERMINFO_FILES := \
	a/ansi c/cons25 c/cygwin d/dumb E/Eterm E/Eterm-color h/hurd l/linux \
	m/mach m/mach-bold m/mach-color p/pcansi r/rxvt r/rxvt-basic  \
	s/screen s/screen-bce s/screen-s s/screen-w s/sun \
	s/screen-256color s/screen-256color-bce v/vt100 v/vt102 \
	v/vt220 v/vt52 x/xterm x/xterm-xfree86 x/xterm-color x/xterm-r5 x/xterm-r6 \
	x/xterm-vt220 x/xterm-256color w/wsvt25 w/wsvt25m

TERMINFO_SOURCE := lib/terminfo

define terminfo
include $$(CLEAR_VARS)
LOCAL_SRC_FILES := $$(TERMINFO_SOURCE)/$1
LOCAL_MODULE := $$(shell basename $1)
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH :=  $$(TARGET_OUT_VENDOR_ETC)/terminfo/$$(shell dirname $1)
include $$(BUILD_PREBUILT)
endef

define all-terminfo
$(foreach f,$(1),\
    $(eval $(call terminfo,$(f))) \
)
endef

define all-terminfo-name
$(eval __name :=) \
$(foreach f,$(1),\
    $(eval __name += $(shell basename $(f)))) \
$(__name)
endef

$(call all-terminfo,$(TERMINFO_FILES))

include $(CLEAR_VARS)
LOCAL_MODULE := terminfo-package
LOCAL_MODULE_OWNER := google
LOCAL_MODULE_TAGS := optional
LOCAL_REQUIRED_MODULES += $(call all-terminfo-name,$(TERMINFO_FILES))
include $(BUILD_PHONY_PACKAGE)
