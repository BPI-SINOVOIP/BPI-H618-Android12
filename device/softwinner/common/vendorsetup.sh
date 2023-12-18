#
# Copyright (C) 2012 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

# This file is executed by build/envsetup.sh, and can use anything
# defined in envsetup.sh.
#
# In particular, you can add lunch options with the add_lunch_combo
# function: add_lunch_combo generic-eng

# vendorsetup.sh help
function vendor_help()
{
    echo "==========================================================================================="
    echo "vendorsetup help:"
    local info=""
    local i fun msg
    local sbfile=$(readlink -f ${BASH_SOURCE[0]})
    local function_pattern="^function \([a-zA-Z][a-zA-Z0-9_-]*\).*"
    for i in $(sed -n "/${function_pattern}/=" ${sbfile}); do
        fun=$(sed -n "${i}s/${function_pattern}/\1/p" ${sbfile})
        msg=$(sed -n "$((i-1))s/^\s*#\s*\(.*\)$/\1/p" ${sbfile})
        info+="`printf "  %-20s -- %s" "$fun" "$msg"`\n"
    done
    printf "$info" | sort
    echo "==========================================================================================="
}

function _vendor_env_setup()
{
    android_top_path=$(cd $(dirname ${BASH_SOURCE[0]})/../../.. && pwd)

    [ -f $android_top_path/device/softwinner/common/version.txt ] && \
    source $android_top_path/device/softwinner/common/version.txt

    if [ "$TOP_DIR" != "$android_top_path" ]; then
        unset TOP_DIR
    fi

    if [ "$android_top_path" != "$ANDROID_BUILD_TOP" ]; then
        unset TARGET_PRODUCT
        unset android_device_path
    fi

    if [ "$lichee_top_path" != "$android_top_path/longan" ]; then
        lichee_top_path=$android_top_path/longan
        lichee_build_config=$lichee_top_path/.buildconfig
    fi

    return 0
}

function _get_device_dir()
{
    if [ -z "${android_device_path}" ]; then
        echo ${android_top_path}
    else
        echo ${android_device_path}
    fi
}

# goto device dir
function cdevice()
{
    local device_dir=$(_get_device_dir)
    [ ! -d $device_dir ] && echo "Device path $device_dir not found" && return 1
    cd $device_dir
}

# goto $OUT dir
function cout()
{
    cd $OUT
}

# goto android top dir
function ca()
{
    cd $android_top_path
}

# goto common dir
function com()
{
    cd $android_top_path/device/softwinner/common
}

# goto longan dir
function cl()
{
    cd $lichee_top_path
}

function _findvalidfile()
{
    local filelist=$1
    local validlist f
    for f in $filelist; do
        [ -f $f ] && validlist+=" $f"
    done
    echo "$validlist"
}

function _loadconfig()
{
    local cfgkey=$1
    local cfgfile=$2
    local defval=$3
    local val=""
    local cfglist=$(_findvalidfile "$cfgfile")
    [ -n "$cfglist" ] && val="$(\gawk -F'=' '/^([[:space:]]*export[[:space:]]+){0,1}*'"${cfgkey}"'=/{print $2}' $cfglist | tail -n 1)"
    eval echo "${val:-"$defval"}"
}

function _loadandroidconfig()
{
    local cfgkey=$1
    local cfgfile=$2
    local cfglist=$(_findvalidfile "$cfgfile")
    local val=$(_loadconfig $cfgkey "$cfglist")
    local valid
    [ -n "$cfglist" ] && valid="$(sed -n "/\(export\s\+\)\?${cfgkey}=/p" $cfglist)"
    [ -n "$valid" ] && echo $val || echo $(get_build_var $cfgkey)
}

# goto kernel dir
function ck()
{
    if [ -z "$(_findvalidfile "$lichee_build_config")" ]; then
        echo "Please run ./build.sh config first!"
        return 1
    fi
    local kver=$(_loadconfig LICHEE_KERN_VER "$lichee_build_config" "linux-5.4")
    cd $lichee_top_path/kernel/$kver
}

# goto longan/tools dir
function ct()
{
    cd $lichee_top_path/tools
}

# goto longan/build dir
function cb()
{
    cd $lichee_top_path/build
}

# goto longan/out/kernel/build dir
function cok()
{
    cd $lichee_top_path/out/kernel/build
}

# goto brandy dir
function cbr()
{
    local vbr=$(_loadconfig LICHEE_BRANDY_VER "$lichee_build_config")
    if [ "x$vbr" == "x1.0" ]; then
        cd $lichee_top_path/brandy/brandy-1.0/brandy
    elif [ "x$vbr" == "x2.0" ]; then
        cd $lichee_top_path/brandy/brandy-2.0/
    fi
}

# goto booloader(spl) dir
function cs()
{
    local vbr=$(_loadconfig LICHEE_BRANDY_VER "$lichee_build_config")
    if [ "x$vbr" == "x1.0" ]; then
        cd $lichee_top_path/brandy/brandy-1.0/bootloader/uboot_2014_sunxi_spl/sunxi_spl
    elif [ "x$vbr" == "x2.0" ]; then
        cd $lichee_top_path/brandy/brandy-2.0/spl
    fi
}

# goto uboot dir
function cu()
{
    local vbr=$(_loadconfig LICHEE_BRANDY_VER "$lichee_build_config")
    if [ "x$vbr" == "x1.0" ]; then
        cd $lichee_top_path/brandy/brandy-1.0/brandy/u-boot-2014.07
    elif [ "x$vbr" == "x2.0" ]; then
        cd $lichee_top_path/brandy/brandy-2.0/u-boot-2018
    fi
}

# goto longan/device/config/chips/{ic}/configs/{board} dir
function cbd()
{
    if [ -z "$(_findvalidfile "$lichee_build_config")" ]; then
        echo "Please run ./build.sh config first!"
        return 1
    fi
    local ic=$(_loadconfig LICHEE_IC "$lichee_build_config")
    local chip=$(_loadconfig LICHEE_CHIP "$lichee_build_config")
    local board=$(_loadconfig LICHEE_BOARD "$lichee_build_config")
    cd $lichee_top_path/device/config/chips/$ic/configs/$board
}

# goto longan/device/config/chips/{ic}/bin dir
function cbb()
{
    if [ -z "$(_findvalidfile "$lichee_build_config")" ]; then
        echo "Please run ./build.sh config first!"
        return 1
    fi
    local ic=$(_loadconfig LICHEE_IC "$lichee_build_config")
    local chip=$(_loadconfig LICHEE_CHIP "$lichee_build_config")
    local bussiness=$(_loadconfig LICHEE_BUSSINESS "$lichee_build_config")
    if [ "$bussiness" == "" ]; then
        cd $lichee_top_path/device/config/chips/$ic/bin
    else
        cd $lichee_top_path/device/config/chips/$ic/$bussiness/bin
    fi
}

# build & pack
function build()
{
    if [ -z "$TARGET_PRODUCT" ]; then
        echo "Please do lunch first!"
        return 1
    fi

    local ic=$(get_build_var TARGET_BOARD_IC)
    local kernel=$(get_build_var TARGET_BOARD_KERN_VER)
    local board=$(get_build_var PRODUCT_BOARD)
    local arch=$(get_build_var BOARD_KERNEL_ARCH)
    local jn=$(($(cat /proc/cpuinfo | grep processor | wc -l)/2))

    # to suport linux-x.y or x.y value config
    kernel="linux-${kernel#linux-}"
    [ -n "$arch" ] && arch="-a $arch"

    (
        cd $lichee_top_path && \
        ./build.sh autoconfig -i $ic -o android -b $board -k $kernel $arch && \
        ./build.sh distclean && \
        ./build.sh && \
        cd $android_top_path && \
        make installclean && \
        make -j $jn && \
        pack $@
    )
}

# nothing to do
function extract-bsp()
{
    echo "extract-bsp has been deprecated, no need to run it now."
    return 0
}

function _package_usage()
{
    local ic=$(_loadandroidconfig TARGET_BOARD_IC "$OUT/buildinfo")
    local chip=$(_loadandroidconfig TARGET_BOARD_CHIP "$OUT/buildinfo")
    local platform=android
    local board=$(_loadandroidconfig PRODUCT_BOARD "$OUT/buildinfo")

    printf "Usage: pack [-cCHIP] [-pPLATFORM] [-bBOARD] [-d] [-s] [-v] [-h]
    -c CHIP (default: $chip)
    -p PLATFORM (default: $platform)
    -b BOARD (default: $board)
    -d pack firmware with debug info output to card0
    -s pack firmware with signature
    -v pack firmware with secureboot
    -h print this help message\n\n"
}

function _package()
{
    local ic=$(_loadandroidconfig TARGET_BOARD_IC "$OUT/buildinfo")
    local chip=$(_loadandroidconfig TARGET_BOARD_CHIP "$OUT/buildinfo")
    local platform=android
    local platform_version=$(_loadandroidconfig PLATFORM_VERSION "$OUT/buildinfo")
    local board=$(_loadandroidconfig PRODUCT_BOARD "$OUT/buildinfo")
    local debug=uart0
    local sigmode=none
    local securemode=none
    local packpath=$lichee_top_path/build
    local addition=$(_loadandroidconfig BOARD_ADD_PACK_CONFIG "$OUT/buildinfo")
    local addtion_files=""
    local key_path=$android_top_path/vendor/security/toc_keys

    while getopts "i:c:p:b:dsvh" arg
    do
        case $arg in
            i)
                ic=$OPTARG
                ;;
            c)
                chip=$OPTARG
                ;;
            p)
                platform=$OPTARG
                ;;
            b)
                board=$OPTARG
                ;;
            d)
                debug=card0
                ;;
            s)
                sigmode=sig
                ;;
            v)
                securemode=secure
                ;;
            h)
                _package_usage
                return 0
                ;;
            ?)
                return 1
                ;;
        esac
    done

    for f in $addition; do
        [[ "$f" =~ : ]] && \
        addtion_files+=" $android_top_path/${f/:*}:$lichee_top_path/out/pack_out/${f#*:}" || \
        addtion_files+=" $android_top_path/$f:$lichee_top_path/out/pack_out/$(basename $f)"
    done

    (
    cd $packpath
    ./pack -a "$addtion_files" -i $ic -c $chip -p $platform -b $board -d $debug -s $sigmode -v $securemode --platform_version ${platform_version} --key_path ${key_path}
    )

    return $?
}

# pack image
function pack()
{
    local device_dir=$(_get_device_dir)
    local ANDROID_IMAGE_OUT=""

    if [ -f $device_dir/package.sh ]; then
        sh $device_dir/package.sh $*
        [ $? -ne 0 ] && return 1
    else
        #verity_data_init

        OPTIND=1
        ANDROID_IMAGE_OUT=$OUT \
        _package $@
        [ $? -ne 0 ] && return 1
        echo -e "\033[31muse pack4dist for release\033[0m"
    fi
}

function _fex_copy()
{
    if [ -e $1 ]; then
        cp -vf $1 $2
    else
        echo $1" not exist"
    fi
}

function _truncate_zip()
{
  local zip_path=$1
  local filesize=$(stat -c%s "${zip_path}")
  local truncatesize=0
  if [[ $(( filesize % 4096 )) -ne 0 ]]; then
    : $(( truncatesize = (filesize + 4095) & -4096 ))
    local len=$(printf "%04x" $((truncatesize-filesize)))
    local lenstr="\\x${len:2:2}\\x${len:0:2}"
    printf ${lenstr} | dd of=${zip_path} bs=1 seek=$(( filesize - 2 ))
    python -c "open(\"${zip_path}\", 'a').truncate(${truncatesize})"
  fi
}

# update uboot into target-files.zip
function update_uboot()
{
    echo "copy fex into $1"
    mkdir ./IMAGES
    local fex_out=$lichee_top_path/out/pack_out

    if [ "$(get_build_var PRODUCT_VIRTUAL_AB_OTA)" = "true" ]; then
        _fex_copy $fex_out/boot-resource.fex ./boot-resource.fex
        _fex_copy $fex_out/env.fex ./env.fex
        _fex_copy $fex_out/boot0_nand.fex ./boot0_nand.fex
        _fex_copy $fex_out/boot0_sdcard.fex ./boot0_sdcard.fex
        _fex_copy $fex_out/boot_package.fex ./u-boot.fex
        _fex_copy $fex_out/toc1.fex ./toc1.fex
        _fex_copy $fex_out/toc0.fex ./toc0.fex
        local zip_path="./IMAGES/custom.img"
        zip -m ${zip_path} boot-resource.fex env.fex boot0_nand.fex boot0_sdcard.fex u-boot.fex toc1.fex toc0.fex
        _truncate_zip ${zip_path}
    else
        _fex_copy $fex_out/boot-resource.fex ./IMAGES/boot-resource.fex
        _fex_copy $fex_out/env.fex ./IMAGES/env.fex
        _fex_copy $fex_out/boot0_nand.fex ./IMAGES/boot0_nand.fex
        _fex_copy $fex_out/boot0_sdcard.fex ./IMAGES/boot0_sdcard.fex
        _fex_copy $fex_out/boot_package.fex ./IMAGES/u-boot.fex
        _fex_copy $fex_out/toc1.fex ./IMAGES/toc1.fex
        _fex_copy $fex_out/toc0.fex ./IMAGES/toc0.fex
    fi
    zip -r -m $1 ./IMAGES
}

# pack for dist
function pack4dist()
{
    # Found out the number of cores we can use
    local cpu_cores=`cat /proc/cpuinfo | grep "processor" | wc -l`
    local JOBS=`expr ${cpu_cores} / 2`
    if [ ${cpu_cores} -le 8 ] ; then
        JOBS=${cpu_cores}
    fi

    make -j $JOBS target-files-package otatools-package
    [ $? -ne 0 ] && return 1

    local keys_dir="./vendor/security/app_keys"
    local target_product=$(get_build_var TARGET_PRODUCT)
    local target_files=$(ls -t $OUT/obj/PACKAGING/target_files_intermediates/${target_product}-target_files-*.zip | head -n 1)
    local signed_target_files=$OUT/$(basename $target_files | sed "s/-target_files-/-signed_target_files-/g")
    local target_images="$OUT/target_images.zip"
    local old_target_files="./old_target_files.zip"

    local product_apex_sign_arg="--extra_apex_payload_key com.android.art.release.apex=vendor/security/app_keys/product_apex_payload.pem \
                                 --extra_apex_payload_key com.android.i18n.apex=vendor/security/app_keys/product_apex_payload.pem \
                                 --extra_apex_payload_key com.android.runtime.apex=vendor/security/app_keys/product_apex_payload.pem"

    local final_target_files="$target_files"
    local k_arg=""
    if [ -d $keys_dir ]; then
        final_target_files=$signed_target_files
        local sign_apk_tool="sign_target_files_apks"
        echo $sign_apk_tool $product_apex_sign_arg -d $keys_dir -o $target_files $final_target_files
        $sign_apk_tool $product_apex_sign_arg -d $keys_dir -o $target_files $final_target_files
        k_arg="-k $keys_dir/releasekey"
    fi

    [ $? -ne 0 ] && return 1

    local full_ota=$OUT/$(basename $final_target_files | sed "s/-\(signed_\)*target_files-/-full_ota-/g")
    local inc_ota=$OUT/$(basename $final_target_files | sed "s/-\(signed_\)*target_files-/-inc_ota-/g")

    img_from_target_files $final_target_files $target_images && \

    unzip -o $target_images -d $OUT && \

    rm -rf $target_images && \

    pack $@ && \

    update_uboot $final_target_files && \

    local ota_tools="ota_from_target_files"
    $ota_tools $k_arg $final_target_files $full_ota
    [ $? -ne 0 ] && return 1

    if [ -f $old_target_files ]; then
        $ota_tools $k_arg -i $old_target_files $final_target_files $inc_ota
        [ $? -ne 0 ] && return 1
    fi
    echo -e "target files package: \033[31m$final_target_files\033[0m"
    echo -e "full ota zip: \033[31m$full_ota\033[0m"
    if [ -f $inc_ota ]; then
        echo -e "inc ota zip: \033[31m$inc_ota\033[0m"
    fi

    if [ "$(get_build_var PRODUCT_VIRTUAL_AB_OTA)" = "true" ]; then
        local ota_json_script_path="bootable/recovery/updater_sample/tools/gen_update_config"
        if [ -f $ota_json_script_path ]; then
            local download_path="file:///data/ota_package/update.zip"
            local ota_json_path="$OUT/ota_virtual_ab.json"
            PYTHONPATH=$ANDROID_BUILD_TOP/build/make/tools/releasetools:$PYTHONPATH $ota_json_script_path --ab_install_type=STREAMING $full_ota $ota_json_path $download_path
            if [ -f $inc_ota ]; then
                local  ota_inc_json_path="$OUT/ota_virtual_ab_inc.json"
                PYTHONPATH=$ANDROID_BUILD_TOP/build/make/tools/releasetools:$PYTHONPATH $ota_json_script_path --ab_install_type=STREAMING $inc_ota $ota_inc_json_path $download_path
            fi
        fi
    fi
}

function _read_var()
{
    local TMP=$3
    if [ -z $TMP ]; then
        read -p "please enter $1($2): " TMP
    else
        echo "$1=${TMP}"
    fi
    eval $1="${TMP:=\"$2\"}"
}

# create new device
function clone()
{
    if [ $# -ne 0 -a $# -ne 5 ]; then
        echo "don't enter any params or use:"
        echo "    clone <DEVICE> <PRODUCT> <BOARD> <MODEL> <DENSITY>"
        return
    fi
    if [ -z "${android_device_path}" ]; then
        echo "please launch first"
        return
    fi

    local source_device_path=`python -c "import os.path; print os.path.relpath('${android_device_path}', '${android_top_path}')"`
    local source_device=$(get_build_var TARGET_DEVICE)
    local source_product=$(get_build_var TARGET_PRODUCT)
    local source_board=$(get_build_var PRODUCT_BOARD)
    local source_model=$(get_build_var PRODUCT_MODEL)
    # PRODUCT_DEVICE
    _read_var PRODUCT_DEVICE ${source_device} $1
    if [ ${source_device} == ${PRODUCT_DEVICE} ]; then
        echo "don't have the same device name!"
        return
    fi
    # PRODUCT_NAME
    _read_var PRODUCT_NAME ${source_product} $2
    if [ ${source_product} == ${PRODUCT_NAME} ]; then
        echo "don't have the same product name!"
        return
    fi
    # config
    _read_var PRODUCT_BOARD "${source_board}" $3
    _read_var PRODUCT_MODEL "${source_model}" $4
    density=`sed -n 's/.*ro\.sf\.lcd_density=\([0-9]\+\).*/\1/p' ${android_device_path}/${source_product}.mk`
    _read_var DENSITY ${density} $5
    # 160(mdpi) 213(tvdpi) 240(hdpi) 320(xhdpi) 400(400dpi) 480(xxhdpi) 560(560dpi) 640(xxxhdpi)
    if [ $DENSITY -lt 186 ]; then
        PRODUCT_AAPT_PREF_CONFIG=mdpi
    elif [ $DENSITY -lt 226 ]; then
        PRODUCT_AAPT_PREF_CONFIG=tvdpi
    elif [ $DENSITY -lt 280 ]; then
        PRODUCT_AAPT_PREF_CONFIG=hdpi
    elif [ $DENSITY -lt 360 ]; then
        PRODUCT_AAPT_PREF_CONFIG=xhdpi
    elif [ $DENSITY -lt 440 ]; then
        PRODUCT_AAPT_PREF_CONFIG=400dpi
    elif [ $DENSITY -lt 520 ]; then
        PRODUCT_AAPT_PREF_CONFIG=xxhdpi
    elif [ $DENSITY -lt 600 ]; then
        PRODUCT_AAPT_PREF_CONFIG=560dpi
    else
        PRODUCT_AAPT_PREF_CONFIG=xxxhdpi
    fi

    TARGET_PATH=${android_device_path}/${PRODUCT_DEVICE}
    if [ -e ${TARGET_PATH} ]; then
        read -p "${TARGET_PATH} is already exists, delete it?(y/n)"
        case ${REPLY} in
            [Yy])
                echo "delete"
                rm -rf ${TARGET_PATH}
                ;;
            [Nn])
                echo "do nothing"
                return
                ;;
            *)
                echo "do nothing"
                return
                ;;
        esac
    fi
    cp -r ${android_device_path}/${source_device} ${TARGET_PATH}
    # create device
    sed -i "/PRODUCT_MAKEFILES/a\    \$(LOCAL_DIR)/${PRODUCT_NAME}.mk \\\\" ${android_device_path}/AndroidProducts.mk
    sed -i "/COMMON_LUNCH_CHOICES/a\    ${PRODUCT_NAME}-user \\\\" ${android_device_path}/AndroidProducts.mk
    sed -i "/COMMON_LUNCH_CHOICES/a\    ${PRODUCT_NAME}-userdebug \\\\" ${android_device_path}/AndroidProducts.mk
    sed -i "/COMMON_LUNCH_CHOICES/a\    ${PRODUCT_NAME}-eng \\\\" ${android_device_path}/AndroidProducts.mk
    cp ${android_device_path}/${source_product}.mk ${android_device_path}/${PRODUCT_NAME}.mk

    sed -i "s|${source_device}|${PRODUCT_DEVICE}|g" ${android_device_path}/${PRODUCT_NAME}.mk
    sed -i "s|${source_product}|${PRODUCT_NAME}|g" ${android_device_path}/${PRODUCT_NAME}.mk
    sed -i "s|\(PRODUCT_BOARD := \).*|\1${PRODUCT_BOARD}|g" ${android_device_path}/${PRODUCT_NAME}.mk
    sed -i "s|\(PRODUCT_MODEL := \).*|\1${PRODUCT_MODEL}|g" ${android_device_path}/${PRODUCT_NAME}.mk
    sed -i "s|\(ro\.sf\.lcd_density=\).*|\1${DENSITY}|g" ${android_device_path}/${PRODUCT_NAME}.mk
    sed -i "s|\(PRODUCT_AAPT_PREF_CONFIG :=\).*|\1${PRODUCT_AAPT_PREF_CONFIG}|g" ${android_device_path}/${PRODUCT_NAME}.mk

    echo -e "\033[31mclone done\033[0m"
}

function _hook_function_inject()
{
    local orig_func init_func done_func error_act action

    unset OPTIND

    while getopts 'f:s:e:a:h' opt; do
        case $opt in
            f)
                orig_func=$OPTARG
                ;;
            s)
                init_func=$OPTARG
                ;;
            e)
                done_func=$OPTARG
                ;;
            a)
                error_act=$OPTARG
                ;;
            h)
                printf "function star/end inject hook\n"
                ;;
        esac
    done

    [ -z "$orig_func" ] && return 0
    [ -z "$init_func" ] && \
    [ -z "$done_func" ] && return 0

    [ "$error_act" == "b" ] && action="\t[ \$? -ne 0 ] \&\& return 1"

    local pattern
    [ -n "$init_func" ] && pattern+=";s|^{|{\n\t$init_func \"\$@\"\n$action|"
    [ -n "$done_func" ] && pattern+=";s|^}|$action\n\t$done_func \"\$@\"\n}|"

    local TEMP_FUNC="$(declare -f $orig_func | sed -e "$pattern")"
    eval "$TEMP_FUNC"
}

function _update_build_config()
{
    local ic=$(_loadandroidconfig TARGET_BOARD_IC "$OUT/buildinfo")
    local board=$(_loadandroidconfig PRODUCT_BOARD "$OUT/buildinfo")
    lichee_build_config="$lichee_top_path/.buildconfig $lichee_top_path/out/$ic/$board/android/.buildconfig"
    android_device_path=$(dirname $(find ${android_top_path}/device -name ${TARGET_PRODUCT}.mk))
}

_hook_function_inject -f lunch -e _update_build_config

function _check_disclaimer()
{
    $(dirname $(readlink -f ${BASH_SOURCE[0]}))/disclaimer/disclaimer.sh
    if [ $? -ne 0 ]; then
        local fun_list file_list
        file_list=$(echo ${BASH_SOURCE[@]} | xargs -n 1 | sort | uniq)
        fun_list+=" $(sed -n "s/^\s*function\s\+\([^(,), ]\+\).*$/\1/p" $file_list)"

        for i in $fun_list; do
            unset -f $i
        done

        unset android_top_path
        unset android_device_path
        unset lichee_top_path
        unset lichee_build_config

        return 1
    fi
}

_hook_function_inject -f addcompletions -s _check_disclaimer -e vendor_help -a b

_vendor_env_setup
