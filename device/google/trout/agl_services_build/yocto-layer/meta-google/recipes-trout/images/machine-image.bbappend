FILESEXTRAPATHS_prepend := "${THISDIR}/${BPN}:"

GOOGLE_OVERLAY_SRC_URI = "file://google-overlay"

GOOGLE_OVERLAY_ROOT_DIRS = "google-overlay"

IMAGE_INSTALL += "packagegroup-google-agl"

inherit google-image-overlay
