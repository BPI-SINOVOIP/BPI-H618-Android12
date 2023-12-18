#ifndef __AUTO_DETECT
#define __AUTO_DETECT

#ifdef BOARD_WIFI_VENDOR_COMMON

#include "libhwinfo.h"

#ifndef WIFI_DRIVER_MODULE_PATH
#define WIFI_DRIVER_MODULE_PATH "/vendor/lib/modules/"
#endif

#define DRIVER_MODULE_NAME      get_wifi_driver_module_name()
#define DRIVER_MODULE_TAG       hal_get_driver_module_tag()
#define DRIVER_MODULE_PATH      hal_get_driver_module_path()
#define DRIVER_MODULE_ARG       hal_get_driver_module_arg()
#define MODULE_FILE             "/proc/modules"

static const char *hal_get_driver_module_tag(void)
{
  static char driver_module_tag[128];
  sprintf(&driver_module_tag[0], "%s ", get_wifi_driver_module_name());
  return (const char *)driver_module_tag;
}

static const char *hal_get_driver_module_path(void)
{
  static char driver_module_path[128];
  strcpy(&driver_module_path[0], WIFI_DRIVER_MODULE_PATH);
  int i = strlen(driver_module_path);

  if (strncmp(&driver_module_path[0] + i - 3, ".ko", 3) == 0) {
    while (driver_module_path[--i] != '/')
      driver_module_path[i] = 0;
  } else if (driver_module_path[i -1] != '/') {
    driver_module_path[i++] = '/';
    driver_module_path[i++] = 0;
  }

  strcat(driver_module_path, get_wifi_driver_name());
  strcat(driver_module_path, ".ko");
  LOG(INFO) << "driver_module_path: " << driver_module_path;
  return (const char *)driver_module_path;
}

static const char *hal_get_driver_module_arg(void)
{
  const char *driver_module_arg = get_driver_module_arg();
  LOG(INFO) << "driver_module_arg: " << driver_module_arg;
  return (const char *)driver_module_arg;
}
#endif

#endif
