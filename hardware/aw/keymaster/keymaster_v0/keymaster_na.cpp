/********************************************
  KeyMaster OEM API
  ALLWINNER TECH.CO
  version 1.0 by wanglford 20170227

*********************************************
The Keymaster APIs achieve 5 functions
*********************************************
*/

#define LOG_NDEBUG 0
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
#include <UniquePtr.h>
#include <hardware/hardware.h>
#include <hardware/keymaster_common.h>
#include <hardware/keymaster0.h>
#include <private/android_filesystem_config.h>

#include <tee_client_api.h>
#include "include/keymaster_errno.h"
#include "include/keymaster_def.h"

#include <logger.h>

typedef keymaster0_device_t  keymaster_device_t;
typedef UniquePtr<keymaster_device_t> Unique_keymaster_device_t;


enum {
    /*keep in sync with definition in secure os*/
    MSG_KEYMASTER_MIN = 0x200,
    MSG_KEYMASTER_INITIALIZE,
    MSG_KEYMASTER_TERMINATE,
    MSG_KEYMASTER_GENERATE_KEYPARIS,
    MSG_KEYMASTER_IMPORT_KEYPARIS,
    MSG_KEYMASTER_GET_KEYPARIS_PUBLIC,//5
    MSG_KEYMASTER_SIGN_DATA,
    MSG_KEYMASTER_VERIFY_DATA,
    MSG_KEYMASTER_MAX,
};

//UUID for keymaster
static const uint8_t KeyMasterUUID[16] = {
    0x60,0xbe,0xbe,0xd6,0x3e,0xbe,0x46,0x40,
    0xb2,0x39,0x89,0x1e,0x0a,0x59,0x48,0x60
};

static pthread_mutex_t gKeyMasterMutex = PTHREAD_MUTEX_INITIALIZER;
static TEEC_Context *gKeyMasterContext = NULL;
static TEEC_Session *gKeyMasterSession = NULL;

#if 0
void hexdump(unsigned char * data, int  size) {

    int i = 0;
    int line ;

    char tmp[32 + 16 + 11 + 1];
    int  offset = 0;

    for(line = 0; offset < size; line ++ ) {
        sprintf(&tmp[0], "0x%08x:", line*16);
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
      ALOGD("%s", tmp);
    }
}
#endif
int KeyMaster_Initialize(void)
{
	//ALOGD("KeyMaster_Initialize");
    pthread_mutex_lock(&gKeyMasterMutex);
    if (gKeyMasterContext != NULL) {
        pthread_mutex_unlock(&gKeyMasterMutex);
        return 0;
    }

    gKeyMasterContext = (TEEC_Context *)malloc(sizeof(TEEC_Context));
    gKeyMasterSession = (TEEC_Session *)malloc(sizeof(TEEC_Session));
    assert(gKeyMasterContext != NULL);
    assert(gKeyMasterSession != NULL);
    pthread_mutex_unlock(&gKeyMasterMutex);
    TEEC_Result success = TEEC_InitializeContext(NULL, gKeyMasterContext);

    if (success != TEEC_SUCCESS) {
        ALOGE("initialize context failed");
        return -1;
    }

    success = TEEC_OpenSession(gKeyMasterContext, gKeyMasterSession, (const TEEC_UUID *)&KeyMasterUUID[0],
    TEEC_LOGIN_PUBLIC, NULL, NULL, NULL);

    if (success != TEEC_SUCCESS) {
        ALOGE("open session failed");
        return -1;
    }

    TEEC_Operation operation;

    memset(&operation, 0, sizeof(TEEC_Operation));
    operation.paramTypes = TEEC_PARAM_TYPES(TEEC_NONE, TEEC_NONE, TEEC_NONE, TEEC_NONE);
    operation.started = 1;
    success = TEEC_InvokeCommand(gKeyMasterSession, MSG_KEYMASTER_INITIALIZE, &operation, NULL);

    return (int) success;
}

int KeyMaster_Terminate(void)
{
    //ALOGD("KeyMaster_Terminate");

    pthread_mutex_lock(&gKeyMasterMutex);
    if (gKeyMasterContext == NULL) {
        pthread_mutex_unlock(&gKeyMasterMutex);
        return 0;
    }

    TEEC_Operation operation;
    memset(&operation, 0, sizeof(TEEC_Operation));
    operation.paramTypes = TEEC_PARAM_TYPES(TEEC_NONE, TEEC_NONE, TEEC_NONE, TEEC_NONE);
    operation.started = 1;
    TEEC_Result success  = TEEC_InvokeCommand(gKeyMasterSession, MSG_KEYMASTER_TERMINATE, &operation, NULL);

    if (success != TEEC_SUCCESS) {
        ALOGE("call invoke command error");
    }

    TEEC_CloseSession(gKeyMasterSession);
    TEEC_FinalizeContext(gKeyMasterContext);

    free(gKeyMasterSession);
    free(gKeyMasterContext);
    gKeyMasterSession = NULL;
    gKeyMasterContext = NULL;
    pthread_mutex_unlock(&gKeyMasterMutex);

    return (int) success;
}

static int __generate_rsa_key (
        const keymaster_keypair_t key_type, const void* key_params,
        uint8_t** keyBlob, size_t* keyBlobLength)
{
    TEEC_SharedMemory key_p;
    key_p.size = sizeof(keymaster_rsa_keygen_params_t);
    key_p.flags = TEEC_MEM_INPUT;
    if (TEEC_AllocateSharedMemory(gKeyMasterContext, &key_p) != TEEC_SUCCESS) {
        ALOGE("%s: allocate key parameters share memory fail", __func__);

        return -1;
    }
    memcpy(key_p.buffer, key_params, sizeof(keymaster_rsa_keygen_params_t));

    TEEC_SharedMemory outputMem;
    outputMem.size = AW_KEYMASTER_BLOB_SIZE;
    outputMem.flags = TEEC_MEM_OUTPUT;

    if (TEEC_AllocateSharedMemory(gKeyMasterContext, &outputMem) != TEEC_SUCCESS) {
        ALOGE("%s: __generate_rsa_key: allocate key output share memory fail", __func__);

        TEEC_ReleaseSharedMemory(&key_p);

        return -1;
    }

    TEEC_Operation operation;
    memset(&operation, 0, sizeof(TEEC_Operation));
    operation.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INPUT, TEEC_MEMREF_WHOLE,
    TEEC_MEMREF_WHOLE, TEEC_NONE);
    operation.started = 1;

    operation.params[0].value.a = key_type;

    operation.params[1].memref.parent = &key_p;
    operation.params[1].memref.offset = 0;
    operation.params[1].memref.size = 0;

    operation.params[2].memref.parent = &outputMem;
    operation.params[2].memref.offset = 0;
    operation.params[2].memref.size = 0;

    TEEC_Result success = TEEC_InvokeCommand(gKeyMasterSession, MSG_KEYMASTER_GENERATE_KEYPARIS, &operation, NULL);
    //ALOGE("%s %d %d\n", __func__, __LINE__, success);
    if (!success) {
        uint8_t *keyblob_buf = new uint8_t[operation.params[2].memref.size];

        *keyBlobLength = operation.params[2].memref.size;
        memcpy(keyblob_buf, outputMem.buffer, *keyBlobLength);

        *keyBlob = keyblob_buf;
    }
    //ALOGE("%s %d %d\n", __func__, __LINE__, *keyBlobLength);

    TEEC_ReleaseSharedMemory(&key_p);
    TEEC_ReleaseSharedMemory(&outputMem);

    return (int) success;
}


static int __generate_ec_key (
        const keymaster_keypair_t key_type, const void* key_params,
        uint8_t** keyBlob, size_t* keyBlobLength )
{
    //keymaster_ec_keygen_params_t *params = (keymaster_ec_keygen_params_t *)key_params;

    TEEC_SharedMemory key_p;
    key_p.size = sizeof(keymaster_ec_keygen_params_t);
    key_p.flags = TEEC_MEM_INPUT;

    if (TEEC_AllocateSharedMemory(gKeyMasterContext, &key_p) != TEEC_SUCCESS) {
        ALOGE("%s: allocate key parameters share memory fail", __func__);

        return -1;
    }

    memcpy(key_p.buffer, key_params, sizeof(keymaster_ec_keygen_params_t));

    TEEC_SharedMemory outputMem;
    outputMem.size = AW_KEYMASTER_BLOB_SIZE;
    outputMem.flags = TEEC_MEM_OUTPUT;

    if (TEEC_AllocateSharedMemory(gKeyMasterContext, &outputMem) != TEEC_SUCCESS) {
        ALOGE("%s: allocate key output share memory fail", __func__);
        TEEC_ReleaseSharedMemory(&key_p);
        return -1;
    }

    TEEC_Operation operation;
    memset(&operation, 0, sizeof(TEEC_Operation));
    operation.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INPUT, TEEC_MEMREF_WHOLE,
    TEEC_MEMREF_WHOLE, TEEC_NONE);
    operation.started = 1;

    operation.params[0].value.a = key_type;

    operation.params[1].memref.parent = &key_p;
    operation.params[1].memref.offset = 0;
    operation.params[1].memref.size = 0;

    operation.params[2].memref.parent = &outputMem;
    operation.params[2].memref.offset = 0;
    operation.params[2].memref.size = 0;

    TEEC_Result success = TEEC_InvokeCommand(gKeyMasterSession, MSG_KEYMASTER_GENERATE_KEYPARIS, &operation, NULL);
    //ALOGE("%s %d %d\n", __func__, __LINE__, success);
    if (!success) {
        uint8_t *keyblob_buf = new uint8_t[operation.params[2].memref.size];

        *keyBlobLength = operation.params[2].memref.size;
        memcpy(keyblob_buf, outputMem.buffer, *keyBlobLength);

        *keyBlob = keyblob_buf;
    }
    //ALOGE("%s %d %d\n", __func__, __LINE__, *keyBlobLength);
    //ALOGE("%s %d %d\n", __func__, __LINE__, sizeof(aw_key_blob_format_t));
    TEEC_ReleaseSharedMemory(&outputMem);
    TEEC_ReleaseSharedMemory(&key_p);

    return (int) success;
}

static int __generate_dsa_key (
        const keymaster_keypair_t key_type, const void* key_params,
        uint8_t** keyBlob, size_t* keyBlobLength )
{
    const keymaster_dsa_keygen_params_t* dsa_params = (const keymaster_dsa_keygen_params_t*) key_params;
    keymaster_dsa_keygen_params_t *dsa_key_p;
    int success;
    int alloc_flag = 0;

    if ((dsa_params->generator == NULL ) || (dsa_params->prime_p == NULL ) ||
       (dsa_params->prime_q == NULL )) {
        ALOGE("%s: dsa params pointer is NULL\n", __func__);

        return -1;
    }

    if ((dsa_params->generator_len == 0 ) || (dsa_params->prime_p_len == 0 ) ||
       (dsa_params->prime_q_len == 0 )) {
        ALOGE("%s: dsa params size is NULL\n", __func__);

        return -1;
    }

    TEEC_SharedMemory key_p;
    key_p.size = sizeof(keymaster_dsa_keygen_params_t);
    key_p.flags = TEEC_MEM_INPUT;
    if (TEEC_AllocateSharedMemory(gKeyMasterContext, &key_p) != TEEC_SUCCESS) {
        ALOGE("%s: allocate key parameters share memory fail", __func__);
        success = -1;

        goto __generate_dsa_key_exit;
    }

    dsa_key_p = (keymaster_dsa_keygen_params_t *)key_p.buffer;
    alloc_flag |= (1<<0);

    TEEC_SharedMemory generator;
    generator.size = dsa_params->generator_len;
    generator.flags = TEEC_MEM_INPUT;

    if (TEEC_AllocateSharedMemory(gKeyMasterContext, &generator) != TEEC_SUCCESS) {
        ALOGE("%s: allocate dsa key generator share memory fail", __func__);
        success = -1;

        goto __generate_dsa_key_exit;
    }

    alloc_flag |= (1<<1);

    TEEC_SharedMemory prime_p;
    prime_p.size = dsa_params->prime_p_len;
    prime_p.flags = TEEC_MEM_INPUT;

    if (TEEC_AllocateSharedMemory(gKeyMasterContext, &prime_p) != TEEC_SUCCESS) {
        ALOGE("%s: allocate dsa key prime_p share memory fail", __func__);
        success = -1;

        goto __generate_dsa_key_exit;
    }

    alloc_flag |= (1<<2);

    TEEC_SharedMemory prime_q;
    prime_q.size = dsa_params->prime_q_len;
    prime_q.flags = TEEC_MEM_INPUT;

    if (TEEC_AllocateSharedMemory(gKeyMasterContext, &prime_q) != TEEC_SUCCESS) {
        ALOGE("%s: allocate dsa key prime_q share memory fail", __func__);
        success = -1;

        goto __generate_dsa_key_exit;
    }

    alloc_flag |= (1<<3);
    memcpy(generator.buffer, dsa_params->generator, dsa_params->generator_len);
    memcpy(prime_p.buffer,   dsa_params->prime_p,   dsa_params->prime_p_len);
    memcpy(prime_q.buffer,   dsa_params->prime_q,   dsa_params->prime_q_len);

    dsa_key_p->generator = (const uint8_t *)generator.buffer;
    dsa_key_p->prime_p   = (const uint8_t *)prime_p.buffer;
    dsa_key_p->prime_q   = (const uint8_t *)prime_q.buffer;

    dsa_key_p->generator_len = generator.size;
    dsa_key_p->generator_len = prime_p.size;
    dsa_key_p->prime_q_len   = prime_q.size;
    dsa_key_p->key_size      = dsa_params->key_size;

    TEEC_SharedMemory outputMem;
    outputMem.size = AW_KEYMASTER_BLOB_SIZE;
    outputMem.flags = TEEC_MEM_OUTPUT;

    if (TEEC_AllocateSharedMemory(gKeyMasterContext, &outputMem) != TEEC_SUCCESS) {
        ALOGE("%s: allocate key output share memory fail", __func__);
        success = -1;

        goto __generate_dsa_key_exit;
    }
    alloc_flag |= (1<<4);

    TEEC_Operation operation;
    memset(&operation, 0, sizeof(TEEC_Operation));
    operation.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INPUT, TEEC_MEMREF_WHOLE,
    TEEC_MEMREF_WHOLE, TEEC_NONE);
    operation.started = 1;

    operation.params[0].value.a = key_type;

    operation.params[1].memref.parent = &key_p;
    operation.params[1].memref.offset = 0;
    operation.params[1].memref.size = 0;

    operation.params[2].memref.parent = &outputMem;
    operation.params[2].memref.offset = 0;
    operation.params[2].memref.size = 0;

    success = TEEC_InvokeCommand(gKeyMasterSession, MSG_KEYMASTER_GENERATE_KEYPARIS, &operation, NULL);
    //ALOGE("%s %d %d\n", __func__, __LINE__, success);
    if (!success) {
        uint8_t *keyblob_buf = new uint8_t[operation.params[2].memref.size];

        *keyBlobLength = operation.params[2].memref.size;
        memcpy(keyblob_buf, outputMem.buffer, *keyBlobLength);

        *keyBlob = keyblob_buf;
    } else {
        ALOGE("success=%d\n", success);
    }

__generate_dsa_key_exit:
    if (alloc_flag & (1<<0))
        TEEC_ReleaseSharedMemory(&key_p);
    if (alloc_flag & (1<<1))
        TEEC_ReleaseSharedMemory(&generator);
    if (alloc_flag & (1<<2))
        TEEC_ReleaseSharedMemory(&prime_p);
    if (alloc_flag & (1<<3))
        TEEC_ReleaseSharedMemory(&prime_q);
    if (alloc_flag & (1<<4))
        TEEC_ReleaseSharedMemory(&outputMem);
    return success;
}


int KeyMaster_Generate_keypairs(const keymaster_device_t* dev,
        const keymaster_keypair_t key_type, const void* key_params,
        uint8_t** keyBlob, size_t* keyBlobLength)
{
    //ALOGD("KeyMaster_Generate_keypairs");

    if (dev->context == NULL) {
        ALOGE("KeyMaster_Generate_keyparis: Context NULL Ptr Err!");

        return -1;
    }

    if (key_params == NULL) {
        ALOGE("KeyMaster_Generate_keyparis: key_params NULL Ptr Err!, %d", __LINE__);

        return -1;
    }

    if (keyBlob == NULL || keyBlobLength == NULL) {
        ALOGE("KeyMaster_Generate_keyparis: output key blob or length == NULL");

        return -1;
    }

    //ALOGD("%d %s\n", __LINE__, __func__);

    if (key_type == TYPE_DSA) {
        //ALOGD("%d %s\n", __LINE__, __func__);
        return __generate_dsa_key(key_type, key_params, keyBlob, keyBlobLength);
    }
    else if (key_type == TYPE_EC) {
        //ALOGD("%d %s\n", __LINE__, __func__);
        return __generate_ec_key(key_type, key_params, keyBlob, keyBlobLength);
    }
    else if (key_type == TYPE_RSA) {
        //ALOGD("%d %s\n", __LINE__, __func__);
        return __generate_rsa_key(key_type, key_params, keyBlob, keyBlobLength);
    } else {
        ALOGE("%s: unsupporte key type\n", __func__);
        return -1;
    }
}


int KeyMaster_Import_keyparis(const keymaster_device_t* dev,
        const uint8_t* key, const size_t key_length,
        uint8_t** keyBlob, size_t* key_blob_length)
{
    //ALOGD("KeyMaster_Import_keyparis");
    if (dev->context == NULL) {
        ALOGE("KeyMaster_Import_Keypairs: Context  == NULL");
        return -1;
    }

    if (key == NULL) {
        ALOGE("%s: Input key == NULL", __func__);
        return -1;
    }

    if (keyBlob == NULL || key_blob_length == NULL) {
        ALOGE("%s: Output key blob or length == NULL", __func__);
        return -1;
    }

    TEEC_SharedMemory key_p;
    key_p.size = key_length;
    key_p.flags = TEEC_MEM_INPUT;

    if (TEEC_AllocateSharedMemory(gKeyMasterContext, &key_p) != TEEC_SUCCESS) {
        ALOGE("%s: allocate key parameters share memory fail", __func__);

        return -1;
    }
    memcpy(key_p.buffer, key, key_length);

    TEEC_SharedMemory outputMem;
    outputMem.size = AW_KEYMASTER_BLOB_SIZE;
    outputMem.flags = TEEC_MEM_OUTPUT;

    if (TEEC_AllocateSharedMemory(gKeyMasterContext, &outputMem) != TEEC_SUCCESS) {
        ALOGE("%s: allocate key output share memory fail", __func__);
        TEEC_ReleaseSharedMemory(&key_p);

        return -1;
    }

    TEEC_Operation operation;
    memset(&operation, 0, sizeof(TEEC_Operation));
    operation.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_WHOLE, TEEC_MEMREF_WHOLE,
    TEEC_NONE, TEEC_NONE);
    operation.started = 1;

    operation.params[0].memref.parent = &key_p;
    operation.params[0].memref.offset = 0;
    operation.params[0].memref.size = 0;

    operation.params[1].memref.parent = &outputMem;
    operation.params[1].memref.offset = 0;
    operation.params[1].memref.size = 0;

    TEEC_Result success = TEEC_InvokeCommand(gKeyMasterSession, MSG_KEYMASTER_IMPORT_KEYPARIS, &operation, NULL);
    //ALOGE("key_blob_length = %d\n", operation.params[1].memref.size);
    if (!success) {
        uint8_t *keyblob_buf = new uint8_t[operation.params[1].memref.size];

        *key_blob_length = operation.params[1].memref.size;
        memcpy(keyblob_buf, outputMem.buffer, *key_blob_length);

        *keyBlob = keyblob_buf;
    }
    //hexdump(*keyBlob, *key_blob_length);
    //ALOGE("*key_blob_length=%d\n", *key_blob_length);
    TEEC_ReleaseSharedMemory(&key_p);
    TEEC_ReleaseSharedMemory(&outputMem);

    return (int)success;
}

int KeyMaster_Get_keyparis_public (
        const keymaster_device_t* dev, const uint8_t* keyBlob,
        const size_t key_blob_length, uint8_t** x509_data,
        size_t* x509_data_length)
{
    //ALOGD("KeyMaster_Get_keyparis_public");
    if (dev->context == NULL) {
        ALOGE("%s: Context  == NULL", __func__);
        return -1;
    }

    if (x509_data == NULL || x509_data_length == NULL) {
        ALOGE("%s: Output public key buffer == NULL", __func__);
        return -1;
    }

    if (x509_data == NULL) {
        ALOGE("Supplied key blob was NULL");
        return -1;
    }

    if ((keyBlob == NULL) || (key_blob_length ==0)) {
        ALOGE("%s: Supplied key blob was NULL", __func__);
        return -1;
    }

    //ALOGE("*key_blob_length=%d\n", key_blob_length);
    //hexdump((uint8_t*)keyBlob, key_blob_length);
    TEEC_SharedMemory outputMem;
    outputMem.size = AW_KEYMASTER_BLOB_SIZE;
    outputMem.flags = TEEC_MEM_INPUT;
    if (TEEC_AllocateSharedMemory(gKeyMasterContext, &outputMem) != TEEC_SUCCESS) {
        ALOGE("%s: allocate key input share memory fail", __func__);

        return -1;
    }
    memcpy(outputMem.buffer, keyBlob, key_blob_length);

    TEEC_SharedMemory key_p;
    key_p.size = 512;
    key_p.flags = TEEC_MEM_OUTPUT;

    if (TEEC_AllocateSharedMemory(gKeyMasterContext, &key_p) != TEEC_SUCCESS) {
        ALOGE("%s: allocate public key share memory fail", __func__);
        TEEC_ReleaseSharedMemory(&outputMem);

        return -1;
    }

    TEEC_Operation operation;
    memset(&operation, 0, sizeof(TEEC_Operation));
    operation.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_WHOLE, TEEC_MEMREF_WHOLE,
    TEEC_NONE, TEEC_NONE);
    operation.started = 1;

    operation.params[0].memref.parent = &outputMem;
    operation.params[0].memref.offset = 0;
    operation.params[0].memref.size = 0;

    operation.params[1].memref.parent = &key_p;
    operation.params[1].memref.offset = 0;
    operation.params[1].memref.size = 0;

    TEEC_Result success = TEEC_InvokeCommand(gKeyMasterSession, MSG_KEYMASTER_GET_KEYPARIS_PUBLIC, &operation, NULL);
    //ALOGE("%s %d %d\n", __func__, __LINE__, success);
    if (!success) {
        uint8_t *key = new uint8_t[operation.params[1].memref.size];

        uint32_t key_length = operation.params[1].memref.size;
        memcpy(key, key_p.buffer, key_length);

        *x509_data = key;
        *x509_data_length = key_length;

        //hexdump((uint8_t*)key_p.buffer, key_length);
    }

    TEEC_ReleaseSharedMemory(&key_p);
    TEEC_ReleaseSharedMemory(&outputMem);
    //ALOGE("%s %d\n", __func__, __LINE__);
    //hexdump(NULL, *x509_data, *x509_data_length);
    return (int) success;
}

int KeyMaster_Sign_data(const keymaster_device_t* dev,
        const void* params,
        const uint8_t* keyBlob, const size_t keyBlobLength,
        const uint8_t* data, const size_t dataLength,
        uint8_t** signedData, size_t* signedDataLength)
{
    //ALOGD("KeyMaster_Sign_data");
    if (dev->context == NULL) {
        ALOGE("%s: Context  == NULL", __func__);

        return -1;
    }

    if (keyBlob == NULL) {
        ALOGE("%s blob == NULL", __func__);

        return -1;
    }

    if (keyBlobLength == 0) {
        ALOGE("%s keyBlobLength == 0", __func__);

        return -1;
    }

    if (data == NULL) {
        ALOGE("%s: input data to sign == NULL", __func__);

        return -1;
    }

    if (signedData == NULL || signedDataLength == NULL) {
        ALOGE("%s: Output signature buffer == NULL", __func__);

        return -1;
    }

    TEEC_SharedMemory key_lable;
    key_lable.size = keyBlobLength;
    key_lable.flags = TEEC_MEM_INPUT;
    if (TEEC_AllocateSharedMemory(gKeyMasterContext, &key_lable) != TEEC_SUCCESS) {
        ALOGE("%s: allocate key blob share memory fail", __func__);

        return -1;
    }
    memcpy(key_lable.buffer, keyBlob, keyBlobLength);

    TEEC_SharedMemory data_p;
    data_p.size = dataLength;
    data_p.flags = TEEC_MEM_INPUT;
    if (TEEC_AllocateSharedMemory(gKeyMasterContext, &data_p) != TEEC_SUCCESS) {
        ALOGE("%s: allocate data share memory fail", __func__);
        TEEC_ReleaseSharedMemory(&key_lable);

        return -1;
    }

    memcpy(data_p.buffer, data, dataLength);

    TEEC_SharedMemory sign_p;
    sign_p.size = 512;
    sign_p.flags = TEEC_MEM_OUTPUT;
    if (TEEC_AllocateSharedMemory(gKeyMasterContext, &sign_p) != TEEC_SUCCESS) {
        ALOGE("%s: allocate sign data share memory fail", __func__);
        TEEC_ReleaseSharedMemory(&data_p);
        TEEC_ReleaseSharedMemory(&key_lable);

        return -1;
    }

    TEEC_SharedMemory algo_para;
    algo_para.size = 32;
    algo_para.flags = TEEC_MEM_INPUT;
    if (TEEC_AllocateSharedMemory(gKeyMasterContext, &algo_para) != TEEC_SUCCESS) {
        ALOGE("%s: allocate key blob share memory fail", __func__);
        TEEC_ReleaseSharedMemory(&data_p);
        TEEC_ReleaseSharedMemory(&key_lable);
        TEEC_ReleaseSharedMemory(&sign_p);

        return -1;
    }
    memcpy(algo_para.buffer, params, 32);

    TEEC_Operation operation;
    memset(&operation, 0, sizeof(TEEC_Operation));
    operation.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_WHOLE, TEEC_MEMREF_WHOLE,
    TEEC_MEMREF_WHOLE, TEEC_MEMREF_WHOLE);
    operation.started = 1;

    operation.params[0].memref.parent = &algo_para;
    operation.params[0].memref.offset = 0;
    operation.params[0].memref.size = 0;

    operation.params[1].memref.parent = &key_lable;
    operation.params[1].memref.offset = 0;
    operation.params[1].memref.size = 0;

    operation.params[2].memref.parent = &data_p;
    operation.params[2].memref.offset = 0;
    operation.params[2].memref.size = 0;

    operation.params[3].memref.parent = &sign_p;
    operation.params[3].memref.offset = 0;
    operation.params[3].memref.size = 0;

    TEEC_Result success = TEEC_InvokeCommand(gKeyMasterSession, MSG_KEYMASTER_SIGN_DATA, &operation, NULL);
    //ALOGE("%s %d %d\n", __func__, __LINE__, success);
    //ALOGE("sign data size = %d\n", operation.params[3].memref.size);
    if (!success) {
        uint8_t *signedData_buf = new uint8_t[operation.params[3].memref.size];

        *signedDataLength = operation.params[3].memref.size;
        memcpy(signedData_buf, sign_p.buffer, *signedDataLength);

        *signedData = signedData_buf;
    }
    TEEC_ReleaseSharedMemory(&key_lable);
    TEEC_ReleaseSharedMemory(&data_p);
    TEEC_ReleaseSharedMemory(&sign_p);
    TEEC_ReleaseSharedMemory(&algo_para);

    return (int) success;
}

int KeyMaster_Verify_data(const keymaster_device_t* dev,
        const void* params,
        const uint8_t* keyBlob, const size_t keyBlobLength,
        const uint8_t* signedData, const size_t signedDataLength,
        const uint8_t* signature, const size_t signatureLength)
{
    //ALOGD("KeyMaster_Verify_data");
    if (dev->context == NULL) {
        ALOGE("%s: qcom_km_sign_data: Context  == NULL", __func__);
        return -1;
    }

    if (keyBlob == NULL) {
        ALOGE("%s blob == NULL", __func__);

        return -1;
    }

    if (keyBlobLength == 0) {
        ALOGE("%s keyBlobLength == 0", __func__);

        return -1;
    }

    if (signedData == NULL || signature == NULL) {
        ALOGE("%s: data or signature buffers == NULL", __func__);
        return -1;
    }

    if ((signedDataLength == 0) || (signatureLength == 0)) {
        ALOGE("%s: data or signature length == 0", __func__);
        return -1;
    }

    keymaster_private_sign_params_t *private_para = (keymaster_private_sign_params_t *)params;
    if (private_para->digest_type != PADDING_NONE) {
        ALOGE("%s: digest type is NOT PADDING_NONE", __func__);

        return -1;
    }

    TEEC_SharedMemory key_lable;
    key_lable.size = keyBlobLength;
    key_lable.flags = TEEC_MEM_INPUT;
    if (TEEC_AllocateSharedMemory(gKeyMasterContext, &key_lable) != TEEC_SUCCESS) {
        ALOGE("%s: allocate key blob share memory fail", __func__);

        return -1;
    }
    memcpy(key_lable.buffer, keyBlob, keyBlobLength);

    TEEC_SharedMemory data_p;
    data_p.size = signedDataLength;
    data_p.flags = TEEC_MEM_INPUT;
    if (TEEC_AllocateSharedMemory(gKeyMasterContext, &data_p) != TEEC_SUCCESS) {
        ALOGE("%s: allocate data share memory fail", __func__);
        TEEC_ReleaseSharedMemory(&key_lable);

        return -1;
    }
    memcpy(data_p.buffer, signedData, signedDataLength);

    TEEC_SharedMemory sign_p;
    sign_p.size = signatureLength;
    sign_p.flags = TEEC_MEM_INPUT;
    if (TEEC_AllocateSharedMemory(gKeyMasterContext, &sign_p) != TEEC_SUCCESS) {
        ALOGE("%s: allocate sign data share memory fail", __func__);
        TEEC_ReleaseSharedMemory(&sign_p);
        TEEC_ReleaseSharedMemory(&key_lable);

        return -1;
    }
    memcpy(sign_p.buffer, signature, signatureLength);

    TEEC_SharedMemory algo_para;
    algo_para.size = 32;
    algo_para.flags = TEEC_MEM_INPUT;
    if (TEEC_AllocateSharedMemory(gKeyMasterContext, &algo_para) != TEEC_SUCCESS) {
        ALOGE("%s: allocate key blob share memory fail", __func__);
        TEEC_ReleaseSharedMemory(&data_p);
        TEEC_ReleaseSharedMemory(&key_lable);
        TEEC_ReleaseSharedMemory(&sign_p);

        return -1;
    }
    memcpy(algo_para.buffer, params, 32);

    TEEC_Operation operation;
    memset(&operation, 0, sizeof(TEEC_Operation));
    operation.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_WHOLE, TEEC_MEMREF_WHOLE,
    TEEC_MEMREF_WHOLE, TEEC_MEMREF_WHOLE);
    operation.started = 1;

    operation.params[0].memref.parent = &algo_para;
    operation.params[0].memref.offset = 0;
    operation.params[0].memref.size = 0;

    operation.params[1].memref.parent = &key_lable;
    operation.params[1].memref.offset = 0;
    operation.params[1].memref.size = 0;

    operation.params[2].memref.parent = &data_p;
    operation.params[2].memref.offset = 0;
    operation.params[2].memref.size = 0;

    operation.params[3].memref.parent = &sign_p;
    operation.params[3].memref.offset = 0;
    operation.params[3].memref.size = 0;

    TEEC_Result success = TEEC_InvokeCommand(gKeyMasterSession, MSG_KEYMASTER_VERIFY_DATA, &operation, NULL);
    //ALOGE("%s %d %d\n", __func__, __LINE__, success);

    TEEC_ReleaseSharedMemory(&algo_para);
    TEEC_ReleaseSharedMemory(&key_lable);
    TEEC_ReleaseSharedMemory(&data_p);
    TEEC_ReleaseSharedMemory(&sign_p);

    return (int) success;
}

/* Close an opened aw schw instance */
static int sunxi_tee_keymaster_device_close(hw_device_t *dev)
{
    KeyMaster_Terminate();

    delete dev;

    return 0;
}
/*
 * Generic device handling
 */
static int sunxi_tee_keymaster_device_open(const hw_module_t* module, const char* name,
        hw_device_t** device) {

    if (strcmp(name, KEYSTORE_KEYMASTER) != 0)
        return -EINVAL;

    ALOGV("%s:: Enter AW tee keymaster\n",__func__);

    Unique_keymaster_device_t dev(new keymaster_device_t);
    if (dev.get() == NULL)
        return -ENOMEM;

    dev->context = (void *)AW_KEYMASTER_TEE_HAL;
    dev->common.tag = HARDWARE_DEVICE_TAG;
    dev->common.version = 1;
    dev->common.module = (struct hw_module_t*) module;
    dev->common.close = sunxi_tee_keymaster_device_close;

    dev->flags = 0;

    dev->generate_keypair = KeyMaster_Generate_keypairs;
    dev->import_keypair = KeyMaster_Import_keyparis;
    dev->get_keypair_public = KeyMaster_Get_keyparis_public;
    dev->delete_keypair = NULL;
    dev->delete_all = NULL;
    dev->sign_data = KeyMaster_Sign_data;
    dev->verify_data = KeyMaster_Verify_data;

    *device = reinterpret_cast<hw_device_t*>(dev.release());

    KeyMaster_Initialize();

    ALOGV("%s:AW keymaster open sucessfully!\n",__func__);

    return 0;
}

static struct hw_module_methods_t keystore_module_methods = {
    .open = sunxi_tee_keymaster_device_open,
};

struct keystore_module HAL_MODULE_INFO_SYM
__attribute__ ((visibility ("default"))) = {
    .common = {
        .tag = HARDWARE_MODULE_TAG,
        .module_api_version = KEYMASTER_MODULE_API_VERSION_0_2,
        .hal_api_version = HARDWARE_HAL_API_VERSION,
        .id = KEYSTORE_HARDWARE_MODULE_ID,
        .name = "AllWinnerTech KeyMaster Tee HAL",
        .author = "The Android Open Source Project",
        .methods = &keystore_module_methods,
        .dso = 0,
        .reserved = {},
    },
};


