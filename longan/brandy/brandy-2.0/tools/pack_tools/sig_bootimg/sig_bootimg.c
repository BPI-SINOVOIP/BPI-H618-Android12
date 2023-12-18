/*
**
** Copyright 2007, The Android Open Source Project
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>


typedef unsigned int u32;
typedef unsigned long long u64;
#include "android_image.h"

#define ALIGN(x,a)              __ALIGN_MASK(x,(typeof(x))(a)-1)
#define __ALIGN_MASK(x,mask)    (((x)+(mask))&~(mask))

static void *load_file(const char *fn, unsigned *_sz)
{
    char *data;
    int sz;
    int fd;

    data = 0;
    fd = open(fn, O_RDONLY);
    if(fd < 0) return 0;

    sz = lseek(fd, 0, SEEK_END);
    if(sz < 0) goto oops;

    if(lseek(fd, 0, SEEK_SET) != 0) goto oops;

    data = (char*) malloc(sz);
    if(data == 0) goto oops;

    if(read(fd, data, sz) != sz) goto oops;
    close(fd);

    if(_sz) *_sz = sz;
    return data;

oops:
    close(fd);
    if(data != 0) free(data);
    return 0;
}

/*
 * Pack boot image certification
 */
int usage(void)
{
    fprintf(stderr,"usage: sigbootimg\n"
            "       --image <filename>\n"
			"		--cert	<cert file>\n"
            "       -o|--output <filename>\n"
            );
    return 1;
}

static unsigned char padding[16384] = { 0, };

int write_padding(int fd, unsigned pagesize, unsigned itemsize)
{
    unsigned pagemask = pagesize - 1;
    unsigned count;

    if((itemsize & pagemask) == 0) {
        return 0;
    }

    count = pagesize - (itemsize & pagemask);

    if(write(fd, padding, count) != count) {
        return -1;
    } else {
        return 0;
    }
}

int main(int argc, char **argv)
{
	boot_img_hdr_ex hdr_ex;

	char *raw_image_fn = 0;
	void *raw_image_data = 0;
	unsigned raw_image_sz = 0;
	char *cert_fn = 0;
	void *cert_data = 0;
	char *bootimg = 0;

	unsigned pagesize = 2048;
	unsigned cert_offset = 0;
	int fd;

	argc--;
	argv++;

	memset(&hdr_ex, 0, sizeof(hdr_ex));

	while(argc > 0){
		char *arg = argv[0];
		char *val = argv[1];
		if(argc < 2) {
			return usage();
		}
		argc -= 2;
		argv += 2;
		if(!strcmp(arg, "--output") || !strcmp(arg, "-o")) {
			bootimg = val;
		} else if(!strcmp(arg, "--cert")) {
			cert_fn = val;
		} else if(!strcmp(arg, "--image")) {
			raw_image_fn = val;
		} else {
			return usage();
		}
	}

	if(bootimg == 0) {
		fprintf(stderr,"error: no output filename specified\n");
		return usage();
	}

	if(cert_fn == 0) {
		fprintf(stderr,"error: no certification image specified\n");
		return usage();
	}

	raw_image_data = load_file(raw_image_fn, &raw_image_sz);
	if(raw_image_data == 0) {
		fprintf(stderr,"error: could not load raw image '%s'\n", raw_image_fn);
		return usage();
	}

	memcpy(&hdr_ex,raw_image_data,sizeof(hdr_ex));
	strncpy(hdr_ex.cert_magic, AW_CERT_MAGIC , strlen(AW_CERT_MAGIC));
	memset(hdr_ex.padding,0,AW_IMAGE_PADDING_LEN);

	cert_data = load_file(cert_fn, &(hdr_ex.cert_size));
	if(cert_data == 0) {
		fprintf(stderr,"error: could not load certication '%s'\n", cert_fn);
		return 1;
	}

	fd = open(bootimg, O_CREAT | O_TRUNC | O_WRONLY, 0644);
	if(fd < 0) {
		fprintf(stderr,"error: could not create '%s'\n", bootimg);
		return 1;
	}

	cert_offset += ALIGN(sizeof(hdr_ex),pagesize);
	cert_offset += ALIGN(hdr_ex.std_hdr.kernel_size,pagesize);
	if (hdr_ex.std_hdr.ramdisk_size)
		cert_offset += ALIGN(hdr_ex.std_hdr.ramdisk_size, pagesize);
	if(hdr_ex.std_hdr.second_size)
		cert_offset += ALIGN(hdr_ex.std_hdr.second_size, pagesize);
	if (hdr_ex.std_hdr.recovery_dtbo_size)
		cert_offset += ALIGN(hdr_ex.std_hdr.recovery_dtbo_size, pagesize);
	if (hdr_ex.std_hdr.dtb_size)
		cert_offset += ALIGN(hdr_ex.std_hdr.dtb_size, pagesize);

	if(write(fd, raw_image_data, raw_image_sz) != raw_image_sz) goto fail;

	if(lseek(fd,AW_PAGESIZE-AW_CERT_DESC_SIZE,SEEK_SET) < 0) goto fail;
	if(write(fd, hdr_ex.cert_magic, BOOT_MAGIC_SIZE) != BOOT_MAGIC_SIZE) goto fail;
	if(write(fd, &(hdr_ex.cert_size), sizeof(unsigned)) != sizeof(unsigned)) goto fail;

	if(lseek(fd,cert_offset,SEEK_SET) < 0) goto fail;
	if(write(fd, cert_data, hdr_ex.cert_size) != hdr_ex.cert_size)  goto fail;
	if(write_padding(fd, pagesize, hdr_ex.cert_size))  goto fail;

	if(cert_data)
		free(cert_data);
	if(raw_image_data)
		free(raw_image_data);

	return 0;

fail:
	unlink(bootimg);
	close(fd);
	if(cert_data)
		free(cert_data);
	if(raw_image_data)
		free(raw_image_data);

	fprintf(stderr,"error: failed writing '%s': %s\n", bootimg,
			strerror(errno));
	return 1;
}
