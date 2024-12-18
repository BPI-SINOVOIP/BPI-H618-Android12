#include <tee_ta_api.h>
#include <tee_internal_api.h>
#include <utee_syscalls.h>
#include <stdio.h>
#include <string.h>
#include <util.h>
#include <base64.h>

/*
 * Trusted Application Entry Points
 */

/* Called each time a new instance is created */
TEE_Result TA_CreateEntryPoint(void)
{
	return TEE_SUCCESS;
}

/* Called each time an instance is destroyed */
void TA_DestroyEntryPoint(void)
{
}

/* Called each time a session is opened */
TEE_Result TA_OpenSessionEntryPoint(uint32_t nParamTypes, TEE_Param pParams[4],
				    void **ppSessionContext)
{
	(void)nParamTypes;
	(void)pParams;
	(void)ppSessionContext;
	return TEE_SUCCESS;
}

/* Called each time a session is closed */
void TA_CloseSessionEntryPoint(void *pSessionContext)
{
	(void)pSessionContext;
}

void dump(uint8_t *buf, int ttl_len);
#if 1
void dump(uint8_t *buf, int ttl_len)
{
	int len;
	for (len = 0; len < ttl_len; len++) {
		printf("0x%02x ", ((char *)buf)[len]);
		if (len % 8 == 7) {
			printf("\n");
		}
	}
	printf("\n");
}
static __maybe_unused void sunxi_dump(void *buf, int len)
{
	char line[50];
	char *tmp = line;
	char *in  = (char *)buf;
	int i;
	EMSG_RAW("inbuf:%p\n", buf);
	for (i = 0; i < len; i++) {
		snprintf(tmp, 5, "%02x ", *((uint8_t *)in));
		tmp += 3;
		if (((i % 16) == 15)) {
			EMSG_RAW("%s\n", line);
			tmp = line;
		}
		in++;
	}
	if ((i & (16 - 1)) != 0) {
		EMSG_RAW("%s\n", line);
	}
}
#define TA_DBG EMSG
#else
void __maybe_unused dump(__maybe_unused uint8_t *buf,
			 __maybe_unused int ttl_len)
{
	;
}
static __maybe_unused void sunxi_dump(__maybe_unused void *buf,
				      __maybe_unused int len)
{
	;
}
#define TA_DBG(...)
#endif
const char *ResultCodeToStr(TEE_Result resultCode);
TEE_Result HMAC_operation(uint8_t *key, uint32_t key_len, uint8_t *data_in,
			  uint32_t data_in_len, uint8_t *data_out,
			  uint32_t *data_out_len);
TEE_Result AES_operaction(uint8_t *key, uint32_t key_len, uint8_t *iv,
			  uint32_t iv_len, uint8_t *data_in,
			  uint32_t data_in_len, uint8_t *data_out,
			  uint32_t *data_out_len, uint8_t encrypt);
static TEE_Result generate_huk(uint8_t *key_buf, uint32_t *key_len)
{
	TEE_Result res;
	uint8_t rdbuf[200];
	uint8_t rd_len;
	res = utee_sunxi_read_efuse((const char *)"chipid", &rd_len, rdbuf);
	if (res == TEE_SUCCESS) {
		TA_DBG("read chipid:\n");
		dump(rdbuf, rd_len);
	} else {
		TA_DBG("read failed, return:%s\n", ResultCodeToStr(res));
	}
	memset(rdbuf + rd_len, 0x35, 24 - rd_len);
	rd_len = 24;
	memset(key_buf, 0, *key_len);
	res = HMAC_operation(rdbuf, rd_len,
			     (uint8_t *)"DERIVE A KEY FROM CHIP ID",
			     sizeof("DRIVE A KEY FROM CHIP ID"), key_buf,
			     key_len);
	if (res == TEE_SUCCESS) {
		TA_DBG("derive key:\n");
		dump(key_buf, *key_len);
	} else {
		TA_DBG("derive failed, return:%s\n", ResultCodeToStr(res));
	}
	return res;
}

uint8_t input_key[96];
size_t input_key_len;
uint8_t call_cnt=0;
/* Called when a command is invoked */
TEE_Result TA_InvokeCommandEntryPoint(void *pSessionContext,
				      uint32_t nCommandID, uint32_t nParamTypes,
				      TEE_Param pParams[4])
{
	uint8_t key_buf[64];
	uint32_t key_buf_len;
	TEE_Result res;
	uint8_t tmp_buf[96];
	size_t base64_len;
	uint32_t rlt_size;
	uint8_t key_for_key1_to_key2[16]={
		1,2,3,4,5,6,7,8,
		9,10,11,12,13,14,15,16
	};
	uint8_t key_for_key2_to_key3[16]={
		15,14,13,12,11,10,9,
		8,7,6,5,4,3,2,1
	};
	uint8_t key2_buf[96];
	uint32_t key2_len=96;
	uint8_t key3_buf[96];
	uint32_t key3_len=96;

	(void)pSessionContext;
	(void)nParamTypes;
	(void)pParams;

	call_cnt++;
	TA_DBG("call_cnt:%d",call_cnt);
	TA_DBG("TA:rec cmd 0x%x\n", nCommandID);
	switch (nCommandID) {
	case 0x210:
	case 0x211:
		key_buf_len = sizeof(key_buf);
		generate_huk(key_buf, &key_buf_len);
		key_buf_len = 16;

		rlt_size = pParams[1].memref.size;
		if (nCommandID == 0x210) {
			res = AES_operaction(
				key_buf, key_buf_len,
				(uint8_t *)"AES IV:DUMMY IV IS RPOVIDED\n", 16,
				pParams[0].memref.buffer, pParams[2].value.a,
				pParams[1].memref.buffer, &rlt_size, 1);
		} else {
			res = AES_operaction(
				key_buf, key_buf_len,
				(uint8_t *)"AES IV:DUMMY IV IS RPOVIDED\n", 16,
				pParams[0].memref.buffer, pParams[2].value.a,
				pParams[1].memref.buffer, &rlt_size, 0);
		}
		if (res == TEE_SUCCESS) {
			TA_DBG("aes cbc result:\n");
		} else {
			TA_DBG("aes cbc failed, return:%s\n",
			       ResultCodeToStr(res));
		}
		pParams[2].value.b = rlt_size;
		return res;
	case 0x220:
		key_buf_len = sizeof(key_buf);
		generate_huk(key_buf, &key_buf_len);
		key_buf_len = 16;

		rlt_size = sizeof(input_key);
		res      = AES_operaction(key_buf, key_buf_len,
				     (uint8_t *)"AES IV:DUMMY IV IS RPOVIDED\n",
				     16, pParams[0].memref.buffer,
				     pParams[2].value.a, input_key, &rlt_size,
				     0);
		if (res == TEE_SUCCESS) {
			TA_DBG("decrypted input key:\n");
			sunxi_dump(input_key, rlt_size);
		} else {
			TA_DBG("aes cbc failed, return:%s\n",
			       ResultCodeToStr(res));
		}

		if (!EVP_DecodeBase64(tmp_buf, &base64_len, sizeof(tmp_buf),
				      input_key, 24)) {
			return TEE_ERROR_GENERIC;
		}
		TA_DBG("base64 decoded key: len:%d\n", base64_len);
		sunxi_dump(tmp_buf, base64_len);
		memcpy(input_key, tmp_buf, base64_len);
		input_key_len=base64_len;

		return TEE_SUCCESS;
	case 0x221:
		res = AES_operaction(key_for_key1_to_key2, 16,
				     (uint8_t *)"AES IV:DUMMY IV IS RPOVIDED\n",
				     16, input_key,
				     input_key_len,
				     key2_buf, &key2_len, 0);
		if (res != TEE_SUCCESS) {
			TA_DBG("operation failed with:%s\n",
			       ResultCodeToStr(res));
		}

		res = AES_operaction(key_for_key2_to_key3, 16,
				     (uint8_t *)"AES IV:DUMMY IV IS RPOVIDED\n",
				     16, key2_buf,
				     key2_len,
				     key3_buf, &key3_len, 1);
		if (res != TEE_SUCCESS) {
			TA_DBG("operation failed with:%s\n",
			       ResultCodeToStr(res));
		}

		rlt_size=pParams[1].memref.size;
		memset(key3_buf + key3_len, 0x35, 24 - key3_len);
		key3_len = 24;
		res = HMAC_operation(key3_buf, key3_len,
				     pParams[0].memref.buffer,
				     pParams[2].value.a,
				     pParams[1].memref.buffer, &rlt_size);
		pParams[2].value.b=rlt_size;
		if (res != TEE_SUCCESS) {
			TA_DBG("operation failed with:%s\n",
			       ResultCodeToStr(res));
		}
		return res;
	default:
		return TEE_ERROR_BAD_PARAMETERS;
	}
}

const char *ResultCodeToStr(TEE_Result resultCode)
{
	switch (resultCode) {
	case 0x00000000:
		return "TEE_SUCCESS";
	case 0xF0100001:
		return "TEE_ERROR_CORRUPT_OBJECT";
	case 0xF0100002:
		return "TEE_ERROR_CORRUPT_OBJECT_2";
	case 0xF0100003:
		return "TEE_ERROR_STORAGE_NOT_AVAILABLE";
	case 0xF0100004:
		return "TEE_ERROR_STORAGE_NOT_AVAILABLE_2";
	case 0xFFFF0000:
		return "TEE_ERROR_GENERIC";
	case 0xFFFF0001:
		return "TEE_ERROR_ACCESS_DENIED";
	case 0xFFFF0002:
		return "TEE_ERROR_CANCEL";
	case 0xFFFF0003:
		return "TEE_ERROR_ACCESS_CONFLICT";
	case 0xFFFF0004:
		return "TEE_ERROR_EXCESS_DATA";
	case 0xFFFF0005:
		return "TEE_ERROR_BAD_FORMAT";
	case 0xFFFF0006:
		return "TEE_ERROR_BAD_PARAMETERS";
	case 0xFFFF0007:
		return "TEE_ERROR_BAD_STATE";
	case 0xFFFF0008:
		return "TEE_ERROR_ITEM_NOT_FOUND";
	case 0xFFFF0009:
		return "TEE_ERROR_NOT_IMPLEMENTED";
	case 0xFFFF000A:
		return "TEE_ERROR_NOT_SUPPORTED";
	case 0xFFFF000B:
		return "TEE_ERROR_NO_DATA";
	case 0xFFFF000C:
		return "TEE_ERROR_OUT_OF_MEMORY";
	case 0xFFFF000D:
		return "TEE_ERROR_BUSY";
	case 0xFFFF000E:
		return "TEE_ERROR_COMMUNICATION";
	case 0xFFFF000F:
		return "TEE_ERROR_SECURITY";
	case 0xFFFF0010:
		return "TEE_ERROR_SHORT_BUFFER";
	case 0xFFFF0011:
		return "TEE_ERROR_EXTERNAL_CANCEL";
	case 0xFFFF300F:
		return "TEE_ERROR_OVERFLOW";
	case 0xFFFF3024:
		return "TEE_ERROR_TARGET_DEAD";
	case 0xFFFF3041:
		return "TEE_ERROR_STORAGE_NO_SPACE";
	case 0xFFFF3071:
		return "TEE_ERROR_MAC_INVALID";
	case 0xFFFF3072:
		return "TEE_ERROR_SIGNATURE_INVALID";
	case 0xFFFF5000:
		return "TEE_ERROR_TIME_NOT_SET";
	case 0xFFFF5001:
		return "TEE_ERROR_TIME_NEEDS_RESET";
	default:
		TA_DBG("unknown code:%d\n", resultCode);
		return "\n";
	}
}

TEE_Result HMAC_operation(uint8_t *key, uint32_t key_len, uint8_t *data_in,
			  uint32_t data_in_len, uint8_t *data_out,
			  uint32_t *data_out_len)
{
	TEE_Result res;
	TEE_OperationHandle op;
	TEE_Attribute attrs;
	TEE_ObjectHandle key_handle;
	size_t key_size;

	attrs.attributeID	= TEE_ATTR_SECRET_VALUE;
	attrs.content.ref.buffer = (void *)key;
	attrs.content.ref.length = key_len;
	/*key size use for initialization is in bit*/
	key_size = attrs.content.ref.length * 8;

	TA_DBG("key data: len:%d\n", key_len);
	sunxi_dump((void *)key, key_len);

	/*init operation contex*/
	res = TEE_AllocateOperation(&op, TEE_ALG_HMAC_SHA256, TEE_MODE_MAC,
				    key_size);
	if (res != TEE_SUCCESS) {
		TA_DBG("operation failed with:%s\n", ResultCodeToStr(res));
	}

	/*set up key used in operaction*/
	res = TEE_AllocateTransientObject(TEE_TYPE_HMAC_SHA256, key_size,
					  &key_handle);
	if (res != TEE_SUCCESS) {
		TA_DBG("operation failed with:%s\n", ResultCodeToStr(res));
	}
	res = TEE_PopulateTransientObject(key_handle, &attrs, 1);
	if (res != TEE_SUCCESS) {
		TA_DBG("operation failed with:%s\n", ResultCodeToStr(res));
	}
	res = TEE_SetOperationKey(op, key_handle);
	if (res != TEE_SUCCESS) {
		TA_DBG("operation failed with:%s\n", ResultCodeToStr(res));
	}
	TEE_FreeTransientObject(key_handle);

	/*excute operaction*/
	TEE_MACInit(op, NULL, 0);

	/*indicate max size of out buffer*/
	res = TEE_MACComputeFinal(op, data_in, data_in_len, data_out,
				  data_out_len);
	if (res != TEE_SUCCESS) {
		TA_DBG("operation failed with:%s\n", ResultCodeToStr(res));
	}

	TA_DBG("raw data: len:%d\n", data_in_len);
	sunxi_dump((void *)data_in, data_in_len);
	TA_DBG("hmac result: len:%d\n", *data_out_len);
	sunxi_dump((void *)data_out, *data_out_len);

	TEE_FreeOperation(op);
	return res;
}

TEE_Result AES_operaction(uint8_t *key, uint32_t key_len, uint8_t *iv,
			  uint32_t iv_len, uint8_t *data_in,
			  uint32_t data_in_len, uint8_t *data_out,
			  uint32_t *data_out_len, uint8_t encrypt)
{
	TEE_OperationHandle op;
	TEE_ObjectHandle key1_handle = TEE_HANDLE_NULL;
	TEE_Attribute key_attr;
	size_t key_size;
	size_t op_key_size;
	TEE_Result res;

	uint32_t direction = TEE_MODE_ENCRYPT;
	if (encrypt) {
		direction = TEE_MODE_ENCRYPT;
	} else {
		direction = TEE_MODE_DECRYPT;
	}

	key_attr.attributeID	= TEE_ATTR_SECRET_VALUE;
	key_attr.content.ref.buffer = (void *)key;
	key_attr.content.ref.length = key_len;

	key_size = key_attr.content.ref.length * 8;

	op_key_size = key_size;

	TA_DBG("key data: len:%d\n", key_len);
	sunxi_dump((void *)key, key_len);

	res = TEE_AllocateOperation(&op, TEE_ALG_AES_CBC_NOPAD, direction,
				    op_key_size);
	if (res != TEE_SUCCESS) {
		TA_DBG("operation failed with:%s\n", ResultCodeToStr(res));
	}

	res = TEE_AllocateTransientObject(TEE_TYPE_AES, key_size, &key1_handle);
	if (res != TEE_SUCCESS) {
		TA_DBG("operation failed with:%s\n", ResultCodeToStr(res));
	}

	res = TEE_PopulateTransientObject(key1_handle, &key_attr, 1);
	if (res != TEE_SUCCESS) {
		TA_DBG("operation failed with:%s\n", ResultCodeToStr(res));
	}

	TEE_SetOperationKey(op, key1_handle);

	TEE_FreeTransientObject(key1_handle);
	key1_handle = TEE_HANDLE_NULL;

	TEE_CipherInit(op, iv, iv_len);

	res = TEE_CipherDoFinal(op, data_in, data_in_len, data_out,
				data_out_len);
	if (res != TEE_SUCCESS) {
		TA_DBG("operation failed with:%s\n", ResultCodeToStr(res));
	}

	TA_DBG("raw data: len:%d\n", data_in_len);
	sunxi_dump((void *)data_in, data_in_len);
	TA_DBG("result: len:%d\n", *data_out_len);
	sunxi_dump((void *)data_out, *data_out_len);

	TEE_FreeOperation(op);

	return res;
}
