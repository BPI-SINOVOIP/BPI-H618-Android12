
#include <tee_client_api.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define API_DEMO_UUID { 0xcb71e0c9, 0x260c, 0x48e8, \
	{0xb0,0xdd,0x36,0xc9,0x5d,0x3c,0xad,0xba} }

static const TEEC_UUID ta_UUID = API_DEMO_UUID;
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
#else
void dump(__attribute__((unused)) uint8_t *buf,
	  __attribute__((unused)) int ttl_len)
{
	;
}
#endif
void try_rotpk(void);

int ui_encrypt_secretaes1_by_unique_key(__attribute__((unused)) pid_t pid,
					char *data_in, int data_in_len,
					char *data_out, int *data_out_len,
					int encrypt);
__attribute__((unused)) static void demo_A(void)
{
	char raw_data[128];
	char enc_data[128];
	char dec_data[128];
	int out_len;

	printf("raw_data:\n");
	dump((void *)raw_data, 128);
	ui_encrypt_secretaes1_by_unique_key(0, raw_data, 128, enc_data, &out_len,
					    1);
	printf("enc_data:\n");
	dump((void *)enc_data, 128);
	ui_encrypt_secretaes1_by_unique_key(0, enc_data, 128, dec_data, &out_len,
					    0);
	printf("dec_data:\n");
	dump((void *)dec_data, 128);
	if (memcmp(raw_data, dec_data, 128) == 0) {
		printf("data encrypt and decryt success\n");
	}
}
void demo_B(void);
int main(int argc, char **argv)
{
	if (argc == 2) {
		if (strcmp(argv[1], "demo-a") == 0) {
			demo_A();
		} else if (strcmp(argv[1], "demo-b") == 0) {
			demo_B();
		}
	}

	return 0;
}

int ui_encrypt_secretaes1_by_unique_key(__attribute__((unused)) pid_t pid,
					char *data_in, int data_in_len,
					char *data_out, int *data_out_len,
					int encrypt)
{
	TEEC_Context ctx;
	TEEC_Result teecErr;
	TEEC_Session teecSession;
	TEEC_Operation operation;
	int command;

	printf("CA:init context\n");

	teecErr = TEEC_InitializeContext(NULL, &ctx);
	if (teecErr != TEEC_SUCCESS)
		goto failInit;

	printf("CA:open session\n");
	teecErr = TEEC_OpenSession(&ctx, &teecSession, &ta_UUID,
				   TEEC_LOGIN_PUBLIC, NULL, NULL, NULL);
	if (teecErr != TEEC_SUCCESS)
		goto failOpen;

	printf("CA:allocate memory\n");
	TEEC_SharedMemory tee_params_p;
	tee_params_p.size  = data_in_len;
	tee_params_p.flags = TEEC_MEM_INPUT;
	if (TEEC_AllocateSharedMemory(&ctx, &tee_params_p) != TEEC_SUCCESS)
		goto failOpen;

	TEEC_SharedMemory tee_params_q;
	tee_params_q.size  = data_in_len;
	tee_params_q.flags = TEEC_MEM_INPUT | TEEC_MEM_OUTPUT;
	if (TEEC_AllocateSharedMemory(&ctx, &tee_params_q) != TEEC_SUCCESS)
		goto failOpen2;

	memset(&operation, 0, sizeof(TEEC_Operation));
	operation.paramTypes =
		TEEC_PARAM_TYPES(TEEC_MEMREF_WHOLE, TEEC_MEMREF_WHOLE,
				 TEEC_VALUE_INOUT, TEEC_NONE);
	operation.started = 1;

	operation.params[0].memref.parent = &tee_params_p;
	operation.params[0].memref.offset = 0;
	operation.params[0].memref.size   = 0;

	operation.params[1].memref.parent = &tee_params_q;
	operation.params[1].memref.offset = 0;
	operation.params[1].memref.size   = 0;

	if (encrypt) {
		command = 0x210;
	} else {
		command = 0x211;
	}
	memcpy(tee_params_p.buffer, data_in, data_in_len);
	operation.params[2].value.a = data_in_len;
	printf("CA:invoke command\n");
	teecErr = TEEC_InvokeCommand(&teecSession, command, &operation, NULL);
	*data_out_len = operation.params[2].value.b;
	memcpy(data_out, tee_params_q.buffer, *data_out_len);

	TEEC_ReleaseSharedMemory(&tee_params_q);
failOpen2:
	TEEC_ReleaseSharedMemory(&tee_params_p);
failOpen:
	TEEC_FinalizeContext(&ctx);
failInit:
	printf("CA:finish with %d\n", teecErr);
	if (teecErr == TEEC_SUCCESS)
		return 0;
	else
		return teecErr;
}

TEEC_Context demo_B_ctx;
TEEC_Session demo_B_session;
static int set_info(__attribute__((unused)) pid_t pid, char *in_data,
		    int in_len, __attribute__((unused)) int info_type)
{
	TEEC_Result teecErr;
	TEEC_Operation operation;
	int command;

	printf("CA:init context\n");

	teecErr = TEEC_InitializeContext(NULL, &demo_B_ctx);
	if (teecErr != TEEC_SUCCESS)
		goto failInit;

	printf("CA:open session\n");
	teecErr = TEEC_OpenSession(&demo_B_ctx, &demo_B_session, &ta_UUID,
				   TEEC_LOGIN_PUBLIC, NULL, NULL, NULL);
	if (teecErr != TEEC_SUCCESS)
		goto failOpen;

	printf("CA:allocate memory\n");
	TEEC_SharedMemory tee_params_p;
	tee_params_p.size  = in_len;
	tee_params_p.flags = TEEC_MEM_INPUT;
	if (TEEC_AllocateSharedMemory(&demo_B_ctx, &tee_params_p) !=
	    TEEC_SUCCESS)
		goto failOpen;

	memset(&operation, 0, sizeof(TEEC_Operation));
	operation.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_WHOLE, TEEC_NONE,
						TEEC_VALUE_INOUT, TEEC_NONE);
	operation.started = 1;

	operation.params[0].memref.parent = &tee_params_p;
	operation.params[0].memref.offset = 0;
	operation.params[0].memref.size   = 0;

	operation.params[2].value.a = in_len;

	memcpy(tee_params_p.buffer, in_data, in_len);
	printf("CA:invoke command\n");
	command = 0x220;
	teecErr =
		TEEC_InvokeCommand(&demo_B_session, command, &operation, NULL);

	TEEC_ReleaseSharedMemory(&tee_params_p);

	if (teecErr != TEEC_SUCCESS) {
		TEEC_FinalizeContext(&demo_B_ctx);
	}
	return teecErr;

failOpen:
	TEEC_FinalizeContext(&demo_B_ctx);
failInit:
	return teecErr;
}

static int ui_gen_digest_inside(__attribute__((unused)) pid_t pid,
				char *data_in, int data_in_len, char *data_out,
				int *data_out_len,
				__attribute__((unused)) int digest_type)
{
	TEEC_Result teecErr;
	TEEC_Operation operation;
	int command;

	TEEC_SharedMemory tee_params_p;
	tee_params_p.size  = data_in_len;
	tee_params_p.flags = TEEC_MEM_INPUT;
	if (TEEC_AllocateSharedMemory(&demo_B_ctx, &tee_params_p) !=
	    TEEC_SUCCESS)
		goto failInBuf;

	TEEC_SharedMemory tee_params_q;
	tee_params_q.size  = 64;
	tee_params_q.flags = TEEC_MEM_INPUT | TEEC_MEM_OUTPUT;
	if (TEEC_AllocateSharedMemory(&demo_B_ctx, &tee_params_q) !=
	    TEEC_SUCCESS)
		goto failOutBuf;

	operation.paramTypes =
		TEEC_PARAM_TYPES(TEEC_MEMREF_WHOLE, TEEC_MEMREF_WHOLE,
				 TEEC_VALUE_INOUT, TEEC_NONE);
	operation.started = 1;

	operation.params[0].memref.parent = &tee_params_p;
	operation.params[0].memref.offset = 0;
	operation.params[0].memref.size   = 0;

	operation.params[1].memref.parent = &tee_params_q;
	operation.params[1].memref.offset = 0;
	operation.params[1].memref.size   = 0;

	operation.params[2].value.a = data_in_len;
	memcpy(tee_params_p.buffer, data_in, data_in_len);

	command = 0x221;
	teecErr =
		TEEC_InvokeCommand(&demo_B_session, command, &operation, NULL);
	*data_out_len = operation.params[2].value.b;
	memcpy(data_out, tee_params_q.buffer, *data_out_len);

	TEEC_ReleaseSharedMemory(&tee_params_q);
	TEEC_ReleaseSharedMemory(&tee_params_p);

	return teecErr;

failOutBuf:
	TEEC_ReleaseSharedMemory(&tee_params_p);
failInBuf:
	return -1;
}

static void finalize_tee(void)
{
	TEEC_FinalizeContext(&demo_B_ctx);
}

void init_tee(void);
#define nv_read_key(...)
void init_tee(void)
{
	char secret_uuid_base64_unique[96] = {
		0x86, 0x40, 0xbe, 0x5c, 0x3a, 0x1f, 0xdc, 0xcb, 0xa0, 0x76,
		0xa4, 0xbc, 0x07, 0x17, 0x23, 0x23, 0x43, 0xc7, 0xaa, 0x7d,
		0x1b, 0x1f, 0x3c, 0x2a, 0x5a, 0x1e, 0x34, 0x90, 0xd4, 0xeb,
		0xbc, 0x6e, 0xb9, 0x12, 0x7e, 0x7d, 0x95, 0x6f, 0x98, 0xe5,
		0x5d, 0x29, 0xcc, 0x2e, 0x88, 0x0c, 0x20, 0xa1, 0x75, 0x09,
		0x5d, 0x60, 0xb1, 0x7f, 0x80, 0xc9, 0x54, 0xb5, 0x37, 0x16,
		0x6b, 0xdc, 0xbb, 0xf3, 0x8d, 0xa9, 0x05, 0x08, 0x1e, 0xa1,
		0xd6, 0x77, 0xcb, 0x78, 0xce, 0x8e, 0x3c, 0x32, 0x3b, 0xa6,
		0x4b, 0xc0, 0x69, 0x7e, 0xa8, 0x17, 0x8e, 0xe6, 0xba, 0x06,
		0x1d, 0xff, 0xa6, 0x4e, 0x38, 0x30,
	};
	nv_read_key(secret_uuid_base64_unique, 96);
	set_info(0, secret_uuid_base64_unique, 96, 0);
}

void demo_B(void)
{
	char in_data[256];
	char out_data[256];
	int out_len;
	init_tee();
	ui_gen_digest_inside(0, in_data, 256, out_data, &out_len, 0);
	printf("input:\n");
	dump((void *)in_data, 256);
	printf("output:\n");
	dump((void *)out_data, out_len);

	ui_gen_digest_inside(0, in_data, 128, out_data, &out_len, 0);
	printf("input:\n");
	dump((void *)in_data, 128);
	printf("output:\n");
	dump((void *)out_data, out_len);

	ui_gen_digest_inside(0, in_data + 128, 128, out_data, &out_len, 0);
	printf("input:\n");
	dump((void *)(in_data + 128), 128);
	printf("output:\n");
	dump((void *)out_data, out_len);

	finalize_tee();
}
