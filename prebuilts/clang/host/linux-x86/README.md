Android Clang/LLVM Prebuilts
============================

For the latest version of this doc, please make sure to visit:
[Android Clang/LLVM Prebuilts Readme Doc](https://android.googlesource.com/platform/prebuilts/clang/host/linux-x86/+/master/README.md)

LLVM Users
----------

* [**Android Platform**](https://android.googlesource.com/platform/)
  * Currently clang-r416183b
  * clang-r383902b1 for Android R-QPR2 release
  * clang-r383902b for Android R release
  * clang-r353983c1 for Android Q-QPR2 release
  * clang-r353983c for Android Q release
  * Look for "ClangDefaultVersion" and/or "clang-" in [build/soong/cc/config/global.go](https://android.googlesource.com/platform/build/soong/+/master/cc/config/global.go/).
    * [Internal cs/ link](https://cs.corp.google.com/android/build/soong/cc/config/global.go?q=ClangDefaultVersion)

* [**Android Platform LLVM binutils**](https://android.googlesource.com/platform/prebuilts/clang/host/linux-x86/+/refs/heads/master/llvm-binutils-stable/)
  * Currently clang-r416183b
  * These can be updated by running [update-binutils.py](https://android.googlesource.com/toolchain/llvm_android/+/refs/heads/master/update-binutils.py).

* [**RenderScript**](https://developer.android.com/guide/topics/renderscript/index.html)
  * Currently clang-3289846
  * Look for "RSClangVersion" and/or "clang-" in [build/soong/cc/config/global.go](https://android.googlesource.com/platform/build/soong/+/master/cc/config/global.go/).
    * [Internal cs/ link](https://cs.corp.google.com/android/build/soong/cc/config/global.go?q=RSClangVersion)

* [**Android Linux Kernel**](http://go/android-systems)
  * Currently clang-r416183b
  * Look for "clang-" in [mainline build configs](https://android.googlesource.com/kernel/common/+/refs/heads/android-mainline/build.config.common).
  * Look for "clang-" in [android13-5.10 build configs](https://android.googlesource.com/kernel/common/+/refs/heads/android13-5.10/build.config.common)
  * Look for "clang-" in [android12-5.10 build configs](https://android.googlesource.com/kernel/common/+/refs/heads/android12-5.10/build.config.common)
  * Look for "clang-" in [5.4 build configs](https://android.googlesource.com/kernel/common/+/refs/heads/android12-5.4/build.config.common).
  * Look for "clang-" in [4.19 build configs](https://android.googlesource.com/kernel/common/+/refs/heads/android-4.19-stable/build.config.common).
  * Look for "clang-" in [4.14 build configs](https://android.googlesource.com/kernel/common/+/refs/heads/android-4.14-stable/build.config.common).
  * Look for "clang-" in [4.9 build configs](https://android.googlesource.com/kernel/common/+/android-4.9-q/build.config.common).
  * Internal LLVM developers should look in the partner gerrit for more kernel configurations.

* [**NDK**](https://developer.android.com/ndk)
  * Currently clang-r416183b
  * Look for "clang-" in [ndk/toolchains.py](https://android.googlesource.com/platform/ndk/+/refs/heads/master/ndk/toolchains.py)

* [**Bazel**](https://opensource.googleblog.com/2020/11/welcome-android-open-source-project.html)
  * Currently clang-r416183b
  * Look for "clang-" in [prebuilts/clang/host/linux-x86/clang_version.bzl](https://android.googlesource.com/platform/prebuilts/clang/host/linux-x86/+/refs/heads/master/clang_version.bzl)

* [**Trusty**](https://source.android.com/security/trusty/)
  * Currently clang-r383902
  * Look for "clang-" in [vendor/google/aosp/scripts/envsetup.sh](https://android.googlesource.com/trusty/vendor/google/aosp/+/master/scripts/envsetup.sh).

* [**Android Emulator**](https://developer.android.com/studio/run/emulator.html)
  * Currently clang-r416183b
  * Look for "clang-" in [external/qemu/android/build/cmake/toolchain.cmake](https://android.googlesource.com/platform/external/qemu/+/emu-master-dev/android/build/cmake/toolchain.cmake#25).
    * Note that they work out of the emu-master-dev branch.
    * [Internal cs/ link](https://cs.corp.google.com/android/external/qemu/android/build/cmake/toolchain.cmake?q=clang-)

* [**Context Hub Runtime Environment (CHRE)**](https://android.googlesource.com/platform/system/chre/)
  * Currently clang-r416183b
  * Look in [system/chre/build/arch/x86.mk](https://android.googlesource.com/platform/system/chre/+/master/build/arch/x86.mk#12).

* [**OpenJDK (jdk/build)**](https://android.googlesource.com/toolchain/jdk/build/)
  * Currently clang-r399163b
  * Look for "clang-" in [build-jetbrainsruntime-linux.sh](https://android.googlesource.com/toolchain/jdk/build/+/refs/heads/master/build-jetbrainsruntime-linux.sh)
  * Look for "clang-" in [build-openjdk-darwin.sh](https://android.googlesource.com/toolchain/jdk/build/+/refs/heads/master/build-openjdk-darwin.sh)

* [**Clang Tools**](https://android.googlesource.com/platform/prebuilts/clang-tools/)
  * Currently clang-r416183b
  * Look for "clang-r" in [envsetup.sh](https://android.googlesource.com/platform/development/+/refs/heads/master/vndk/tools/header-checker/android/envsetup.sh)
  * Check out branch clang-tools and run test: OUT_DIR=out prebuilts/clang-tools/build-prebuilts.sh

* **Android Rust**
  * Currently clang-r416183b
  * Look for "CLANG_REVISION" in [paths.py](https://android.googlesource.com/toolchain/android_rust/+/refs/heads/master/paths.py)
  * Look for "bindgenClangVersion" in [bindgen.go](https://android.googlesource.com/platform/build/soong/+/refs/heads/master/rust/bindgen.go)

* **Stage 1 compiler**
  * Currently clang-bootstrap (clang-r407598 from a later build)
  * Look for "clang-r" in [toolchain/llvm_android/constants.py](https://android.googlesource.com/toolchain/llvm_android/+/refs/heads/master/constants.py)
  * Note the chicken & egg paradox of a self hosting bootstrapping compiler; this can only be updated AFTER a new prebuilt is checked in.


Prebuilt Versions
-----------------

* [clang-3289846](https://android.googlesource.com/platform/prebuilts/clang/host/linux-x86/+/master/clang-3289846/) - September 2016
* [clang-r328903](https://android.googlesource.com/platform/prebuilts/clang/host/linux-x86/+/master/clang-r328903/) - May 2018
* [clang-r339409b](https://android.googlesource.com/platform/prebuilts/clang/host/linux-x86/+/master/clang-r339409b/) - October 2018
* [clang-r344140b](https://android.googlesource.com/platform/prebuilts/clang/host/linux-x86/+/master/clang-r344140b/) - November 2018
* [clang-r346389b](https://android.googlesource.com/platform/prebuilts/clang/host/linux-x86/+/master/clang-r346389b/) - December 2018
* [clang-r346389c](https://android.googlesource.com/platform/prebuilts/clang/host/linux-x86/+/master/clang-r346389c/) - January 2019
* [clang-r349610](https://android.googlesource.com/platform/prebuilts/clang/host/linux-x86/+/master/clang-r349610/) - February 2019
* [clang-r349610b](https://android.googlesource.com/platform/prebuilts/clang/host/linux-x86/+/master/clang-r349610b/) - February 2019
* [clang-r353983b](https://android.googlesource.com/platform/prebuilts/clang/host/linux-x86/+/master/clang-r353983b/) - March 2019
* [clang-r353983c](https://android.googlesource.com/platform/prebuilts/clang/host/linux-x86/+/master/clang-r353983c/) - April 2019
* [clang-r353983d](https://android.googlesource.com/platform/prebuilts/clang/host/linux-x86/+/master/clang-r353983d/) - June 2019
* [clang-r365631b](https://android.googlesource.com/platform/prebuilts/clang/host/linux-x86/+/master/clang-r365631b/) - September 2019
* [clang-r365631c](https://android.googlesource.com/platform/prebuilts/clang/host/linux-x86/+/refs/heads/master/clang-r365631c/) - September 2019
* [clang-r365631c1](https://android.googlesource.com/platform/prebuilts/clang/host/linux-x86/+/refs/heads/master/clang-r365631c/) - March 2020
* [clang-r370808](https://android.googlesource.com/platform/prebuilts/clang/host/linux-x86/+/refs/heads/master/clang-r370808/) - December 2019
* [clang-r370808b](https://android.googlesource.com/platform/prebuilts/clang/host/linux-x86/+/refs/heads/master/clang-r370808b/) - January 2020
* [clang-r377782b](https://android.googlesource.com/platform/prebuilts/clang/host/linux-x86/+log/refs/heads/master/clang-r377782b) - February 2020
* [clang-r377782c](https://android.googlesource.com/platform/prebuilts/clang/host/linux-x86/+log/refs/heads/master/clang-r377782c) - March 2020
* [clang-r377782d](https://android.googlesource.com/platform/prebuilts/clang/host/linux-x86/+log/refs/heads/master/clang-r377782d) - April 2020
* [clang-r383902](https://android.googlesource.com/platform/prebuilts/clang/host/linux-x86/+log/refs/heads/master/clang-r383902) - May 2020
* [clang-r383902b](https://android.googlesource.com/platform/prebuilts/clang/host/linux-x86/+log/refs/heads/master/clang-r383902b) - June 2020
* [clang-r383902b1](https://android.googlesource.com/platform/prebuilts/clang/host/linux-x86/+log/refs/heads/master/clang-r383902b1) - October 2020
* [clang-r383902c](https://android.googlesource.com/platform/prebuilts/clang/host/linux-x86/+log/refs/heads/master/clang-r383902c) - June 2020
* [clang-r399163](https://android.googlesource.com/platform/prebuilts/clang/host/linux-x86/+log/refs/heads/master/clang-r399163) - August 2020
* [clang-r399163b](https://android.googlesource.com/platform/prebuilts/clang/host/linux-x86/+log/refs/heads/master/clang-r399163b) - October 2020
* [clang-r407598](https://android.googlesource.com/platform/prebuilts/clang/host/linux-x86/+log/refs/heads/master/clang-r407598) - January 2021
* [clang-r407598b](https://android.googlesource.com/platform/prebuilts/clang/host/linux-x86/+log/refs/heads/master/clang-r407598b) - January 2021
* [clang-r412851](https://android.googlesource.com/platform/prebuilts/clang/host/linux-x86/+log/refs/heads/master/clang-r412851) - February 2021
* [clang-r416183](https://android.googlesource.com/platform/prebuilts/clang/host/linux-x86/+log/refs/heads/master/clang-r416183) - March 2021
* [clang-r416183b](https://android.googlesource.com/platform/prebuilts/clang/host/linux-x86/+log/refs/heads/master/clang-r416183b) - April 2021


More Information
----------------

We have a public mailing list that you can subscribe to:
[android-llvm@googlegroups.com](https://groups.google.com/forum/#!forum/android-llvm)

See also our [release notes](RELEASE_NOTES.md).
