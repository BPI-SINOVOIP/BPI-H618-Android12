# This file is used to define Android Mainline module for GO device
# Mainline module is a GMS requirement since Q

# Q mainline required modules
PRODUCT_PACKAGES += \
    GoogleExtServicesPrebuilt \
    ModuleMetadataGooglePrebuilt \
    GoogleModuleMetadataFrameworkOverlay \
    GooglePermissionControllerPrebuilt

# mainline network framework modules
PRODUCT_PACKAGES += \
    InProcessNetworkStack \
    PlatformCaptivePortalLogin \
    PlatformNetworkPermissionConfig

# mainline module overlay
PRODUCT_PACKAGES += \
    GoogleExtServicesConfigOverlay \
    GooglePermissionControllerFrameworkOverlay \
    GooglePermissionControllerOverlay
