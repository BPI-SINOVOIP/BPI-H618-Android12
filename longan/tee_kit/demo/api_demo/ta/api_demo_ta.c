#include <tee_ta_api.h>
#include <tee_internal_api.h>
#include <utee_syscalls.h>
#include <stdio.h>
#include <string.h>
#include <util.h>

/*
 * Trusted Application Entry Points
 */

/* Called each time a new instance is created */
TEE_Result TA_CreateEntryPoint(void)
{
	printf("TA:creatyentry!\n");
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
	printf("TA:open session!\n");
	return TEE_SUCCESS;
}

/* Called each time a session is closed */
void TA_CloseSessionEntryPoint(void *pSessionContext)
{
	(void)pSessionContext;
}

	static const uint8_t mac_data_sha256_key[24] = {
		0x6B, 0x65, 0x79, /* key */
	};

	static const uint8_t mac_data_sha256_in[] = {
		0x54, 0x68, 0x65, 0x20, 0x71, 0x75, 0x69, 0x63, /* The quic */
		0x6B, 0x20, 0x62, 0x72, 0x6F, 0x77, 0x6E, 0x20, /* k brown  */
		0x66, 0x6F, 0x78, 0x20, 0x6A, 0x75, 0x6D, 0x70, /* fox jump */
		0x73, 0x20, 0x6F, 0x76, 0x65, 0x72, 0x20, 0x74, /* s over t */
		0x68, 0x65, 0x20, 0x6C, 0x61, 0x7A, 0x79, 0x20, /* he lazy  */
		0x64, 0x6F, 0x67, /* dog */
	};

	static const uint8_t mac_data_sha256_out[] = {
		0xF7, 0xBC, 0x83, 0xF4, 0x30, 0x53, 0x84, 0x24, /* ....0S.$ */
		0xB1, 0x32, 0x98, 0xE6, 0xAA, 0x6F, 0xB1, 0x43, /* .2...o.C */
		0xEF, 0x4D, 0x59, 0xA1, 0x49, 0x46, 0x17, 0x59, /* .MY.IF.Y */
		0x97, 0x47, 0x9D, 0xBC, 0x2D, 0x1A, 0x3C, 0xD8, /* .G..-.<. */
	};
static const uint8_t ciph_data_aes_key1[] = {
	0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, /* 01234567 */
	0x38, 0x39, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, /* 89ABCDEF */
};
static const uint8_t ciph_data_128_iv1[] = {
	0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, /* 12345678 */
	0x39, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x30, /* 9ABCDEF0 */
};
static const uint8_t ciph_data_in1[] = {
	0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, /* 23456789 */
	0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x30, 0x31, /* ABCDEF01 */
	0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x41, /* 3456789A */
	0x42, 0x43, 0x44, 0x45, 0x46, 0x30, 0x31, 0x32, /* BCDEF012 */
	0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x41, 0x42, /* 456789AB */
	0x43, 0x44, 0x45, 0x46, 0x30, 0x31, 0x32, 0x33, /* CDEF0123 */
};
static const uint8_t ciph_data_aes_cbc_nopad_out1[] = {
	0x8D, 0x9F, 0x88, 0xD8, 0xAF, 0x9F, 0xC1, 0x3B, /* .......; */
	0x02, 0x15, 0x43, 0x6A, 0x8C, 0x1E, 0x34, 0x5C, /* ..Cj..4\ */
	0x83, 0xF4, 0x85, 0x3E, 0x43, 0x0F, 0xE5, 0x5F, /* ...>C.._ */
	0x81, 0x4C, 0xC0, 0x28, 0x3F, 0xD9, 0x98, 0x53, /* .L.(?..S */
	0xB1, 0x44, 0x51, 0x38, 0x21, 0xAB, 0x10, 0xCE, /* .DQ8!... */
	0xC2, 0xEC, 0x65, 0x54, 0xDD, 0x5C, 0xEA, 0xDC, /* ..eT.\.. */
};

void dump(uint8_t *buf, int ttl_len);
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
TEE_Result HMAC_operation(uint8_t *key, uint32_t key_len, uint8_t *data_in,
			  uint32_t data_in_len, uint8_t *data_out,
			  uint32_t *data_out_len);
TEE_Result AES_operaction(uint8_t *key, uint32_t key_len, uint8_t *iv,
			  uint32_t iv_len, uint8_t *data_in,
			  uint32_t data_in_len, uint8_t *data_out,
			  uint32_t *data_out_len, uint8_t encrypt);
/* Called when a command is invoked */
TEE_Result TA_InvokeCommandEntryPoint(void *pSessionContext,
				      uint32_t nCommandID, uint32_t nParamTypes,
				      TEE_Param pParams[4])
{
	char *strsrc = (char *)pParams[0].memref.buffer;
	uint8_t rdbuf[200];
	int i;
	uint8_t rd_len;
	uint8_t keyname[50] = "chipid";
	uint8_t *wrBuf      = (uint8_t *)pParams[1].memref.buffer;
	uint8_t wr_len      = (uint8_t)pParams[2].value.a;
	uint32_t rlt_size;
	uint8_t rlt_buf[2048];

	(void)pSessionContext;
	(void)nParamTypes;
	(void)pParams;
	printf("TA:rec cmd 0x%x\n", nCommandID);
	switch (nCommandID) {
	case 0x210:
		if (strsrc[0] != 0) {
			for (i = 0; i < 49; i++) {
				if (strsrc[i] == 0)
					break;
				keyname[i] = strsrc[i];
			}
			keyname[i] = 0;
		}
		printf("read efuse:%s\n", keyname);

		i = utee_sunxi_read_efuse((const char *)keyname, &rd_len,
					  rdbuf);
		if (i == TEE_SUCCESS) {
			printf("read result:\n");
			dump(rdbuf, rd_len);
		} else {
			printf("read failed, return:%x\n", i);
		}
		memcpy(wrBuf, rdbuf, rd_len);
		pParams[2].value.a = rd_len;

		return i;
	case 0x220:
		if (strsrc[0] != 0) {
			for (i = 0; i < 49; i++) {
				if (strsrc[i] == 0)
					break;
				keyname[i] = strsrc[i];
			}
			keyname[i] = 0;
		} else {
			memcpy(keyname, "testkey", sizeof("testkey"));
			keyname[49] = 0;
		}
		i = utee_sunxi_keybox((const char *)keyname, rdbuf, 16);
		if (i != TEE_SUCCESS) {
			printf("read key:%s from keybox failed with:%d",
			       keyname, i);
			return i;
		} else {
			i = utee_sunxi_read_efuse("oem_secure", &rd_len,
						  rdbuf + 16);
			if (i == TEE_SUCCESS) {
				printf("read result:\n");
				dump(rdbuf, rd_len + 16);
			} else {
				printf("read efuse failed with:%x\n", i);
			}
		}
		return i;
	case 0x221:
		if (strsrc[0] != 0) {
			for (i = 0; i < 49; i++) {
				if (strsrc[i] == 0)
					break;
				keyname[i] = strsrc[i];
			}
			keyname[i] = 0;
		} else {
			printf("invalid key name\n");
			return TEE_ERROR_BAD_PARAMETERS;
			memcpy(keyname, "testkey", sizeof("testkey"));
			keyname[49] = 0;
		}
		printf("keyname:%s,key len:%d,keydata:\n", keyname, wr_len);
		dump(wrBuf, wr_len);
		i = utee_sunxi_write_efuse((const char *)keyname, wr_len,
					   wrBuf);
		if (i != TEE_SUCCESS) {
			printf("burn efuse:%s failed with:%d\n", keyname, i);
			return i;
		}
	case 0x230:
		rlt_size=ARRAY_SIZE(rlt_buf);
		return HMAC_operation((uint8_t*)mac_data_sha256_key,ARRAY_SIZE(mac_data_sha256_key),
				(uint8_t*)mac_data_sha256_in,ARRAY_SIZE(mac_data_sha256_in),
				rlt_buf,&rlt_size);
	case 0x231:
		rlt_size=ARRAY_SIZE(rlt_buf);
		AES_operaction((uint8_t*)ciph_data_aes_key1,ARRAY_SIZE(ciph_data_aes_key1),
				(uint8_t*)ciph_data_128_iv1,ARRAY_SIZE(ciph_data_128_iv1),
				(uint8_t*)ciph_data_aes_cbc_nopad_out1,ARRAY_SIZE(ciph_data_aes_cbc_nopad_out1),
				rlt_buf,&rlt_size,0);
		rlt_size=ARRAY_SIZE(rlt_buf);
		AES_operaction((uint8_t*)ciph_data_aes_key1,ARRAY_SIZE(ciph_data_aes_key1),
				(uint8_t*)ciph_data_128_iv1,ARRAY_SIZE(ciph_data_128_iv1),
				(uint8_t*)ciph_data_in1,ARRAY_SIZE(ciph_data_in1),
				rlt_buf,&rlt_size,1);
		return TEE_SUCCESS;
	default:
		return TEE_ERROR_BAD_PARAMETERS;
	}
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

const char *ResultCodeToStr(TEE_Result resultCode);
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
		return "unknown coed";
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

	EMSG("key data: len:%d\n", key_len);
	sunxi_dump((void *)key, key_len);

	/*init operation contex*/
	res = TEE_AllocateOperation(&op, TEE_ALG_HMAC_SHA256, TEE_MODE_MAC,
				    key_size);
	if (res != TEE_SUCCESS) {
		EMSG("operation failed with:%s\n", ResultCodeToStr(res));
	}

	/*set up key used in operaction*/
	res = TEE_AllocateTransientObject(TEE_TYPE_HMAC_SHA256, key_size,
					  &key_handle);
	if (res != TEE_SUCCESS) {
		EMSG("operation failed with:%s\n", ResultCodeToStr(res));
	}
	res = TEE_PopulateTransientObject(key_handle, &attrs, 1);
	if (res != TEE_SUCCESS) {
		EMSG("operation failed with:%s\n", ResultCodeToStr(res));
	}
	res = TEE_SetOperationKey(op, key_handle);
	if (res != TEE_SUCCESS) {
		EMSG("operation failed with:%s\n", ResultCodeToStr(res));
	}
	TEE_FreeTransientObject(key_handle);

	/*excute operaction*/
	TEE_MACInit(op, NULL, 0);

	/*indicate max size of out buffer*/
	res = TEE_MACComputeFinal(op, data_in, data_in_len, data_out,
				  data_out_len);
	if (res != TEE_SUCCESS) {
		EMSG("operation failed with:%s\n", ResultCodeToStr(res));
	}

	EMSG("raw data: len:%d\n", data_in_len);
	sunxi_dump((void *)data_in, data_in_len);
	EMSG("hmac result: len:%d\n", *data_out_len);
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

	EMSG("key data: len:%d\n", key_len);
	sunxi_dump((void *)key, key_len);

	res = TEE_AllocateOperation(&op, TEE_ALG_AES_CBC_NOPAD, direction,
				    op_key_size);
	if (res != TEE_SUCCESS) {
		EMSG("operation failed with:%s\n", ResultCodeToStr(res));
	}

	res = TEE_AllocateTransientObject(TEE_TYPE_AES, key_size, &key1_handle);
	if (res != TEE_SUCCESS) {
		EMSG("operation failed with:%s\n", ResultCodeToStr(res));
	}

	res = TEE_PopulateTransientObject(key1_handle, &key_attr, 1);
	if (res != TEE_SUCCESS) {
		EMSG("operation failed with:%s\n", ResultCodeToStr(res));
	}

	TEE_SetOperationKey(op, key1_handle);

	TEE_FreeTransientObject(key1_handle);
	key1_handle = TEE_HANDLE_NULL;

	TEE_CipherInit(op, iv, iv_len);

	res = TEE_CipherDoFinal(op, data_in, data_in_len, data_out,
				data_out_len);
	if (res != TEE_SUCCESS) {
		EMSG("operation failed with:%s\n", ResultCodeToStr(res));
	}

	EMSG("raw data: len:%d\n", data_in_len);
	sunxi_dump((void *)data_in, data_in_len);
	EMSG("result: len:%d\n", *data_out_len);
	sunxi_dump((void *)data_out, *data_out_len);

	TEE_FreeOperation(op);

	return res;
}
