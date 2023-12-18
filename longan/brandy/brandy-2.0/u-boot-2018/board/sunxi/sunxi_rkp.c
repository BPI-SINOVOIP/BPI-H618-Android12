/*
 * (C) Copyright 2022-2025
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 *
 * ouyangkun <ouyangkun@allwinnertech.com>
 * SPDX-License-Identifier:     GPL-2.0+
 */
#include <command.h>
#include <sunxi_board.h>
#include <sunxi_avb.h>
#include <asm/arch/ce.h>
#include <securestorage.h>
#include <sunxi_verify_boot_info.h>

#define GEN_WITH_OS 1

#define FINGER_PRIN_MAGIC "vendor_boot.fingerprint"

static int __finger_print_searcher(const AvbDescriptor *dh, size_t dest_len,
				   void *args)
{
	switch (be64_to_cpu(dh->tag)) {
	case AVB_DESCRIPTOR_TAG_PROPERTY: {
		const AvbPropertyDescriptor *pdh =
			(const AvbPropertyDescriptor *)dh;
		const uint8_t *p  = (const uint8_t *)dh;
		const char *name  = (char *)(p + 32);
		const char *value = (char *)(p + sizeof(AvbPropertyDescriptor) +
					     be64_to_cpu(pdh->key_num_bytes) +
					     1 /*siezof('0')*/);
		//sunxi_dump((void*)dh, dest_len);
		//		printf("name:%p value:%p namelast:%p",
		//		(void*)name,value,name + be64_to_cpu(pdh->key_num_bytes) - strlen("fingerprint")
		//		);
		if (strcmp(name + be64_to_cpu(pdh->key_num_bytes) -
				   strlen(FINGER_PRIN_MAGIC),
			   FINGER_PRIN_MAGIC) == 0) {
			strcpy(args, value);
			//printf("get %s\n",(char*)args);
			return 1;
		}
	}

	default:
		return 0;
	}
}

int sunxi_walk_avb_descriptor(const uint8_t *image_data, size_t image_size,
			      int(call_back)(const AvbDescriptor *descriptor,
					     size_t dest_len, void *args),
			      void *call_back_arg);
static int __get_finger_print(char *out_buf)
{
	uint8_t *vb_meta_data;
	size_t vb_len;
	//uint8_t finger_print[64];
	if (sunxi_avb_read_vbmeta_data(&vb_meta_data, &vb_len) == 0) {
		if (sunxi_walk_avb_descriptor(vb_meta_data, vb_len,
					      __finger_print_searcher,
					      out_buf) > 0) {
			//strcpy(out_buf, (char *)finger_print);
			return 0;
		}
	}
	return -1;
}
//const char csr[]={
//"hIKpZWJyYW5kZmdvb2dsZWVtb2RlbGdQaXhlbCA2ZmRldmljZWZvcmlvbGVncHJvZHVjdGZvcmlvbGVodmJfc3RhdGVmb3Jhbmdlam9zX3ZlcnNpb25mMTMwMDAwbG1hbnVmYWN0dXJlcmZHb29nbGVwYm9vdGxvYWRlcl9zdGF0ZWh1bmxvY2tlZHJzeXN0ZW1fcGF0Y2hfbGV2ZWwaAAMV4KBQXMG8+LLov47bvQ0U8e3nloRDoQEDoQVMJwEBon7ESYLioDuzWQE+FEKdZZx+Wf7862CoWHCENzFBr557mz8aUbOJQ1Q7+/ORQthAEuAfZWwwqR7BtO4MItNDUrm+XqC1wlGZvejLmqoolEYjdOb/ZuSvmt0vJR+w9o4ef2+lCwQjNc1FjoQarA2Xj+c+UTKHav8WPQ3zVMCFfUp1YsiTTFakhu6lipTidrxT1HicE0Y8i0A1xGrxw0GGv52PiAwYOOMlgkcG92OASdlXassbq21C2+EKef9nTjHwY3EbkYyi2D4WfrrXeOfwkap+is1K4y84jlwr75VNJmDXAOqw6lRVALLsjBJxKO3sFhi6Md89OJoFZCa07h3+U0q2I6O3vnfhn3WXE8/9aQK9FT4oaH/IOdZyC/2uUbJZzDPDuPkWSn8A8H9bGwlqleexwIZGTMFzH001nm3hFfVL+t+yXTTfbTgigYNEoQE4GKIEWCDQrsEVyirPc65rzMvRlh1l6LHd10oaN7lDOpfVmd+YCCCjAQEgBCFYIMkS8o+9w9iUXs/egFUs8OSluuDBbxD7Cg/ovLP/I3Nc9oRAoPZYIGT+j4ZxERFPaWnRQlH99R06tFBctTjoqo2fjVAILjdO"
//};

char *uploaded_csr;

void base64_csr_generate(char *out);
int key_extraction_output_generate(char *out, size_t out_size)
{
	char fp_buf[128];
	__maybe_unused char base64_csr[4096];
	char tmp_csr[4096];
	char *active_csr;
	int tmp_csr_len;
	int ret;
	//cache
	if (uploaded_csr) {
		strcpy(out, uploaded_csr);
		return 0;
	}
	memset(tmp_csr, 0, sizeof(tmp_csr));
	uploaded_csr = malloc(4096);
	//get finger print
	if (__get_finger_print(fp_buf)) {
		pr_err("get fingerprint fail");
		return -1;
	}
	pr_err("get fingerprint %s\n", fp_buf);
	//csr may from android or generate in place
	ret = sunxi_secure_object_read("kRkpCsr", tmp_csr, 4096, &tmp_csr_len);
	if (ret < 0) {
#if SUNXI_RKP_FROM_UBOOT
		printf("read csr from android fail, use fall back\n");
		memset(base64_csr, 0, sizeof(base64_csr));
		base64_csr_generate(base64_csr);
		active_csr = base64_csr;
#else
		printf("read csr from android fail\n");
		return -1;
#endif
	} else {
		active_csr = tmp_csr;
	}

	strncpy(out, "{\"build_fingerprint\":\"", out_size - 1);
	strncat(out, fp_buf, out_size - strlen(out) - 1);
	strncat(out, "\",\"csr\":\"", out_size - strlen(out) - 1);
	strncat(out, active_csr, out_size - strlen(out) - 1);
	strncat(out, "\",\"name\":\"default\" }", out_size - strlen(out) - 1);

	//"{\"build_fingerprint\":\"fp_buf\",\"csr\":\"active_csr\",\"name\":\"default\" }"

	//save csr cache
	strcpy(uploaded_csr, out);

	return 0;
}

#if SUNXI_RKP_FROM_UBOOT
#define CHAR_BIT 8
#define _GCC_STDINT_H
#include <dice/cbor_writer.h>
#include "mbedtls/hkdf.h"
#include "mbedtls/ecdh.h"
#include "mbedtls/md.h"
#include "mbedtls/cipher.h"
#include "mbedtls/ecp.h"
#include "mbedtls/error.h"
#include "mbedtls/gcm.h"
#include "mbedtls/base64.h"

extern struct ecc_key_t dk;
#if 1 // crypto base
struct ecc_key_t {
	uint8_t private[32];
	uint8_t qx[32];
	uint8_t qy[32];
	int isX25519;
};

#pragma pack(push)
#pragma pack(1)
struct device_info_set_param {
	uint32_t type;
	char info[16]; //len is not important, left space in buffer will be used
};

struct rkp_protected_gen_param {
	uint8_t mac_key[32];
	uint8_t mac_tag[32];
	uint8_t challenge[32];
	uint32_t challenge_size;
	uint32_t out_size;

	//len is not important, left space in buffer will be used
	uint8_t out_buffer[256];
};
#pragma pack(pop)
enum DEVICE_INFO_TYPE_E {
	//string
	DEVICE_ID_BRAND,
	DEVICE_ID_DEVICE,
	DEVICE_ID_PRODUCT,
	DEVICE_ID_SERIAL,
	DEVICE_ID_MANUFACTURER,
	DEVICE_ID_MODEL,
	DEVICE_ID_COUNT,
	//int
	DEVICE_INFO_OS_VER = 32,
	DEVICE_INFO_OS_PATCH,
	DEVICE_INFO_BOOT_PATCH,
	DEVICE_INFO_VENDOR_PATCH,
};
enum RKP_SUMCMD_E {
	RKP_SET_INFO		       = 1,
	RKP_GEN_PROTECTED_DATA_PAYLOAD = 2,
	RKP_GEN_DEVICE_INFO	       = 3,
};

int smc_tee_rkp_op(int sub_cmd, void *input, size_t input_size, void *output);
static void __prepare_device_info(void)
{
	uint32_t tmp;
	struct device_info_set_param info;
	memset(&info, 0, sizeof(info));
	info.type = DEVICE_ID_BRAND;
	strcpy(info.info, "Allwinner");
	smc_tee_rkp_op(RKP_SET_INFO, &info, sizeof(info), NULL);

	memset(&info, 0, sizeof(info));
	info.type = DEVICE_ID_MODEL;
	strcpy(info.info, "DUMMY");
	smc_tee_rkp_op(RKP_SET_INFO, &info, sizeof(info), NULL);

	memset(&info, 0, sizeof(info));
	info.type = DEVICE_ID_PRODUCT;
	strcpy(info.info, "DUMMY");
	smc_tee_rkp_op(RKP_SET_INFO, &info, sizeof(info), NULL);

	memset(&info, 0, sizeof(info));
	info.type = DEVICE_ID_DEVICE;
	strcpy(info.info, "DUMMY");
	smc_tee_rkp_op(RKP_SET_INFO, &info, sizeof(info), NULL);

	memset(&info, 0, sizeof(info));
	info.type = DEVICE_ID_MANUFACTURER;
	strcpy(info.info, "allwinner");
	smc_tee_rkp_op(RKP_SET_INFO, &info, sizeof(info), NULL);

	memset(&info, 0, sizeof(info));
	info.type = DEVICE_INFO_OS_VER;
	tmp	  = 130000;
	memcpy(info.info, &tmp, 4);
	smc_tee_rkp_op(RKP_SET_INFO, &info, sizeof(info), NULL);

	memset(&info, 0, sizeof(info));
	info.type = DEVICE_INFO_OS_PATCH;
	tmp	  = 0;
	memcpy(info.info, &tmp, 4);
	smc_tee_rkp_op(RKP_SET_INFO, &info, sizeof(info), NULL);
	info.type = DEVICE_INFO_VENDOR_PATCH;
	smc_tee_rkp_op(RKP_SET_INFO, &info, sizeof(info), NULL);
	info.type = DEVICE_INFO_OS_PATCH;
	smc_tee_rkp_op(RKP_SET_INFO, &info, sizeof(info), NULL);
	info.type = DEVICE_INFO_BOOT_PATCH;
	smc_tee_rkp_op(RKP_SET_INFO, &info, sizeof(info), NULL);
}

static void __prepare_verify_boot_data(void)
{
	uint8_t hash[32];
	sunxi_set_verify_boot_number(SUNXI_VB_INFO_LOCK, 1);
	sunxi_set_verify_boot_number(SUNXI_VB_INFO_BOOTSTATE,
				     KM_VERIFIED_BOOT_VERIFIED);

	memset(hash, 0, 32);
	sunxi_set_verify_boot_blob(SUNXI_VB_INFO_HASH, hash, 32);
	sunxi_keymaster_verify_boot_params_install();
}
static int myrand(void *rng_state, unsigned char *output, size_t len)
{
	u8 tmp_buf[32];
	u8 *p = output;
	u8 tmp_len;
	if (rng_state != NULL)
		rng_state = NULL;

	while (len) {
		tmp_len = len > 32 ? 32 : len;
		sunxi_trng_gen(tmp_buf, 32);
		memcpy(p, tmp_buf, tmp_len);
		p += 32;
		len -= tmp_len;
	}

	return 0;
}

static int ecc_sign(uint8_t *input, uint8_t *random,
		    const struct ecc_curve_param_t *ecc_param, uint8_t *private,
		    struct ecc_signature_t *signature)
{
	sunxi_ecc_sign(input, random, &p256_param, private, signature);
	return 0;
}

static int __hash(uint8_t *input, size_t input_len, uint8_t *output)
{
	sunxi_sha_calc(output, 32, input, input_len);
	return 0;
}

static void __rsa_padding(u8 *dst_buf, u8 *src_buf, u32 data_len, u32 group_len)
{
	int i = 0;

	memset(dst_buf, 0, group_len);
	for (i = group_len - data_len; i < group_len; i++) {
		dst_buf[i] = src_buf[group_len - 1 - i];
	}
}
static int sign(uint8_t *input, size_t input_size, uint8_t *private,
		uint8_t *output)
{
	struct ecc_signature_t sig;
	uint8_t hash[32];
	uint8_t le_input[32];
	uint8_t random[32];
	__hash(input, input_size, hash);
	myrand(NULL, random, 32);
	//pc treat input(hash) as big-enden
	__rsa_padding(le_input, hash, 32, 32);
	ecc_sign(le_input, random, &p256_param, private, &sig);
	__rsa_padding(output, sig.r, 32, 32);
	__rsa_padding(output + 32, sig.s, 32, 32);
	return 0;
}
static int hmac(uint8_t *input, uint8_t *key, uint8_t *output, size_t size)
{
	mbedtls_md_hmac(mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), key, 32,
			input, size, output);
	return 0;
}

static int x25519_pair_gen(struct ecc_key_t *key)
{
	mbedtls_ecdh_context ctx_cli;
	mbedtls_ecdh_init(&ctx_cli);
	mbedtls_ecp_group_load(&ctx_cli.grp, MBEDTLS_ECP_DP_CURVE25519);
	mbedtls_ecdh_gen_public(&ctx_cli.grp, &ctx_cli.d, &ctx_cli.Q, myrand,
				NULL);
	mbedtls_mpi_write_binary_le(&ctx_cli.d, key->private, 32);
	mbedtls_mpi_write_binary_le(&ctx_cli.Q.X, key->qx, 32);
	key->isX25519 = 1;
	return 0;
}

static int __aes_gcm(uint8_t *aes_key, uint8_t *iv, uint8_t *in, uint8_t *out,
		     size_t input_len, uint8_t *aad, size_t aad_len)
{
	mbedtls_gcm_context gcm_ctx;
	mbedtls_cipher_id_t cipher = MBEDTLS_CIPHER_ID_AES;
	mbedtls_gcm_init(&gcm_ctx);
	mbedtls_gcm_setkey(&gcm_ctx, cipher, aes_key, 256);
	mbedtls_gcm_crypt_and_tag(&gcm_ctx, MBEDTLS_GCM_ENCRYPT, input_len, iv,
				  12, aad, aad_len, in, out, 16,
				  &out[input_len]);
	return 0;
}

static inline int DiceKdf(size_t length, const uint8_t *ikm, size_t ikm_size,
			  const uint8_t *salt, size_t salt_size,
			  const uint8_t *info, size_t info_size,
			  uint8_t *output)
{
	ALLOC_CACHE_ALIGN_BUFFER(u8, prk_key, ALIGN(64, CACHE_LINE_SIZE));
	u8 prk[64];
	memset(prk_key, 0, 64);
	memset(prk, 0, 64);
	sunxi_sha_calc(prk_key, 32, (void *)salt, salt_size);
	hmac((void *)ikm, prk_key, prk, 32);
	hmac((void *)info, prk, output, 32);
	return 0;
}
static const uint8_t kAsymSalt[] = {
	0x63, 0xB6, 0xA0, 0x4D, 0x2C, 0x07, 0x7F, 0xC1, 0x0F, 0x63, 0x9F,
	0x21, 0xDA, 0x79, 0x38, 0x44, 0x35, 0x6C, 0xC2, 0xB0, 0xB4, 0x41,
	0xB3, 0xA7, 0x71, 0x24, 0x03, 0x5C, 0x03, 0xF8, 0xE1, 0xBE, 0x60,
	0x35, 0xD3, 0x1F, 0x28, 0x28, 0x21, 0xA7, 0x45, 0x0A, 0x02, 0x22,
	0x2A, 0xB1, 0xB3, 0xCF, 0xF1, 0x67, 0x9B, 0x05, 0xAB, 0x1C, 0xA5,
	0xD1, 0xAF, 0xFB, 0x78, 0x9C, 0xCD, 0x2B, 0x0B, 0x3B
};
static const size_t kAsymSaltSize = 64;

int private_gen(uint8_t *input, size_t input_len, uint8_t *private)
{
	ALLOC_CACHE_ALIGN_BUFFER(u8, tmp_salt, ALIGN(64, CACHE_LINE_SIZE));
	u8 info[] = "key Pair";
	memcpy(tmp_salt, kAsymSalt, 64);
	DiceKdf(32, input, input_len, tmp_salt, kAsymSaltSize, info, 8,
		private);
	return 0;
}

#endif

#if 1 // keys

struct ecc_key_t google = {
	.private = {0},// not used
	.qx = {
		0xbe, 0x85, 0xe7, 0x46, 0xc4, 0xa3, 0x42, 0x5a,
		0x40, 0xd9, 0x36, 0x3a, 0xa6, 0x15, 0xd0, 0x2c,
		0x58, 0x7e, 0x3d, 0xdc, 0x33, 0x02, 0x32, 0xd2,
		0xfc, 0x5e, 0x1e, 0x87, 0x25, 0x5f, 0x72, 0x60,
	},
	//x25519 have qx only
	.qy = {0},
	.isX25519 = 1
};

struct ecc_key_t dk = { .private = { 0 }, .qx = { 1 }, .qy = { 2 } };

struct ecc_key_t ephemeral = { .private = { 0 }, .qx = { 0 }, .qy = { 0 } };
uint8_t google_eek_id[32]  = { 0xD0, 0xAE, 0xC1, 0x15, 0xCA, 0x2A, 0xCF, 0x73,
			       0xAE, 0x6B, 0xCC, 0xCB, 0xD1, 0x96, 0x1D, 0x65,
			       0xE8, 0xB1, 0xDD, 0xD7, 0x4A, 0x1A, 0x37, 0xB9,
			       0x43, 0x3A, 0x97, 0xD5, 0x99, 0xDF, 0x98, 0x08 };

uint8_t ephemeral_mac_key[32];
#endif

int fill_context(struct CborOut *cout);

int fill_context(struct CborOut *cout)
{
	//     Context = [
	//         AlgorithmID : 3             // AES-GCM 256
	//         PartyUInfo : [
	//             identity : bstr "client"
	//             nonce : bstr .size 0,
	//             other : bstr            // Ephemeral_pub
	//         ],
	//         PartyVInfo : [
	//             identity : bstr "server",
	//             nonce : bstr .size 0,
	//             other : bstr            // EEK pubkey
	//         ],
	//         SuppPubInfo : [
	//             256,                    // Output key length
	//             protected : bstr .size 0
	//         ]
	//     ]
	int before = CborOutSize(cout);
	uint8_t *p;
	CborWriteArray(4, cout);
	//algo id
	CborWriteUint(3, cout);
	//u info
	CborWriteArray(3, cout);
	CborWriteBstr(strlen("client"), (uint8_t *)"client", cout);
	CborWriteBstr(0, cout->buffer, cout);
//u->ephemeral_pub
#if 0
    p = CborAllocBstr(64, cout);
    memcpy(p, ephemeral.qx, 32);
    memcpy(p+32, ephemeral.qy, 32);
#else
	p = CborAllocBstr(32, cout);
	memcpy(p, ephemeral.qx, 32);
#endif
	//i info
	CborWriteArray(3, cout);
	CborWriteBstr(strlen("server"), (uint8_t *)"server", cout);
	CborWriteBstr(0, cout->buffer, cout);
//i->eek pub
#if 0
    p = CborAllocBstr(64, cout);
    memcpy(p, google.qx, 32);
    memcpy(p+32, google.qy, 32);
#else
	p = CborAllocBstr(32, cout);
	memcpy(p, google.qx, 32);
#endif
	//pub info
	CborWriteArray(2, cout);
	CborWriteUint(256, cout);
	CborWriteBstr(0, cout->buffer, cout);

#if DEBUG_DUMP
	printf("\n%s:\n", __func__);
	sunxi_dump(cout->buffer + before, CborOutSize(cout) - before);
#endif
	return CborOutSize(cout) - before;
}

int fill_deviceinfo(struct CborOut *cout)
{
	//     DeviceInfo = {
	//         "brand" : tstr,
	//         "manufacturer" : tstr,
	//         "product" : tstr,
	//         "model" : tstr,
	//         "device" : tstr,
	//         "vb_state" : "green" / "yellow" / "orange",    // Taken from the AVB values
	//         "bootloader_state" : "locked" / "unlocked",    // Taken from the AVB values
	//         "vbmeta_digest": bstr,                         // Taken from the AVB values
	//         ? "os_version" : tstr,                         // Same as
	//                                                        // android.os.Build.VERSION.release
	//                                                        // Not optional for TEE.
	//         "system_patch_level" : uint,                   // YYYYMMDD
	//         "boot_patch_level" : uint,                     // YYYYMMDD
	//         "vendor_patch_level" : uint,                   // YYYYMMDD
	//         "version" : 2,                                 // The CDDL schema version.
	//         "security_level" : "tee" / "strongbox",
	//         "fused": 1 / 0,  // 1 if secure boot is enforced for the processor that the IRPC
	//                          // implementation is contained in. 0 otherwise.
	//     }
	int before = CborOutSize(cout);
#if GEN_WITH_OS
	uint8_t buf[4096];
	struct rkp_protected_gen_param param;
	smc_tee_rkp_op(RKP_GEN_DEVICE_INFO, &param, sizeof(param), buf);
	struct rkp_protected_gen_param *generated =
		(struct rkp_protected_gen_param *)buf;
#if DEBUG_DUMP
	printf("\n%s:\n", __func__);
	sunxi_dump(generated->out_buffer, generated->out_size);
#endif
	//cbor write API do not support attach a complete cbor
	//object, we got to do some hanking here. modifying cout
	//directory instead of throungh APIs
	if (cout->buffer_size >= cout->cursor + generated->out_size) {
		memcpy(&cout->buffer[cout->cursor], generated->out_buffer,
		       generated->out_size);
		cout->cursor += generated->out_size;
	}
#else
	uint8_t empty_digest[32] = { 0 };
	CborWriteMap(15, cout);
	//brand(str)
	CborWriteTstr("brand", cout);
	CborWriteTstr("Allwinner", cout);
	//fused(uint)
	CborWriteTstr("fused", cout);
	CborWriteUint(0, cout);
	//model(str)
	CborWriteTstr("model", cout);
	CborWriteTstr("DUMMY", cout);
	//device(str)
	CborWriteTstr("device", cout);
	CborWriteTstr("DUMMY", cout);
	//product(str)
	CborWriteTstr("product", cout);
	CborWriteTstr("DUMMY", cout);
	//version(uint)
	CborWriteTstr("version", cout);
	CborWriteUint(2, cout);
	//vb_state(str)
	CborWriteTstr("vb_state", cout);
	CborWriteTstr("green", cout);
	//os_version(str)
	CborWriteTstr("os_version", cout);
	CborWriteTstr("130000", cout);
	//manufacturer(str)
	CborWriteTstr("manufacturer", cout);
	CborWriteTstr("allwinner", cout);
	//vbmeta_digest(bytes)
	CborWriteTstr("vbmeta_digest", cout);
	CborWriteBstr(32, empty_digest, cout);
	//security_level(str)
	CborWriteTstr("security_level", cout);
	CborWriteTstr("tee", cout);
	//boot_patch_level(uint)
	CborWriteTstr("boot_patch_level", cout);
	CborWriteUint(0, cout);
	//bootloader_state(str)
	CborWriteTstr("bootloader_state", cout);
	CborWriteTstr("locked", cout);
	//system_patch_level(uint)
	CborWriteTstr("system_patch_level", cout);
	CborWriteUint(0, cout);
	//vendor_patch_level(uint)
	CborWriteTstr("vendor_patch_level", cout);
	CborWriteUint(0, cout);
#if 0 //compare
	uint8_t tmp_plain_text2[4096];
	struct rkp_protected_gen_param param1;
	smc_tee_rkp_op(RKP_GEN_DEVICE_INFO, &param1, sizeof(param1),
		       tmp_plain_text2);
	struct rkp_protected_gen_param *generated1 =
		(struct rkp_protected_gen_param *)tmp_plain_text2;
	printf("\n%s:\n", __func__);
	sunxi_dump(generated1->out_buffer, generated1->out_size);

	printf("\n%s:\n", __func__);
	sunxi_dump(cout->buffer + before, CborOutSize(cout) - before);
#endif
#endif

#if DEBUG_DUMP
	printf("\n%s:\n", __func__);
	sunxi_dump(cout->buffer + before, CborOutSize(cout) - before);
#endif
	return CborOutSize(cout) - before;
}

int fill_p256_pubkey(struct CborOut *cout, uint8_t *px, uint8_t *py, int isDh)
{
	//     PubKeyEcdhP256 = {               // COSE_Key
	//          1 : 2,                      // Key type : EC2
	//          -1 : 1,                     // Curve : P256
	//          -2 : bstr                   // Sender X coordinate
	//          -3 : bstr                   // Sender Y coordinate
	//     }
	//
	//     PubKeyECDSA256 = {               // COSE_Key
	//         1 : 2,                       // Key type : EC2
	//         3 : AlgorithmES256,          // Algorithm : ECDSA w/ SHA-256
	//         -1 : 1,                      // Curve: P256
	//         -2 : bstr,                   // X coordinate
	//         -3 : bstr                    // Y coordinate
	//     }
	int before = CborOutSize(cout);
	CborWriteMap(4 + (isDh ? 0 : 1), cout);
	//key type
	CborWriteUint(1, cout);
	CborWriteUint(2, cout);
	//algorithm
	if (!isDh) {
		//     AlgorithmES256 = -7
		CborWriteUint(3, cout);
		CborWriteInt(-7, cout);
	}
	//curve
	CborWriteInt(-1, cout);
	CborWriteUint(1, cout);
	//x
	CborWriteInt(-2, cout);
	CborWriteBstr(32, px, cout);
	//y
	CborWriteInt(-3, cout);
	CborWriteBstr(32, py, cout);
#if DEBUG_DUMP
	printf("\n%s:\n", __func__);
	sunxi_dump(cout->buffer + before, CborOutSize(cout) - before);
#endif
	return CborOutSize(cout) - before;
}

int fill_x25519_pubkey(struct CborOut *cout, struct ecc_key_t *key, int isDh)
{
	//     PubKeyX25519 = {                 // COSE_Key
	//          1 : 1,                      // Key type : Octet Key Pair
	//         -1 : 4,                      // Curve : X25519
	//         -2 : bstr                    // Sender X25519 public key
	//     }
	//
	//     PubKeyEd25519 = {                // COSE_Key
	//         1 : 1,                       // Key type : octet key pair
	//         3 : AlgorithmEdDSA,          // Algorithm : EdDSA
	//         -1 : 6,                      // Curve : Ed25519
	//         -2 : bstr                    // X coordinate, little-endian
	//     }
	int before = CborOutSize(cout);
	CborWriteMap(3 + (isDh ? 0 : 1), cout);
	//key type
	CborWriteUint(1, cout);
	CborWriteUint(1, cout);
	//curve
	CborWriteInt(-1, cout);
	CborWriteUint(4, cout);
	//x
	CborWriteInt(-2, cout);
	CborWriteBstr(32, key->qx, cout);
	//algorithm
	if (!isDh) {
		//     AlgorithmEdDSA = -8
		CborWriteUint(3, cout);
		CborWriteInt(-8, cout);
	}
#if DEBUG_DUMP
	printf("\n%s:\n", __func__);
	sunxi_dump(cout->buffer + before, CborOutSize(cout) - before);
#endif
	return CborOutSize(cout) - before;
}

int fill_pub_key(struct CborOut *cout, struct ecc_key_t *key, int isDh)
{
	if (key->isX25519) {
		return fill_x25519_pubkey(cout, key, isDh);
	} else {
		return fill_p256_pubkey(cout, key->qx, key->qy, isDh);
	}
}

int fill_protected_algo(struct CborOut *cout)
{
	int before = CborOutSize(cout);
	uint8_t tmp_pro[4096];
	struct CborOut protected;
	CborOutInit(tmp_pro, 4096, &protected);
	CborWriteMap(1, &protected);
	CborWriteInt(1, &protected);
	CborWriteInt(-7, &protected);
	CborWriteBstr(CborOutSize(&protected), tmp_pro, cout);
#if DEBUG_DUMP
	printf("\n%s:\n", __func__);
	sunxi_dump(cout->buffer + before, CborOutSize(cout) - before);
#endif
	return CborOutSize(cout) - before;
}

int fill_cose_sign_struct(struct CborOut *cout, uint8_t *payload,
			  size_t payload_len, uint8_t *aad, size_t aad_len)
{
	int before = CborOutSize(cout);
	CborWriteArray(4, cout);
	CborWriteTstr("Signature1", cout);
	fill_protected_algo(cout);
	CborWriteBstr(aad_len, aad, cout);
	CborWriteBstr(payload_len, payload, cout);

#if DEBUG_DUMP
	printf("\n%s:\n", __func__);
	sunxi_dump(cout->buffer + before, CborOutSize(cout) - before);
#endif
	return CborOutSize(cout) - before;
}

int cose_sign(uint8_t *payload, size_t payload_len, uint8_t *aad,
	      size_t aad_len, uint8_t *private, uint8_t *signature)
{
	uint8_t tmp_sign[4096];
	struct CborOut csign;
	fill_cose_sign_struct(&csign, payload, payload_len, aad, aad_len);
	sign(tmp_sign, CborOutSize(&csign), private, signature);
	return 0;
}

int gen_signed_mac_signature(uint8_t *signature, uint8_t *challenge,
			     uint8_t *mac_tag)
{
	//     SignedMac_structure = [                      //  COSE Sig_structure
	//         "Signature1",
	//         bstr .cbor {                             // Protected params
	//             1 : AlgorithmEdDSA / AlgorithmES256, // Algorithm
	//         },
	//         bstr .cbor SignedMacAad,
	//         bstr .size 32                            // MAC key
	//     ]
	//     SignedMacAad = [
	//         challenge : bstr .size (32..64),   // Size between 32 - 64
	//                                            // bytes inclusive
	//         VerifiedDeviceInfo,
	//         tag: bstr                 // This is the tag from COSE_Mac0 of
	//                                   // KeysToCertify, to tie the key set to
	//                                   // the signature.
	//     ]
	//     VerifiedDeviceInfo = DeviceInfo  // See DeviceInfo.aidl
	uint8_t tmp_aad[4096];
	struct CborOut aad;

	//aad
	CborOutInit(tmp_aad, 4096, &aad);
	CborWriteArray(3, &aad);
	CborWriteBstr(16, challenge, &aad);
	//CborWriteBstr(CborOutSize(&info), info.buffer, &aad);
	fill_deviceinfo(&aad);
	CborWriteBstr(32, mac_tag, &aad);
	ALLOC_CACHE_ALIGN_BUFFER(u8, tmp_mac_struct, 4096);
	struct CborOut mac_struct;
	CborOutInit(tmp_mac_struct, 4096, &mac_struct);

	fill_cose_sign_struct(&mac_struct, ephemeral_mac_key, 32, tmp_aad,
			      CborOutSize(&aad));
	sign(tmp_mac_struct, CborOutSize(&mac_struct), dk.private, signature);

	return 0;
}

int fill_signed_mac(struct CborOut *cout, uint8_t *challenge, uint8_t *mac_tag)
{
	//     SignedMac = [                                // COSE_Sign1
	//         bstr .cbor {                             // Protected params
	//             1 : AlgorithmEdDSA / AlgorithmES256, // Algorithm
	//         },
	//         {},                                      // Unprotected params
	//         bstr .size 32,                           // Payload: MAC key
	//         bstr // PureEd25519(KM_priv, bstr .cbor SignedMac_structure) /
	//              // ECDSA(KM_priv, bstr .cbor SignedMac_structure)
	//     ]
	int before = CborOutSize(cout);
	CborWriteArray(4, cout);
	//protected
	fill_protected_algo(cout);
	//unprotected
	CborWriteMap(0, cout);
	//mac key
	CborWriteBstr(32, ephemeral_mac_key, cout);

	// signature
	uint8_t *fill;
	fill = CborAllocBstr(64, cout);
	gen_signed_mac_signature(fill, challenge, mac_tag);

#if DEBUG_DUMP
	printf("\n%s:\n", __func__);
	sunxi_dump(cout->buffer + before, CborOutSize(cout) - before);
#endif
	return CborOutSize(cout) - before;
}

int fill_bcc_payload(struct CborOut *cout)
{
	//     BccPayload = {                               // CWT [RFC8392]
	//         1 : tstr,                                // Issuer
	//         2 : tstr,                                // Subject
	//         -4670552 : bstr .cbor PubKeyEd25519 /
	//                    bstr .cbor PubKeyECDSA256,    // Subject Public Key
	//         -4670553 : bstr                          // Key Usage
	//
	//         // NOTE: All of the following fields may be omitted for a "Degenerate BCC", as
	//         //       described by IRemotelyProvisionedComponent.aidl.
	//         -4670545 : bstr,                         // Code Hash
	//         ? -4670546 : bstr,                       // Code Descriptor
	//         ? -4670547 : bstr,                       // Configuration Hash
	//         -4670548 : bstr .cbor {                  // Configuration Descriptor
	//             ? -70002 : tstr,                         // Component name
	//             ? -70003 : int,                          // Firmware version
	//             ? -70004 : null,                         // Resettable
	//         },
	//         -4670549 : bstr,                         // Authority Hash
	//         ? -4670550 : bstr,                       // Authority Descriptor
	//         -4670551 : bstr,                         // Mode
	//     }
	int before	      = CborOutSize(cout);
	const uint8_t mode[1] = { 0x01 };

	struct CborOut tmp;
	uint8_t tmp_buffer[4096];

	CborWriteMap(8, cout);
	//issuer
	CborWriteUint(1, cout);
	CborWriteTstr("DEVICE_KEY", cout);
	//subject
	CborWriteUint(2, cout);
	CborWriteTstr("DEVICE_KEY", cout);
	//pub
	CborOutInit(tmp_buffer, 4096, &tmp);
	fill_p256_pubkey(&tmp, dk.qx, dk.qy, 0);

	CborWriteInt(-4670552, cout);
	CborWriteBstr(CborOutSize(&tmp), tmp_buffer, cout);
	//key usage
	CborWriteInt(-4670553, cout);
	CborWriteBstr(1, (uint8_t *)" ", cout);
	//code hash
	CborWriteInt(-4670545, cout);
	CborWriteBstr(32, dk.qx, cout);
	//configuration descriptor
	CborOutInit(tmp_buffer, 4096, &tmp);

	CborWriteMap(2, &tmp);
	CborWriteInt(-70002, &tmp);
	CborWriteTstr("TEE", &tmp);
	CborWriteInt(-70003, &tmp);
	CborWriteUint(0, &tmp);

	CborWriteInt(-4670548, cout);
	CborWriteBstr(CborOutSize(&tmp), tmp_buffer, cout);
	//authority hash
	CborWriteInt(-4670549, cout);
	CborWriteBstr(32, dk.qx, cout);
	//mode
	CborWriteInt(-4670551, cout);
	CborWriteBstr(1, mode, cout);

#if DEBUG_DUMP
	printf("\n%s:\n", __func__);
	sunxi_dump(cout->buffer + before, CborOutSize(cout) - before);
#endif
	return CborOutSize(cout) - before;
}

int fill_bcc_entry_input(struct CborOut *cout)
{
	//     BccEntryInput = [
	//         context: "Signature1",
	//         protected: bstr .cbor {
	//             1 : AlgorithmEdDSA / AlgorithmES256,  // Algorithm
	//         },
	//         external_aad: bstr .size 0,
	//         payload: bstr .cbor BccPayload
	//     ]
	int before = CborOutSize(cout);
	CborWriteArray(4, cout);
	//context
	CborWriteTstr("Signature1", cout);
	//protected
	fill_protected_algo(cout);
	//external_aad
	CborWriteBstr(0, dk.qx, cout);
	//payload
	fill_bcc_payload(cout);

#if DEBUG_DUMP
	printf("\n%s:\n", __func__);
	sunxi_dump(cout->buffer + before, CborOutSize(cout) - before);
#endif
	return CborOutSize(cout) - before;
}

int fill_bcc_entry(struct CborOut *cout)
{
	//     BccEntry = [                                  // COSE_Sign1 (untagged)
	//         protected : bstr .cbor {
	//             1 : AlgorithmEdDSA / AlgorithmES256,  // Algorithm
	//         },
	//         unprotected: {},
	//         payload: bstr .cbor BccPayload,
	//         signature: bstr // PureEd25519(SigningKey, bstr .cbor BccEntryInput) /
	//                         // ECDSA(SigningKey, bstr .cbor BccEntryInput)
	//         // See RFC 8032 for details of how to encode the signature value for Ed25519.
	//     ]
	int before = CborOutSize(cout);
	uint8_t tmp_entry_input[4096], *psign;
	struct CborOut entry_input;
	CborOutInit(tmp_entry_input, 4096, &entry_input);

	uint8_t payload_buf[1024];
	struct CborOut payload;
	CborOutInit(payload_buf, 1024, &payload);
	fill_bcc_payload(&payload);

	CborWriteArray(4, cout);
	//protected
	fill_protected_algo(cout);
	//unprotected
	CborWriteMap(0, cout);
	//payload
	CborWriteBstr(CborOutSize(&payload), payload_buf, cout);
	//signature
	psign = CborAllocBstr(64, cout);

	ALLOC_CACHE_ALIGN_BUFFER(u8, tmp_sign_struct, 4096);
	struct CborOut mac_struct;
	CborOutInit(tmp_sign_struct, 4096, &mac_struct);
	fill_cose_sign_struct(&mac_struct, payload_buf, CborOutSize(&payload),
			      payload_buf /*0 len, any thing*/, 0);
	sign(tmp_sign_struct, CborOutSize(&mac_struct), dk.private, psign);

#if DEBUG_DUMP
	printf("\n%s:\n", __func__);
	sunxi_dump(cout->buffer + before, CborOutSize(cout) - before);
#endif
	return CborOutSize(cout) - before;
}

int fill_bcc(struct CborOut *cout)
{
	//     Bcc = [
	//         PubKeyEd25519 / PubKeyECDSA256, // DK_pub
	//         + BccEntry,                     // Root -> leaf (KM_pub)
	//     ]
	int before = CborOutSize(cout);

	CborWriteArray(2, cout);
	fill_p256_pubkey(cout, dk.qx, dk.qy, 0);
	fill_bcc_entry(cout);

#if DEBUG_DUMP
	printf("\n%s:\n", __func__);
	sunxi_dump(cout->buffer + before, CborOutSize(cout) - before);
#endif
	return CborOutSize(cout) - before;
}

int fill_protected_data_payload_plaintext(struct CborOut *cout,
					  uint8_t *challenge, uint8_t *mac_tag)
{
	//     ProtectedDataPayload [
	//         SignedMac,
	//         Bcc,
	//         ? AdditionalDKSignatures,
	//     ]
	int before = CborOutSize(cout);
	CborWriteArray(2, cout);
	fill_signed_mac(cout, challenge, mac_tag);
	fill_bcc(cout);

#if DEBUG_DUMP
	printf("\n%s:\n", __func__);
	sunxi_dump(cout->buffer + before, CborOutSize(cout) - before);
#endif
	return CborOutSize(cout) - before;
}

int fill_recipients(struct CborOut *cout)
{
	//         recipients : [
	//             [                       // COSE_Recipient
	//                 protected : bstr .cbor {
	//                     1 : -25         // Algorithm : ECDH-ES + HKDF-256
	//                 },
	//                 unprotected : {
	//                     -1 : PubKeyX25519 / PubKeyEcdhP256  // Ephemeral_pub
	//                     4 : bstr,       // KID : EEK ID
	//                 },
	//                 ciphertext : nil
	//             ]
	int before	    = CborOutSize(cout);
	uint8_t protected[] = { 0xA1, 0x01, 0x38, 0x18 };
	CborWriteArray(1, cout);
	CborWriteArray(3, cout);
	//protected
	CborWriteBstr(sizeof(protected), protected, cout);
	//unprotected
	CborWriteMap(2, cout);
	//eek in unprotected
	CborWriteUint(4, cout);
	CborWriteBstr(sizeof(google_eek_id), google_eek_id, cout);
	//pubkey in unprotected
	CborWriteInt(-1, cout);
	fill_pub_key(cout, &ephemeral, 1);
	//ciphertext
	CborWriteNull(cout);

#if DEBUG_DUMP
	printf("\n%s:\n", __func__);
	sunxi_dump(cout->buffer + before, CborOutSize(cout) - before);
#endif
	return CborOutSize(cout) - before;
}

void protected_data_encrypted(uint8_t *iv, uint8_t *in, size_t input_len,
			      uint8_t *out);

int fill_protected_data(struct CborOut *cout, uint8_t *mac_key,
			uint8_t *challenge, uint8_t *mac_tag)
{
	int before	    = CborOutSize(cout);
	uint8_t protected[] = { 0xA1, 0x01, 0x03 };
	uint8_t iv[32]	    = { 0 };
	uint8_t *cipher;
	CborWriteArray(4, cout);
	//protected (constant)
	CborWriteBstr(sizeof(protected), protected, cout);
	//unprotected
	CborWriteMap(1, cout);
	CborWriteUint(5, cout);
	myrand(NULL, iv, 12);
	CborWriteBstr(12, iv, cout);
#if 0 //compare
	uint8_t tmp_plain_text1[4096];
	struct CborOut plain_text1;
	CborOutInit(tmp_plain_text1, 4096, &plain_text1);
	fill_protected_data_payload_plaintext(&plain_text1, challenge, mac_tag);
	sunxi_dump(tmp_plain_text1, CborOutSize(&plain_text1));

	uint8_t tmp_plain_text2[4096];
	struct rkp_protected_gen_param param1;
	memcpy(param1.challenge, challenge, 16);
	param1.challenge_size = 16;
	memcpy(param1.mac_key, ephemeral_mac_key, 32);
	memcpy(param1.mac_tag, mac_tag, 32);
	smc_tee_rkp_op(RKP_GEN_PROTECTED_DATA_PAYLOAD, &param1, sizeof(param1),
		       tmp_plain_text2);
	struct rkp_protected_gen_param *generated1 =
		(struct rkp_protected_gen_param *)tmp_plain_text2;
	sunxi_dump(generated1->out_buffer, generated1->out_size);
#endif
	//protectedData ciphertext
#if GEN_WITH_OS
	uint8_t tmp_plain_text[4096];
	struct rkp_protected_gen_param param;
	memcpy(param.challenge, challenge, 16);
	param.challenge_size = 16;
	memcpy(param.mac_key, ephemeral_mac_key, 32);
	memcpy(param.mac_tag, mac_tag, 32);
	smc_tee_rkp_op(RKP_GEN_PROTECTED_DATA_PAYLOAD, &param, sizeof(param),
		       tmp_plain_text);
	struct rkp_protected_gen_param *generated =
		(struct rkp_protected_gen_param *)tmp_plain_text;
	cipher = CborAllocBstr(generated->out_size + 16 /*aes gcm tag*/, cout);
	protected_data_encrypted(iv, generated->out_buffer, generated->out_size,
				 cipher);
#else
	uint8_t tmp_plain_text[4096];
	struct CborOut plain_text;
	CborOutInit(tmp_plain_text, 4096, &plain_text);
	fill_protected_data_payload_plaintext(&plain_text, challenge, mac_tag);
	cipher = CborAllocBstr(CborOutSize(&plain_text) + 16 /*aes gcm tag*/,
			       cout);
	protected_data_encrypted(iv, tmp_plain_text, CborOutSize(&plain_text),
				 cipher);
#endif
	//recipients
	fill_recipients(cout);

#if DEBUG_DUMP
	printf("\n%s:\n", __func__);
	sunxi_dump(cout->buffer + before, CborOutSize(cout) - before);
#endif
	return CborOutSize(cout) - before;
}

int fill_maced_keys_to_sign(struct CborOut *cout, uint8_t *mac_tag)
{
	int before = CborOutSize(cout);
	CborWriteArray(4, cout);
	CborWriteBstr(0, (uint8_t *)" ", cout);
	CborWriteMap(0, cout);
	CborWriteNull(cout);
	CborWriteBstr(32, mac_tag, cout);

#if DEBUG_DUMP
	printf("\n%s:\n", __func__);
	sunxi_dump(cout->buffer + before, CborOutSize(cout) - before);
#endif
	return CborOutSize(cout) - before;
}

void mpi_dump(mbedtls_mpi *d)
{
	uint8_t dmp[32];

	mbedtls_mpi_write_binary(d, dmp, 32);
	sunxi_dump(dmp, 32);
}

int fill_enc_ahead(struct CborOut *cout)
{
	//        cppbor::Array()            // Enc strucure as AAD
	//            .add("Encrypt")        // Context
	//            .add(protectedParams)  // Protected -- { 1: 3 }
	//            .add(aad)              // External AAD -- {}
	//            .encode(),
	int before = CborOutSize(cout);

	uint8_t tmp_buf[64];
	struct CborOut co;
	CborOutInit(tmp_buf, 64, &co);
	CborWriteMap(1, &co);
	CborWriteUint(1, &co);
	CborWriteUint(3, &co);

	CborWriteArray(3, cout);
	CborWriteTstr("Encrypt", cout);
	CborWriteBstr(CborOutSize(&co), tmp_buf, cout);
	CborWriteBstr(0, tmp_buf, cout);

#if DEBUG_DUMP
	printf("\n%s:\n", __func__);
	sunxi_dump(cout->buffer + before, CborOutSize(cout) - before);
#endif
	return CborOutSize(cout) - before;
}

static void __check_and_dump(uint8_t *expected, uint8_t *actual, size_t size,
			     const char *prefix)
{
	int ret = memcmp(expected, actual, size);
	printf("%s: %s\n", prefix, ret == 0 ? "pass" : "failed");
	if (ret) {
		printf("expected:\n");
		sunxi_dump(expected, size);
		printf("actual:\n");
		sunxi_dump(actual, size);
	}
}

uint8_t test_ephemeral_priv[32] = {
	0x2f, 0xc5, 0x35, 0xdb, 0x1b, 0x11, 0xe3, 0xa3, 0xe5, 0xa0, 0x01,
	0x78, 0x3c, 0xf9, 0x05, 0x32, 0x98, 0x4b, 0x05, 0xf6, 0xd1, 0x26,
	0x69, 0x51, 0x71, 0x3a, 0x4b, 0x18, 0x65, 0x88, 0x8d, 0x8b,
};
uint8_t test_ephemeral_pub[32] = {
	0x98, 0xc4, 0x84, 0xfa, 0x66, 0x54, 0x35, 0x79, 0xde, 0x48, 0x18,
	0xf5, 0x0a, 0x56, 0x01, 0xab, 0xed, 0x04, 0x30, 0x47, 0x3d, 0xd6,
	0xfe, 0xa0, 0x4a, 0x73, 0x13, 0x00, 0xa4, 0x2c, 0x70, 0x4b,
};

static void __share_secret_gen(uint8_t *sharesecret)
{
	mbedtls_ecdh_context ctx_cli;
	mbedtls_ecdh_init(&ctx_cli);
	mbedtls_ecp_group_load(&ctx_cli.grp, MBEDTLS_ECP_DP_CURVE25519);
	mbedtls_ecdh_gen_public(&ctx_cli.grp, &ctx_cli.d, &ctx_cli.Q, myrand,
				NULL);
	mbedtls_mpi_read_binary_le(&ctx_cli.d, ephemeral.private, 32);

	mbedtls_mpi_read_binary_le(&ctx_cli.Q.X, ephemeral.qx, 32);

	mbedtls_ecp_copy(&ctx_cli.Qp, &ctx_cli.Q);
	mbedtls_mpi_read_binary_le(&ctx_cli.Qp.X, google.qx, 32);

	mbedtls_ecdh_compute_shared(&ctx_cli.grp, &ctx_cli.z, &ctx_cli.Qp,
				    &ctx_cli.d, NULL, NULL);
	mbedtls_mpi_write_binary_le(&ctx_cli.z, sharesecret, 32);
}

static void __aes_key_hkdf(uint8_t *aes_key, uint8_t *sharesecret)
{
	uint8_t ctx_buf[4096];
	struct CborOut ctx;
	CborOutInit(ctx_buf, 4096, &ctx);
	fill_context(&ctx);

	mbedtls_hkdf(mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), NULL, 0,
		     sharesecret, 32, ctx_buf, CborOutSize(&ctx), aes_key,
		     256 / 8);
}

static void __payload_gcm_encrypt(uint8_t *aes_key, uint8_t *iv,
				  uint8_t *payload_plaintext, uint8_t *out,
				  size_t plaintext_len)
{
	uint8_t aad_buf[4096];
	struct CborOut aad;
	CborOutInit(aad_buf, 4096, &aad);
	fill_enc_ahead(&aad);
	__aes_gcm(aes_key, iv, payload_plaintext, out, plaintext_len, aad_buf,
		  CborOutSize(&aad));
}

void protected_data_encrypted(uint8_t *iv, uint8_t *in, size_t input_len,
			      uint8_t *out)
{
	uint8_t sharesecret[32] = { 0 };
	uint8_t aes_key[32];
	__share_secret_gen(sharesecret);
	__aes_key_hkdf(aes_key, sharesecret);
#if DEBUG_DUMP
	printf("session key:\n");
	sunxi_dump(aes_key, 32);
#endif
	__payload_gcm_encrypt(aes_key, iv, in, out, input_len);
}

static void __aes_test(void)
{
	//	uint8_t google_25519_pub[32] = {
	//		0xbe, 0x85, 0xe7, 0x46, 0xc4, 0xa3, 0x42, 0x5a,
	//		0x40, 0xd9, 0x36, 0x3a, 0xa6, 0x15, 0xd0, 0x2c,
	//		0x58, 0x7e, 0x3d, 0xdc, 0x33, 0x02, 0x32, 0xd2,
	//		0xfc, 0x5e, 0x1e, 0x87, 0x25, 0x5f, 0x72, 0x60,
	//	};
	uint8_t sharesecret[32] = { 0 };

	test_ephemeral_priv[0] &= 248;
	test_ephemeral_priv[31] &= 127;
	test_ephemeral_priv[31] |= 64;
	memcpy(ephemeral.private, test_ephemeral_priv, 32);
	memcpy(ephemeral.qx, test_ephemeral_pub, 32);

	__share_secret_gen(sharesecret);
	uint8_t expected_raw_share[] = {
		0x7c, 0x61, 0xdf, 0x0c, 0x67, 0xe3, 0x14, 0x7b,
		0x65, 0xe5, 0x9d, 0xb1, 0x43, 0x08, 0x9e, 0x4e,
		0x36, 0xe4, 0x8d, 0xdd, 0xc9, 0xfd, 0x4a, 0x85,
		0xdd, 0x16, 0x5d, 0x35, 0x8c, 0xba, 0x95, 0x62,
	};
	__check_and_dump(expected_raw_share, sharesecret, 32, "raw share");

	uint8_t aes_key[32];
	__aes_key_hkdf(aes_key, sharesecret);

	uint8_t expected_aes_key[] = {
		0xc5, 0x09, 0x62, 0xa3, 0x2a, 0xfc, 0x6d, 0x9a,
		0x93, 0xf5, 0x91, 0x8a, 0x60, 0x1c, 0x78, 0x3d,
		0x57, 0x3e, 0x18, 0x8e, 0xfc, 0xd5, 0x7b, 0x12,
		0x7c, 0xd8, 0x4f, 0x46, 0xfc, 0x9b, 0xd2, 0x26,
	};
	__check_and_dump(expected_aes_key, aes_key, 32, "aes key");

	uint8_t out[1024];
	uint8_t in[] = {
		0x82, 0x84, 0x43, 0xa1, 0x01, 0x27, 0xa0, 0x58, 0x20, 0x2f,
		0xc5, 0x35, 0xdb, 0x1b, 0x11, 0xe3, 0xa3, 0xe5, 0xa0, 0x01,
		0x78, 0x3c, 0xf9, 0x05, 0x32, 0x98, 0x4b, 0x05, 0xf6, 0xd1,
		0x26, 0x69, 0x51, 0x71, 0x3a, 0x4b, 0x18, 0x65, 0x88, 0x8d,
		0x8b, 0x58, 0x40, 0x00, 0x25, 0x4d, 0xae, 0x83, 0xec, 0x66,
		0x78, 0x2f, 0xd7, 0x1c, 0x0f, 0x69, 0xf5, 0x9b, 0x09, 0x13,
		0xed, 0x9a, 0xdb, 0xb0, 0x7d, 0x97, 0xf7, 0xb6, 0x1c, 0x52,
		0xce, 0xfa, 0x79, 0x1f, 0xd8, 0x6f, 0x02, 0x85, 0x06, 0x38,
		0xdd, 0x93, 0xf5, 0x48, 0x14, 0x3c, 0x1c, 0x62, 0xbe, 0x87,
		0xbd, 0xb6, 0xc8, 0x53, 0x25, 0x54, 0xe4, 0x6a, 0xe6, 0xe1,
		0xad, 0xb4, 0xbb, 0x38, 0x12, 0xee, 0x08, 0x82, 0xa5, 0x01,
		0x01, 0x03, 0x27, 0x04, 0x02, 0x20, 0x06, 0x21, 0x58, 0x20,
		0xa9, 0x37, 0x78, 0x63, 0xfc, 0xf4, 0x46, 0xf9, 0x85, 0x6b,
		0x27, 0x65, 0xba, 0x5e, 0x4d, 0x30, 0x29, 0xf0, 0xd4, 0x72,
		0xb7, 0x04, 0xcb, 0x2d, 0x5f, 0x76, 0xa8, 0x13, 0x75, 0x3a,
		0xeb, 0x9b, 0x84, 0x43, 0xa1, 0x01, 0x27, 0xa0, 0x58, 0x4c,
		0xa4, 0x01, 0x66, 0x49, 0x73, 0x73, 0x75, 0x65, 0x72, 0x02,
		0x67, 0x53, 0x75, 0x62, 0x6a, 0x65, 0x63, 0x74, 0x3a, 0x00,
		0x47, 0x44, 0x57, 0x58, 0x2c, 0xa5, 0x01, 0x01, 0x03, 0x27,
		0x04, 0x02, 0x20, 0x06, 0x21, 0x58, 0x20, 0xa9, 0x37, 0x78,
		0x63, 0xfc, 0xf4, 0x46, 0xf9, 0x85, 0x6b, 0x27, 0x65, 0xba,
		0x5e, 0x4d, 0x30, 0x29, 0xf0, 0xd4, 0x72, 0xb7, 0x04, 0xcb,
		0x2d, 0x5f, 0x76, 0xa8, 0x13, 0x75, 0x3a, 0xeb, 0x9b, 0x3a,
		0x00, 0x47, 0x44, 0x58, 0x41, 0x20, 0x58, 0x40, 0x99, 0x32,
		0x25, 0x32, 0x85, 0xa3, 0x35, 0x0d, 0xf0, 0x1e, 0x54, 0x47,
		0x44, 0xba, 0x9a, 0xa5, 0x13, 0xc4, 0x35, 0xfe, 0x2f, 0x84,
		0x79, 0x78, 0x61, 0x35, 0x14, 0x5c, 0x78, 0xc3, 0x40, 0xae,
		0x51, 0x07, 0x94, 0x8b, 0xd1, 0xed, 0xce, 0xf9, 0x94, 0xa7,
		0x0e, 0xe8, 0xfe, 0x7d, 0x28, 0x01, 0x67, 0x90, 0xe2, 0x25,
		0xfb, 0x9f, 0x40, 0xe5, 0xe0, 0x39, 0xd9, 0xc9, 0xbb, 0x6b,
		0xf6, 0x00,
	};
	uint8_t iv[32] = {
		0x98, 0xc4, 0x84, 0xfa, 0x66, 0x54,
		0x35, 0x79, 0xde, 0x48, 0x18, 0xf5,
	};

	uint8_t expected_ciphertext[] = {
		0x22, 0x88, 0x7b, 0xba, 0xba, 0x36, 0x8e, 0xd3, 0xf9, 0x4b,
		0x92, 0x6e, 0xc4, 0x8f, 0xd3, 0x4f, 0xe5, 0x7a, 0xe3, 0xa1,
		0x5f, 0xfd, 0x4d, 0x06, 0x0a, 0x3f, 0x03, 0xc9, 0x92, 0xef,
		0x4e, 0x1e, 0x04, 0x94, 0x39, 0xef, 0xfc, 0x62, 0x5f, 0x47,
		0xb9, 0x79, 0xc3, 0x0e, 0xd0, 0x0e, 0x88, 0x31, 0x78, 0x31,
		0x86, 0x8c, 0xe7, 0x43, 0x89, 0x65, 0x67, 0x06, 0xdb, 0x06,
		0xa4, 0x15, 0x80, 0x9c, 0x91, 0x13, 0x21, 0xf8, 0xaf, 0xc2,
		0x49, 0xb1, 0xf8, 0x58, 0x03, 0x5e, 0x11, 0x9d, 0xd1, 0x9d,
		0x13, 0x95, 0x7e, 0xd9, 0xe3, 0x9e, 0x6c, 0xaf, 0x7a, 0x61,
		0x76, 0x0e, 0x99, 0x6b, 0x16, 0xf7, 0x08, 0xf9, 0xa6, 0x37,
		0x47, 0x4c, 0xc2, 0xd7, 0xe7, 0x0c, 0xc4, 0xbc, 0xc3, 0x7f,
		0x66, 0x28, 0xfd, 0x2e, 0x66, 0xe3, 0x2b, 0x36, 0x75, 0x68,
		0x5b, 0xcb, 0x7e, 0xa6, 0x9d, 0xf9, 0x80, 0xe5, 0xd3, 0xe3,
		0x52, 0x62, 0x35, 0x2e, 0xdc, 0xa6, 0x64, 0x4a, 0xbc, 0x09,
		0x7d, 0xc7, 0xb7, 0xb2, 0x9c, 0xdd, 0xe9, 0xa6, 0xfb, 0x49,
		0xed, 0xf2, 0x50, 0xe9, 0x2c, 0xd8, 0x19, 0x8d, 0x60, 0x9a,
		0x8c, 0xca, 0xe3, 0x0a, 0x44, 0x9f, 0x33, 0xdf, 0x12, 0x94,
		0x16, 0x73, 0x41, 0x05, 0x4c, 0xc1, 0xc8, 0x74, 0x24, 0x9e,
		0xd3, 0x1f, 0x7a, 0xb3, 0x78, 0x5d, 0xaf, 0x8d, 0xd2, 0xc4,
		0x16, 0xe9, 0x9a, 0x6c, 0xae, 0x7b, 0xcb, 0x43, 0xdd, 0x0a,
		0x67, 0x2c, 0xc7, 0x4d, 0xad, 0x4a, 0x61, 0x31, 0x7f, 0x61,
		0x2b, 0xc0, 0x1a, 0x21, 0xa2, 0x5e, 0x3a, 0x44, 0x96, 0xf1,
		0x70, 0x65, 0xae, 0xf0, 0x56, 0x8b, 0xe2, 0x82, 0xf4, 0xd9,
		0x5e, 0xfc, 0x46, 0xe7, 0x51, 0x9a, 0x78, 0x2b, 0x03, 0x6e,
		0x08, 0x46, 0xc1, 0xd8, 0xd5, 0xdd, 0x17, 0xdf, 0x9a, 0x4e,
		0x2d, 0xce, 0xc6, 0x58, 0x7b, 0xe3, 0x74, 0xa2, 0xa7, 0x46,
		0xf2, 0x8e, 0xee, 0xe6, 0xec, 0x2b, 0x10, 0x21, 0x44, 0x4a,
		0x73, 0x1c, 0x3f, 0xb4, 0xbc, 0xe1, 0xc2, 0xcc, 0xcb, 0xac,
		0xe5, 0x15, 0x2b, 0x42, 0xa6, 0xf5, 0x89, 0xaf, 0xd2, 0xea,
		0xfb, 0x5e, 0x6b, 0x82, 0xe0, 0xe5, 0x04, 0xf4, 0x6a, 0xb6,
		0x4c, 0x9b, 0x50, 0x8e, 0xe0, 0xe4, 0x00, 0xe1, 0xee, 0x03,
		0xfb, 0x35, 0xd6, 0xc6, 0x50, 0x3e, 0x33, 0xaa,
	};

	//__payload_gcm_encrypt(aes_key,iv,in,out,sizeof(in));
	protected_data_encrypted(iv, in, sizeof(in), out);
	__check_and_dump(expected_ciphertext, out, sizeof(expected_ciphertext),
			 "cipher text");
}

static void __recipent_test(void)
{
	uint8_t tmp_buf[1024];
	struct CborOut co;

	memcpy(ephemeral.qx, test_ephemeral_pub, sizeof(test_ephemeral_pub));
	memcpy(ephemeral.private, test_ephemeral_pub,
	       sizeof(test_ephemeral_pub));
	ephemeral.isX25519 = 1;

	memset(tmp_buf, 0, 1024);
	CborOutInit(tmp_buf, 1024, &co);
	fill_recipients(&co);

	uint8_t expected[] = {
		0x81, 0x83, 0x44, 0xa1, 0x01, 0x38, 0x18, 0xa2, 0x04, 0x58,
		0x20, 0xd0, 0xae, 0xc1, 0x15, 0xca, 0x2a, 0xcf, 0x73, 0xae,
		0x6b, 0xcc, 0xcb, 0xd1, 0x96, 0x1d, 0x65, 0xe8, 0xb1, 0xdd,
		0xd7, 0x4a, 0x1a, 0x37, 0xb9, 0x43, 0x3a, 0x97, 0xd5, 0x99,
		0xdf, 0x98, 0x08, 0x20, 0xa3, 0x01, 0x01, 0x20, 0x04, 0x21,
		0x58, 0x20, 0x98, 0xc4, 0x84, 0xfa, 0x66, 0x54, 0x35, 0x79,
		0xde, 0x48, 0x18, 0xf5, 0x0a, 0x56, 0x01, 0xab, 0xed, 0x04,
		0x30, 0x47, 0x3d, 0xd6, 0xfe, 0xa0, 0x4a, 0x73, 0x13, 0x00,
		0xa4, 0x2c, 0x70, 0x4b, 0xf6,
	};
	__check_and_dump(expected, tmp_buf, sizeof(expected), "recipent");
}

int fill_maced_public_key_mac_struct(struct CborOut *cout)
{
	//     MAC_structure = [
	//         context : "MAC0",
	//         protected : bstr .cbor { 1 : 5 },
	//         external_aad : bstr .size 0,
	//         payload : bstr .cbor PublicKey (no public key at CSR, [ 0x80(0 len array) ])
	//     ]
	int before = CborOutSize(cout);

	CborWriteArray(4, cout);

	CborWriteTstr("MAC0", cout);

	uint8_t tmp_buf[64];
	struct CborOut co;
	CborOutInit(tmp_buf, 64, &co);
	CborWriteMap(1, &co);
	CborWriteUint(1, &co);
	CborWriteUint(5, &co);
	CborWriteBstr(CborOutSize(&co), tmp_buf, cout);
	CborWriteBstr(0, tmp_buf, cout);
	tmp_buf[0] = 0x80;
	CborWriteBstr(1, tmp_buf, cout);

#if DEBUG_DUMP
	printf("\n%s:\n", __func__);
	sunxi_dump(cout->buffer + before, CborOutSize(cout) - before);
#endif
	return CborOutSize(cout) - before;
}

int __mac_tag_generate(uint8_t *mac_key, uint8_t *mac_tag)
{
	uint8_t buf[1024];
	struct CborOut cout;
	CborOutInit(buf, 1024, &cout);
	fill_maced_public_key_mac_struct(&cout);

	hmac(buf, mac_key, mac_tag, CborOutSize(&cout));
	return 0;
}

static void __mac_test(void)
{
	uint8_t test_mac_key[] = {
		0x2f, 0xc5, 0x35, 0xdb, 0x1b, 0x11, 0xe3, 0xa3,
		0xe5, 0xa0, 0x01, 0x78, 0x3c, 0xf9, 0x05, 0x32,
		0x98, 0x4b, 0x05, 0xf6, 0xd1, 0x26, 0x69, 0x51,
		0x71, 0x3a, 0x4b, 0x18, 0x65, 0x88, 0x8d, 0x8b,
	};

	uint8_t mac_tag[32];
	__mac_tag_generate(test_mac_key, mac_tag);

	uint8_t mac_pub_buf[1024];
	struct CborOut mac_pub;
	CborOutInit(mac_pub_buf, 1024, &mac_pub);
	fill_maced_keys_to_sign(&mac_pub, mac_tag);

	uint8_t expected[] = {
		0x84, 0x40, 0xa0, 0xf6, 0x58, 0x20, 0x83, 0x86, 0x0d, 0x78,
		0xfc, 0xa3, 0x01, 0x68, 0x1d, 0x38, 0xd9, 0x7f, 0xed, 0xf4,
		0xf4, 0x78, 0x2f, 0x4e, 0x46, 0x14, 0xa3, 0x2b, 0xb4, 0x5c,
		0x0e, 0xf1, 0x4f, 0x4f, 0x66, 0x88, 0x80, 0xa2,
	};
	__check_and_dump(expected, mac_pub_buf, sizeof(expected), "mac test");
}

int csr_generate(uint8_t *out, size_t *len)
{
	uint8_t challenge[16];
	uint8_t mac_tag[32];
	//ephemeral key generate
	x25519_pair_gen(&ephemeral);
	//mac key generate
	hmac(ephemeral.private, ephemeral.qx, ephemeral_mac_key, 32);
	//challenge
	myrand(NULL, challenge, 16);

	//mac tag generate
	__mac_tag_generate(ephemeral_mac_key, mac_tag);

	struct CborOut cout;
	CborOutInit(out, *len, &cout);
	CborWriteArray(4, &cout);

	CborWriteArray(2, &cout);
	fill_deviceinfo(&cout);
	//an empty map
	CborWriteMap(0, &cout);

	//challenge
	CborWriteBstr(16, challenge, &cout);
	//protected data
	fill_protected_data(&cout, ephemeral_mac_key, challenge, mac_tag);
	fill_maced_keys_to_sign(&cout, mac_tag);

	*len = CborOutSize(&cout);
	return 0;
}

void csr_gen_self_test(void)
{
	__aes_test();
	__mac_test();
	__recipent_test();
}

void base64_csr_generate(char *out)
{
	uint8_t tmp_csr[4096];
	size_t olen = 4096, base64_len;
	uint8_t pri_gen_input[32];
	struct ecc_pubkey_t pubkey;
	//dk gen
	sunxi_trng_gen(pri_gen_input, 32);
	private_gen(pri_gen_input, 32, dk.private);
	sunxi_ecc_pub_gen(dk.private, &pubkey, &p256_param);
	__rsa_padding(dk.qx, pubkey.qx, 32, 32);
	__rsa_padding(dk.qy, pubkey.qy, 32, 32);
	dk.isX25519 = 0;

	__prepare_device_info();
	__prepare_verify_boot_data();

	csr_generate(tmp_csr, &olen);
#ifdef DEBUG_DUMP
	printf("generated csr:\n");
	sunxi_dump(tmp_csr, ALIGN(olen, 16));
#endif
	mbedtls_base64_encode((void *)out, 4096, &base64_len, tmp_csr, olen);
}

int do_rkp(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
#if 1
	memset((void *)0x40000000, 0, 8192);
	key_extraction_output_generate((void *)0x40000000, 8192);
	printf("\n\n");
	puts((char *)0x40000000);
	printf("\n\n");
#else
	uint8_t pri_gen_input[32];
	struct ecc_pubkey_t pubkey;
	//dk gen
	sunxi_trng_gen(pri_gen_input, 32);
	private_gen(pri_gen_input, 32, dk.private);
	sunxi_ecc_pub_gen(dk.private, &pubkey, &p256_param);
	__rsa_padding(dk.qx, pubkey.qx, 32, 32);
	__rsa_padding(dk.qy, pubkey.qy, 32, 32);
	dk.isX25519 = 0;
//csr gen
//csr_generate((void*)0x40000000, &olen);
#endif
	csr_gen_self_test();
	return 0;
}
U_BOOT_CMD(sunxi_rkp, 6, 1, do_rkp, "sunxi rkp", "");
#endif
