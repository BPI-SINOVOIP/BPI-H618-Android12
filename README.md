# BPI-H618-Android12

----------

**Prepare**

Get the docker image from [Sinovoip Docker Hub](https://hub.docker.com/r/sinovoip/bpi-build-android-11/) , Build the android source with this docker environment.

Other environment requirements, please refer to google Android's official build environment [requirements](https://source.android.com/setup/build/requirements) and [set up guide](https://source.android.com/setup/build/initializing) 

----------

Get source code

    $ git clone https://github.com/BPI-SINOVOIP/BPI-H618-Android12 --depth=1

Because github limit 100MB size for single file, please download the [oversize files](https://drive.google.com/drive/folders/1ye-uzyABf9LZEKp5R1f6FjARTZndTkEb?usp=sharing) and merge them to correct directory before build.

Another way is get the source code tar archive from [BaiduPan(pincode: 8888)]() or [GoogleDrive]()

----------

**Build**

Build longan

    $ cd longan

    # create buildconfig for m4berry build
    $ ./build.sh autoconfig -i h618 -o android -n default -a arm64 -k linux-5.4 -b m4berry

    # or create buildconfig for m4zero build
    # ./build.sh autoconfig -i h618 -o android -n default -a arm64 -k linux-5.4 -b m4zero

    $ ./build.sh

Build Android

    $ cd ..
    $ source build/envsetup.sh

    # set variable to true for box image build, false for normal image build
    $ export BOARD_BUILD_BOX=false

    # build android for m4berry
    $ lunch bananapi_m4berry-userdebug

    # or build android for m4zero
    $ lunch bananapi_m4zero-userdebug

    $ make -j20
    $ pack
----------
**Flash**

The target  download image is in longan/out/ dir, flash it to your device by Allwinner PhoenixSuit. Please refer to [Bananapi M4Berry wiki](https://wiki.banana-pi.org/Getting_Started_with_BPI-M4_Berry) for how to flash the android image.
