# Android Automotive 'Chassis' re-packaged androidx
Please refer to [Android Automotive 'Chassiss' shared library](../sharedlibrary/README.md)

All the applications that are using both shared library version of car-ui-lib and and AndroidX will end up having 2 copies of AndroidX in their class path. One copy from the shared library and one version from the application itself. At runtime, the Android class loader finds the AndroidX classes from the shared library first. This could lead to compatibility issues, for example when the two versions are not compatible. In order to avoid that we're repackaging AndroidX binaries from `androiodx.*` to `androidx.car.ui.*`. `car-ui-lib-androidx` build target will be used with shared library version of car-ui-lib.

car-ui-jetifier-reverse.cfg file is a manually edited file to match the requirements. This file needs maintainance as AndroidX adds more classes to the packages that are currently used by car-ui and/or if any new AndroidX package is needed by car-ui that's not listed there.
