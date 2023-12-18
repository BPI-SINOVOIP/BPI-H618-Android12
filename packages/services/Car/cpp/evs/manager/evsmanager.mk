# Extended View System Manager
PRODUCT_PACKAGES += android.automotive.evs.manager@1.1

# EVS manager selinux policies
PRODUCT_PUBLIC_SEPOLICY_DIRS += packages/services/Car/cpp/evs/manager/sepolicy/public
PRODUCT_PRIVATE_SEPOLICY_DIRS += packages/services/Car/cpp/evs/manager/sepolicy/private
