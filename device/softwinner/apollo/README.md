# 方案配置


## 方案配置原则
1. 方案配置尽量以模块作为分界线，一个模块的配置尽量放在同一级目录下，使配置的层次结构清晰。
2. 方案配置尽量提炼出公共部分，公共部分放在common目录下，新增板级配置时只需修改定制化的内容。
3. 方案配置尽量在每个模块下提供配置说明/版本信息等。



## 方案配置结构
```
├── AndroidProducts.mk                  -- 添加引用方案mk及lunch名称
├── BoardConfig.mk                      -- 芯片平台共用的BoardConfig.mk
├── apollo-p2                           -- 板级目录，命名与PRODUCT_DEVICE相同
│   ├── BoardConfig.mk                  -- 板级BoardConfig.mk
│   ├── camera                          -- 板级模块配置目录
│   │   ├── config.mk                   -- 模块配置文件，固定名称config.mk
│   │   ├── init.camera.rc              -- 模块init启动初始化rc文件
│   │   └── camera.cfg                  -- 模块其他配置文件
│   ├── input
│   └── system
│       ├── bootanimation.zip           -- 开机动画
│       ├── sys_partition.fex           -- 分区配置文件
│       └── vendor_ramdisk.modules      -- 启动必须模块，非必须的放在init.secondmodules.rc
├── apollo_p2.mk                        -- 板级总配置文件，主要配置方案信息及一些配置开关
├── common                              -- 方案的公共配置目录，主要放同一芯片平台方案共用配置
│   ├── camera                          -- 公共模块的配置目录
│   │   └── config.mk                   -- 公共模块的配置文件config.mk
│   ├── sepolicy
│   ├── system
│   │   ├── *.xml
│   │   ├── fstab.sun50iw9p1           -- fstab文件系统挂载配置文件
│   │   ├── init.sun50iw9p1.rc
│   │   ├── init.secondmodules.rc       -- 非启动必须模块，在这里会加载一些不紧急的驱动
│   │   ├── init.recovery.sun50iw9p1.rc
│   │   ├── env.cfg                     --env环境变量
│   │   └── ...
│   └── wireless
├── for_q_ota                           -- 用于从Android 10升级所有配置，新方案可忽略
└── README                              -- 本文档，方案配置说明。
```

注意事项：

模块初始化文件尽量使用init.${module}.rc的命名规则，并在模块config.mk中拷贝到vendor/etc/init目录中。

板级BoardConfig.mk主要配置kernel cmdline及板级外设配置。



## 新增一个模块

在common或者板级目录下创建一个子目录，新建一个config.mk作为该模块配置，模块配置会自动加载：

```makefile
LOCAL_MODULE_PATH := $(shell dirname $(lastword $(MAKEFILE_LIST)))

# copy files
PRODUCT_COPY_FILES += \
    $(LOCAL_MODULE_PATH)/model.cfg:$(TARGET_COPY_OUT_VENDOR)/etc/model.cfg
```



## 新增一个方案

```
# cd android
# source ./build/envsetup.sh
# lunch（选择对应平台公版方案配置）
# clone
```

根据提示输入新增方案信息：
```
please enter PRODUCT_DEVICE({device-name}): newdevice
please enter PRODUCT_NAME({product-name}): newproduct
please enter PRODUCT_BOARD({BOARD}): newboard
please enter PRODUCT_MODEL({MODEL_NAME}): NEWMODEL
please enter DENSITY(160): 320
```

完成以上操作后，将会新增newdevice目录配置及newproduct.mk文件，配置是参考lunch的参考方案。
可根据实际外设及方案需求，修改方案目录中相关的配置及目录。

