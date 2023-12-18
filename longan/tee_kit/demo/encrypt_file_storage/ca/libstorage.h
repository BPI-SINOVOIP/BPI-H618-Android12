#ifndef _LIBSTORAGE_H__
#define _LIBSTORAGE_H__
#include <string.h>
#include <stdio.h>

typedef void *OPTEE_FS_CONTEXT_HANDLER;
typedef uint32_t OPTEE_FS_FILE_HANDLER;

uint32_t optee_fs_init_ctx(OPTEE_FS_CONTEXT_HANDLER *ctx_handler);
uint32_t optee_fs_finalize_ctx(OPTEE_FS_CONTEXT_HANDLER *ctx_handler);
uint32_t optee_fs_create(OPTEE_FS_CONTEXT_HANDLER ctx, void *file_name,
			 OPTEE_FS_FILE_HANDLER *obj);
uint32_t optee_fs_open(OPTEE_FS_CONTEXT_HANDLER ctx, void *file_name,
		       OPTEE_FS_FILE_HANDLER *obj);
uint32_t optee_fs_read(OPTEE_FS_CONTEXT_HANDLER ctx, OPTEE_FS_FILE_HANDLER obj,
		       void *data, uint32_t data_size, uint32_t *count);
uint32_t optee_fs_write(OPTEE_FS_CONTEXT_HANDLER ctx, OPTEE_FS_FILE_HANDLER obj,
			void *data, uint32_t data_size);
uint32_t optee_fs_unlink(OPTEE_FS_CONTEXT_HANDLER ctx,
			 OPTEE_FS_FILE_HANDLER obj);

#endif
