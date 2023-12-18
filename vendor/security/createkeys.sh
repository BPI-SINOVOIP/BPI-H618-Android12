#!/bin/bash

localpath=$(cd $(dirname $BASH_SOURCE[0]) && pwd)

# use to create key
function android_createkeys()
{
    if [ -z "$ANDROID_BUILD_TOP" ] || [ -z "$TARGET_PRODUCT" ]; then
        echo "Must do source & lunch before create key."
        return 1
    fi

    local target_device=`get_build_var TARGET_DEVICE`
    if [ -z "$target_device" ]; then
        cur_shell_file_name=$(basename $0)
        echo "Error: Must run $cur_shell_file_name in current process(. $cur_shell_file_name), not new process(./$cur_shell_file_name)"
        return 1
    fi

    local android_top_path=$ANDROID_BUILD_TOP
    local lichee_top_path=$android_top_path/longan
    local device_dir=$(dirname $(find ${android_top_path}/device -name ${TARGET_PRODUCT}.mk))
    local toc_cfg=$device_dir/$target_device/system/dragon_toc.cfg
    local key_out=$localpath/toc_keys
    local app_keys_out=$localpath/app_keys
    local tools_path=$lichee_top_path/tools/pack/pctools/linux/openssl
    local subj='/C=US/ST=California/L=Mountain View/O=Android/OU=Android/CN=Android/emailAddress=android@android.com'

    if [ ! -f $toc_cfg ]; then
        echo -e "\033[47;31mERROR: $*\033[0m" "$toc_cfg file not found"
        return 1
    elif [ ! -d $tools_path ]; then
        echo -e "\033[47;31mERROR: $*\033[0m" "$tools_path directory not found"
        return 1
    fi

    export PATH=$tools_path:$PATH

    # create dragon_toc keys
    dragonsecboot -key $toc_cfg $key_out

    if [ $? -ne 0 ]; then
        echo -e "\033[47;31mERROR: $*\033[0m" "dragon toc run error"
        return 1
    fi

    echo -e "\033[0;31;1mINFO: $*\033[0m" "use $toc_cfg to create keys to $key_out"

    export OPENSSL_CONF=$localpath/openssl.cnf
    (
    rm -rf $app_keys_out
    mkdir -p $app_keys_out
    cd $app_keys_out

    # create APP and OTA keys
    $android_top_path/development/tools/make_key platform "$subj"
    $android_top_path/development/tools/make_key media "$subj"
    $android_top_path/development/tools/make_key shared "$subj"
    $android_top_path/development/tools/make_key networkstack "$subj"
    $android_top_path/development/tools/make_key releasekey "$subj"
    openssl genrsa -out product_apex_payload.pem 4096
    )

    echo -e "\033[0;31;1mINFO: $*\033[0m" "create APP and OTA keys to $app_keys_out"
}

android_createkeys
