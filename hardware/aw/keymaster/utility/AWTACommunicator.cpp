#include "include/AW_KM_utility/AWTACommunicator.h"
#include <tee_client_api.h>
#include <keymaster/keymaster_configuration.h>
#include <hardware/keymaster_defs.h>

using ::keymaster::AddEntropyRequest;
using ::keymaster::AddEntropyResponse;
using ::keymaster::ConfigureRequest;
using ::keymaster::ConfigureResponse;

namespace aw {
namespace hardware {
namespace keymaster {
namespace {
enum {
    MSG_KEYMASTER_V2_CONFIGURE = 0x210,
};

static const char* TeecErrorString(TEEC_Result error) {
    switch (error) {
    case TEEC_SUCCESS:
        return "TEEC_SUCCESS";
    case TEEC_ERROR_GENERIC:
        return "TEEC_ERROR_GENERIC";
    case TEEC_ERROR_ACCESS_DENIED:
        return "TEEC_ERROR_ACCESS_DENIED";
    case TEEC_ERROR_CANCEL:
        return "TEEC_ERROR_CANCEL";
    case TEEC_ERROR_ACCESS_CONFLICT:
        return "TEEC_ERROR_ACCESS_CONFLICT";
    case TEEC_ERROR_EXCESS_DATA:
        return "TEEC_ERROR_EXCESS_DATA";
    case TEEC_ERROR_BAD_FORMAT:
        return "TEEC_ERROR_BAD_FORMAT";
    case TEEC_ERROR_BAD_PARAMETERS:
        return "TEEC_ERROR_BAD_PARAMETERS";
    case TEEC_ERROR_BAD_STATE:
        return "TEEC_ERROR_BAD_STATE";
    case TEEC_ERROR_ITEM_NOT_FOUND:
        return "TEEC_ERROR_ITEM_NOT_FOUND";
    case TEEC_ERROR_NOT_IMPLEMENTED:
        return "TEEC_ERROR_NOT_IMPLEMENTED";
    case TEEC_ERROR_NOT_SUPPORTED:
        return "TEEC_ERROR_NOT_SUPPORTED";
    case TEEC_ERROR_NO_DATA:
        return "TEEC_ERROR_NO_DATA";
    case TEEC_ERROR_OUT_OF_MEMORY:
        return "TEEC_ERROR_OUT_OF_MEMORY";
    case TEEC_ERROR_BUSY:
        return "TEEC_ERROR_BUSY";
    case TEEC_ERROR_COMMUNICATION:
        return "TEEC_ERROR_COMMUNICATION";
    case TEEC_ERROR_SECURITY:
        return "TEEC_ERROR_SECURITY";
    case TEEC_ERROR_SHORT_BUFFER:
        return "TEEC_ERROR_SHORT_BUFFER";
    case TEEC_ERROR_EXTERNAL_CANCEL:
        return "TEEC_ERROR_EXTERNAL_CANCEL";
    case TEEC_ERROR_TARGET_DEAD:
        return "TEEC_ERROR_TARGET_DEAD";
    default:
        return "unknown error";
    }
}

static const char* KmErrorCodeString(keymaster_error_t error_code) {
    switch (error_code) {
    case KM_ERROR_OK:
        return "KM_ERROR_OK";
    case KM_ERROR_ROOT_OF_TRUST_ALREADY_SET:
        return "KM_ERROR_ROOT_OF_TRUST_ALREADY_SET";
    case KM_ERROR_UNSUPPORTED_PURPOSE:
        return "KM_ERROR_UNSUPPORTED_PURPOSE";
    case KM_ERROR_INCOMPATIBLE_PURPOSE:
        return "KM_ERROR_INCOMPATIBLE_PURPOSE";
    case KM_ERROR_UNSUPPORTED_ALGORITHM:
        return "KM_ERROR_UNSUPPORTED_ALGORITHM";
    case KM_ERROR_INCOMPATIBLE_ALGORITHM:
        return "KM_ERROR_INCOMPATIBLE_ALGORITHM";
    case KM_ERROR_UNSUPPORTED_KEY_SIZE:
        return "KM_ERROR_UNSUPPORTED_KEY_SIZE";
    case KM_ERROR_UNSUPPORTED_BLOCK_MODE:
        return "KM_ERROR_UNSUPPORTED_BLOCK_MODE";
    case KM_ERROR_INCOMPATIBLE_BLOCK_MODE:
        return "KM_ERROR_INCOMPATIBLE_BLOCK_MODE";
    case KM_ERROR_UNSUPPORTED_MAC_LENGTH:
        return "KM_ERROR_UNSUPPORTED_MAC_LENGTH";
    case KM_ERROR_UNSUPPORTED_PADDING_MODE:
        return "KM_ERROR_UNSUPPORTED_PADDING_MODE";
    case KM_ERROR_INCOMPATIBLE_PADDING_MODE:
        return "KM_ERROR_INCOMPATIBLE_PADDING_MODE";
    case KM_ERROR_UNSUPPORTED_DIGEST:
        return "KM_ERROR_UNSUPPORTED_DIGEST";
    case KM_ERROR_INCOMPATIBLE_DIGEST:
        return "KM_ERROR_INCOMPATIBLE_DIGEST";
    case KM_ERROR_INVALID_EXPIRATION_TIME:
        return "KM_ERROR_INVALID_EXPIRATION_TIME";
    case KM_ERROR_INVALID_USER_ID:
        return "KM_ERROR_INVALID_USER_ID";
    case KM_ERROR_INVALID_AUTHORIZATION_TIMEOUT:
        return "KM_ERROR_INVALID_AUTHORIZATION_TIMEOUT";
    case KM_ERROR_UNSUPPORTED_KEY_FORMAT:
        return "KM_ERROR_UNSUPPORTED_KEY_FORMAT";
    case KM_ERROR_INCOMPATIBLE_KEY_FORMAT:
        return "KM_ERROR_INCOMPATIBLE_KEY_FORMAT";
    case KM_ERROR_UNSUPPORTED_KEY_ENCRYPTION_ALGORITHM:
        return "KM_ERROR_UNSUPPORTED_KEY_ENCRYPTION_ALGORITHM";
    case KM_ERROR_UNSUPPORTED_KEY_VERIFICATION_ALGORITHM:
        return "KM_ERROR_UNSUPPORTED_KEY_VERIFICATION_ALGORITHM";
    case KM_ERROR_INVALID_INPUT_LENGTH:
        return "KM_ERROR_INVALID_INPUT_LENGTH";
    case KM_ERROR_KEY_EXPORT_OPTIONS_INVALID:
        return "KM_ERROR_KEY_EXPORT_OPTIONS_INVALID";
    case KM_ERROR_DELEGATION_NOT_ALLOWED:
        return "KM_ERROR_DELEGATION_NOT_ALLOWED";
    case KM_ERROR_KEY_NOT_YET_VALID:
        return "KM_ERROR_KEY_NOT_YET_VALID";
    case KM_ERROR_KEY_EXPIRED:
        return "KM_ERROR_KEY_EXPIRED";
    case KM_ERROR_KEY_USER_NOT_AUTHENTICATED:
        return "KM_ERROR_KEY_USER_NOT_AUTHENTICATED";
    case KM_ERROR_OUTPUT_PARAMETER_NULL:
        return "KM_ERROR_OUTPUT_PARAMETER_NULL";
    case KM_ERROR_INVALID_OPERATION_HANDLE:
        return "KM_ERROR_INVALID_OPERATION_HANDLE";
    case KM_ERROR_INSUFFICIENT_BUFFER_SPACE:
        return "KM_ERROR_INSUFFICIENT_BUFFER_SPACE";
    case KM_ERROR_VERIFICATION_FAILED:
        return "KM_ERROR_VERIFICATION_FAILED";
    case KM_ERROR_TOO_MANY_OPERATIONS:
        return "KM_ERROR_TOO_MANY_OPERATIONS";
    case KM_ERROR_UNEXPECTED_NULL_POINTER:
        return "KM_ERROR_UNEXPECTED_NULL_POINTER";
    case KM_ERROR_INVALID_KEY_BLOB:
        return "KM_ERROR_INVALID_KEY_BLOB";
    case KM_ERROR_IMPORTED_KEY_NOT_ENCRYPTED:
        return "KM_ERROR_IMPORTED_KEY_NOT_ENCRYPTED";
    case KM_ERROR_IMPORTED_KEY_DECRYPTION_FAILED:
        return "KM_ERROR_IMPORTED_KEY_DECRYPTION_FAILED";
    case KM_ERROR_IMPORTED_KEY_NOT_SIGNED:
        return "KM_ERROR_IMPORTED_KEY_NOT_SIGNED";
    case KM_ERROR_IMPORTED_KEY_VERIFICATION_FAILED:
        return "KM_ERROR_IMPORTED_KEY_VERIFICATION_FAILED";
    case KM_ERROR_INVALID_ARGUMENT:
        return "KM_ERROR_INVALID_ARGUMENT";
    case KM_ERROR_UNSUPPORTED_TAG:
        return "KM_ERROR_UNSUPPORTED_TAG";
    case KM_ERROR_INVALID_TAG:
        return "KM_ERROR_INVALID_TAG";
    case KM_ERROR_MEMORY_ALLOCATION_FAILED:
        return "KM_ERROR_MEMORY_ALLOCATION_FAILED";
    case KM_ERROR_IMPORT_PARAMETER_MISMATCH:
        return "KM_ERROR_IMPORT_PARAMETER_MISMATCH";
    case KM_ERROR_SECURE_HW_ACCESS_DENIED:
        return "KM_ERROR_SECURE_HW_ACCESS_DENIED";
    case KM_ERROR_OPERATION_CANCELLED:
        return "KM_ERROR_OPERATION_CANCELLED";
    case KM_ERROR_CONCURRENT_ACCESS_CONFLICT:
        return "KM_ERROR_CONCURRENT_ACCESS_CONFLICT";
    case KM_ERROR_SECURE_HW_BUSY:
        return "KM_ERROR_SECURE_HW_BUSY";
    case KM_ERROR_SECURE_HW_COMMUNICATION_FAILED:
        return "KM_ERROR_SECURE_HW_COMMUNICATION_FAILED";
    case KM_ERROR_UNSUPPORTED_EC_FIELD:
        return "KM_ERROR_UNSUPPORTED_EC_FIELD";
    case KM_ERROR_MISSING_NONCE:
        return "KM_ERROR_MISSING_NONCE";
    case KM_ERROR_INVALID_NONCE:
        return "KM_ERROR_INVALID_NONCE";
    case KM_ERROR_MISSING_MAC_LENGTH:
        return "KM_ERROR_MISSING_MAC_LENGTH";
    case KM_ERROR_KEY_RATE_LIMIT_EXCEEDED:
        return "KM_ERROR_KEY_RATE_LIMIT_EXCEEDED";
    case KM_ERROR_CALLER_NONCE_PROHIBITED:
        return "KM_ERROR_CALLER_NONCE_PROHIBITED";
    case KM_ERROR_KEY_MAX_OPS_EXCEEDED:
        return "KM_ERROR_KEY_MAX_OPS_EXCEEDED";
    case KM_ERROR_INVALID_MAC_LENGTH:
        return "KM_ERROR_INVALID_MAC_LENGTH";
    case KM_ERROR_MISSING_MIN_MAC_LENGTH:
        return "KM_ERROR_MISSING_MIN_MAC_LENGTH";
    case KM_ERROR_UNSUPPORTED_MIN_MAC_LENGTH:
        return "KM_ERROR_UNSUPPORTED_MIN_MAC_LENGTH";
    case KM_ERROR_UNSUPPORTED_KDF:
        return "KM_ERROR_UNSUPPORTED_KDF";
    case KM_ERROR_UNSUPPORTED_EC_CURVE:
        return "KM_ERROR_UNSUPPORTED_EC_CURVE";
    case KM_ERROR_KEY_REQUIRES_UPGRADE:
        return "KM_ERROR_KEY_REQUIRES_UPGRADE";
    case KM_ERROR_ATTESTATION_CHALLENGE_MISSING:
        return "KM_ERROR_ATTESTATION_CHALLENGE_MISSING";
    case KM_ERROR_KEYMASTER_NOT_CONFIGURED:
        return "KM_ERROR_KEYMASTER_NOT_CONFIGURED";
    case KM_ERROR_ATTESTATION_APPLICATION_ID_MISSING:
        return "KM_ERROR_ATTESTATION_APPLICATION_ID_MISSING";
    case KM_ERROR_CANNOT_ATTEST_IDS:
        return "KM_ERROR_CANNOT_ATTEST_IDS";
    case KM_ERROR_ROLLBACK_RESISTANCE_UNAVAILABLE:
        return "KM_ERROR_ROLLBACK_RESISTANCE_UNAVAILABLE";
    case KM_ERROR_NO_USER_CONFIRMATION:
        return "KM_ERROR_NO_USER_CONFIRMATION";
    case KM_ERROR_DEVICE_LOCKED:
        return "KM_ERROR_DEVICE_LOCKED";
    case KM_ERROR_EARLY_BOOT_ENDED:
        return "KM_ERROR_EARLY_BOOT_ENDED";
    case KM_ERROR_ATTESTATION_KEYS_NOT_PROVISIONED:
        return "KM_ERROR_ATTESTATION_KEYS_NOT_PROVISIONED";
    case KM_ERROR_ATTESTATION_IDS_NOT_PROVISIONED:
        return "KM_ERROR_ATTESTATION_IDS_NOT_PROVISIONED";
    case KM_ERROR_INCOMPATIBLE_MGF_DIGEST:
        return "KM_ERROR_INCOMPATIBLE_MGF_DIGEST";
    case KM_ERROR_UNSUPPORTED_MGF_DIGEST:
        return "KM_ERROR_UNSUPPORTED_MGF_DIGEST";
    case KM_ERROR_MISSING_NOT_BEFORE:
        return "KM_ERROR_MISSING_NOT_BEFORE";
    case KM_ERROR_MISSING_NOT_AFTER:
        return "KM_ERROR_MISSING_NOT_AFTER";
    case KM_ERROR_MISSING_ISSUER_SUBJECT:
        return "KM_ERROR_MISSING_ISSUER_SUBJECT";
    case KM_ERROR_INVALID_ISSUER_SUBJECT:
        return "KM_ERROR_INVALID_ISSUER_SUBJECT";
    case KM_ERROR_BOOT_LEVEL_EXCEEDED:
        return "KM_ERROR_BOOT_LEVEL_EXCEEDED";

    case KM_ERROR_UNIMPLEMENTED:
        return "KM_ERROR_UNIMPLEMENTED";
    case KM_ERROR_VERSION_MISMATCH:
        return "KM_ERROR_VERSION_MISMATCH";

    case KM_ERROR_UNKNOWN_ERROR:
        return "KM_ERROR_UNKNOWN_ERROR";

    default:
        if (error_code == -999)
            return "KM_ERROR_AW_TRUSTTRAIN_NOT_READY";
        else
            return "NONE AOSP ERROR";
    }
}
}  // namespace

#define TEE_KEYMASTER_KEY_INOUT_COMMON_SIZE (70 * 1024)

AWTACommunicator::AWTACommunicator(AWKeymasterLogger* logger, int32_t message_version)
    : logger_(logger), MessageVersion_(message_version) {
    pTeeContext = NULL;
    pTeeMutex = NULL;
    pTeeSession = NULL;
    logger_->log("message_version:%d\n", MessageVersion_);
}

AWTACommunicator::~AWTACommunicator() {}
int AWTACommunicator::LoadTa(const uint8_t* TA_UUID) {
    ConfigureRequest req(MessageVersion_);
    ConfigureResponse rsp(MessageVersion_);
    TEEC_Result success;
    logger_->log("load ta:");
    logger_->dumphex(TA_UUID, sizeof(TEEC_UUID));
    pTeeMutex = malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init((pthread_mutex_t*)pTeeMutex, NULL);
    pthread_mutex_lock((pthread_mutex_t*)pTeeMutex);
    if (pTeeContext != NULL) {
        pthread_mutex_unlock((pthread_mutex_t*)pTeeMutex);
        logger_->log("ta already loaded");
        goto repeated;
    }
    pTeeContext = (TEEC_Context*)malloc(sizeof(TEEC_Context));
    pTeeSession = (TEEC_Session*)malloc(sizeof(TEEC_Session));
    assert(pTeeContext != NULL);
    assert(pTeeSession != NULL);
    pthread_mutex_unlock((pthread_mutex_t*)pTeeMutex);
    success = TEEC_InitializeContext(NULL, (TEEC_Context*)pTeeContext);
    if (success != TEEC_SUCCESS) {
        logger_->log(ANDROID_LOG_ERROR, "initialize context failed with %d", success);
        goto init_context_failed;
    }
    success = TEEC_OpenSession((TEEC_Context*)pTeeContext, (TEEC_Session*)pTeeSession,
                               (const TEEC_UUID*)&TA_UUID[0], TEEC_LOGIN_PUBLIC, NULL, NULL, NULL);
    if (success != TEEC_SUCCESS) {
        logger_->log(ANDROID_LOG_ERROR, "open session failed with %d", success);
        goto failed;
    }

    /*set os version as well*/
    req.os_version = ::keymaster::GetOsVersion();
    req.os_patchlevel = ::keymaster::GetOsPatchlevel();
    InvokeTaCommand(MSG_KEYMASTER_V2_CONFIGURE, req, &rsp);
    if (rsp.error != KM_ERROR_OK) {
        logger_->log(ANDROID_LOG_ERROR, "keymaster set up failed with %d", rsp.error);
        goto failed;
    }

    return 0;
failed:
    TEEC_FinalizeContext((TEEC_Context*)pTeeContext);
init_context_failed:
    free(pTeeContext);
    free(pTeeSession);
    pTeeContext = NULL;
    pTeeSession = NULL;
repeated:
    free(pTeeMutex);
    pTeeMutex = NULL;
    return -1;
}

int AWTACommunicator::InvokeTaCommand(int commandID, const ::keymaster::Serializable& req,
                                      ::keymaster::KeymasterResponse* rsp) {
    uint32_t req_size = req.SerializedSize();
    if (req_size > TEE_KEYMASTER_KEY_INOUT_COMMON_SIZE) {
        logger_->log(ANDROID_LOG_ERROR, "Request too big: %u Max size: %u", req_size,
                     TEE_KEYMASTER_KEY_INOUT_COMMON_SIZE);
        rsp->error = KM_ERROR_INVALID_INPUT_LENGTH;
        return KM_ERROR_INVALID_INPUT_LENGTH;
    }

    TEEC_SharedMemory TeeInputBuffer;
    TeeInputBuffer.size = TEE_KEYMASTER_KEY_INOUT_COMMON_SIZE;
    TeeInputBuffer.flags = TEEC_MEM_INPUT;
    if (TEEC_AllocateSharedMemory((TEEC_Context*)pTeeContext, &TeeInputBuffer) != TEEC_SUCCESS) {
        logger_->log(ANDROID_LOG_ERROR, "%s: allocate request share memory fail", __func__);
        rsp->error = KM_ERROR_UNKNOWN_ERROR;
        return KM_ERROR_UNKNOWN_ERROR;
    }
    req.Serialize((uint8_t*)TeeInputBuffer.buffer,
                  (uint8_t*)TeeInputBuffer.buffer + TeeInputBuffer.size);

    TEEC_SharedMemory TeeOutputBuffer;
    TeeOutputBuffer.size = TEE_KEYMASTER_KEY_INOUT_COMMON_SIZE;
    TeeOutputBuffer.flags = TEEC_MEM_INPUT | TEEC_MEM_OUTPUT;
    if (TEEC_AllocateSharedMemory((TEEC_Context*)pTeeContext, &TeeOutputBuffer) != TEEC_SUCCESS) {
        logger_->log(ANDROID_LOG_ERROR, "%s: allocate respone share memory fail", __func__);
        TEEC_ReleaseSharedMemory(&TeeInputBuffer);
        rsp->error = KM_ERROR_UNKNOWN_ERROR;
        return KM_ERROR_UNKNOWN_ERROR;
    }

    TEEC_Operation operation;
    memset(&operation, 0, sizeof(TEEC_Operation));
    operation.paramTypes =
        TEEC_PARAM_TYPES(TEEC_MEMREF_WHOLE, TEEC_MEMREF_WHOLE, TEEC_VALUE_INOUT, TEEC_NONE);
    operation.started = 1;

    operation.params[0].memref.parent = &TeeInputBuffer;
    operation.params[0].memref.offset = 0;
    operation.params[0].memref.size = 0;

    operation.params[1].memref.parent = &TeeOutputBuffer;
    operation.params[1].memref.offset = 0;
    operation.params[1].memref.size = 0;

    operation.params[2].value.a = req_size;
    logger_->log(ANDROID_LOG_INFO, "km tee command:0x%x", commandID);

    int success = TEEC_InvokeCommand((TEEC_Session*)pTeeSession, commandID, &operation, NULL);
    if (success != TEEC_SUCCESS) {
        logger_->log(ANDROID_LOG_INFO, "tee smc failed with:0x%x:%s", success,
                     TeecErrorString(success));
        rsp->error = (keymaster_error_t)success;
    }
    uint32_t rsp_size = operation.params[2].value.b;
    const uint8_t* p = (uint8_t*)TeeOutputBuffer.buffer;
    if (!rsp->Deserialize(&p, p + rsp_size)) {
        logger_->log(ANDROID_LOG_ERROR, "Error deserializing response of size %d\n", (int)rsp_size);
    } else if (rsp->error != KM_ERROR_OK) {
        logger_->log(ANDROID_LOG_ERROR, "Response of size %d contained error code %d:%s\n",
                     (int)rsp_size, (int)rsp->error, KmErrorCodeString(rsp->error));
    }
    TEEC_ReleaseSharedMemory(&TeeInputBuffer);
    TEEC_ReleaseSharedMemory(&TeeOutputBuffer);
    return success;
}

int AWTACommunicator::InvokeTaCommand(int commandID, ::keymaster::KeymasterResponse* rsp) {
    AddEntropyRequest dummyrequest(MessageVersion_);
    return InvokeTaCommand(commandID, dummyrequest, rsp);
}

}  // namespace keymaster
}  // namespace hardware
}  // namespace aw
