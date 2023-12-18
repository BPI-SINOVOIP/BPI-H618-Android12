SUMMARY = "Google package group for AGL services"

inherit packagegroup

PACKAGES = "packagegroup-google-agl"

RDEPENDS_${PN} += "\
    google-trout-agl-services \
"
