/*
 * Sunxi Soc uboot env update utils
 * Base on u-boot/tools/env
 *
 * Copyright (C) 2015-2018 AllwinnerTech, Inc.
 *
 * Contacts:
 * Zeng.Yajian <zengyajian@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include "libboot_recovery.h"
#include "libboot_info.h"
static struct libboot_info libbootinfo;



static void
usage(FILE *out, const char *name) {
	fprintf(out, "Usage: %s [input_file] [input file type]\n\n", name);
	fprintf(out,
		"       [input file], toc0.fex, toc1.fex, board_config.fex \n"
		"  [input file type], toc0, toc1, board-cfg\n"
		"               like, liboot_t toc0.fex toc0\n"
		"               like, liboot_t toc1.fex toc1\n"
		"               like, liboot_t board_config.fex board-cfg\n");
}

/*int forktoExec(int (*fun)(const char*), const char* path) {
    pid_t pid = fork();
    if (pid == 0) {
        exit(fun(path));
    } else if (pid > 0) {
        int status;
        waitpid(pid, &status, 0);
        if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
            return -1;
        }
        return 0;
    }
    return -1;
}*/

int main (int argc, char **argv)
{
	if (argc != 3) {
		printf("error argv\n");
		usage(stdout, argv[0]);
		return 0;
	}

	printf("argv[1]=%s, argv[2]=%s\n", argv[1], argv[2]);

    setIntermediaDir("/cache/libboot");
    /*
    if (strcmp("toc0",argv[2])==0) {
        updateToc0(argv[1]);
    } else if (strcmp("toc1",argv[2])==0) {
        updateToc1(argv[1]);
    } else if (strcmp("board-cfg",argv[2])==0) {
        updateBoardConfig(argv[1]);
    } else {
        printf("error!");
    }
    */

//        updateToc0(argv[1]);
 //       updateToc1(argv[2]);
  //      updateBoardConfig(argv[3]);
    //forktoExec(&updateBoot0,argv[1]);
    //forktoExec(&updateToc0,argv[1]);
    getLibbootInfo(&libbootinfo);
	libbootinfo.burnBoot0(argv[1]);
	printf("\n");
    printf("\n");
    printf("\n");   
    printf("\n");  
    //forktoExec(&updateToc1,argv[2]);
    //forktoExec(&updateUboot,argv[2]);
    libbootinfo.burnUboot(argv[2]);
    printf("\n");
    printf("\n");
    printf("\n");
    printf("\n");
    //forktoExec(&updateBoardConfig,argv[3]);

	return 0;
}


