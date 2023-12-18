# TA/CA开发指引

## 开发环境目录结构

开发环境目录结构如图

.  
├── build.sh -> dev_kit/build.sh  
├── demo  
├── dev_kit  
├── platform_config.mk  
└── README.md  


|文件（夹）|说明|
|:-|:-|
|build.sh|编译脚本|
|dev_kit|编译依赖(平台相关)|
|demo|demo|
|platform_config.mk|平台信息|
|README.md|本说明文件|
## 编译

>

1. 运行./build.sh config 配置编译选项，与平台、是否加密ta等

2. 运行./build.sh 编译所有DEMO

### 编译脚本使用说明

命令|功能
-|-
-h|显示帮助消息
helloworld|编译demo helloworld
encrypt_storage|编译demo REE上的加密文件存储
clean|清除所有demo编译输出
config|选择编译平台
## 拷贝

拷贝下列文件到设备

### TA/CA运行必须的公共文件
TA/CA正常交互需要两个文件
文件名|说明
-|-
tee-supplicant |处理optee os的请求，如读取文件系统中的TA等
libteec.so |提供API供应用程序向安全侧发送请求，如加载TA，项TA发送命令等

要使用TEE环境，就需要在文件系统中准备这两个文件，由于运行环境差异巨大，安卓和linux
固件准备这两个文件的方法不一样，下面逐一说明
#### linux固件

文件|拷贝到
-|-
./dev_kit/${platform}/export-ca/bin/tee-supplicant |不限定位置
./dev_kit/${platform}/export-ca/exportlib/libteec.so |/lib
./dev_kit/${platform}/export-ca/exportlib/libteec.so.1|/lib
./dev_kit/${platform}/export-ca/exportlib/libteec.so.1.0|/lib

#### android固件
在方案配置中加入下面两行，安卓在编译过中就会编译并拷贝这两个文件到文件系统中
```
PRODUCT_PACKAGES += \
    libteec \
    tee_supplicant
```

### 实际的TA/CA程序

demo对应的输出文件(详见各demo的说明)

## 运行

### 运行辅助REE与TEE通信的守护进程
#### linux固件

```
${Supp_dir}/tee-supplicant &
```

Supp_dir为放置tee-supplicant的目录

tee-supplicant作为守护进程运行，不会返回，必须带```"&"```运行

#### android固件
在方案配置中加入下面两行，安卓系统在启动后会自动将tee-supplicant运行起来
```
PRODUCT_PACKAGES += \
    libteec \
    tee_supplicant
```

### 运行DEMO

#### helloworld

本demo展示CA如何调用TA，以及如何通过共享内容向TA传输数据

a. 拷贝的文件

文件|拷贝到
------------------------------------------|----------------------------------------
./demo/optee_helloworld/ca/hello_world_ca |不限定位置
./demo/optee_helloworld/ta/12345678-4321-8765-9b74f3fc357c7c61.ta|/lib/optee_armtz（安卓为{teec_load_path}/optee_armtz/, ${teec_load_path}的路径视安卓版本而定，详见android/hardware/aw/optee_client/tee_supplicant/Android.bp或Android.mk中对TEEC_LOAD_PATH的定义）

b. 运行

命令|输出
------------------------------------------|----------------------------------------
hello_world_ca |NA:init context\linebreak NA:open session\linebreak TA:creatyentry!\linebreak TA:open session!\linebreak NA:allocate memory\linebreak NA:invoke command\linebreak TA:rec cmd 0x210\linebreak TA:hello world!\linebreak NA:finish with 0
hello_world_ca 1234\linebreak 注:{1234}可以为任意字符串|NA:init context\linebreak NA:open session\linebreak TA:creatyentry!\linebreak TA:open session!\linebreak NA:allocate memory\linebreak NA:invoke command: hello 1234\linebreak TA:rec cmd 0x210\linebreak TA:hello 1234\linebreak NA:finish with 0
#### encrypt_file_storage

本demo展示如何通过TA在REE的文件系统创建、读、写、删除加密文件

a. 拷贝的文件

文件|拷贝到
------------------------------------------|----------------------------------------
demo/encrypt_file_storage/ca/demo/demo|任意位置
demo/encrypt_file_storage/ta/2977f028-30d8-478b-975c-beeb3c134c34.ta|/lib/optee_armtz（安卓为{teec_load_path}/optee_armtz/, ${teec_load_path}的路径视安卓版本而定，详见android/hardware/aw/optee_client/tee_supplicant/Android.bp或Android.mk中对TEEC_LOAD_PATH的定义）

b. 运行

命令\linebreak 文件名以test为例|输出
------------------------|----------------------------------------
demo -c test| 创建文件成功没有特殊输出
demo -w test\linebreak 写的数据由demo随机生成|---- Write file:test with 256 Bytes data: ----\linebreak 99 9c 9b 66 88 2c c8 c9 19 55 72 10 f7 c3 70 7f\linebreak  1a 51 56 74 35 03 e4 6f 1b 40 4d 64 29 b5 ba c2\linebreak  52 56 a8 db 03 f1 25 1c 47 97 ac bf da 1d 3f f4\linebreak  ed 15 69 a3 18 cd 92 33 0f df 98 b7 15 d2 fa 67\linebreak  a8 23 c2 ab 15 68 48 dc 00 f4 9c db 91 5b d0 80\linebreak  70 ba a3 88 08 36 3b 96 16 53 ce aa 26 c9 12 4f\linebreak  ec 55 fa 82 bd c2 5f 3d b7 7b 98 4a 56 e9 4a c6\linebreak  a4 ed ce 2c 24 89 c3 b9 dd 92 64 83 db f5 d2 48\linebreak  ca 4e ca 08 11 a9 46 49 a4 de 93 fa c8 5d c1 ec\linebreak  4b 10 1a ee 9a 5d 28 f7 6f 8c fa 4b 02 ce 13 cd\linebreak  9c de d5 ad 08 9b 76 ad 7b 0a a8 c3 e6 ea 31 b1\linebreak  7a 4b a0 94 28 c8 8c 97 d4 08 62 d7 56 75 25 f2\linebreak  d3 7a 20 5c 17 97 0a 12 21 32 55 09 1d 86 ba 18\linebreak  51 db ac 79 a4 b9 90 f9 c1 72 51 18 68 76 8a 3c\linebreak  70 aa 98 07 c1 22 19 63 55 ee 6c f1 75 a6 89 c7\linebreak  02 37 c0 27 f0 d1 21 32 c3 72 c9 2c 68 54 e8 d8\linebreak \linebreak ---- Write file:test end! ----
demo -r test|---- Read file:test 256 Bytes data: ----\linebreak 99 9c 9b 66 88 2c c8 c9 19 55 72 10 f7 c3 70 7f\linebreak 1a 51 56 74 35 03 e4 6f 1b 40 4d 64 29 b5 ba c2\linebreak 52 56 a8 db 03 f1 25 1c 47 97 ac bf da 1d 3f f4\linebreak ed 15 69 a3 18 cd 92 33 0f df 98 b7 15 d2 fa 67\linebreak a8 23 c2 ab 15 68 48 dc 00 f4 9c db 91 5b d0 80\linebreak 70 ba a3 88 08 36 3b 96 16 53 ce aa 26 c9 12 4f\linebreak ec 55 fa 82 bd c2 5f 3d b7 7b 98 4a 56 e9 4a c6\linebreak a4 ed ce 2c 24 89 c3 b9 dd 92 64 83 db f5 d2 48\linebreak ca 4e ca 08 11 a9 46 49 a4 de 93 fa c8 5d c1 ec\linebreak 4b 10 1a ee 9a 5d 28 f7 6f 8c fa 4b 02 ce 13 cd\linebreak 9c de d5 ad 08 9b 76 ad 7b 0a a8 c3 e6 ea 31 b1\linebreak 7a 4b a0 94 28 c8 8c 97 d4 08 62 d7 56 75 25 f2\linebreak d3 7a 20 5c 17 97 0a 12 21 32 55 09 1d 86 ba 18\linebreak 51 db ac 79 a4 b9 90 f9 c1 72 51 18 68 76 8a 3c\linebreak 70 aa 98 07 c1 22 19 63 55 ee 6c f1 75 a6 89 c7\linebreak 02 37 c0 27 f0 d1 21 32 c3 72 c9 2c 68 54 e8 d8\linebreak \linebreak ---- Read file:test end! ----
demo -d test|Delete file:test !
demo -h|显示帮助信息
#### base64-usage

本demo展示提供的base64软实现如何使用

a. 拷贝的文件

文件|拷贝到
------------------------------------------|----------------------------------------
./demo/base64-usage/ca/base64_demo |不限定位置
./demo/base64-usage/ta/b0e8fef8-b857-4dd4-bfa6088373069255.ta|/lib/optee_armtz（安卓为{teec_load_path}/optee_armtz/, ${teec_load_path}的路径视安卓版本而定，详见android/hardware/aw/optee_client/tee_supplicant/Android.bp或Android.mk中对TEEC_LOAD_PATH的定义）

b. 运行

命令|输出
------------------------------------------|----------------------------------------
base64_demo -e 123456\linebreak -e 后为需要编码的字节串，两个字符对应一个字节 |input bytes:\linebreak 0x12 0x34 0x56 \linebreak NA:open session\linebreak TA:creatyentry!\linebreak TA:open session!\linebreak NA:allocate memory\linebreak TA:rec cmd 0x221\linebreak input size:3\linebreak encode result:\linebreak EjRW\linebreak NA:finish with 0\linebreak 
base64_demo -d EjRW\linebreak -d 后为需要解码的字符串|NA:open session\linebreak TA:creatyentry!\linebreak TA:open session!\linebreak NA:allocate memory\linebreak TA:rec cmd 0x222\linebreak input size:4\linebreak decode result:\linebreak 0x12 0x34 0x56 \linebreak NA:finish with 0

* 提供给TA的base64实现的接口

接口|说明
------------------------------------------|----------------------------------------
size_t EVP_EncodeBlock(uint8_t *dst, const uint8_t *src,size_t src_len);|对src开始长为src_len的u8数组进行编码，输出到dst，输出的字符串用'\\0'结尾。返回值为编码后字符串长度，不含结尾的'\\0'
int EVP_EncodedLength(size_t *out_len, size_t len);\linebreak int EVP_DecodedLength(size_t *out_len, size_t len);|计算len长度的输入在编码/解码后的输出长度，写到out_len中。计算成功返回1，计算失败返回0
int EVP_DecodeBase64(uint8_t *out, size_t *out_len,size_t max_out, const uint8_t *in,size_t in_len);|对in开始长为in_len的输入进行解码，输出到out,解码后的长度填入out_len,max_out为out对应的buffer的大小。成功返回1，失败返回0。
## 编译配置

下面介绍开发新的TA/CA时如何配置依赖,篇幅有限，这里只提一些关键点，具体请参考demo的源码

### TA

a. 在TA源码的根目录创建Makefile,包含以下内容

内容|说明
------------------------------------------|----------------------------------------
BINARY=$(UUID)| 最终生成的 .ta 文件的文件名，TEE加载TA时已UUID为文件名读取文件，必须与TA的UUID一致
include ${DIR}/ta_dev_kit.mk |TA代码编译的二进制有特殊处理的需要，使用dev_ki/arm-plat-${chip}/export-ta_arm32/mk/ta_dev_kit.mk进行编译

b. 在源码的根目录创建sub.mk，提供编译信息

命令|作用
------------------------------------------|----------------------------------------
srcs-y += filename|filename加入编译
subdirs-y += dirname|include dirname下的sub.mk
global-incdirs-y += dirname|编译TA时头文件搜索目录
cflags-/<filename>-y +=| c文件编译flag\linebreak filename为生效的文件，cflags-y对所有c文件有效
aflags-/<filename>-y +=| s文件编译flag
cppflags-/<filename>-y +=| c及s文件编译flag
TA_PRIVATE_FLAGS += |连接生成TA的elf时使用的flag

c. 创建user_ta_header_defines.h文件提供TA的信息

optee_os在接在TA时会根据这里的配置项给TA配置运行环境。

如TA的堆栈，TA可用的堆栈大小在此时固定，加载后不可再调整。如果TA需要使用较大的堆栈空间，请调整 TA_STACK_SIZE、TA_DATA_SIZE的值，让optee_os为TA分区更多堆栈空间。

宏|作用
------------------------------------------|----------------------------------------
TA_UUID|TA的uuid，与$(BINARY)一致
TA_FLAGS|运行配置，使用demo默认值即可。\linebreak 完整选项见dev_kit/arm-plat-${chip} /export-ta_arm32/include/user_ta_header.h
TA_STACK_SIZE|栈大小
TA_DATA_SIZE|堆大小

d. 包含描述TEE TA API的头文件并提供TEE TA API要求的实现

实现|描述
------------------------------------------|----------------------------------------
TEE_Result TA_CreateEntryPoint(void)|打开TA的回调
void TA_DestroyEntryPoint(void)|关闭TA的回调
TEE_Result TA_OpenSessionEntryPoint(uint32_t nParamTypes, TEE_Param pParams[4], void **ppSessionContext)|创建新session时的回调
void TA_CloseSessionEntryPoint(void *pSessionContext)|关闭session时的回调
TEE_Result TA_InvokeCommandEntryPoint(void *pSessionContext, uint32_t nCommandID, uint32_t nParamTypes, TEE_Param pParams[4])|执行命令的回调
### CA

a. 包含描述TEE client API的头文件

a. 调用TEE client API与TA交互，详见demo

a. 将dev_kit/arm-plat-${chip}/export-ca/include加入包含目录

a. 将dev_kit/arm-plat-${chip}/export-ca/exportlib加入库搜索目录

a. 使用 ```"-lteec"```选项连接实现TEE client API 的动态库libteec.so
#### 安卓环境下CA编译
对于安卓固件，ca需要使用安卓的编译环境进行编译，具体操作方法如下：
```
    a. 将demo文件夹拷贝到安卓环境
    b.I 手动编译（安卓环境lunch后）
        在demo/*/ca下执行mm命令
    b.II 统一编译
        将demo ca加入安卓产品mk的 PRODUCT_PACKAGES ，package名字见demo/*/Android.bp
```


### TA加密

a. 打开/关闭TA加密

对于支持TA加密的平台，在使用./build.sh config 选中该平台后会提示 encrypt TA(y/n): ,y、n对应是否对TA进行加密，secure os加载TA时会自动判断TA是否需要解密。

b. 配置加密密钥

TA加密使用通过aes进行，密钥长度为128bit(16字节)，存放在dev_kit/arm-plat-${chip}/export-ta_arm32/keys/ta_aes_key.bin文件中。如果需要修改TA加密使用的密钥，请修改此bin文件。

c. 解密密钥源
目前支持使用两种key解密TA，如下：
1. ssk
    对应 using derive key TA(y/n): n
    芯片使用efuse中的ssk key对TA进行解密。请确保ta_aes_key.bin的内容与芯片的efuse中的ssk key一致。
2. rotpk 派生的密钥
    对应 using derive key TA(y/n): y
    芯片使用rotpk派生的key对TA进行解密。请使用以下命令生成aes key bin文件并覆盖ta_aes_key.bin。
```
   dev_kit/${platform}/export-ta-arm32/scripts/generate_ta_key.py --rotpk longan/out/${product}/common/keys/rotpk.bin --out ${out_put_file}
```

### TA签名密钥更换
OPTEE在运行TA前会对TA进行验签，可以使用以下工具更新签名TA、验签TA使用的密钥
```
    dev_kit/arm-plat-${chip}/export-ta_arm32/scripts/update_optee_key    //更新OPTEE验签使用的公钥
    dev_kit/arm-plat-${chip}/export-ta_arm32/scripts/resign.py           //更新TA签名使用的私钥
```

* 注意，安卓固件本身已经含有预编译的TA，如果更新密钥，请同步更新安卓的TA，TA路径为
* android/device/softwinner/common/optee_ta/

