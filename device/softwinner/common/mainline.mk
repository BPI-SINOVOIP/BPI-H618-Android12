# This file is used to define Android Mainline module  to be used
# Mainline module is a GMS requirement since Q

# Q mainline required modules
PRODUCT_PACKAGES += \
    GoogleANGLEPrebuilt \
    GoogleDocumentsUIPrebuilt \
    GoogleExtServicesPrebuilt \
    ModuleMetadataGooglePrebuilt \
    GoogleModuleMetadataFrameworkOverlay \
    GooglePermissionControllerPrebuilt

# Q mainline strongly recommended modules
#PRODUCT_PACKAGES += \
    GoogleCaptivePortalLogin \
    GoogleNetworkStack \
    GoogleNetworkPermissionConfig \
    com.android.conscrypt \
    com.android.resolv

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

PRODUCT_PROPERTY_OVERRIDES += \
    ro.apex.updatable=true
