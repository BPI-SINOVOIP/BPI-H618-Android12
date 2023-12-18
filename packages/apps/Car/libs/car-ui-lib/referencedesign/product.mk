# Inherit from this product to include the "Reference Design" RROs for CarUi

#############################################
#                  WARNING                  #
#############################################
# The OEM APIs as they appear on this       #
# branch of android are not finalized!      #
# If a shared library is built using them,  #
# it will cause apps to crash!              #
#                                           #
# Please only use a shared library with     #
# a later version of car-ui-lib.            #
#############################################
#PRODUCT_PACKAGES += \
#   car-ui-lib-sharedlibrary \

PRODUCT_PRODUCT_PROPERTIES += ro.build.automotive.car.ui.shared.library.package.name=com.chassis.car.ui.sharedlibrary

PRODUCT_COPY_FILES += \
    packages/apps/Car/libs/car-ui-lib/referencedesign/car-ui-lib-preinstalled-packages.xml:system/etc/sysconfig/car-ui-lib-preinstalled-packages.xml \


# Include generated RROs
PRODUCT_PACKAGES += \
    googlecarui-com-android-car-ui-paintbooth \
    googlecarui-com-google-android-car-ui-paintbooth \
    googlecarui-com-google-android-carui-ats \
    googlecarui-com-android-car-rotaryplayground \
    googlecarui-com-android-car-themeplayground \
    googlecarui-com-android-car-carlauncher \
    googlecarui-com-android-car-home \
    googlecarui-com-android-car-media \
    googlecarui-com-android-car-radio \
    googlecarui-com-android-car-calendar \
    googlecarui-com-android-car-companiondevicesupport \
    googlecarui-com-android-car-systemupdater \
    googlecarui-com-android-car-dialer \
    googlecarui-com-android-car-linkviewer \
    googlecarui-com-android-car-settings \
    googlecarui-com-android-car-voicecontrol \
    googlecarui-com-android-car-faceenroll \
    googlecarui-com-android-managedprovisioning \
    googlecarui-com-android-settings-intelligence \
    googlecarui-com-google-android-apps-automotive-inputmethod \
    googlecarui-com-google-android-apps-automotive-inputmethod-dev \
    googlecarui-com-google-android-apps-automotive-templates-host \
    googlecarui-com-google-android-embedded-projection \
    googlecarui-com-google-android-gms \
    googlecarui-com-google-android-gsf \
    googlecarui-com-google-android-packageinstaller \
    googlecarui-com-google-android-permissioncontroller \
    googlecarui-com-google-android-carassistant \
    googlecarui-com-google-android-tts \
    googlecarui-com-android-htmlviewer \
    googlecarui-com-android-vending \

# This system property is used to enable the RROs on startup via
# the requiredSystemPropertyName/Value attributes in the manifest
PRODUCT_PRODUCT_PROPERTIES += ro.build.car_ui_rros_enabled=true
