/********************************************
  KeyMaster V2 OEM API
  ALLWINNER TECH.CO
  2018-01-18
********************************************/
#define LOG_TAG "Keymaster_na"
#define LOG_NDEBUG 1
#include <utils/Log.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/time.h>
#include <errno.h>
#include <assert.h>
#include <keymaster/UniquePtr.h>
#include <hardware/hardware.h>
#include <hardware/keymaster_common.h>
#include <hardware/keymaster2.h>
#include <private/android_filesystem_config.h>
#include <keymaster/authorization_set.h>
#include <keymaster/android_keymaster_utils.h>
#include <tee_client_api.h>
#include "include/keymaster_errno.h"
#include "include/keymaster_def.h"
#include <algorithm>
#include <vector>

#include <type_traits>
#include <logger.h>

typedef keymaster2_device_t  keymaster_device_t;
typedef keymaster::UniquePtr<keymaster_device_t> Unique_keymaster_device_t;

enum {
    MSG_KEYMASTER_V2_INITIALIZE = 0x201,
    MSG_KEYMASTER_V2_TERMINATE = 0x202,
    MSG_KEYMASTER_V2_CONFIGURE = 0x210,
    MSG_KEYMASTER_V2_ADD_RNG_ENTROPY = 0x212,
    MSG_KEYMASTER_V2_GENERATE_KEY = 0x214,
    MSG_KEYMASTER_V2_GET_KEY_CHARAC = 0x216,

    MSG_KEYMASTER_V2_IMPORT_KEY = 0x21B,
    MSG_KEYMASTER_V2_EXPORT_KEY = 0x21D,
    MSG_KEYMASTER_V2_ATTEST_KEY = 0x21E,
    MSG_KEYMASTER_V2_UPGRAGE_KEY = 0x220,
    MSG_KEYMASTER_V2_DELETE_KEY = 0x222,
    MSG_KEYMASTER_V2_DELETE_ALL_KEYS = 0x224,

    MSG_KEYMASTER_V2_BEGIN  = 0x226,
    MSG_KEYMASTER_V2_UPDATE = 0x228,
    MSG_KEYMASTER_V2_FINISH = 0x22A,
    MSG_KEYMASTER_V2_ABORT = 0x22C,

    MSG_KEYMASTER_V2_MAX
};

#define TEE_KEYMASTER_KEY_INOUT_COMMON_SIZE   (70 * 1024)

//UUID for keymaster v2
static const uint8_t KeyMaster_v2_UUID[16] = {
  0x49,0xb5,0xf7,0xf5,0x64,0xba,0xfe,0x44,
  0x9b,0x74,0xf3,0xfc,0x35,0x7c,0x7c,0x61
};

static pthread_mutex_t gKeyMasterV2Mutex = PTHREAD_MUTEX_INITIALIZER;
static TEEC_Context *gKeyMasterV2Context = NULL;
static TEEC_Session *gKeyMasterV2Session = NULL;
static bool keymaster_v2_configured = 0;

typedef struct {
    uint32_t buf;
    int     size;
}TEE_params_set;

typedef struct {
    int buff_count;
    int buff_total_size;
}TEE_params_expand;

typedef struct {
    uint32_t params; /* may be NULL if length == 0 */
    uint32_t length;
} tee_keymaster_key_param_set_t;

typedef struct {
    uint32_t data_offset;
    uint32_t data_length;
} tee_keymaster_blob_t;

typedef struct {
    keymaster_tag_t tag;
    union {
        uint32_t enumerated;   /* KM_ENUM and KM_ENUM_REP */
        bool boolean;          /* KM_BOOL */
        uint32_t integer;      /* KM_INT and KM_INT_REP */
        uint64_t long_integer; /* KM_LONG */
        uint64_t date_time;    /* KM_DATE */
        tee_keymaster_blob_t blob; /* KM_BIGNUM and KM_BYTES*/
    };
}tee_keymaster_key_param_t;

typedef struct {
    uint32_t entries_offset;
    uint32_t entry_count;
} tee_keymaster_cert_chain_t;

typedef struct {
    tee_keymaster_key_param_set_t hw_enforced;
    tee_keymaster_key_param_set_t sw_enforced;
} tee_keymaster_key_characteristics_t;

static inline int tag_is_blob(keymaster_tag_t tag)
{
    keymaster_tag_type_t t = (keymaster_tag_type_t)(tag & (0xF << 28));

    return ( t== KM_BYTES || t == KM_BIGNUM);
}

#if 0
void tee_hexdump(unsigned char * data, int  size)
{
    int i = 0;
    int line ;

    char tmp[32 + 16 + 11 + 1];
    int  offset = 0;

    ALOGV("data buf=%p, size=%d\n", data, size);
    for(line = 0; offset < size; line ++ ) {
        sprintf(&tmp[0], "0x%08x:  ", line*16);
        if(size - offset >= 16) {
            for(i = 0; i < 16; i++) {
                sprintf(&tmp[(i + 1) * 3 + 8], "%02x ", data[offset + i]);
            }
            offset += 16;
        } else {
            for(i = 0; i < size - offset; i++) {
                sprintf(&tmp[(i + 1) * 3 + 8], "%02x ", data[offset + i]);
            }
            offset = size;
        }
        tmp[32 + 16 + 11] = '\0';
        ALOGV("%s", tmp);
    }
}

static void tee_ParamSet_view(const keymaster_key_param_set_t* set)
{
    size_t i;
    keymaster_key_param_t* params = set->params;

    ALOGV("params count=%d", (unsigned int)set->length);
    for(i=0;i<set->length;i++) {
        ALOGV("params[%d].tag = 0x%x", (unsigned int)i, params[i].tag);
        if (tag_is_blob(params[i].tag)) {
            ALOGV("    buffer: size=0x%x", (unsigned int)params[i].blob.data_length);
        } else {
            ALOGV("    value: 0x%x", params[i].integer);
        }
    }
}
#endif

static size_t compute_param_set_length(const keymaster_key_param_set_t* set)
{
    size_t i, total_length;
    keymaster_key_param_t* params = set->params;

    total_length = sizeof(tee_keymaster_key_param_set_t) +
                   sizeof(tee_keymaster_key_param_t) * set->length;

    for(i=0;i<set->length;i++) {
        if (tag_is_blob(params[i].tag)) {
            total_length += params[i].blob.data_length;
        }
    }
    return total_length;
}

static size_t tee_copy_params2_buf(uint8_t* buf,
                                   const keymaster_key_param_set_t *params)
{
    size_t i, buf_size = 0;
    tee_keymaster_key_param_t *tee_params_buf;
    uint8_t *tee_data_buf;

    tee_keymaster_key_param_set_t *set = (tee_keymaster_key_param_set_t *)buf;

    //tee_hexdump((uint8_t*)params->params, 256);

    //fill key_param_set
    set->params = sizeof(tee_keymaster_key_param_set_t);
    set->length = params->length;
    //fill key_params
    tee_params_buf =(tee_keymaster_key_param_t*)(buf + sizeof(tee_keymaster_key_param_set_t));
    tee_data_buf = buf + sizeof(tee_keymaster_key_param_set_t) +
            sizeof(tee_keymaster_key_param_t) * params->length;

    //fill blob
    for(i=0;i<params->length;i++) {
        tee_params_buf[i].tag = params->params[i].tag;
        keymaster_tag_type_t type = keymaster_tag_get_type(params->params[i].tag);
        switch(type){
        case KM_INVALID:
            break;
        case KM_ENUM:
        case KM_ENUM_REP:
            tee_params_buf[i].enumerated = params->params[i].enumerated;
            break;
        case KM_BOOL:
            tee_params_buf[i].boolean = params->params[i].boolean;
            break;
        case KM_UINT:
        case KM_UINT_REP:
            tee_params_buf[i].integer = params->params[i].integer;
            break;
        case KM_ULONG:
        case KM_ULONG_REP:
            tee_params_buf[i].long_integer = params->params[i].long_integer;
            break;
        case KM_DATE:
            tee_params_buf[i].date_time = params->params[i].date_time;
            break;
        case KM_BIGNUM:
        case KM_BYTES:
            tee_params_buf[i].blob.data_offset = (uint32_t)(tee_data_buf - buf);
            tee_params_buf[i].blob.data_length = params->params[i].blob.data_length;
            memcpy(tee_data_buf, params->params[i].blob.data,
                    params->params[i].blob.data_length);
            tee_data_buf += params->params[i].blob.data_length;
            break;
        }
    }
    buf_size = (size_t)(tee_data_buf - buf);

    return buf_size;
}

static void tee_CopyToParamSet(uint8_t *src, keymaster_key_param_set_t *set, uint8_t *base)
{
    tee_keymaster_key_param_set_t *tee_input = (tee_keymaster_key_param_set_t *)src;
    uint8_t *params_buf = base + tee_input->params;
    uint32_t length = tee_input->length;

    set->length = length;

    if (length == 0) {
        set->params = NULL;
        return ;
    }
    set->params = reinterpret_cast<keymaster_key_param_t*>
                        (malloc(sizeof(keymaster_key_param_t) * length));

    memset(set->params, 0, sizeof(keymaster_key_param_t) * length);
    tee_keymaster_key_param_t *param = (tee_keymaster_key_param_t *)params_buf;
    keymaster_key_param_t *dst = set->params;

    for (size_t i = 0; i < length; ++i) {
        dst[i].tag = param[i].tag;
        keymaster_tag_type_t type = keymaster_tag_get_type(param[i].tag);
        switch(type){
        case KM_INVALID:
            break;
        case KM_ENUM:
        case KM_ENUM_REP:
            dst[i].enumerated = param[i].enumerated;
            break;
        case KM_BOOL:
            dst[i].boolean = param[i].boolean;
            break;
        case KM_UINT:
        case KM_UINT_REP:
            dst[i].integer= param[i].integer;
            break;
        case KM_ULONG:
        case KM_ULONG_REP:
            dst[i].long_integer= param[i].long_integer;
            break;
        case KM_DATE:
            dst[i].date_time= param[i].date_time;
            break;
        case KM_BIGNUM:
        case KM_BYTES:
            void *tmp = malloc(param[i].blob.data_length);

            memcpy(tmp, (void *)(base + param[i].blob.data_offset), param[i].blob.data_length);
            dst[i].blob.data = reinterpret_cast<uint8_t*>(tmp);
            dst[i].blob.data_length = param[i].blob.data_length;
            break;
        }
    }
}

int KeyMaster_V2_Initialize(void)
{
    ALOGV("%s %d", __func__, __LINE__);
    pthread_mutex_lock(&gKeyMasterV2Mutex);
    if(gKeyMasterV2Context != NULL) {
        pthread_mutex_unlock(&gKeyMasterV2Mutex);
        return -1;
    }
    gKeyMasterV2Context = (TEEC_Context *)malloc(sizeof(TEEC_Context));
    gKeyMasterV2Session = (TEEC_Session *)malloc(sizeof(TEEC_Session));
    assert(gKeyMasterV2Context != NULL);
    assert(gKeyMasterV2Session != NULL);
    pthread_mutex_unlock(&gKeyMasterV2Mutex);
    TEEC_Result success = TEEC_InitializeContext(NULL, gKeyMasterV2Context);
     if (success != TEEC_SUCCESS) {
        ALOGE("initialize context failed");
        return -1;
    }
    success = TEEC_OpenSession(gKeyMasterV2Context, gKeyMasterV2Session,
        (const TEEC_UUID *)&KeyMaster_v2_UUID[0], TEEC_LOGIN_PUBLIC, NULL, NULL, NULL);
    if(success != TEEC_SUCCESS) {
        ALOGE("open session failed");
        return -1;
    }

    TEEC_Operation operation;

    memset(&operation, 0, sizeof(TEEC_Operation));
    operation.paramTypes = TEEC_PARAM_TYPES(TEEC_NONE,
                                    TEEC_NONE, TEEC_NONE, TEEC_NONE);
    operation.started = 1;
    success = TEEC_InvokeCommand(gKeyMasterV2Session,
                        MSG_KEYMASTER_V2_INITIALIZE, &operation, NULL);

    return (int) success;
}

int KeyMaster_V2_Terminate(void)
{
    ALOGV("%s %d", __func__, __LINE__);
    pthread_mutex_lock(&gKeyMasterV2Mutex);
    if(gKeyMasterV2Context == NULL) {
        pthread_mutex_unlock(&gKeyMasterV2Mutex);
        return 0;
    }
    TEEC_Operation operation;

    memset(&operation, 0, sizeof(TEEC_Operation));
    operation.paramTypes = TEEC_PARAM_TYPES(TEEC_NONE,
                                    TEEC_NONE, TEEC_NONE, TEEC_NONE);
    operation.started = 1;
    TEEC_Result success  = TEEC_InvokeCommand(gKeyMasterV2Session,
        MSG_KEYMASTER_V2_TERMINATE, &operation, NULL);
    if (success != TEEC_SUCCESS) {
        ALOGE("call invoke command error");
    }
    TEEC_CloseSession(gKeyMasterV2Session);
    TEEC_FinalizeContext(gKeyMasterV2Context);

    free(gKeyMasterV2Session);
    free(gKeyMasterV2Context);
    gKeyMasterV2Session = NULL;
    gKeyMasterV2Context = NULL;
    pthread_mutex_unlock(&gKeyMasterV2Mutex);
    keymaster_v2_configured = 0;
    return (int) success;
}

keymaster_error_t
KeyMaster_V2_Configure(const keymaster_device_t* dev,
                       const keymaster_key_param_set_t* params){
    ALOGV("%s %d", __func__, __LINE__);
    if (!dev)
        return KM_ERROR_UNEXPECTED_NULL_POINTER;

    keymaster_error_t err_type = KM_ERROR_OK;
    uint32_t paramsize;
    paramsize = compute_param_set_length(params);

    TEEC_SharedMemory paramMem;
    paramMem.size = paramsize;
    paramMem.flags = TEEC_MEM_INPUT ;
    if(TEEC_AllocateSharedMemory(gKeyMasterV2Context, &paramMem) != TEEC_SUCCESS) {
        ALOGE("allocate share memory fail");
        return KM_ERROR_UNKNOWN_ERROR;
    }
    tee_copy_params2_buf((uint8_t*)paramMem.buffer, params);

    TEEC_Operation operation;
    memset(&operation, 0, sizeof(TEEC_Operation));
    operation.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_WHOLE, TEEC_NONE,
        TEEC_NONE, TEEC_NONE);
    operation.started = 1;

    operation.params[0].memref.parent = &paramMem;
    operation.params[0].memref.offset = 0;
    operation.params[0].memref.size = 0;

    int success = TEEC_InvokeCommand(gKeyMasterV2Session,
                    MSG_KEYMASTER_V2_CONFIGURE, &operation, NULL);

    if (!success){
        keymaster_v2_configured = 1;
    }

    if (success < KM_ERROR_UNKNOWN_ERROR){
        err_type = KM_ERROR_UNKNOWN_ERROR;
    }else {
        err_type = (keymaster_error_t) success;
    }

//__configure_err:
    TEEC_ReleaseSharedMemory(&paramMem);
    if(err_type != KM_ERROR_OK)
        ALOGE("%s failed with %d", __func__, err_type);

    return err_type;
}

keymaster_error_t
KeyMaster_V2_Add_Rng_Entropy(const keymaster_device_t* dev, const uint8_t* data,
                             size_t data_length){
    ALOGV("%s %d", __func__, __LINE__);
    if (!dev)
        return KM_ERROR_UNEXPECTED_NULL_POINTER;

    if (!keymaster_v2_configured)
        return KM_ERROR_KEYMASTER_NOT_CONFIGURED;

    keymaster_error_t err_type = KM_ERROR_OK;
    TEEC_SharedMemory data_p;
    data_p.size = data_length;
    data_p.flags = TEEC_MEM_INPUT;
    if(TEEC_AllocateSharedMemory(gKeyMasterV2Context, &data_p) != TEEC_SUCCESS) {
        ALOGE("%s: allocate output data share memory fail", __func__);
        return KM_ERROR_UNKNOWN_ERROR;
    }
    memcpy(data_p.buffer, data, data_length);

    TEEC_Operation operation;
    memset(&operation, 0, sizeof(TEEC_Operation));
    operation.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_WHOLE, TEEC_NONE,
            TEEC_NONE, TEEC_NONE);
    operation.started = 1;

    operation.params[0].memref.parent = &data_p;
    operation.params[0].memref.offset = 0;
    operation.params[0].memref.size = 0;

    int success = TEEC_InvokeCommand(gKeyMasterV2Session,
               MSG_KEYMASTER_V2_ADD_RNG_ENTROPY, &operation, NULL);

    if (success < KM_ERROR_UNKNOWN_ERROR){
        err_type = KM_ERROR_UNKNOWN_ERROR;
    }else {
        err_type = (keymaster_error_t) success;
    }

//__add_rng_entropy_err:
    TEEC_ReleaseSharedMemory(&data_p);
    if(err_type != KM_ERROR_OK)
        ALOGE("%s failed with %d", __func__, err_type);

    return err_type;
}

keymaster_error_t
KeyMaster_V2_Generate_Key(const keymaster_device_t* dev,
                          const keymaster_key_param_set_t* params,
                          keymaster_key_blob_t* key_blob,
                          keymaster_key_characteristics_t* characteristics){
    ALOGV("%s %d", __func__, __LINE__);
    if (!dev)
        return KM_ERROR_UNEXPECTED_NULL_POINTER;

    if (!key_blob)
        return KM_ERROR_OUTPUT_PARAMETER_NULL;

    if (!keymaster_v2_configured)
        return KM_ERROR_KEYMASTER_NOT_CONFIGURED;

    keymaster_error_t err_type = KM_ERROR_OK;
    uint32_t paramsize;
    paramsize = compute_param_set_length(params);

    TEEC_SharedMemory tee_params_p;
    tee_params_p.size = paramsize;
    tee_params_p.flags = TEEC_MEM_INPUT;
    if(TEEC_AllocateSharedMemory(gKeyMasterV2Context, &tee_params_p) != TEEC_SUCCESS) {
        ALOGE("%s: allocate params share memory fail", __func__);
        return KM_ERROR_UNKNOWN_ERROR;
    }
    //ALOGE("tee_params_p.buffer=%p", tee_params_p.buffer);
    tee_copy_params2_buf((uint8_t*)tee_params_p.buffer, params);

    //tee_hexdump((uint8_t*)tee_params_p.buffer, params_buf_size);

    TEEC_SharedMemory tee_material_p;
    tee_material_p.size = TEE_KEYMASTER_KEY_INOUT_COMMON_SIZE;
    tee_material_p.flags = TEEC_MEM_OUTPUT;
    if(TEEC_AllocateSharedMemory(gKeyMasterV2Context, &tee_material_p) != TEEC_SUCCESS) {
        ALOGE("%s: allocate key material share memory fail", __func__);
        TEEC_ReleaseSharedMemory(&tee_params_p);
        return KM_ERROR_UNKNOWN_ERROR;
    }

    TEEC_SharedMemory tee_characteristics_p;
    tee_characteristics_p.size = TEE_KEYMASTER_KEY_INOUT_COMMON_SIZE;
    tee_characteristics_p.flags = TEEC_MEM_OUTPUT;
    if(TEEC_AllocateSharedMemory(gKeyMasterV2Context, &tee_characteristics_p) != TEEC_SUCCESS) {
        ALOGE("%s: allocate characteristics share memory fail", __func__);
        TEEC_ReleaseSharedMemory(&tee_params_p);
        TEEC_ReleaseSharedMemory(&tee_material_p);
        return KM_ERROR_UNKNOWN_ERROR;
    }

    TEEC_Operation operation;
    memset(&operation, 0, sizeof(TEEC_Operation));
    operation.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_WHOLE, TEEC_MEMREF_WHOLE,
                                TEEC_MEMREF_WHOLE, TEEC_NONE);
    operation.started = 1;

    operation.params[0].memref.parent = &tee_params_p;
    operation.params[0].memref.offset = 0;
    operation.params[0].memref.size = 0;

    operation.params[1].memref.parent = &tee_material_p;
    operation.params[1].memref.offset = 0;
    operation.params[1].memref.size = 0;

    operation.params[2].memref.parent = &tee_characteristics_p;
    operation.params[2].memref.offset = 0;
    operation.params[2].memref.size = 0;

    int success = TEEC_InvokeCommand(gKeyMasterV2Session,
                    MSG_KEYMASTER_V2_GENERATE_KEY, &operation, NULL);

    if (success) {
        if (success < KM_ERROR_UNKNOWN_ERROR) {
            err_type = KM_ERROR_UNKNOWN_ERROR;
        } else {
            err_type = (keymaster_error_t)success;
        }
    }else{
        key_blob->key_material_size = operation.params[1].memref.size;
        uint8_t* tmp = reinterpret_cast<uint8_t*>(malloc(key_blob->key_material_size));
        if (!tmp){
            err_type = KM_ERROR_MEMORY_ALLOCATION_FAILED;
            goto __generate_key_err;
        }
        memcpy(tmp, tee_material_p.buffer, key_blob->key_material_size);
        key_blob->key_material = tmp;

        //ALOGV("key_blob", 0);
        //tee_hexdump((uint8_t*)key_blob->key_material, key_blob->key_material_size);
        if (characteristics) {
            tee_keymaster_key_characteristics_t *tee_enforced =
                    (tee_keymaster_key_characteristics_t *)tee_characteristics_p.buffer;

            tee_CopyToParamSet((uint8_t*)&(tee_enforced->hw_enforced),
                    &(characteristics->hw_enforced), (uint8_t*)tee_enforced);
            tee_CopyToParamSet((uint8_t*)&(tee_enforced->sw_enforced),
                    &(characteristics->sw_enforced), (uint8_t*)tee_enforced);

            //tee_ParamSet_view(&characteristics->hw_enforced);
            //tee_ParamSet_view(&characteristics->sw_enforced);

            err_type = KM_ERROR_OK;
        }
    }

__generate_key_err:
    TEEC_ReleaseSharedMemory(&tee_params_p);
    TEEC_ReleaseSharedMemory(&tee_material_p);
    TEEC_ReleaseSharedMemory(&tee_characteristics_p);
    if(err_type != KM_ERROR_OK)
        ALOGE("%s failed with %d", __func__, err_type);

    return err_type;
}

keymaster_error_t
KeyMaster_V2_Get_Key_Characteristics(const keymaster_device_t* dev,
                                     const keymaster_key_blob_t* key_blob,
                                     const keymaster_blob_t* client_id,
                                     const keymaster_blob_t* app_data,
                                     keymaster_key_characteristics_t* characteristics){
    ALOGV("%s %d", __func__, __LINE__);
    if (!dev)
        return KM_ERROR_UNEXPECTED_NULL_POINTER;

    if (!characteristics)
        return KM_ERROR_OUTPUT_PARAMETER_NULL;

    if (!keymaster_v2_configured)
        return KM_ERROR_KEYMASTER_NOT_CONFIGURED;

    int client_id_flag, app_data_flag;
    keymaster_error_t err_type = KM_ERROR_OK;

    TEEC_SharedMemory key_blob_p;
    key_blob_p.size = key_blob->key_material_size;
    key_blob_p.flags = TEEC_MEM_INPUT;
    if(TEEC_AllocateSharedMemory(gKeyMasterV2Context, &key_blob_p) != TEEC_SUCCESS) {
        ALOGE("%s: allocate key blob share memory fail", __func__);
        return KM_ERROR_UNKNOWN_ERROR;
    }
    memcpy(key_blob_p.buffer, key_blob->key_material, key_blob->key_material_size);

    TEEC_SharedMemory client_id_p;
    if (client_id) {
        client_id_p.size = client_id->data_length;
        client_id_p.flags = TEEC_MEM_INPUT;
        if(TEEC_AllocateSharedMemory(gKeyMasterV2Context, &client_id_p) != TEEC_SUCCESS) {
            ALOGE("%s: allocate client id blob share memory fail", __func__);
            TEEC_ReleaseSharedMemory(&key_blob_p);
            return KM_ERROR_UNKNOWN_ERROR;
        }
        memcpy(client_id_p.buffer, client_id->data, client_id->data_length);
        client_id_flag = TEEC_MEMREF_WHOLE;
    } else {
        client_id_p.buffer = NULL;
        client_id_flag = TEEC_VALUE_INPUT;
    }

    TEEC_SharedMemory app_data_p;
    if (app_data) {
        app_data_p.size = app_data->data_length;
        app_data_p.flags = TEEC_MEM_INPUT;
        if(TEEC_AllocateSharedMemory(gKeyMasterV2Context, &app_data_p) != TEEC_SUCCESS) {
            ALOGE("%s: allocate app data blob share memory fail", __func__);
            TEEC_ReleaseSharedMemory(&key_blob_p);
            if (client_id)
                TEEC_ReleaseSharedMemory(&client_id_p);

            return KM_ERROR_UNKNOWN_ERROR;
        }
        memcpy(app_data_p.buffer, app_data->data, app_data->data_length);
        app_data_flag = TEEC_MEMREF_WHOLE;
    } else {
        app_data_p.buffer = NULL;
        app_data_flag = TEEC_VALUE_INPUT;
    }

    TEEC_SharedMemory tee_characteristics_p;
    tee_characteristics_p.size = TEE_KEYMASTER_KEY_INOUT_COMMON_SIZE;
    tee_characteristics_p.flags = TEEC_MEM_OUTPUT;
    if(TEEC_AllocateSharedMemory(gKeyMasterV2Context, &tee_characteristics_p) != TEEC_SUCCESS) {
        ALOGE("%s: allocate characteristics share memory fail", __func__);
        TEEC_ReleaseSharedMemory(&key_blob_p);
        if (client_id)
            TEEC_ReleaseSharedMemory(&client_id_p);
        if (app_data)
            TEEC_ReleaseSharedMemory(&app_data_p);

        return KM_ERROR_UNKNOWN_ERROR;
    }

    TEEC_Operation operation;
    memset(&operation, 0, sizeof(TEEC_Operation));
    operation.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_WHOLE, client_id_flag,
            app_data_flag, TEEC_MEMREF_WHOLE);
    operation.started = 1;

    operation.params[0].memref.parent = &key_blob_p;
    operation.params[0].memref.offset = 0;
    operation.params[0].memref.size = 0;

    if (client_id) {
        operation.params[1].memref.parent = &client_id_p;
        operation.params[1].memref.offset = 0;
        operation.params[1].memref.size = 0;
        //ALOGV("client_id 1 is full", 0);
    } else {
        operation.params[1].value.a = 0;
        //ALOGV("client_id 1 is data", 0);
    }

    if (app_data) {
        operation.params[2].memref.parent = &app_data_p;
        operation.params[2].memref.offset = 0;
        operation.params[2].memref.size = 0;
        //ALOGV("app_data 2 is full", 0);
    } else {
        operation.params[2].value.a = 0;
        //ALOGV("app_data 2 is data", 0);
    }

    operation.params[3].memref.parent = &tee_characteristics_p;
    operation.params[3].memref.offset = 0;
    operation.params[3].memref.size = 0;

    int success = TEEC_InvokeCommand(gKeyMasterV2Session,
                    MSG_KEYMASTER_V2_GET_KEY_CHARAC, &operation, NULL);

    if (success) {
        if (success < KM_ERROR_UNKNOWN_ERROR) {
            err_type = KM_ERROR_UNKNOWN_ERROR;
        } else {
            err_type = (keymaster_error_t)success;
        }
    } else {
        //tee_hexdump((uint8_t*)tee_characteristics_p.buffer, 512);
        tee_keymaster_key_characteristics_t *tee_enforced =
                    (tee_keymaster_key_characteristics_t *)tee_characteristics_p.buffer;

        tee_CopyToParamSet((uint8_t*)&tee_enforced->hw_enforced,
                &characteristics->hw_enforced, (uint8_t*)tee_enforced);
        tee_CopyToParamSet((uint8_t*)&tee_enforced->sw_enforced,
                &characteristics->sw_enforced, (uint8_t*)tee_enforced);

        //tee_ParamSet_view(&characteristics->hw_enforced);
        //tee_ParamSet_view(&characteristics->sw_enforced);
    }

//__get_key_characteristics_err:
    TEEC_ReleaseSharedMemory(&key_blob_p);
    if (client_id)
        TEEC_ReleaseSharedMemory(&client_id_p);
    if (app_data)
        TEEC_ReleaseSharedMemory(&app_data_p);
    TEEC_ReleaseSharedMemory(&tee_characteristics_p);
    if(err_type != KM_ERROR_OK)
        ALOGE("%s failed with %d", __func__, err_type);

    return err_type;
}

keymaster_error_t
KeyMaster_V2_Import_Key(const keymaster_device_t* dev,
                        const keymaster_key_param_set_t* params,
                        keymaster_key_format_t key_format,
                        const keymaster_blob_t* key_data,
                        keymaster_key_blob_t* key_blob,
                        keymaster_key_characteristics_t* characteristics){
    ALOGV("%s %d", __func__, __LINE__);
    if (!dev || !params || !key_data)
        return KM_ERROR_UNEXPECTED_NULL_POINTER;

    if (!key_blob)
        return KM_ERROR_OUTPUT_PARAMETER_NULL;

    if (!keymaster_v2_configured)
        return KM_ERROR_KEYMASTER_NOT_CONFIGURED;

    //if (characteristics)
    //    *characteristics = nullptr;

    keymaster_error_t err_type = KM_ERROR_OK;
    uint32_t params_size;
    params_size = compute_param_set_length(params);

    uint32_t paramsize;
    paramsize = sizeof(TEE_params_expand) + sizeof(TEE_params_set) +sizeof(TEE_params_set) +
                sizeof(TEE_params_set) + params_size/*params*/+
                sizeof(keymaster_key_format_t)/*key_format*/ + key_data->data_length/*key_data */;

    TEEC_SharedMemory paramMem;
    paramMem.size = paramsize;
    paramMem.flags = TEEC_MEM_INPUT ;
    if(TEEC_AllocateSharedMemory(gKeyMasterV2Context, &paramMem) != TEEC_SUCCESS) {
        ALOGE("allocate share memory fail");
        return KM_ERROR_UNKNOWN_ERROR;
    }
    {
        /***************copy data*****************/
        uint8_t *buf = (uint8_t *)paramMem.buffer;
        memset(buf, 0, paramsize);

        TEE_params_expand *param_expand = (TEE_params_expand *)buf;
        param_expand->buff_count = 3;
        TEE_params_set *params_set = (TEE_params_set *)(buf + sizeof(TEE_params_expand));
        uint32_t offset = param_expand->buff_count * sizeof(TEE_params_set) + \
                          sizeof(TEE_params_expand);
        uint8_t *tmp_buf = buf + offset;
        uint32_t this_len;
        int i = 0;

        /*params*/
        this_len = params_size;
        params_set[i].buf  = offset;
        params_set[i].size = this_len;
        tee_copy_params2_buf(tmp_buf, params);
        offset += this_len;
        tmp_buf += this_len;
        i++;
        /*key_format*/
        this_len = sizeof(keymaster_key_format_t);
        params_set[i].buf  = offset;
        params_set[i].size = this_len;
        memcpy(tmp_buf, &key_format, this_len);
        offset += this_len;
        tmp_buf += this_len;
        i++;
        /*key_data*/
        this_len = key_data->data_length;
        params_set[i].buf  = offset;
        params_set[i].size = this_len;
        memcpy(tmp_buf, key_data->data, this_len);
    }
    TEEC_SharedMemory tee_material_p;
    tee_material_p.size = TEE_KEYMASTER_KEY_INOUT_COMMON_SIZE;
    tee_material_p.flags = TEEC_MEM_OUTPUT;
    if(TEEC_AllocateSharedMemory(gKeyMasterV2Context, &tee_material_p) != TEEC_SUCCESS) {
        ALOGE("%s: allocate key blob share memory fail", __func__);
        return KM_ERROR_UNKNOWN_ERROR;
    }

    TEEC_SharedMemory tee_characteristics_p;
    int  tee_chara_flag = TEEC_MEMREF_WHOLE;
    if (characteristics) {
        tee_characteristics_p.size = TEE_KEYMASTER_KEY_INOUT_COMMON_SIZE;
        tee_characteristics_p.flags = TEEC_MEM_OUTPUT;
        if(TEEC_AllocateSharedMemory(gKeyMasterV2Context, &tee_characteristics_p) != TEEC_SUCCESS) {
            ALOGE("%s: allocate characteristics share memory fail", __func__);
            TEEC_ReleaseSharedMemory(&tee_material_p);
            return KM_ERROR_UNKNOWN_ERROR;
        }
    } else {
        tee_chara_flag = TEEC_VALUE_INPUT;
    }

    TEEC_Operation operation;

    memset(&operation, 0, sizeof(TEEC_Operation));
    operation.started = 1;
    operation.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_WHOLE, TEEC_MEMREF_WHOLE,
            tee_chara_flag, TEEC_NONE);

    operation.params[0].memref.parent = &paramMem;
    operation.params[0].memref.offset = 0;
    operation.params[0].memref.size = 0;

    operation.params[1].memref.parent = &tee_material_p;
    operation.params[1].memref.offset = 0;
    operation.params[1].memref.size = 0;

    if (characteristics) {
        operation.params[2].memref.parent = &tee_characteristics_p;
        operation.params[2].memref.offset = 0;
        operation.params[2].memref.size = 0;
    } else {
        operation.params[2].value.a = 0;
    }

    int success = TEEC_InvokeCommand(gKeyMasterV2Session,
                    MSG_KEYMASTER_V2_IMPORT_KEY, &operation, NULL);

    if (success) {
        if (success < KM_ERROR_UNKNOWN_ERROR) {
            err_type = KM_ERROR_UNKNOWN_ERROR;
        } else {
            err_type = (keymaster_error_t)success;
        }
    } else {
        key_blob->key_material_size = operation.params[1].memref.size;
        key_blob->key_material = reinterpret_cast<uint8_t*>(malloc(key_blob->key_material_size));
        if (!key_blob->key_material) {
            err_type = KM_ERROR_MEMORY_ALLOCATION_FAILED;
            goto __import_key_err;
        }
        memcpy(const_cast<uint8_t*>(key_blob->key_material), tee_material_p.buffer,
               key_blob->key_material_size);
        //tee_hexdump((uint8_t*)tee_characteristics_p.buffer, 512);

        if (characteristics) {
            keymaster_key_characteristics_t *tee_enforced =
                        (keymaster_key_characteristics_t *)tee_characteristics_p.buffer;

            tee_CopyToParamSet((uint8_t*)&tee_enforced->hw_enforced,
                    &characteristics->hw_enforced, (uint8_t*)tee_enforced);
            tee_CopyToParamSet((uint8_t*)&tee_enforced->sw_enforced,
                    &characteristics->sw_enforced, (uint8_t*)tee_enforced);
            //tee_ParamSet_view(&characteristics->hw_enforced);
            //tee_ParamSet_view(&characteristics->sw_enforced);

            err_type = KM_ERROR_OK;
        }
    }

__import_key_err:
    TEEC_ReleaseSharedMemory(&paramMem);
    TEEC_ReleaseSharedMemory(&tee_material_p);
    if (characteristics)
        TEEC_ReleaseSharedMemory(&tee_characteristics_p);
    if(err_type != KM_ERROR_OK)
        ALOGE("%s failed with %d", __func__, err_type);

    return err_type;
}

keymaster_error_t
KeyMaster_V2_Export_Key(const keymaster_device_t* dev,
                        keymaster_key_format_t export_format,
                        const keymaster_key_blob_t* key_to_export,
                        const keymaster_blob_t* client_id,
                        const keymaster_blob_t* app_data,
                        keymaster_blob_t* export_data){
    ALOGV("%s %d", __func__, __LINE__);
    if (!dev || !key_to_export || !key_to_export->key_material)
        return KM_ERROR_UNEXPECTED_NULL_POINTER;

    if (!export_data)
        return KM_ERROR_OUTPUT_PARAMETER_NULL;

    if (!keymaster_v2_configured)
        return KM_ERROR_KEYMASTER_NOT_CONFIGURED;

    export_data->data = nullptr;
    export_data->data_length = 0;

    keymaster_error_t err_type = KM_ERROR_OK;
    uint32_t paramsize;
    paramsize = sizeof(TEE_params_set) + sizeof(keymaster_key_format_t)   /*export_format*/+
                sizeof(TEE_params_set) + key_to_export->key_material_size  /*key_to_export*/+
                sizeof(TEE_params_set) + /*client_id */
                sizeof(TEE_params_set) + /*app_data */
                sizeof(TEE_params_expand);

    if (client_id)
        paramsize += client_id->data_length;

    if (app_data)
        paramsize += app_data->data_length;

    TEEC_SharedMemory paramMem;
    paramMem.size = paramsize;
    paramMem.flags = TEEC_MEM_INPUT ;
    if(TEEC_AllocateSharedMemory(gKeyMasterV2Context, &paramMem) != TEEC_SUCCESS) {
        ALOGE("allocate share memory fail");
        return KM_ERROR_UNKNOWN_ERROR;
    }
    uint8_t *buf = (uint8_t *)paramMem.buffer;
    memset(buf, 0, paramsize);

    TEE_params_expand *param_expand = (TEE_params_expand *)buf;
    param_expand->buff_count = 4;
    TEE_params_set *params_set = (TEE_params_set *)(buf + sizeof(TEE_params_expand));
    size_t offset = param_expand->buff_count * sizeof(TEE_params_set) + \
                      sizeof(TEE_params_expand);
    uint8_t *tmp_buf = buf + offset;

    int i = 0;
    uint32_t this_len;

    /*export_format*/
    this_len = sizeof(keymaster_key_format_t);
    params_set[i].buf  = offset;
    params_set[i++].size = this_len;
    memcpy(tmp_buf, &export_format, this_len);
    offset += this_len;
    tmp_buf += this_len;

    //ALOGV("params_set[0].buf=%p", params_set[0].buf);
    //ALOGV("params_set[0].size=0x%x", params_set[0].size);
    //tee_hexdump(params_set[0].buf + (uint32_t)buf, params_set[0].size);
    /*key_to_export*/
    this_len = key_to_export->key_material_size;
    params_set[i].buf  = offset;
    params_set[i++].size = this_len;
    memcpy(tmp_buf, key_to_export->key_material, this_len);
    this_len = (this_len + 3) & (~3);
    offset += this_len;
    tmp_buf += this_len;

    //ALOGV("params_set[1].buf=%p", params_set[1].buf);
    //ALOGV("params_set[1].size=0x%x", params_set[1].size);
    //tee_hexdump(params_set[1].buf + (uint32_t)buf, params_set[1].size);
    /*client_id*/
    if (client_id) {
        this_len = client_id->data_length;
        params_set[i].buf  = offset;
        params_set[i].size = this_len;
        memcpy(tmp_buf, client_id->data, this_len);
        this_len = (this_len + 3) & (~3);
        offset += this_len;
        tmp_buf += this_len;
    } else {
        params_set[i].buf  = 0;
        params_set[i].size = 0;
    }
    i++;
    /*app_data*/
    if (app_data) {
        this_len = app_data->data_length;
        params_set[i].buf  = offset;
        params_set[i].size = this_len;
        memcpy(tmp_buf, app_data->data, this_len);
        this_len = (this_len + 3) & (~3);
        offset += this_len;
        tmp_buf += this_len;
    } else {
        params_set[i].buf  = 0;
        params_set[i].size = 0;
    }

    TEEC_SharedMemory export_data_p;
    export_data_p.size = TEE_KEYMASTER_KEY_INOUT_COMMON_SIZE;
    export_data_p.flags = TEEC_MEM_OUTPUT;
    if(TEEC_AllocateSharedMemory(gKeyMasterV2Context, &export_data_p) != TEEC_SUCCESS) {
        ALOGE("%s: allocate export_data params share memory fail", __func__);
        TEEC_ReleaseSharedMemory(&paramMem);
        return KM_ERROR_UNKNOWN_ERROR;
    }

    TEEC_Operation operation;
    memset(&operation, 0, sizeof(TEEC_Operation));
    operation.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_WHOLE, TEEC_MEMREF_WHOLE,
            TEEC_NONE, TEEC_NONE);
    operation.started = 1;

    operation.params[0].memref.parent = &paramMem;
    operation.params[0].memref.offset = 0;
    operation.params[0].memref.size = 0;

    operation.params[1].memref.parent = &export_data_p;
    operation.params[1].memref.offset = 0;
    operation.params[1].memref.size = 0;

    int success = TEEC_InvokeCommand(gKeyMasterV2Session,
                    MSG_KEYMASTER_V2_EXPORT_KEY, &operation, NULL);

    if (success) {
        if (success < KM_ERROR_UNKNOWN_ERROR) {
            err_type = KM_ERROR_UNKNOWN_ERROR;
        } else {
            err_type = (keymaster_error_t)success;
        }
    } else {
        export_data->data_length = operation.params[1].memref.size;
        uint8_t* tmp = reinterpret_cast<uint8_t*>(malloc(export_data->data_length));
        if (!tmp) {
            err_type = KM_ERROR_MEMORY_ALLOCATION_FAILED;
        } else {
            memcpy(tmp, export_data_p.buffer, export_data->data_length);
            export_data->data = tmp;
        }
    }

//__export_key_err:
    TEEC_ReleaseSharedMemory(&paramMem);
    TEEC_ReleaseSharedMemory(&export_data_p);
    if(err_type != KM_ERROR_OK)
        ALOGE("%s failed with %d", __func__, err_type);

    return err_type;
}

keymaster_error_t
KeyMaster_V2_Attest_Key(const keymaster_device_t* dev,
                        const keymaster_key_blob_t* key_to_attest,
                        const keymaster_key_param_set_t* attest_params,
                        keymaster_cert_chain_t* cert_chain){
    ALOGV("%s %d", __func__, __LINE__);
    if (!dev || !key_to_attest || !attest_params || !cert_chain)
        return KM_ERROR_UNEXPECTED_NULL_POINTER;

    if (!keymaster_v2_configured)
        return KM_ERROR_KEYMASTER_NOT_CONFIGURED;

    keymaster_error_t err_type = KM_ERROR_OK;
    TEEC_SharedMemory key_to_attest_p;
    key_to_attest_p.size = key_to_attest->key_material_size;
    key_to_attest_p.flags = TEEC_MEM_INPUT;
    if(TEEC_AllocateSharedMemory(gKeyMasterV2Context, &key_to_attest_p) != TEEC_SUCCESS) {
        ALOGE("%s: allocate key to attest blob share memory fail", __func__);
        return KM_ERROR_UNKNOWN_ERROR;
    }
    memcpy(key_to_attest_p.buffer, key_to_attest->key_material, key_to_attest->key_material_size);
    //ALOGV("key_to_attest: %d", key_to_attest->key_material_size);
    //tee_hexdump((uint8_t*)key_to_attest_p.buffer, key_to_attest->key_material_size);

    //tee_ParamSet_view(attest_params);

    uint32_t attest_params_size;
    attest_params_size = compute_param_set_length(attest_params);

    TEEC_SharedMemory tee_key_params_p;
    tee_key_params_p.size = attest_params_size;
    tee_key_params_p.flags = TEEC_MEM_INPUT;
    if(TEEC_AllocateSharedMemory(gKeyMasterV2Context, &tee_key_params_p) != TEEC_SUCCESS) {
        ALOGE("%s: allocate key params share memory fail", __func__);
        TEEC_ReleaseSharedMemory(&key_to_attest_p);
        return KM_ERROR_UNKNOWN_ERROR;
    }
    tee_copy_params2_buf((uint8_t*)tee_key_params_p.buffer, attest_params);

    TEEC_SharedMemory tee_cert_chain_p;
    tee_cert_chain_p.size = TEE_KEYMASTER_KEY_INOUT_COMMON_SIZE;
    tee_cert_chain_p.flags = TEEC_MEM_OUTPUT;
    if(TEEC_AllocateSharedMemory(gKeyMasterV2Context, &tee_cert_chain_p) != TEEC_SUCCESS) {
        ALOGE("%s: allocate cert chain share memory fail", __func__);
        TEEC_ReleaseSharedMemory(&key_to_attest_p);
        TEEC_ReleaseSharedMemory(&tee_key_params_p);
        return KM_ERROR_UNKNOWN_ERROR;
    }

    TEEC_Operation operation;
    memset(&operation, 0, sizeof(TEEC_Operation));
    operation.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_WHOLE, TEEC_MEMREF_WHOLE,
            TEEC_MEMREF_WHOLE, TEEC_NONE);
    operation.started = 1;

    operation.params[0].memref.parent = &key_to_attest_p;
    operation.params[0].memref.offset = 0;
    operation.params[0].memref.size = 0;

    operation.params[1].memref.parent = &tee_key_params_p;
    operation.params[1].memref.offset = 0;
    operation.params[1].memref.size = 0;

    operation.params[2].memref.parent = &tee_cert_chain_p;
    operation.params[2].memref.offset = 0;
    operation.params[2].memref.size = 0;

    int success = TEEC_InvokeCommand(gKeyMasterV2Session,
                    MSG_KEYMASTER_V2_ATTEST_KEY, &operation, NULL);
    if (success) {
        if (success < KM_ERROR_UNKNOWN_ERROR) {
            err_type = KM_ERROR_UNKNOWN_ERROR;
        } else {
            err_type = (keymaster_error_t)success;
        }
    } else {
        // Allocate and clear storage for cert_chain.
        tee_keymaster_cert_chain_t *rsp_chain = (tee_keymaster_cert_chain_t *)tee_cert_chain_p.buffer;

        //tee_hexdump((uint8_t*)tee_cert_chain_p.buffer, 64);

        cert_chain->entries = reinterpret_cast<keymaster_blob_t*>(
            malloc(rsp_chain->entry_count * sizeof(*cert_chain->entries)));
        if (!cert_chain->entries) {
            err_type = KM_ERROR_MEMORY_ALLOCATION_FAILED;
        } else {
            cert_chain->entry_count = rsp_chain->entry_count;

            for (keymaster_blob_t& entry : keymaster::array_range(cert_chain->entries, cert_chain->entry_count))
                entry = {};

            // Copy cert_chain contents
            size_t i = 0;
            uint8_t* buffer = (uint8_t*)tee_cert_chain_p.buffer;

            tee_keymaster_blob_t *entry =
                    (tee_keymaster_blob_t *)(buffer + sizeof(tee_keymaster_cert_chain_t));

            for (i=0;i<rsp_chain->entry_count;i++) {
                cert_chain->entries[i].data =
                            reinterpret_cast<uint8_t*>(malloc(entry[i].data_length));

                if (!cert_chain->entries[i].data) {
                    keymaster_free_cert_chain(cert_chain);
                    err_type = KM_ERROR_MEMORY_ALLOCATION_FAILED;
                    goto __attest_key_err;
                }
                cert_chain->entries[i].data_length = entry[i].data_length;
                memcpy(const_cast<uint8_t*>(cert_chain->entries[i].data),
                       entry[i].data_offset + buffer, entry[i].data_length);
                //tee_hexdump((uint8_t *)(entry[i].data_offset + buffer), entry[i].data_length);
            }
        }
    }

__attest_key_err:
    TEEC_ReleaseSharedMemory(&key_to_attest_p);
    TEEC_ReleaseSharedMemory(&tee_key_params_p);
    TEEC_ReleaseSharedMemory(&tee_cert_chain_p);
    if(err_type != KM_ERROR_OK)
        ALOGE("%s failed with %d", __func__, err_type);

    return err_type;
}

keymaster_error_t
KeyMaster_V2_Upgrade_Key(const keymaster_device_t* dev,
                         const keymaster_key_blob_t* key_to_upgrade,
                         const keymaster_key_param_set_t* upgrade_params,
                         keymaster_key_blob_t* upgraded_key){
    ALOGV("%s %d", __func__, __LINE__);
    if (!dev || !key_to_upgrade || !upgrade_params)
        return KM_ERROR_UNEXPECTED_NULL_POINTER;

    if (!upgraded_key)
        return KM_ERROR_OUTPUT_PARAMETER_NULL;

    if (!keymaster_v2_configured)
        return KM_ERROR_KEYMASTER_NOT_CONFIGURED;

    keymaster_error_t err_type = KM_ERROR_OK;
    TEEC_SharedMemory key_to_upgrade_p;
    key_to_upgrade_p.size = key_to_upgrade->key_material_size;
    key_to_upgrade_p.flags = TEEC_MEM_INPUT;
    if(TEEC_AllocateSharedMemory(gKeyMasterV2Context, &key_to_upgrade_p) != TEEC_SUCCESS) {
        ALOGE("%s: allocate key to upgrade key share memory fail", __func__);
        return KM_ERROR_UNKNOWN_ERROR;
    }
    memcpy(key_to_upgrade_p.buffer, key_to_upgrade->key_material,key_to_upgrade->key_material_size);
    //tee_hexdump((uint8_t*)key_to_upgrade_p.buffer, key_to_upgrade->key_material_size);

    uint32_t upgrade_params_size;
    upgrade_params_size = compute_param_set_length(upgrade_params);

    TEEC_SharedMemory tee_key_params_p;
    tee_key_params_p.size = upgrade_params_size;
    tee_key_params_p.flags = TEEC_MEM_INPUT;
    if(TEEC_AllocateSharedMemory(gKeyMasterV2Context, &tee_key_params_p) != TEEC_SUCCESS) {
        ALOGE("%s: allocate key params share memory fail", __func__);
        TEEC_ReleaseSharedMemory(&key_to_upgrade_p);
        return KM_ERROR_UNKNOWN_ERROR;
    }
    tee_copy_params2_buf((uint8_t*)tee_key_params_p.buffer, upgrade_params);

    TEEC_SharedMemory upgraded_key_p;
    upgraded_key_p.size = TEE_KEYMASTER_KEY_INOUT_COMMON_SIZE;
    upgraded_key_p.flags = TEEC_MEM_OUTPUT;
    if(TEEC_AllocateSharedMemory(gKeyMasterV2Context, &upgraded_key_p) != TEEC_SUCCESS) {
        ALOGE("%s: allocate cert chain share memory fail", __func__);
        TEEC_ReleaseSharedMemory(&key_to_upgrade_p);
        TEEC_ReleaseSharedMemory(&tee_key_params_p);
        return KM_ERROR_UNKNOWN_ERROR;
    }

    TEEC_Operation operation;
    memset(&operation, 0, sizeof(TEEC_Operation));
    operation.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_WHOLE, TEEC_MEMREF_WHOLE,
            TEEC_MEMREF_WHOLE, TEEC_NONE);
    operation.started = 1;

    operation.params[0].memref.parent = &key_to_upgrade_p;
    operation.params[0].memref.offset = 0;
    operation.params[0].memref.size = 0;

    operation.params[1].memref.parent = &tee_key_params_p;
    operation.params[1].memref.offset = 0;
    operation.params[1].memref.size = 0;

    operation.params[2].memref.parent = &upgraded_key_p;
    operation.params[2].memref.offset = 0;
    operation.params[2].memref.size = 0;

    int success = TEEC_InvokeCommand(gKeyMasterV2Session,
                    MSG_KEYMASTER_V2_UPGRAGE_KEY, &operation, NULL);

    if (success) {
        if (success < KM_ERROR_UNKNOWN_ERROR) {
            err_type = KM_ERROR_UNKNOWN_ERROR;
        } else {
            err_type = (keymaster_error_t)success;
        }
    } else {
        upgraded_key->key_material_size = operation.params[2].memref.size;
        uint8_t* tmp = reinterpret_cast<uint8_t*>(malloc(upgraded_key->key_material_size));
        if (!tmp) {
            err_type = KM_ERROR_MEMORY_ALLOCATION_FAILED;
            goto __upgrade_key_err;
        }
        memcpy(tmp, upgraded_key_p.buffer, upgraded_key->key_material_size);
        upgraded_key->key_material = tmp;
        //tee_hexdump(tmp, upgraded_key->key_material_size);
    }

__upgrade_key_err:
    TEEC_ReleaseSharedMemory(&key_to_upgrade_p);
    TEEC_ReleaseSharedMemory(&tee_key_params_p);
    TEEC_ReleaseSharedMemory(&upgraded_key_p);
    if(err_type != KM_ERROR_OK)
        ALOGE("%s failed with %d", __func__, err_type);

    return err_type;
}

keymaster_error_t
KeyMaster_V2_Delete_Key(const keymaster_device_t* dev,
                        const keymaster_key_blob_t* key){
    ALOGV("%s %d", __func__, __LINE__);
    if (!dev || !key || !key->key_material)
        return KM_ERROR_UNEXPECTED_NULL_POINTER;

    keymaster_error_t err_type = KM_ERROR_OK;
    TEEC_SharedMemory key_p;
    key_p.size = key->key_material_size;
    key_p.flags = TEEC_MEM_INPUT;
    if(TEEC_AllocateSharedMemory(gKeyMasterV2Context, &key_p) != TEEC_SUCCESS) {
        ALOGE("%s: allocate delete key share memory fail", __func__);
        return KM_ERROR_UNKNOWN_ERROR;
    }
    memcpy(key_p.buffer, key->key_material, key->key_material_size);

    TEEC_Operation operation;
    memset(&operation, 0, sizeof(TEEC_Operation));
    operation.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_WHOLE, TEEC_NONE,
            TEEC_NONE, TEEC_NONE);
    operation.started = 1;

    operation.params[0].memref.parent = &key_p;
    operation.params[0].memref.offset = 0;
    operation.params[0].memref.size = 0;

    int success = TEEC_InvokeCommand(gKeyMasterV2Session,
                    MSG_KEYMASTER_V2_DELETE_KEY, &operation, NULL);

    if (success < KM_ERROR_UNKNOWN_ERROR) {
        err_type = KM_ERROR_UNKNOWN_ERROR;
    } else {
        err_type = (keymaster_error_t)success;
    }

//__delete_Key_err:
    TEEC_ReleaseSharedMemory(&key_p);
    if(err_type != KM_ERROR_OK)
        ALOGE("%s failed with %d", __func__, err_type);

    return err_type;
}

keymaster_error_t
KeyMaster_V2_Delete_All_Keys(const keymaster_device_t* dev){
    ALOGV("%s %d", __func__, __LINE__);
    if (!dev)
        return KM_ERROR_UNEXPECTED_NULL_POINTER;

    if (!keymaster_v2_configured)
        return KM_ERROR_KEYMASTER_NOT_CONFIGURED;

    keymaster_error_t err_type = KM_ERROR_OK;
    TEEC_Operation operation;
    memset(&operation, 0, sizeof(TEEC_Operation));
    operation.paramTypes = TEEC_PARAM_TYPES(TEEC_NONE, TEEC_NONE,
            TEEC_NONE, TEEC_NONE);
    operation.started = 1;

    int success = TEEC_InvokeCommand(gKeyMasterV2Session,
                    MSG_KEYMASTER_V2_DELETE_ALL_KEYS, &operation, NULL);

    if (success < KM_ERROR_UNKNOWN_ERROR) {
        err_type = KM_ERROR_UNKNOWN_ERROR;
    } else {
        err_type = (keymaster_error_t)success;
    }

//__delete_all_keys_err:
    if(err_type != KM_ERROR_OK)
        ALOGE("%s failed with %d", __func__, err_type);

    return err_type;
}

keymaster_error_t
KeyMaster_V2_Begin(const keymaster_device_t* dev, keymaster_purpose_t purpose,
                   const keymaster_key_blob_t* key,
                   const keymaster_key_param_set_t* in_params,
                   keymaster_key_param_set_t* out_params,
                   keymaster_operation_handle_t* operation_handle){
    ALOGV("%s %d", __func__, __LINE__);
    if (!dev || !key || !key->key_material)
        return KM_ERROR_UNEXPECTED_NULL_POINTER;

    if (!operation_handle)
        return KM_ERROR_OUTPUT_PARAMETER_NULL;

    if (!keymaster_v2_configured)
        return KM_ERROR_KEYMASTER_NOT_CONFIGURED;

    if (out_params) {
        out_params->params = nullptr;
        out_params->length = 0;
    }

    //ALOGV("key length=%d\n", key->key_material_size);
    keymaster_error_t err_type = KM_ERROR_OK;
    uint32_t in_params_size;
    in_params_size = compute_param_set_length(in_params);

    uint32_t paramsize;
    paramsize = sizeof(TEE_params_expand) +
                sizeof(TEE_params_set) + sizeof(keymaster_purpose_t)   /*purpose*/+
                sizeof(TEE_params_set) + key->key_material_size  /*key*/+
                sizeof(TEE_params_set) + in_params_size;/*in_params */

    TEEC_SharedMemory paramMem;
    paramMem.size = paramsize;
    paramMem.flags = TEEC_MEM_INPUT ;
    if(TEEC_AllocateSharedMemory(gKeyMasterV2Context, &paramMem) != TEEC_SUCCESS) {
        ALOGE("allocate share memory fail");
        return KM_ERROR_UNKNOWN_ERROR;
    }
    uint8_t *buf = (uint8_t *)paramMem.buffer;
    memset(buf, 0, paramsize);

    //ALOGV("buf=%p", buf);
    TEE_params_expand *param_expand = (TEE_params_expand *)buf;
    param_expand->buff_count = 3;
    TEE_params_set *params_set = (TEE_params_set *)(buf + sizeof(TEE_params_expand));
    size_t offset = param_expand->buff_count * sizeof(TEE_params_set) + \
                      sizeof(TEE_params_expand);
    uint8_t *tmp_buf = buf + offset;
    uint32_t this_len;
    int i = 0;

    /*purpose*/
    this_len = sizeof(keymaster_purpose_t);
    params_set[i].buf  = offset;
    params_set[i++].size = this_len;
    memcpy(tmp_buf, &purpose, this_len);
    offset += this_len;
    tmp_buf += this_len;

    //ALOGV("params_set[0].buf=%x", params_set[0].buf);
    //ALOGV("params_set[0].size=0x%x", params_set[0].size);
    //tee_hexdump(buf + params_set[0].buf, params_set[0].size);
    /*key*/
    this_len = key->key_material_size;
    params_set[i].buf  = offset;
    params_set[i++].size = this_len;
    memcpy(tmp_buf, key->key_material, this_len);
    this_len = (this_len + 3) & (~3);
    offset += this_len;
    tmp_buf += this_len;

    //ALOGV("params_set[1].buf=%x", params_set[1].buf);
    //ALOGV("params_set[1].size=0x%x", params_set[1].size);
    //tee_hexdump(buf + params_set[1].buf, params_set[1].size);
    /*params*/
    params_set[i].buf  = offset;
    params_set[i].size = in_params_size;

    //tee_ParamSet_view(in_params);
    //ALOGV("params buf=%p\n", buf + params_set[2].buf);
    //ALOGV("params_set[2].buf=0x%x", params_set[2].buf);

    tee_copy_params2_buf(buf + params_set[i].buf, in_params);

    TEEC_SharedMemory out_params_p;
    out_params_p.size = TEE_KEYMASTER_KEY_INOUT_COMMON_SIZE;
    out_params_p.flags = TEEC_MEM_OUTPUT;
    if(TEEC_AllocateSharedMemory(gKeyMasterV2Context, &out_params_p) != TEEC_SUCCESS) {
        ALOGE("%s: allocate output params share memory fail", __func__);
        TEEC_ReleaseSharedMemory(&paramMem);
        return KM_ERROR_UNKNOWN_ERROR;
    }

    TEEC_Operation operation;
    memset(&operation, 0, sizeof(TEEC_Operation));
    operation.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_WHOLE, TEEC_MEMREF_WHOLE,
            TEEC_VALUE_OUTPUT, TEEC_NONE);
    operation.started = 1;

    operation.params[0].memref.parent = &paramMem;
    operation.params[0].memref.offset = 0;
    operation.params[0].memref.size = 0;

    operation.params[1].memref.parent = &out_params_p;
    operation.params[1].memref.offset = 0;
    operation.params[1].memref.size = 0;

    int success = TEEC_InvokeCommand(gKeyMasterV2Session,
                    MSG_KEYMASTER_V2_BEGIN, &operation, NULL);

    if (success) {
        if (success < KM_ERROR_UNKNOWN_ERROR) {
            err_type = KM_ERROR_UNKNOWN_ERROR;
        } else {
            err_type = (keymaster_error_t)success;
        }
    } else {
        tee_keymaster_key_param_set_t *set = (tee_keymaster_key_param_set_t *)out_params_p.buffer;
        //fix tee_output_params
        if (set->length) {
            if (out_params) {
                tee_CopyToParamSet((uint8_t*)out_params_p.buffer,
                            out_params, (uint8_t*)out_params_p.buffer);
            } else {
                err_type = KM_ERROR_OUTPUT_PARAMETER_NULL;
                goto __begin_err;
            }
        }

        *operation_handle  = operation.params[2].value.b;
        *operation_handle <<= 32;
        *operation_handle |= operation.params[2].value.a;
    }

__begin_err:
    TEEC_ReleaseSharedMemory(&paramMem);
    TEEC_ReleaseSharedMemory(&out_params_p);
    if(err_type != KM_ERROR_OK)
        ALOGE("%s failed with %d", __func__, err_type);

    return err_type;
}

keymaster_error_t
KeyMaster_V2_Update(const keymaster_device_t* dev,
                    keymaster_operation_handle_t operation_handle,
                    const keymaster_key_param_set_t* in_params,
                    const keymaster_blob_t* input, size_t* input_consumed,
                    keymaster_key_param_set_t* out_params, keymaster_blob_t* output){
    ALOGV("%s %d", __func__, __LINE__);
    if (!dev || !input)
        return KM_ERROR_UNEXPECTED_NULL_POINTER;

    if (!input_consumed)
        return KM_ERROR_OUTPUT_PARAMETER_NULL;

    if (!keymaster_v2_configured)
        return KM_ERROR_KEYMASTER_NOT_CONFIGURED;

    if (out_params) {
        out_params->params = nullptr;
        out_params->length = 0;
    }
    if (output) {
        output->data = nullptr;
        output->data_length = 0;
    }

    keymaster_error_t err_type = KM_ERROR_OK;
    uint32_t in_params_size;
    if(in_params) {
        in_params_size = compute_param_set_length(in_params);
    } else {
        in_params_size = 0;
    }

    uint32_t paramsize;
    paramsize = sizeof(TEE_params_expand) +
                sizeof(TEE_params_set) + sizeof(keymaster_operation_handle_t) /*operation_handle*/+
                sizeof(TEE_params_set) + in_params_size /*in_params */+
                sizeof(TEE_params_set) + input->data_length; /*input*/

    TEEC_SharedMemory paramMem;
    paramMem.size = paramsize;
    paramMem.flags = TEEC_MEM_INPUT ;
    if(TEEC_AllocateSharedMemory(gKeyMasterV2Context, &paramMem) != TEEC_SUCCESS) {
        ALOGE("allocate share memory fail");
        return KM_ERROR_UNKNOWN_ERROR;
    }
    uint8_t *buf = (uint8_t *)paramMem.buffer;
    memset(buf, 0, paramsize);

    //ALOGV("buf=%p", buf);
    TEE_params_expand *param_expand = (TEE_params_expand *)buf;
    param_expand->buff_count = 3;
    TEE_params_set *params_set = (TEE_params_set *)(buf + sizeof(TEE_params_expand));
    size_t offset = param_expand->buff_count * sizeof(TEE_params_set) + \
                      sizeof(TEE_params_expand);
    uint8_t *tmp_buf = buf + offset;

    int i = 0;
    uint32_t this_len;
    /*operation_handle*/
    this_len = sizeof(keymaster_operation_handle_t);
    params_set[i].buf  = offset;
    params_set[i++].size = this_len;
    memcpy(tmp_buf, &operation_handle, this_len);
    offset += this_len;
    tmp_buf += this_len;

    /*in_params*/
    if (in_params) {
        this_len = in_params_size;
        params_set[i].buf    = offset;
        params_set[i].size = this_len;

        tee_copy_params2_buf(buf + params_set[i].buf, in_params);

        //tee_ParamSet_view(in_params);
        //ALOGV("params buf=%p\n", buf + params_set[i].buf);
        //ALOGV("params_set[1].buf=%x", params_set[i].buf);
    } else {
        this_len = 0;
        params_set[i].buf = 0;
        params_set[i].size = 0;
    }
    offset += this_len;
    tmp_buf += this_len;
    i++;
    /*input*/
    this_len = input->data_length;
    params_set[i].buf  = offset;
    params_set[i].size = this_len;
    memcpy(tmp_buf, input->data, this_len);
    //tee_hexdump((uint8_t *)input->data, input->data_length);

    //ALOGV("params_set[2].buf=%p", params_set[2].buf);
    //ALOGV("params_set[2].size=0x%x", params_set[2].size);
    //tee_hexdump(params_set[2].buf + (uint32_t)buf, params_set[2].size);

    TEEC_SharedMemory out_params_p;
    out_params_p.size = TEE_KEYMASTER_KEY_INOUT_COMMON_SIZE;
    out_params_p.flags = TEEC_MEM_OUTPUT;
    if(TEEC_AllocateSharedMemory(gKeyMasterV2Context, &out_params_p) != TEEC_SUCCESS) {
        ALOGE("%s: allocate output params share memory fail", __func__);
        TEEC_ReleaseSharedMemory(&paramMem);
        return KM_ERROR_UNKNOWN_ERROR;
    }

    TEEC_SharedMemory tee_output_p;
    tee_output_p.size = TEE_KEYMASTER_KEY_INOUT_COMMON_SIZE;
    tee_output_p.flags = TEEC_MEM_OUTPUT;
    if(TEEC_AllocateSharedMemory(gKeyMasterV2Context, &tee_output_p) != TEEC_SUCCESS) {
        ALOGE("%s: allocate output share memory fail", __func__);
        TEEC_ReleaseSharedMemory(&paramMem);
        TEEC_ReleaseSharedMemory(&out_params_p);
        return KM_ERROR_UNKNOWN_ERROR;
    }

    TEEC_Operation operation;
    memset(&operation, 0, sizeof(TEEC_Operation));
    operation.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_WHOLE, TEEC_MEMREF_WHOLE,
            TEEC_MEMREF_WHOLE, TEEC_VALUE_OUTPUT);
    operation.started = 1;

    operation.params[0].memref.parent = &paramMem;
    operation.params[0].memref.offset = 0;
    operation.params[0].memref.size = 0;

    operation.params[1].memref.parent = &out_params_p;
    operation.params[1].memref.offset = 0;
    operation.params[1].memref.size = 0;

    operation.params[2].memref.parent = &tee_output_p;
    operation.params[2].memref.offset = 0;
    operation.params[2].memref.size = 0;

    int success = TEEC_InvokeCommand(gKeyMasterV2Session,
                    MSG_KEYMASTER_V2_UPDATE, &operation, NULL);

    if (success) {
        if (success < KM_ERROR_UNKNOWN_ERROR) {
            err_type = KM_ERROR_UNKNOWN_ERROR;
        } else {
            err_type = (keymaster_error_t)success;
        }
    } else {
        //fix tee_output_params
        tee_CopyToParamSet((uint8_t*)out_params_p.buffer,
                    out_params, (uint8_t*)out_params_p.buffer);

        *input_consumed = operation.params[3].value.a;
        if (output) {
            output->data_length = operation.params[2].memref.size;
            uint8_t* tmp = reinterpret_cast<uint8_t*>(malloc(output->data_length));
            if (!tmp)
                err_type = KM_ERROR_MEMORY_ALLOCATION_FAILED;
            else {
                memcpy(tmp, tee_output_p.buffer, output->data_length);
                output->data = tmp;
            }
        } else if (operation.params[2].memref.size > 0)
            err_type = KM_ERROR_OUTPUT_PARAMETER_NULL;
    }

//__update_err:
    TEEC_ReleaseSharedMemory(&paramMem);
    TEEC_ReleaseSharedMemory(&out_params_p);
    TEEC_ReleaseSharedMemory(&tee_output_p);
    if(err_type != KM_ERROR_OK)
        ALOGE("%s failed with %d", __func__, err_type);

    return err_type;
}

keymaster_error_t
KeyMaster_V2_Finish(const keymaster_device_t* dev,
                    keymaster_operation_handle_t operation_handle,
                    const keymaster_key_param_set_t* in_params,
                    const keymaster_blob_t* input, const keymaster_blob_t* signature,
                    keymaster_key_param_set_t* out_params, keymaster_blob_t* output){
    ALOGV("%s %d", __func__, __LINE__);
    if (!dev)
        return KM_ERROR_UNEXPECTED_NULL_POINTER;

    if (!keymaster_v2_configured)
        return KM_ERROR_KEYMASTER_NOT_CONFIGURED;

    if (out_params) {
        out_params->params = nullptr;
        out_params->length = 0;
    }

    if (output) {
        output->data = nullptr;
        output->data_length = 0;
    }

    uint32_t in_params_size;
    in_params_size = compute_param_set_length(in_params);

    uint32_t paramsize;
    paramsize = sizeof(TEE_params_set) + sizeof(keymaster_operation_handle_t)/*operation_handle*/+
                sizeof(TEE_params_set) + in_params_size /*in_params*/+
                sizeof(TEE_params_set) +/*rest for input*/
                sizeof(TEE_params_set) /*rest for signature*/;

    if (input)
        paramsize += input->data_length; /*input*/

    if (signature)
        paramsize += signature->data_length; /*signature*/

    TEEC_SharedMemory paramMem;
    paramMem.size = paramsize;
    paramMem.flags = TEEC_MEM_INPUT ;
    if(TEEC_AllocateSharedMemory(gKeyMasterV2Context, &paramMem) != TEEC_SUCCESS) {
        ALOGE("allocate share memory fail");
        return KM_ERROR_UNKNOWN_ERROR;
    }
    uint8_t *buf = (uint8_t *)paramMem.buffer;
    memset(buf, 0, paramsize);

    TEE_params_expand *param_expand = (TEE_params_expand *)buf;
    param_expand->buff_count = 4;
    TEE_params_set *params_set = (TEE_params_set *)(buf + sizeof(TEE_params_expand));
    size_t offset = param_expand->buff_count * sizeof(TEE_params_set) + \
                      sizeof(TEE_params_expand);
    uint8_t *tmp_buf = buf + offset;

    int i = 0;
    /*operation_handle*/
    params_set[i].buf  = offset;
    params_set[i++].size = sizeof(keymaster_operation_handle_t);
    memcpy(tmp_buf, &operation_handle, sizeof(keymaster_operation_handle_t));
    offset += sizeof(keymaster_operation_handle_t);
    tmp_buf += sizeof(keymaster_operation_handle_t);

    /*in_params*/
    params_set[i].buf  = offset;
    params_set[i].size = in_params_size;
    tee_copy_params2_buf(buf + params_set[i].buf, in_params);
    offset += in_params_size;
    tmp_buf += in_params_size;

    /*input*/
    if (input) {
        params_set[++i].buf  = offset;
        params_set[i].size = input->data_length;
        memcpy(tmp_buf, input->data, input->data_length);
        offset += input->data_length;
        tmp_buf += input->data_length;
    } else {
        params_set[++i].buf  = 0;
        params_set[i].size = 0;
    }

    /*signature*/
    if (signature) {
        params_set[++i].buf  = offset;
        params_set[i].size = signature->data_length;
        memcpy(tmp_buf, signature->data, signature->data_length);
    } else {
        params_set[++i].buf  = 0;
        params_set[i].size = 0;
    }

    TEEC_SharedMemory out_params_p;
    out_params_p.size = TEE_KEYMASTER_KEY_INOUT_COMMON_SIZE;
    out_params_p.flags = TEEC_MEM_OUTPUT;
    if(TEEC_AllocateSharedMemory(gKeyMasterV2Context, &out_params_p) != TEEC_SUCCESS) {
        ALOGE("%s: allocate output params share memory fail", __func__);
        TEEC_ReleaseSharedMemory(&paramMem);
        return KM_ERROR_UNKNOWN_ERROR;
    }

    TEEC_SharedMemory tee_output_p;
    tee_output_p.size = TEE_KEYMASTER_KEY_INOUT_COMMON_SIZE;
    tee_output_p.flags = TEEC_MEM_OUTPUT;
    if(TEEC_AllocateSharedMemory(gKeyMasterV2Context, &tee_output_p) != TEEC_SUCCESS) {
        ALOGE("%s: allocate output share memory fail", __func__);
        TEEC_ReleaseSharedMemory(&paramMem);
        TEEC_ReleaseSharedMemory(&out_params_p);
        return KM_ERROR_UNKNOWN_ERROR;
    }

    TEEC_Operation operation;
    memset(&operation, 0, sizeof(TEEC_Operation));
    operation.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_WHOLE, TEEC_MEMREF_WHOLE,
            TEEC_MEMREF_WHOLE, TEEC_NONE);
    operation.started = 1;

    operation.params[0].memref.parent = &paramMem;
    operation.params[0].memref.offset = 0;
    operation.params[0].memref.size = 0;

    operation.params[1].memref.parent = &out_params_p;
    operation.params[1].memref.offset = 0;
    operation.params[1].memref.size = 0;

    operation.params[2].memref.parent = &tee_output_p;
    operation.params[2].memref.offset = 0;
    operation.params[2].memref.size = 0;

    int success = TEEC_InvokeCommand(gKeyMasterV2Session,
                    MSG_KEYMASTER_V2_FINISH, &operation, NULL);

    keymaster_error_t err_type = KM_ERROR_OK;
    if (success) {
        if (success < KM_ERROR_UNKNOWN_ERROR) {
            err_type = KM_ERROR_UNKNOWN_ERROR;
        } else {
            err_type = (keymaster_error_t)success;
        }
    } else {
        //fix tee_output_params
        //tee_hexdump((uint8_t*)out_params_p.buffer, 16);
        tee_keymaster_key_param_set_t *set = (tee_keymaster_key_param_set_t *)out_params_p.buffer;
        if (set->length) {
            if (out_params) {
                tee_CopyToParamSet((uint8_t*)out_params_p.buffer,
                            out_params, (uint8_t*)out_params_p.buffer);
            } else {
                err_type = KM_ERROR_OUTPUT_PARAMETER_NULL;
                goto __finish_err;
            }
        }

        if (output) {
            output->data_length = operation.params[2].memref.size;
            uint8_t* tmp = reinterpret_cast<uint8_t*>(malloc(output->data_length));
            if (!tmp)
                err_type = KM_ERROR_MEMORY_ALLOCATION_FAILED;
            else {
                memcpy(tmp, tee_output_p.buffer, output->data_length);
                output->data = tmp;
            }
        } else if (operation.params[2].memref.size > 0)
            err_type = KM_ERROR_OUTPUT_PARAMETER_NULL;
    }

__finish_err:
    TEEC_ReleaseSharedMemory(&paramMem);
    TEEC_ReleaseSharedMemory(&out_params_p);
    TEEC_ReleaseSharedMemory(&tee_output_p);
    if(err_type != KM_ERROR_OK)
        ALOGE("%s failed with %d", __func__, err_type);

    return err_type;
}

keymaster_error_t
KeyMaster_V2_Abort(const keymaster_device_t* dev,
                   keymaster_operation_handle_t operation_handle){
    ALOGV("%s %d", __func__, __LINE__);
    if (!dev)
        return KM_ERROR_UNEXPECTED_NULL_POINTER;

    if (!keymaster_v2_configured)
        return KM_ERROR_KEYMASTER_NOT_CONFIGURED;

    keymaster_error_t err_type = KM_ERROR_OK;
    TEEC_Operation operation;
    memset(&operation, 0, sizeof(TEEC_Operation));
    operation.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INPUT, TEEC_NONE,
            TEEC_NONE, TEEC_NONE);
    operation.started = 1;

    operation.params[0].value.a = (operation_handle >> 0  ) & 0xffffffff;
    operation.params[0].value.b = (operation_handle >> 32 ) & 0xffffffff;

    int success = TEEC_InvokeCommand(gKeyMasterV2Session,
                        MSG_KEYMASTER_V2_ABORT,&operation, NULL);

    if (success < KM_ERROR_UNKNOWN_ERROR) {
        err_type = KM_ERROR_UNKNOWN_ERROR;
    } else {
        err_type = (keymaster_error_t)success;
    }

//__abort_err:
    if(err_type != KM_ERROR_OK)
        ALOGE("%s failed with %d", __func__, err_type);

    return err_type;
}

static int sunxi_tee_keymaster_v2_device_close(hw_device_t *dev)
{
    KeyMaster_V2_Terminate();
    delete dev;
    return 0;
}

static int sunxi_tee_keymaster_v2_device_open(const hw_module_t* module,
    const char* name, hw_device_t** device) {

    if (strcmp(name, KEYSTORE_KEYMASTER) != 0)
        return -EINVAL;

    ALOGD("%s:: Enter AW tee keymaster v2\n",__func__);

    Unique_keymaster_device_t dev(new keymaster_device_t);
    if (dev.get() == NULL)
        return -ENOMEM;

    dev->context = (void *)AW_KEYMASTER_V2_TEE_HAL;
    dev->common.tag = HARDWARE_DEVICE_TAG;
    dev->common.version = 1;
    dev->common.module = const_cast<hw_module_t*>(module);
    dev->common.close = sunxi_tee_keymaster_v2_device_close;

    dev->flags = KEYMASTER_BLOBS_ARE_STANDALONE | KEYMASTER_SUPPORTS_EC;

    dev->configure = KeyMaster_V2_Configure;
    dev->add_rng_entropy = KeyMaster_V2_Add_Rng_Entropy;
    dev->generate_key = KeyMaster_V2_Generate_Key;
    dev->get_key_characteristics = KeyMaster_V2_Get_Key_Characteristics;
    dev->import_key = KeyMaster_V2_Import_Key;
    dev->export_key = KeyMaster_V2_Export_Key;
    dev->attest_key = KeyMaster_V2_Attest_Key;
    dev->upgrade_key = KeyMaster_V2_Upgrade_Key;
    dev->delete_key = KeyMaster_V2_Delete_Key;
    dev->delete_all_keys = KeyMaster_V2_Delete_All_Keys;
    dev->begin = KeyMaster_V2_Begin;
    dev->update = KeyMaster_V2_Update;
    dev->finish = KeyMaster_V2_Finish;
    dev->abort = KeyMaster_V2_Abort;

    *device = reinterpret_cast<hw_device_t*>(dev.release());

    KeyMaster_V2_Initialize();
    ALOGD("%s:AW keymaster v2 open sucessfully!\n",__func__);

    return 0;
}

static struct hw_module_methods_t keystore_module_methods = {
    .open = sunxi_tee_keymaster_v2_device_open,
};

struct keystore_module HAL_MODULE_INFO_SYM
__attribute__ ((visibility ("default"))) = {
    .common = {
        .tag = HARDWARE_MODULE_TAG,
        .module_api_version = KEYMASTER_MODULE_API_VERSION_2_0,
        .hal_api_version = HARDWARE_HAL_API_VERSION,
        .id = KEYSTORE_HARDWARE_MODULE_ID,
        .name = "AllWinnerTech KeyMaster V2 Tee HAL",
        .author = "The Android Open Source Project",
        .methods = &keystore_module_methods,
        .dso = 0,
        .reserved = {},
    },
};
