#ifndef __HCIATTACH_SPRD_H__
#define __HCIATTACH_SPRD_H__

#include <termios.h>

int sprd_vendor_init(void);
int sprd_vendor_power_on(void);
int sprd_vendor_power_off(void);
int sprd_vendor_config_init(int fd, char *bdaddr, struct termios *ti);

#endif /* __HCIATTACH_SPRD_H__ */
