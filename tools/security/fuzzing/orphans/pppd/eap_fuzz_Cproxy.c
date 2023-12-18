#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <pwd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#include <errno.h>

#include "pppd.h"
#include "pathnames.h"
#include "md5.h"
#include "eap.h"
#include "magic.h"

void init()
{
    eap_protent.init(0);
}


void proxy_packet(uint8_t *data, int len)
{
    eap_protent.input(0, data, len);
}
