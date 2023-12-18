LOCAL_PATH := $(call my-dir)

ifeq ($(TARGET_BOARD_CHIP),sun8iw7p1)
    include $(call all-named-subdir-makefiles,h3)
else ifeq ($(TARGET_BOARD_CHIP),sun50iw9p1)
    include $(call all-named-subdir-makefiles,h618)
else
    include $(call all-named-subdir-makefiles,h3pro)
endif
