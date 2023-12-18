
#include <tee_client_api.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#define API_DEMO_UUID { 0x17bce0c9, 0x260c, 0x48e8, \
	{0xb0,0xdd,0x36,0xc9,0x5d,0x3c,0xad,0xba} }

static const TEEC_UUID ta_UUID = API_DEMO_UUID;
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
void try_rotpk(void);

int main(int argc, char **argv)
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
	tee_params_p.size  = 20;
	tee_params_p.flags = TEEC_MEM_INPUT;
	if (TEEC_AllocateSharedMemory(&ctx, &tee_params_p) != TEEC_SUCCESS)
		goto failOpen;

	TEEC_SharedMemory tee_params_q;
	tee_params_q.size  = 128;
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

	command = 0x210;
	if (argc == 2) {
		memcpy(tee_params_p.buffer, argv[1], 10);
	} else if (argc == 3) {
		if (strcmp("-ext_key", argv[1]) == 0) {
			memcpy(tee_params_p.buffer, argv[2], 10);
			command = 0x220;
		} else if (strcmp("op", argv[1]) == 0 &&
			   strcmp("hmac", argv[2]) == 0) {
			command = 0x230;
		} else if (strcmp("op", argv[1]) == 0 &&
			   strcmp("aes_cbc", argv[2]) == 0) {
			command = 0x231;
		}
	} else if (argc == 5) {
		if (strcmp("-burn", argv[1]) == 0) {
			memcpy(tee_params_p.buffer, argv[2], 10);
			memcpy(tee_params_q.buffer, argv[3], atoi(argv[4]));
			command = 0x221;

			operation.params[2].value.a = atoi(argv[4]);
		}
	}
	printf("CA:invoke command\n");
	teecErr = TEEC_InvokeCommand(&teecSession, command, &operation, NULL);
	if (command == 0x210) {
		dump((uint8_t *)tee_params_q.buffer,
		     operation.params[2].value.a);
	}

	TEEC_ReleaseSharedMemory(&tee_params_q);
failOpen2:
	TEEC_ReleaseSharedMemory(&tee_params_p);
failOpen:
	TEEC_FinalizeContext(&ctx);
failInit:
	printf("CA:finish with %x\n", teecErr);

	if (teecErr == TEEC_SUCCESS)
		return 0;
	else
		return teecErr;
}

int write_rotpk_hash(const char *buf);
int write_rotpk_hash(__attribute__((unused)) const char *buf)
{
	TEEC_Context ctx;
	TEEC_Result teecErr;
	TEEC_Session teecSession;
	TEEC_Operation operation;
	int command;
	uint8_t rotpk_hash[32];

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
	tee_params_p.size  = 20;
	tee_params_p.flags = TEEC_MEM_INPUT;
	if (TEEC_AllocateSharedMemory(&ctx, &tee_params_p) != TEEC_SUCCESS)
		goto failOpen;

	TEEC_SharedMemory tee_params_q;
	tee_params_q.size  = 128;
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

	memcpy(tee_params_p.buffer, "rotpk", sizeof("rotpk"));
	memcpy(tee_params_q.buffer, rotpk_hash, sizeof(rotpk_hash));
	operation.params[2].value.a = sizeof(rotpk_hash);
	command			    = 0x221;

	printf("CA:invoke command\n");
	teecErr = TEEC_InvokeCommand(&teecSession, command, &operation, NULL);

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
	return 0;
}

int read_rotpk_hash(char *buf);
int read_rotpk_hash(char *buf)
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
	tee_params_p.size  = 20;
	tee_params_p.flags = TEEC_MEM_INPUT;
	if (TEEC_AllocateSharedMemory(&ctx, &tee_params_p) != TEEC_SUCCESS)
		goto failOpen;

	TEEC_SharedMemory tee_params_q;
	tee_params_q.size  = 128;
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

	command = 0x210;
	memcpy(tee_params_p.buffer, "rotpk", sizeof("rotpk"));

	printf("CA:invoke command\n");
	teecErr = TEEC_InvokeCommand(&teecSession, command, &operation, NULL);
	if (teecErr == 0) {
		dump((uint8_t *)tee_params_q.buffer,
		     operation.params[2].value.a);
		memcpy(buf, tee_params_q.buffer, operation.params[2].value.a);
	}

	TEEC_ReleaseSharedMemory(&tee_params_q);
failOpen2:
	TEEC_ReleaseSharedMemory(&tee_params_p);
failOpen:
	TEEC_FinalizeContext(&ctx);
failInit:
	printf("CA:finish with %d\n", teecErr);

	return operation.params[2].value.a;
}

void try_rotpk(void)
{
	uint8_t rotpk_buf[256];
	int read_len;
	write_rotpk_hash((const char *)rotpk_buf);
	read_len = read_rotpk_hash((char *)rotpk_buf);
	printf("read result\n");
	dump(rotpk_buf, read_len);
}
