/*
 * Copyright (c) 2014, STMicroelectronics International N.V.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License Version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <tee_client_api.h>
#include <tee_api_defines.h>
#include <tee_api_defines_extensions.h>

#include "libstorage.h"
#define TA_STORAGE_UUID { 0x2977f028, 0x30d8, 0x478b, \
	{ 0x97, 0x5c, 0xbe, 0xeb, 0x3c, 0x13, 0x4c, 0x34 } }

#define TA_STORAGE_CMD_OPEN			0
#define TA_STORAGE_CMD_READ			1
#define TA_STORAGE_CMD_WRITE			2
#define TA_STORAGE_CMD_UNLINK			3
#define TA_STORAGE_CMD_CREATE			4

#define TA_STORAGE_CMD_OPEN 0
#define TA_STORAGE_CMD_READ 1
#define TA_STORAGE_CMD_WRITE 2
#define TA_STORAGE_CMD_UNLINK 3
#define TA_STORAGE_CMD_CREATE 4

#define TEEC_OPERATION_INITIALIZER {0}

struct optee_fs_context {
	TEEC_Context ctx;
	TEEC_Session sess;
};

uint32_t optee_fs_init_ctx(OPTEE_FS_CONTEXT_HANDLER *ctx_handler)
{
	char *_device	  = NULL;
	TEEC_UUID ta_test_uuid = TA_STORAGE_UUID;
	TEEC_Result tee_res;
	struct optee_fs_context *fs_ctx;
	uint32_t ret_orig = 0;

	if (!ctx_handler) {
		return 0xFFFF8002;
	}
	*ctx_handler = (OPTEE_FS_CONTEXT_HANDLER)malloc(sizeof(struct optee_fs_context));
	if (*ctx_handler == (OPTEE_FS_CONTEXT_HANDLER)NULL) {
		return 0xFFFF8001;
	}
	fs_ctx  = (struct optee_fs_context *)*ctx_handler;
	tee_res = TEEC_InitializeContext(_device, &(fs_ctx->ctx));
	if (tee_res) {
		goto init_malloced;
	}
	tee_res =
		TEEC_OpenSession(&(fs_ctx->ctx), &(fs_ctx->sess), &ta_test_uuid,
				 TEEC_LOGIN_PUBLIC, NULL, NULL, &ret_orig);
	if (tee_res) {
		goto init_ctx_inited;
	}
	return tee_res;

init_ctx_inited:
	TEEC_FinalizeContext(&(fs_ctx->ctx));
init_malloced:
	free((void *)*ctx_handler);
	*ctx_handler = (OPTEE_FS_CONTEXT_HANDLER)NULL;
	return tee_res;
}

uint32_t optee_fs_finalize_ctx(OPTEE_FS_CONTEXT_HANDLER *ctx_handler)
{
	struct optee_fs_context *fs_ctx =
		(struct optee_fs_context *)*ctx_handler;
	if (!(*ctx_handler)) {
		return 0xFFFF8002;
	}
	TEEC_CloseSession(&(fs_ctx->sess));
	TEEC_FinalizeContext(&(fs_ctx->ctx));
	free((void *)*ctx_handler);
	*ctx_handler = (OPTEE_FS_CONTEXT_HANDLER)NULL;
	return 0;
}

uint32_t optee_fs_create(OPTEE_FS_CONTEXT_HANDLER ctx, void *file_name,
			 OPTEE_FS_FILE_HANDLER *obj)
{
	TEEC_Operation op = TEEC_OPERATION_INITIALIZER;
	TEEC_Result res;
	uint32_t org;
	struct optee_fs_context *fs_ctx = (struct optee_fs_context *)ctx;

	TEEC_SharedMemory share_id;
	share_id.size  = strlen(file_name) + 1;
	share_id.flags = TEEC_MEM_INPUT | TEEC_MEM_OUTPUT;

	if (TEEC_AllocateSharedMemory(&(fs_ctx->ctx), &share_id) !=
	    TEEC_SUCCESS) {
		printf("%s: allocate  share memory fail", __func__);
		return TEEC_ERROR_OUT_OF_MEMORY;
	}

	memcpy(share_id.buffer, file_name, strlen(file_name) + 1);

	op.started		   = 1;
	op.params[0].memref.parent = &share_id;
	op.params[0].memref.size   = 0;
	op.params[1].value.a       = TEE_DATA_FLAG_ACCESS_WRITE |
			       TEE_DATA_FLAG_ACCESS_READ |
			       TEE_DATA_FLAG_ACCESS_WRITE_META;
	op.params[1].value.b = 0;
	op.params[2].value.a = TEE_STORAGE_PRIVATE_REE;
	op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_WHOLE, TEEC_VALUE_INOUT,
					 TEEC_VALUE_INPUT, TEEC_NONE);

	res = TEEC_InvokeCommand(&(fs_ctx->sess), TA_STORAGE_CMD_CREATE, &op,
				 &org);

	if (res == TEEC_SUCCESS)
		*obj = op.params[1].value.b;

	TEEC_ReleaseSharedMemory(&share_id);

	return res;
}

uint32_t optee_fs_open(OPTEE_FS_CONTEXT_HANDLER ctx, void *file_name,
		       OPTEE_FS_FILE_HANDLER *obj)
{
	TEEC_Operation op = TEEC_OPERATION_INITIALIZER;
	TEEC_Result res;
	uint32_t org;
	struct optee_fs_context *fs_ctx = (struct optee_fs_context *)ctx;

	TEEC_SharedMemory share_id;
	share_id.size  = strlen(file_name) + 1;
	share_id.flags = TEEC_MEM_INPUT | TEEC_MEM_OUTPUT;

	if (TEEC_AllocateSharedMemory(&(fs_ctx->ctx), &share_id) !=
	    TEEC_SUCCESS) {
		printf("%s: allocate  share memory fail", __func__);
		return TEEC_ERROR_OUT_OF_MEMORY;
	}

	memcpy(share_id.buffer, file_name, strlen(file_name) + 1);

	op.started		   = 1;
	op.params[0].memref.parent = &share_id;
	op.params[0].memref.size   = 0;
	op.params[1].value.a       = TEE_DATA_FLAG_ACCESS_WRITE |
			       TEE_DATA_FLAG_ACCESS_READ |
			       TEE_DATA_FLAG_ACCESS_WRITE_META;
	op.params[1].value.b = 0;
	op.params[2].value.a = TEE_STORAGE_PRIVATE_REE;
	op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_WHOLE, TEEC_VALUE_INOUT,
					 TEEC_VALUE_INPUT, TEEC_NONE);

	res = TEEC_InvokeCommand(&(fs_ctx->sess), TA_STORAGE_CMD_OPEN, &op,
				 &org);

	if (res == TEEC_SUCCESS)
		*obj = op.params[1].value.b;

	TEEC_ReleaseSharedMemory(&share_id);

	return res;
}

uint32_t optee_fs_read(OPTEE_FS_CONTEXT_HANDLER ctx, OPTEE_FS_FILE_HANDLER obj,
		       void *data, uint32_t data_size, uint32_t *count)
{
	TEEC_Result res;
	TEEC_Operation op = TEEC_OPERATION_INITIALIZER;
	uint32_t org;
	struct optee_fs_context *fs_ctx = (struct optee_fs_context *)ctx;

	TEEC_SharedMemory share_id;
	share_id.size  = data_size;
	share_id.flags = TEEC_MEM_INPUT | TEEC_MEM_OUTPUT;

	if (TEEC_AllocateSharedMemory(&(fs_ctx->ctx), &share_id) !=
	    TEEC_SUCCESS) {
		printf("%s: allocate  share memory fail", __func__);
		return TEEC_ERROR_OUT_OF_MEMORY;
	}

	memcpy(share_id.buffer, data, data_size);

	op.started = 1;

	op.params[0].memref.parent = &share_id;
	op.params[0].memref.size   = 0;
	op.params[1].value.a       = obj;
	op.params[1].value.b       = 0;
	op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_WHOLE, TEEC_VALUE_INOUT,
					 TEEC_NONE, TEEC_NONE);

	res = TEEC_InvokeCommand(&(fs_ctx->sess), TA_STORAGE_CMD_READ, &op,
				 &org);

	if (res == TEEC_SUCCESS)
		*count = op.params[1].value.b;

	memcpy(data, share_id.buffer, data_size);
	TEEC_ReleaseSharedMemory(&share_id);

	return res;
}

uint32_t optee_fs_write(OPTEE_FS_CONTEXT_HANDLER ctx, OPTEE_FS_FILE_HANDLER obj,
			void *data, uint32_t data_size)
{
	TEEC_Operation op = TEEC_OPERATION_INITIALIZER;
	uint32_t org;
	TEEC_Result res;
	struct optee_fs_context *fs_ctx = (struct optee_fs_context *)ctx;

	TEEC_SharedMemory share_id;
	share_id.size  = data_size;
	share_id.flags = TEEC_MEM_INPUT | TEEC_MEM_OUTPUT;

	if (TEEC_AllocateSharedMemory(&(fs_ctx->ctx), &share_id) !=
	    TEEC_SUCCESS) {
		printf("%s: allocate  share memory fail", __func__);
		return TEEC_ERROR_OUT_OF_MEMORY;
	}
	memcpy(share_id.buffer, data, data_size);

	op.started		   = 1;
	op.params[0].memref.parent = &share_id;
	op.params[0].memref.size   = 0;
	op.params[1].value.a       = obj;
	op.params[1].value.b       = 0;
	op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_WHOLE, TEEC_VALUE_INPUT,
					 TEEC_NONE, TEEC_NONE);

	res = TEEC_InvokeCommand(&(fs_ctx->sess), TA_STORAGE_CMD_WRITE, &op,
				 &org);
	memcpy(data, share_id.buffer, data_size);
	TEEC_ReleaseSharedMemory(&share_id);
	return res;
}

uint32_t optee_fs_unlink(OPTEE_FS_CONTEXT_HANDLER ctx, OPTEE_FS_FILE_HANDLER obj)
{
	TEEC_Operation op = TEEC_OPERATION_INITIALIZER;
	uint32_t org;
	struct optee_fs_context *fs_ctx = (struct optee_fs_context *)ctx;

	op.params[0].value.a = obj;
	op.started	   = 1;
	op.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INPUT, TEEC_NONE, TEEC_NONE,
					 TEEC_NONE);

	return TEEC_InvokeCommand(&(fs_ctx->sess), TA_STORAGE_CMD_UNLINK, &op,
				  &org);
}
