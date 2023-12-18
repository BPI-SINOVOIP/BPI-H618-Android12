#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <time.h>
#include <ui/GraphicBuffer.h>
#include <ui/GraphicBufferAllocator.h>
#include <unistd.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

#include "HostConnection.h"
#include "cros_gralloc_handle.h"

using namespace android;

struct buffer_object {
  uint32_t width;
  uint32_t height;
  uint32_t pitch;
  uint32_t handle;
  uint32_t size;
  uint8_t *vaddr;
  uint32_t fb_id;
  const native_handle_t *fb;
};

struct buffer_object buf;

static int modeset_create_fb(int fd, struct buffer_object *bo) {
  struct drm_mode_create_dumb create = {};
  struct drm_mode_map_dumb map = {};

  create.width = bo->width;
  create.height = bo->height;
  create.bpp = 32;
  drmIoctl(fd, DRM_IOCTL_MODE_CREATE_DUMB, &create);
  printf("create dumb w %d h %d\n", bo->width, bo->height);
  getchar();

  bo->pitch = create.pitch;
  bo->size = create.size;
  bo->handle = create.handle;
  drmModeAddFB(fd, bo->width, bo->height, 24, 32, bo->pitch, bo->handle,
               &bo->fb_id);
  printf("drmModeAddFB\n");
  getchar();

  map.handle = create.handle;
  drmIoctl(fd, DRM_IOCTL_MODE_MAP_DUMB, &map);
  printf("map dumb\n");
  getchar();
  bo->vaddr = static_cast<unsigned char *>(mmap64(
      0, create.size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, map.offset));
  memset(bo->vaddr, 0xff, bo->size);
  return 0;
}

static void modeset_destroy_fb(int fd, struct buffer_object *bo) {
  struct drm_mode_destroy_dumb destroy = {};

  drmModeRmFB(fd, bo->fb_id);
  munmap(bo->vaddr, bo->size);
  destroy.handle = bo->handle;
  drmIoctl(fd, DRM_IOCTL_MODE_DESTROY_DUMB, &destroy);
}

static uint32_t get_property_id(int fd, drmModeObjectProperties *props,
                                const char *name) {
  drmModePropertyPtr property;
  uint32_t i, id = 0;

  /* find property according to the name */
  for (i = 0; i < props->count_props; i++) {
    property = drmModeGetProperty(fd, props->props[i]);
    if (!strcmp(property->name, name)) id = property->prop_id;
    drmModeFreeProperty(property);
    if (id) {
      break;
    }
  }
  return id;
}

static std::unique_ptr<HostConnection> sHostCon;

static HostConnection *createOrGetHostConnection() {
  if (!sHostCon) {
    sHostCon = HostConnection::createUnique();
  }
  return sHostCon.get();
}

#define DEFINE_AND_VALIDATE_HOST_CONNECTION                          \
  HostConnection *hostCon = createOrGetHostConnection();             \
  if (!hostCon) {                                                    \
    ALOGE("drmTest: Failed to get host connection\n");               \
    return;                                                          \
  }                                                                  \
  ExtendedRCEncoderContext *rcEnc = hostCon->rcEncoder();            \
  if (!rcEnc) {                                                      \
    ALOGE("drmTest: Failed to get renderControl encoder context\n"); \
    return;                                                          \
  }

#include "include/drmhwcgralloc.h"
void convertBoInfo(buffer_handle_t handle, hwc_drm_bo_t *bo) {
  cros_gralloc_handle *gr_handle = (cros_gralloc_handle *)handle;
  if (!gr_handle) return;

  bo->width = gr_handle->width;
  bo->height = gr_handle->height;
  bo->hal_format = gr_handle->droid_format;
  bo->format = gr_handle->format;
  bo->usage = gr_handle->usage;
  bo->prime_fds[0] = gr_handle->fds[0];
  bo->pitches[0] = gr_handle->strides[0];
  bo->offsets[0] = gr_handle->offsets[0];
}

void grallocAllocBuffer(int fd, struct buffer_object *bo) {
  buffer_handle_t h;
  uint32_t stride;

  if (GraphicBufferAllocator::get().allocate(
          bo->width, bo->height, android::PIXEL_FORMAT_RGBA_8888, 1,
          (GraphicBuffer::USAGE_HW_COMPOSER | GraphicBuffer::USAGE_HW_RENDER),
          &h, &stride, 0, "emulatorDrmTest") == android::OK) {
    hwc_drm_bo tmp_bo{};
    convertBoInfo(h, &tmp_bo);

    int ret = drmPrimeFDToHandle(fd, tmp_bo.prime_fds[0], tmp_bo.gem_handles);
    for (int i = 1; i < HWC_DRM_BO_MAX_PLANES; i++) {
      tmp_bo.gem_handles[i] = tmp_bo.gem_handles[0];
    }
    if (ret) {
      printf("%s: DRM_IOCTL_PRIME_FD_TO_HANDLE failed: %s (errno %d)\n",
             __func__, strerror(errno), errno);
      return;
    }
    ret = drmModeAddFB2(fd, tmp_bo.width, tmp_bo.height, tmp_bo.format,
                        tmp_bo.gem_handles, tmp_bo.pitches, tmp_bo.offsets,
                        &bo->fb_id, 0);

    printf("allocate buffer\n");
    DEFINE_AND_VALIDATE_HOST_CONNECTION
    bo->fb = static_cast<const native_handle_t *>(h);
    getchar();
    printf("resource id is %d\n",
           hostCon->grallocHelper()->getHostHandle(bo->fb));
  } else {
    bo->fb = nullptr;
  }
}

int main(int argc, char **argv) {
  int fd;
  drmModeConnector *conn;
  drmModeRes *res;
  drmModePlaneRes *plane_res = nullptr;
  uint32_t conn_id;
  uint32_t crtc_id;
  uint32_t plane_id;

  fd = open("/dev/dri/card0", O_RDWR | O_CLOEXEC);

  int ret = drmSetClientCap(fd, DRM_CLIENT_CAP_UNIVERSAL_PLANES, 0);
  if (ret) {
    printf("fail to set universal plane %d\n", ret);
  }

  res = drmModeGetResources(fd);
  crtc_id = res->crtcs[0];
  conn_id = res->connectors[0];

  plane_res = drmModeGetPlaneResources(fd);
  plane_id = plane_res->planes[0];

  conn = drmModeGetConnector(fd, conn_id);
  buf.width = conn->modes[0].hdisplay;
  buf.height = conn->modes[0].vdisplay;
  // modeset_create_fb(fd, &buf);
  grallocAllocBuffer(fd, &buf);

  drmModeSetCrtc(fd, crtc_id, buf.fb_id, 0, 0, &conn_id, 1, &conn->modes[0]);
  printf("drmModeSetCrtc\n");
  getchar();

  drmModePageFlip(fd, crtc_id, buf.fb_id, DRM_MODE_PAGE_FLIP_EVENT, &crtc_id);
  printf("drmModePageFlip\n");
  getchar();

  //    drmModeSetPlane(fd, plane_id, crtc_id, buf.fb_id, 0, 50, 50, 320, 320,
  //                    100 << 16, 150 << 16, 320 << 16, 320 << 16);
  //    printf("drmModeSetPlane\n");
  //    modeset_destroy_fb(fd, &buf);

  drmModeFreeConnector(conn);
  drmModeFreeResources(res);
  close(fd);

  return 0;
}

int main_atom(int argc, char **argv) {
  int fd;
  drmModeConnector *conn = nullptr;
  drmModeRes *res = nullptr;
  drmModePlaneRes *plane_res = nullptr;
  drmModeObjectProperties *props = nullptr;
  drmModeAtomicReq *req;
  uint32_t conn_id;
  uint32_t crtc_id;
  uint32_t plane_id;
  uint32_t blob_id;
  uint32_t property_crtc_id;
  uint32_t property_mode_id;
  uint32_t property_active;

  printf("drm available %d\n", drmAvailable());
  fd = open("/dev/dri/card0", O_RDWR);
  printf("openg drm fd %d\n", fd);

  int ret = drmSetClientCap(fd, DRM_CLIENT_CAP_UNIVERSAL_PLANES, 1);
  if (ret) {
    printf("fail to set universal plane %d\n", ret);
  }

  ret = drmSetClientCap(fd, DRM_CLIENT_CAP_ATOMIC, 1);
  if (ret) {
    printf("fail to set atomic operation %d\n", ret);
  }

  res = drmModeGetResources(fd);
  if (!res) {
    printf("error to get drmModeGetResources: %d\n", errno);
  }

  crtc_id = res->crtcs[0];
  conn_id = res->connectors[0];
  plane_res = drmModeGetPlaneResources(fd);
  plane_id = plane_res->planes[0];

  conn = drmModeGetConnector(fd, conn_id);
  buf.width = conn->modes[0].hdisplay;
  buf.height = conn->modes[0].vdisplay;
  modeset_create_fb(fd, &buf);

  /* get connector properties */
  props = drmModeObjectGetProperties(fd, conn_id, DRM_MODE_OBJECT_CONNECTOR);
  property_crtc_id = get_property_id(fd, props, "CRTC_ID");
  drmModeFreeObjectProperties(props);

  /* get crtc properties */
  props = drmModeObjectGetProperties(fd, crtc_id, DRM_MODE_OBJECT_CRTC);
  property_active = get_property_id(fd, props, "ACTIVE");
  property_mode_id = get_property_id(fd, props, "MODE_ID");
  drmModeFreeObjectProperties(props);

  /* create blob to store current mode, and return the blob id */
  drmModeCreatePropertyBlob(fd, &conn->modes[0], sizeof(conn->modes[0]),
                            &blob_id);

  /* start modeseting */
  req = drmModeAtomicAlloc();
  drmModeAtomicAddProperty(req, crtc_id, property_active, 1);
  drmModeAtomicAddProperty(req, crtc_id, property_mode_id, blob_id);
  drmModeAtomicAddProperty(req, conn_id, property_crtc_id, crtc_id);
  drmModeAtomicCommit(fd, req, DRM_MODE_ATOMIC_ALLOW_MODESET, NULL);
  drmModeAtomicFree(req);
  printf("drmModeAtomicCommit SetCrtc\n");
  getchar();

  drmModeSetPlane(fd, plane_id, crtc_id, buf.fb_id, 0, 50, 50, 320, 320, 0, 0,
                  320 << 16, 320 << 16);
  printf("drmModeSetPlane\n");
  getchar();

  modeset_destroy_fb(fd, &buf);
  drmModeFreeConnector(conn);
  drmModeFreePlaneResources(plane_res);
  drmModeFreeResources(res);
  close(fd);

  return 0;
}
