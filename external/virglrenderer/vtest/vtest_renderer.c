/**************************************************************************
 *
 * Copyright (C) 2015 Red Hat Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <limits.h>

#include "virgl_hw.h"
#include "virglrenderer.h"

#include <sys/uio.h>
#include <sys/socket.h>
#include <sys/mman.h>

#include "vtest.h"
#include "vtest_shm.h"
#include "vtest_protocol.h"

#include "util.h"
#include "util/u_debug.h"
#include "util/u_double_list.h"
#include "util/u_math.h"
#include "util/u_memory.h"
#include "util/u_hash_table.h"

struct vtest_resource {
   uint32_t res_id;

   struct iovec iov;
};

struct vtest_context {
   struct list_head head;

   int ctx_id;

   struct vtest_input *input;
   int out_fd;

   char *debug_name;

   unsigned protocol_version;
   unsigned capset_id;
   bool context_initialized;

   struct util_hash_table *resource_table;
};

struct vtest_renderer {
   const char *rendernode_name;

   uint32_t max_length;

   int fence_id;
   int last_fence;

   struct list_head active_contexts;
   struct list_head free_contexts;
   int next_context_id;

   struct vtest_context *current_context;
};

static void vtest_write_fence(UNUSED void *cookie, uint32_t fence_id_in)
{
   struct vtest_renderer *renderer = (struct vtest_renderer*)cookie;
   renderer->last_fence = fence_id_in;
}

static int vtest_get_drm_fd(void *cookie)
{
   int fd = -1;
   struct vtest_renderer *renderer = (struct vtest_renderer*)cookie;
   if (!renderer->rendernode_name)
      return -1;
   fd = open(renderer->rendernode_name, O_RDWR | O_CLOEXEC | O_NOCTTY | O_NONBLOCK);
   if (fd == -1)
      fprintf(stderr, "Unable to open rendernode '%s' falling back to default search\n",
              renderer->rendernode_name);
   return fd;
}

static struct virgl_renderer_callbacks renderer_cbs = {
   .version = 2,
   .write_fence = vtest_write_fence,
   .get_drm_fd = vtest_get_drm_fd
};


static struct vtest_renderer renderer = {
   .max_length = UINT_MAX,
   .fence_id = 1,
   .next_context_id = 1,
};

static unsigned
resource_hash_func(void *key)
{
   intptr_t ip = pointer_to_intptr(key);
   return (unsigned)(ip & 0xffffffff);
}

static int
resource_compare_func(void *key1, void *key2)
{
   if (key1 < key2) {
      return -1;
   } else if (key1 > key2) {
      return 1;
   } else {
      return 0;
   }
}

static void
resource_destroy_func(void *value)
{
   struct vtest_resource *res = value;

   /* virgl_renderer_ctx_detach_resource and virgl_renderer_resource_detach_iov
    * are implied
    */
   virgl_renderer_resource_unref(res->res_id);

   if (res->iov.iov_base)
      munmap(res->iov.iov_base, res->iov.iov_len);
   free(res);
}

static int vtest_block_write(int fd, void *buf, int size)
{
   char *ptr = buf;
   int left;
   int ret;
   left = size;

   do {
      ret = write(fd, ptr, left);
      if (ret < 0) {
         return -errno;
      }

      left -= ret;
      ptr += ret;
   } while (left);

   return size;
}

int vtest_block_read(struct vtest_input *input, void *buf, int size)
{
   int fd = input->data.fd;
   char *ptr = buf;
   int left;
   int ret;
   static int savefd = -1;

   left = size;
   do {
      ret = read(fd, ptr, left);
      if (ret <= 0) {
         return ret == -1 ? -errno : 0;
      }

      left -= ret;
      ptr += ret;
   } while (left);

   if (getenv("VTEST_SAVE")) {
      if (savefd == -1) {
         savefd = open(getenv("VTEST_SAVE"),
                       O_CLOEXEC|O_CREAT|O_WRONLY|O_TRUNC|O_DSYNC, S_IRUSR|S_IWUSR);
         if (savefd == -1) {
            perror("error opening save file");
            exit(1);
         }
      }
      if (write(savefd, buf, size) != size) {
         perror("failed to save");
         exit(1);
      }
   }

   return size;
}

static int vtest_send_fd(int socket_fd, int fd)
{
    struct iovec iovec;
    char buf[CMSG_SPACE(sizeof(int))];
    char c = 0;
    struct msghdr msgh = { 0 };
    memset(buf, 0, sizeof(buf));

    iovec.iov_base = &c;
    iovec.iov_len = sizeof(char);

    msgh.msg_name = NULL;
    msgh.msg_namelen = 0;
    msgh.msg_iov = &iovec;
    msgh.msg_iovlen = 1;
    msgh.msg_control = buf;
    msgh.msg_controllen = sizeof(buf);
    msgh.msg_flags = 0;

    struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msgh);
    cmsg->cmsg_level = SOL_SOCKET;
    cmsg->cmsg_type = SCM_RIGHTS;
    cmsg->cmsg_len = CMSG_LEN(sizeof(int));

    *((int *) CMSG_DATA(cmsg)) = fd;

    int size = sendmsg(socket_fd, &msgh, 0);
    if (size < 0) {
      return report_failure("Failed to send fd", -EINVAL);
    }

    return 0;
}

int vtest_buf_read(struct vtest_input *input, void *buf, int size)
{
   struct vtest_buffer *inbuf = input->data.buffer;
   if (size > inbuf->size) {
      return 0;
   }

   memcpy(buf, inbuf->buffer, size);
   inbuf->buffer += size;
   inbuf->size -= size;

   return size;
}

int vtest_init_renderer(int ctx_flags, const char *render_device)
{
   int ret;

   renderer.rendernode_name = render_device;
   list_inithead(&renderer.active_contexts);
   list_inithead(&renderer.free_contexts);

   ret = virgl_renderer_init(&renderer,
         ctx_flags | VIRGL_RENDERER_THREAD_SYNC, &renderer_cbs);
   if (ret) {
      fprintf(stderr, "failed to initialise renderer.\n");
      return -1;
   }

   return 0;
}

static void vtest_free_context(struct vtest_context *ctx, bool cleanup);

void vtest_cleanup_renderer(void)
{
   if (renderer.next_context_id > 1) {
      struct vtest_context *ctx, *tmp;

      LIST_FOR_EACH_ENTRY_SAFE(ctx, tmp, &renderer.active_contexts, head) {
         vtest_destroy_context(ctx);
      }
      LIST_FOR_EACH_ENTRY_SAFE(ctx, tmp, &renderer.free_contexts, head) {
         vtest_free_context(ctx, true);
      }
      list_inithead(&renderer.active_contexts);
      list_inithead(&renderer.free_contexts);

      renderer.next_context_id = 1;
      renderer.current_context = NULL;
   }

   virgl_renderer_cleanup(&renderer);
}

static struct vtest_context *vtest_new_context(struct vtest_input *input,
                                               int out_fd)
{
   struct vtest_context *ctx;

   if (LIST_IS_EMPTY(&renderer.free_contexts)) {
      ctx = malloc(sizeof(*ctx));
      if (!ctx) {
         return NULL;
      }

      ctx->resource_table = util_hash_table_create(resource_hash_func,
                                                   resource_compare_func,
                                                   resource_destroy_func);
      if (!ctx->resource_table) {
         free(ctx);
         return NULL;
      }

      ctx->ctx_id = renderer.next_context_id++;
   } else {
      ctx = LIST_ENTRY(struct vtest_context, renderer.free_contexts.next, head);
      list_del(&ctx->head);
   }

   ctx->input = input;
   ctx->out_fd = out_fd;

   ctx->debug_name = NULL;
   /* By default we support version 0 unless VCMD_PROTOCOL_VERSION is sent */
   ctx->protocol_version = 0;
   ctx->capset_id = 0;
   ctx->context_initialized = false;

   return ctx;
}

static void vtest_free_context(struct vtest_context *ctx, bool cleanup)
{
   if (cleanup) {
      util_hash_table_destroy(ctx->resource_table);
      free(ctx);
   } else {
      list_add(&ctx->head, &renderer.free_contexts);
   }
}

int vtest_create_context(struct vtest_input *input, int out_fd,
                         uint32_t length, struct vtest_context **out_ctx)
{
   struct vtest_context *ctx;
   char *vtestname;
   int ret;

   if (length > 1024 * 1024) {
      return -1;
   }

   ctx = vtest_new_context(input, out_fd);
   if (!ctx) {
      return -1;
   }

   vtestname = calloc(1, length + 1);
   if (!vtestname) {
      ret = -1;
      goto err;
   }

   ret = ctx->input->read(ctx->input, vtestname, length);
   if (ret != (int)length) {
      ret = -1;
      goto err;
   }

   ctx->debug_name = vtestname;

   list_addtail(&ctx->head, &renderer.active_contexts);
   *out_ctx = ctx;

   return 0;

err:
   free(vtestname);
   vtest_free_context(ctx, false);
   return ret;
}

int vtest_lazy_init_context(struct vtest_context *ctx)
{
   int ret;

   if (ctx->context_initialized)
      return 0;

   if (ctx->capset_id) {
      ret = virgl_renderer_context_create_with_flags(ctx->ctx_id,
                                                     ctx->capset_id,
                                                     strlen(ctx->debug_name),
                                                     ctx->debug_name);
   } else {
      ret = virgl_renderer_context_create(ctx->ctx_id,
                                          strlen(ctx->debug_name),
                                          ctx->debug_name);
   }
   ctx->context_initialized = (ret == 0);

   return ret;
}

void vtest_destroy_context(struct vtest_context *ctx)
{
   if (renderer.current_context == ctx) {
      renderer.current_context = NULL;
   }
   list_del(&ctx->head);

   free(ctx->debug_name);
   if (ctx->context_initialized)
      virgl_renderer_context_destroy(ctx->ctx_id);
   util_hash_table_clear(ctx->resource_table);
   vtest_free_context(ctx, false);
}

void vtest_set_current_context(struct vtest_context *ctx)
{
   renderer.current_context = ctx;
}

static struct vtest_context *vtest_get_current_context(void)
{
   return renderer.current_context;
}

int vtest_ping_protocol_version(UNUSED uint32_t length_dw)
{
   struct vtest_context *ctx = vtest_get_current_context();
   uint32_t hdr_buf[VTEST_HDR_SIZE];
   int ret;

   hdr_buf[VTEST_CMD_LEN] = VCMD_PING_PROTOCOL_VERSION_SIZE;
   hdr_buf[VTEST_CMD_ID] = VCMD_PING_PROTOCOL_VERSION;
   ret = vtest_block_write(ctx->out_fd, hdr_buf, sizeof(hdr_buf));
   if (ret < 0) {
      return ret;
   }

   return 0;
}

int vtest_protocol_version(UNUSED uint32_t length_dw)
{
   struct vtest_context *ctx = vtest_get_current_context();
   uint32_t hdr_buf[VTEST_HDR_SIZE];
   uint32_t version_buf[VCMD_PROTOCOL_VERSION_SIZE];
   unsigned version;
   int ret;

   ret = ctx->input->read(ctx->input, &version_buf, sizeof(version_buf));
   if (ret != sizeof(version_buf))
      return -1;

   version = MIN2(version_buf[VCMD_PROTOCOL_VERSION_VERSION],
                  VTEST_PROTOCOL_VERSION);

   /*
    * We've deprecated protocol version 1. All of it's called sites are being
    * moved protocol version 2. If the server supports version 2 and the guest
    * supports verison 1, fall back to version 0.
    */
   if (version == 1) {
      printf("Older guest Mesa detected, fallbacking to protocol version 0\n");
      version = 0;
   }

   /* Protocol version 2 requires shm support. */
   if (!vtest_shm_check()) {
      printf("Shared memory not supported, fallbacking to protocol version 0\n");
      version = 0;
   }

   ctx->protocol_version = version;

   hdr_buf[VTEST_CMD_LEN] = VCMD_PROTOCOL_VERSION_SIZE;
   hdr_buf[VTEST_CMD_ID] = VCMD_PROTOCOL_VERSION;

   version_buf[VCMD_PROTOCOL_VERSION_VERSION] = ctx->protocol_version;

   ret = vtest_block_write(ctx->out_fd, hdr_buf, sizeof(hdr_buf));
   if (ret < 0) {
      return ret;
   }

   ret = vtest_block_write(ctx->out_fd, version_buf, sizeof(version_buf));
   if (ret < 0) {
      return ret;
   }

   return 0;
}

int vtest_get_param(UNUSED uint32_t length_dw)
{
   struct vtest_context *ctx = vtest_get_current_context();
   uint32_t get_param_buf[VCMD_GET_PARAM_SIZE];
   uint32_t resp_buf[VTEST_HDR_SIZE + 2];
   uint32_t param;
   uint32_t *resp;
   int ret;

   ret = ctx->input->read(ctx->input, get_param_buf, sizeof(get_param_buf));
   if (ret != sizeof(get_param_buf))
      return -1;

   param = get_param_buf[VCMD_GET_PARAM_PARAM];

   resp_buf[VTEST_CMD_LEN] = 2;
   resp_buf[VTEST_CMD_ID] = VCMD_GET_PARAM;
   resp = &resp_buf[VTEST_CMD_DATA_START];
   switch (param) {
   default:
      resp[0] = false;
      resp[1] = 0;
      break;
   }

   ret = vtest_block_write(ctx->out_fd, resp_buf, sizeof(resp_buf));
   if (ret < 0)
      return -1;

   return 0;
}

int vtest_get_capset(UNUSED uint32_t length_dw)
{
   struct vtest_context *ctx = vtest_get_current_context();
   uint32_t get_capset_buf[VCMD_GET_CAPSET_SIZE];
   uint32_t resp_buf[VTEST_HDR_SIZE + 1];
   uint32_t id;
   uint32_t version;
   uint32_t max_version;
   uint32_t max_size;
   void *caps;
   int ret;

   ret = ctx->input->read(ctx->input, get_capset_buf, sizeof(get_capset_buf));
   if (ret != sizeof(get_capset_buf))
      return -1;

   id = get_capset_buf[VCMD_GET_CAPSET_ID];
   version = get_capset_buf[VCMD_GET_CAPSET_VERSION];

   virgl_renderer_get_cap_set(id, &max_version, &max_size);

   /* unsupported id or version */
   if ((!max_version && !max_size) || version > max_version) {
      resp_buf[VTEST_CMD_LEN] = 1;
      resp_buf[VTEST_CMD_ID] = VCMD_GET_CAPSET;
      resp_buf[VTEST_CMD_DATA_START] = false;
      return vtest_block_write(ctx->out_fd, resp_buf, sizeof(resp_buf));
   }

   if (max_size % 4)
      return -EINVAL;

   caps = malloc(max_size);
   if (!caps)
      return -ENOMEM;

   virgl_renderer_fill_caps(id, version, caps);

   resp_buf[VTEST_CMD_LEN] = 1 + max_size / 4;
   resp_buf[VTEST_CMD_ID] = VCMD_GET_CAPSET;
   resp_buf[VTEST_CMD_DATA_START] = true;
   ret = vtest_block_write(ctx->out_fd, resp_buf, sizeof(resp_buf));
   if (ret >= 0)
      ret = vtest_block_write(ctx->out_fd, caps, max_size);

   free(caps);
   return ret >= 0 ? 0 : ret;
}

int vtest_context_init(UNUSED uint32_t length_dw)
{
   struct vtest_context *ctx = vtest_get_current_context();
   uint32_t context_init_buf[VCMD_CONTEXT_INIT_SIZE];
   uint32_t capset_id;
   int ret;

   ret = ctx->input->read(ctx->input, context_init_buf, sizeof(context_init_buf));
   if (ret != sizeof(context_init_buf))
      return -1;

   capset_id = context_init_buf[VCMD_CONTEXT_INIT_CAPSET_ID];
   if (!capset_id)
      return -EINVAL;

   if (ctx->context_initialized) {
      return ctx->capset_id == capset_id ? 0 : -EINVAL;
   }

   ctx->capset_id = capset_id;

   return vtest_lazy_init_context(ctx);
}

int vtest_send_caps2(UNUSED uint32_t length_dw)
{
   struct vtest_context *ctx = vtest_get_current_context();
   uint32_t hdr_buf[2];
   void *caps_buf;
   int ret;
   uint32_t max_ver, max_size;

   virgl_renderer_get_cap_set(2, &max_ver, &max_size);

   if (max_size == 0) {
      return -1;
   }

   caps_buf = malloc(max_size);
   if (!caps_buf) {
      return -1;
   }

   virgl_renderer_fill_caps(2, 1, caps_buf);

   hdr_buf[0] = max_size + 1;
   hdr_buf[1] = 2;
   ret = vtest_block_write(ctx->out_fd, hdr_buf, 8);
   if (ret < 0) {
      goto end;
   }

   vtest_block_write(ctx->out_fd, caps_buf, max_size);
   if (ret < 0) {
      goto end;
   }

end:
   free(caps_buf);
   return 0;
}

int vtest_send_caps(UNUSED uint32_t length_dw)
{
   struct vtest_context *ctx = vtest_get_current_context();
   uint32_t  max_ver, max_size;
   void *caps_buf;
   uint32_t hdr_buf[2];
   int ret;

   virgl_renderer_get_cap_set(1, &max_ver, &max_size);

   caps_buf = malloc(max_size);
   if (!caps_buf) {
      return -1;
   }

   virgl_renderer_fill_caps(1, 1, caps_buf);

   hdr_buf[0] = max_size + 1;
   hdr_buf[1] = 1;
   ret = vtest_block_write(ctx->out_fd, hdr_buf, 8);
   if (ret < 0) {
      goto end;
   }

   vtest_block_write(ctx->out_fd, caps_buf, max_size);
   if (ret < 0) {
      goto end;
   }

end:
   free(caps_buf);
   return 0;
}

static int vtest_create_resource_decode_args(struct vtest_context *ctx,
                                             struct virgl_renderer_resource_create_args *args)
{
   uint32_t res_create_buf[VCMD_RES_CREATE_SIZE];
   int ret;

   ret = ctx->input->read(ctx->input, &res_create_buf,
                          sizeof(res_create_buf));
   if (ret != sizeof(res_create_buf)) {
      return -1;
   }

   args->handle = res_create_buf[VCMD_RES_CREATE_RES_HANDLE];
   args->target = res_create_buf[VCMD_RES_CREATE_TARGET];
   args->format = res_create_buf[VCMD_RES_CREATE_FORMAT];
   args->bind = res_create_buf[VCMD_RES_CREATE_BIND];

   args->width = res_create_buf[VCMD_RES_CREATE_WIDTH];
   args->height = res_create_buf[VCMD_RES_CREATE_HEIGHT];
   args->depth = res_create_buf[VCMD_RES_CREATE_DEPTH];
   args->array_size = res_create_buf[VCMD_RES_CREATE_ARRAY_SIZE];
   args->last_level = res_create_buf[VCMD_RES_CREATE_LAST_LEVEL];
   args->nr_samples = res_create_buf[VCMD_RES_CREATE_NR_SAMPLES];
   args->flags = 0;

   return 0;
}

static int vtest_create_resource_decode_args2(struct vtest_context *ctx,
                                              struct virgl_renderer_resource_create_args *args,
                                              size_t *shm_size)
{
   uint32_t res_create_buf[VCMD_RES_CREATE2_SIZE];
   int ret;

   ret = ctx->input->read(ctx->input, &res_create_buf,
                          sizeof(res_create_buf));
   if (ret != sizeof(res_create_buf)) {
      return -1;
   }

   args->handle = res_create_buf[VCMD_RES_CREATE2_RES_HANDLE];
   args->target = res_create_buf[VCMD_RES_CREATE2_TARGET];
   args->format = res_create_buf[VCMD_RES_CREATE2_FORMAT];
   args->bind = res_create_buf[VCMD_RES_CREATE2_BIND];

   args->width = res_create_buf[VCMD_RES_CREATE2_WIDTH];
   args->height = res_create_buf[VCMD_RES_CREATE2_HEIGHT];
   args->depth = res_create_buf[VCMD_RES_CREATE2_DEPTH];
   args->array_size = res_create_buf[VCMD_RES_CREATE2_ARRAY_SIZE];
   args->last_level = res_create_buf[VCMD_RES_CREATE2_LAST_LEVEL];
   args->nr_samples = res_create_buf[VCMD_RES_CREATE2_NR_SAMPLES];
   args->flags = 0;

   *shm_size = res_create_buf[VCMD_RES_CREATE2_DATA_SIZE];

   return 0;
}

static int vtest_create_resource_setup_shm(struct vtest_resource *res,
                                           size_t size)
{
   int fd;
   void *ptr;

   fd = vtest_new_shm(res->res_id, size);
   if (fd < 0)
      return report_failed_call("vtest_new_shm", fd);

   ptr = mmap(NULL, size, PROT_WRITE | PROT_READ, MAP_SHARED, fd, 0);
   if (ptr == MAP_FAILED) {
      close(fd);
      return -1;
   }

   res->iov.iov_base = ptr;
   res->iov.iov_len = size;

   return fd;
}

static int vtest_create_resource_internal(struct vtest_context *ctx,
                                          struct virgl_renderer_resource_create_args *args,
                                          size_t shm_size)
{
   struct vtest_resource *res;
   int ret;

   // Check that the handle doesn't already exist.
   if (util_hash_table_get(ctx->resource_table, intptr_to_pointer(args->handle)))
      return -EEXIST;

   ret = virgl_renderer_resource_create(args, NULL, 0);
   if (ret)
      return report_failed_call("virgl_renderer_resource_create", ret);

   virgl_renderer_ctx_attach_resource(ctx->ctx_id, args->handle);

   res = CALLOC_STRUCT(vtest_resource);
   if (!res)
      return -ENOMEM;
   res->res_id = args->handle;

   /* no shm for v1 resources or v2 multi-sample resources */
   if (shm_size) {
      int fd;

      fd = vtest_create_resource_setup_shm(res, shm_size);
      if (fd < 0) {
         FREE(res);
         return -ENOMEM;
      }

      ret = vtest_send_fd(ctx->out_fd, fd);
      if (ret < 0) {
         munmap(res->iov.iov_base, res->iov.iov_len);
         close(fd);
         FREE(res);
         return report_failed_call("vtest_send_fd", ret);
      }

      /* Closing the file descriptor does not unmap the region. */
      close(fd);

      virgl_renderer_resource_attach_iov(args->handle, &res->iov, 1);
   }

   util_hash_table_set(ctx->resource_table, intptr_to_pointer(res->res_id), res);

   return 0;
}

int vtest_create_resource(UNUSED uint32_t length_dw)
{
   struct vtest_context *ctx = vtest_get_current_context();
   struct virgl_renderer_resource_create_args args;
   int ret;

   ret = vtest_create_resource_decode_args(ctx, &args);
   if (ret < 0) {
      return ret;
   }

   return vtest_create_resource_internal(ctx, &args, 0);
}

int vtest_create_resource2(UNUSED uint32_t length_dw)
{
   struct vtest_context *ctx = vtest_get_current_context();
   struct virgl_renderer_resource_create_args args;
   size_t shm_size;
   int ret;

   ret = vtest_create_resource_decode_args2(ctx, &args, &shm_size);
   if (ret < 0) {
      return ret;
   }

   return vtest_create_resource_internal(ctx, &args, shm_size);
}

int vtest_resource_unref(UNUSED uint32_t length_dw)
{
   struct vtest_context *ctx = vtest_get_current_context();
   uint32_t res_unref_buf[VCMD_RES_UNREF_SIZE];
   int ret;
   uint32_t handle;

   ret = ctx->input->read(ctx->input, &res_unref_buf,
                          sizeof(res_unref_buf));
   if (ret != sizeof(res_unref_buf)) {
      return -1;
   }

   handle = res_unref_buf[VCMD_RES_UNREF_RES_HANDLE];
   util_hash_table_remove(ctx->resource_table, intptr_to_pointer(handle));

   return 0;
}

int vtest_submit_cmd(uint32_t length_dw)
{
   struct vtest_context *ctx = vtest_get_current_context();
   uint32_t *cbuf;
   int ret;

   if (length_dw > renderer.max_length / 4) {
      return -1;
   }

   cbuf = malloc(length_dw * 4);
   if (!cbuf) {
      return -1;
   }

   ret = ctx->input->read(ctx->input, cbuf, length_dw * 4);
   if (ret != (int)length_dw * 4) {
      free(cbuf);
      return -1;
   }

   ret = virgl_renderer_submit_cmd(cbuf, ctx->ctx_id, length_dw);

   free(cbuf);
   return ret ? -1 : 0;
}

struct vtest_transfer_args {
   uint32_t handle;
   uint32_t level;
   uint32_t stride;
   uint32_t layer_stride;
   struct virgl_box box;
   uint32_t offset;
};

static int vtest_transfer_decode_args(struct vtest_context *ctx,
                                      struct vtest_transfer_args *args,
                                      uint32_t *data_size)
{
   uint32_t thdr_buf[VCMD_TRANSFER_HDR_SIZE];
   int ret;

   ret = ctx->input->read(ctx->input, thdr_buf, sizeof(thdr_buf));
   if (ret != sizeof(thdr_buf)) {
      return -1;
   }

   args->handle = thdr_buf[VCMD_TRANSFER_RES_HANDLE];
   args->level = thdr_buf[VCMD_TRANSFER_LEVEL];
   args->stride = thdr_buf[VCMD_TRANSFER_STRIDE];
   args->layer_stride = thdr_buf[VCMD_TRANSFER_LAYER_STRIDE];
   args->box.x = thdr_buf[VCMD_TRANSFER_X];
   args->box.y = thdr_buf[VCMD_TRANSFER_Y];
   args->box.z = thdr_buf[VCMD_TRANSFER_Z];
   args->box.w = thdr_buf[VCMD_TRANSFER_WIDTH];
   args->box.h = thdr_buf[VCMD_TRANSFER_HEIGHT];
   args->box.d = thdr_buf[VCMD_TRANSFER_DEPTH];
   args->offset = 0;

   *data_size = thdr_buf[VCMD_TRANSFER_DATA_SIZE];

   if (*data_size > renderer.max_length) {
      return -ENOMEM;
   }

   return 0;
}

static int vtest_transfer_decode_args2(struct vtest_context *ctx,
                                       struct vtest_transfer_args *args)
{
   uint32_t thdr_buf[VCMD_TRANSFER2_HDR_SIZE];
   int ret;

   ret = ctx->input->read(ctx->input, thdr_buf, sizeof(thdr_buf));
   if (ret != sizeof(thdr_buf)) {
      return -1;
   }

   args->handle = thdr_buf[VCMD_TRANSFER2_RES_HANDLE];
   args->level = thdr_buf[VCMD_TRANSFER2_LEVEL];
   args->stride = 0;
   args->layer_stride = 0;
   args->box.x = thdr_buf[VCMD_TRANSFER2_X];
   args->box.y = thdr_buf[VCMD_TRANSFER2_Y];
   args->box.z = thdr_buf[VCMD_TRANSFER2_Z];
   args->box.w = thdr_buf[VCMD_TRANSFER2_WIDTH];
   args->box.h = thdr_buf[VCMD_TRANSFER2_HEIGHT];
   args->box.d = thdr_buf[VCMD_TRANSFER2_DEPTH];
   args->offset = thdr_buf[VCMD_TRANSFER2_OFFSET];

   return 0;
}

static int vtest_transfer_get_internal(struct vtest_context *ctx,
                                       struct vtest_transfer_args *args,
                                       uint32_t data_size,
                                       bool do_transfer)
{
   struct vtest_resource *res;
   struct iovec data_iov;
   int ret = 0;

   res = util_hash_table_get(ctx->resource_table,
                             intptr_to_pointer(args->handle));
   if (!res) {
      return report_failed_call("util_hash_table_get", -ESRCH);
   }

   if (data_size) {
      data_iov.iov_len = data_size;
      data_iov.iov_base = malloc(data_size);
      if (!data_iov.iov_base) {
         return -ENOMEM;
      }
   } else {
      if (args->offset >= res->iov.iov_len) {
         return report_failure("offset larger then length of backing store", -EFAULT);
      }
   }

   if (do_transfer) {
      ret = virgl_renderer_transfer_read_iov(res->res_id,
                                             ctx->ctx_id,
                                             args->level,
                                             args->stride,
                                             args->layer_stride,
                                             &args->box,
                                             args->offset,
                                             data_size ? &data_iov : NULL,
                                             data_size ? 1 : 0);
      if (ret) {
         report_failed_call("virgl_renderer_transfer_read_iov", ret);
      }
   } else if (data_size) {
      memset(data_iov.iov_base, 0, data_iov.iov_len);
   }

   if (data_size) {
      ret = vtest_block_write(ctx->out_fd, data_iov.iov_base, data_iov.iov_len);
      if (ret > 0)
         ret = 0;

      free(data_iov.iov_base);
   }

   return ret;
}

static int vtest_transfer_put_internal(struct vtest_context *ctx,
                                       struct vtest_transfer_args *args,
                                       uint32_t data_size,
                                       bool do_transfer)
{
   struct vtest_resource *res;
   struct iovec data_iov;
   int ret = 0;

   res = util_hash_table_get(ctx->resource_table,
                             intptr_to_pointer(args->handle));
   if (!res) {
      return report_failed_call("util_hash_table_get", -ESRCH);
   }

   if (data_size) {
      data_iov.iov_len = data_size;
      data_iov.iov_base = malloc(data_size);
      if (!data_iov.iov_base) {
         return -ENOMEM;
      }

      ret = ctx->input->read(ctx->input, data_iov.iov_base, data_iov.iov_len);
      if (ret < 0) {
         return ret;
      }
   }

   if (do_transfer) {
      ret = virgl_renderer_transfer_write_iov(res->res_id,
                                              ctx->ctx_id,
                                              args->level,
                                              args->stride,
                                              args->layer_stride,
                                              &args->box,
                                              args->offset,
                                              data_size ? &data_iov : NULL,
                                              data_size ? 1 : 0);
      if (ret) {
         report_failed_call("virgl_renderer_transfer_write_iov", ret);
      }
   }

   if (data_size) {
      free(data_iov.iov_base);
   }

   return ret;
}

int vtest_transfer_get(UNUSED uint32_t length_dw)
{
   struct vtest_context *ctx = vtest_get_current_context();
   int ret;
   struct vtest_transfer_args args;
   uint32_t data_size;

   ret = vtest_transfer_decode_args(ctx, &args, &data_size);
   if (ret < 0) {
      return ret;
   }

   return vtest_transfer_get_internal(ctx, &args, data_size, true);
}

int vtest_transfer_get_nop(UNUSED uint32_t length_dw)
{
   struct vtest_context *ctx = vtest_get_current_context();
   int ret;
   struct vtest_transfer_args args;
   uint32_t data_size;

   ret = vtest_transfer_decode_args(ctx, &args, &data_size);
   if (ret < 0) {
      return ret;
   }

   return vtest_transfer_get_internal(ctx, &args, data_size, false);
}

int vtest_transfer_put(UNUSED uint32_t length_dw)
{
   struct vtest_context *ctx = vtest_get_current_context();
   int ret;
   struct vtest_transfer_args args;
   uint32_t data_size;

   ret = vtest_transfer_decode_args(ctx, &args, &data_size);
   if (ret < 0) {
      return ret;
   }

   return vtest_transfer_put_internal(ctx, &args, data_size, true);
}

int vtest_transfer_put_nop(UNUSED uint32_t length_dw)
{
   struct vtest_context *ctx = vtest_get_current_context();
   int ret;
   struct vtest_transfer_args args;
   uint32_t data_size;

   ret = vtest_transfer_decode_args(ctx, &args, &data_size);
   if (ret < 0) {
      return ret;
   }

   return vtest_transfer_put_internal(ctx, &args, data_size, false);
}

int vtest_transfer_get2(UNUSED uint32_t length_dw)
{
   struct vtest_context *ctx = vtest_get_current_context();
   int ret;
   struct vtest_transfer_args args;

   ret = vtest_transfer_decode_args2(ctx, &args);
   if (ret < 0) {
      return ret;
   }

   return vtest_transfer_get_internal(ctx, &args, 0, true);
}

int vtest_transfer_get2_nop(UNUSED uint32_t length_dw)
{
   struct vtest_context *ctx = vtest_get_current_context();
   int ret;
   struct vtest_transfer_args args;

   ret = vtest_transfer_decode_args2(ctx, &args);
   if (ret < 0) {
      return ret;
   }

   return vtest_transfer_get_internal(ctx, &args, 0, false);
}

int vtest_transfer_put2(UNUSED uint32_t length_dw)
{
   struct vtest_context *ctx = vtest_get_current_context();
   int ret;
   struct vtest_transfer_args args;

   ret = vtest_transfer_decode_args2(ctx, &args);
   if (ret < 0) {
      return ret;
   }

   return vtest_transfer_put_internal(ctx, &args, 0, true);
}

int vtest_transfer_put2_nop(UNUSED uint32_t length_dw)
{
   struct vtest_context *ctx = vtest_get_current_context();
   int ret;
   struct vtest_transfer_args args;

   ret = vtest_transfer_decode_args2(ctx, &args);
   if (ret < 0) {
      return ret;
   }

   return vtest_transfer_put_internal(ctx, &args, 0, false);
}

int vtest_resource_busy_wait(UNUSED uint32_t length_dw)
{
   struct vtest_context *ctx = vtest_get_current_context();
   uint32_t bw_buf[VCMD_BUSY_WAIT_SIZE];
   int ret, fd;
   int flags;
   uint32_t hdr_buf[VTEST_HDR_SIZE];
   uint32_t reply_buf[1];
   bool busy = false;

   ret = ctx->input->read(ctx->input, &bw_buf, sizeof(bw_buf));
   if (ret != sizeof(bw_buf)) {
      return -1;
   }

   /* clients often send VCMD_PING_PROTOCOL_VERSION followed by
    * VCMD_RESOURCE_BUSY_WAIT with handle 0 to figure out if
    * VCMD_PING_PROTOCOL_VERSION is supported.  We need to make a special case
    * for that.
    */
   if (!ctx->context_initialized && bw_buf[VCMD_BUSY_WAIT_HANDLE])
      return -1;

   /*  handle = bw_buf[VCMD_BUSY_WAIT_HANDLE]; unused as of now */
   flags = bw_buf[VCMD_BUSY_WAIT_FLAGS];

   if (flags == VCMD_BUSY_WAIT_FLAG_WAIT) {
      do {
         if (renderer.last_fence == (renderer.fence_id - 1)) {
            break;
         }

         fd = virgl_renderer_get_poll_fd();
         if (fd != -1) {
            vtest_wait_for_fd_read(fd);
         }

         virgl_renderer_poll();
      } while (1);

      busy = false;
   } else {
      busy = renderer.last_fence != (renderer.fence_id - 1);
   }

   hdr_buf[VTEST_CMD_LEN] = 1;
   hdr_buf[VTEST_CMD_ID] = VCMD_RESOURCE_BUSY_WAIT;
   reply_buf[0] = busy ? 1 : 0;

   ret = vtest_block_write(ctx->out_fd, hdr_buf, sizeof(hdr_buf));
   if (ret < 0) {
      return ret;
   }

   ret = vtest_block_write(ctx->out_fd, reply_buf, sizeof(reply_buf));
   if (ret < 0) {
      return ret;
   }

   return 0;
}

int vtest_renderer_create_fence(void)
{
   struct vtest_context *ctx = vtest_get_current_context();
   virgl_renderer_create_fence(renderer.fence_id++, ctx->ctx_id);
   return 0;
}

int vtest_poll(void)
{
   virgl_renderer_poll();
   return 0;
}

void vtest_set_max_length(uint32_t length)
{
   renderer.max_length = length;
}
