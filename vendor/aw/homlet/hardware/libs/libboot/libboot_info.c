#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/wait.h>
//#include <sys/unistd.h>

#include "libboot_info.h"
#include "sunxi_boot_api.h"
#include "libboot.h"
#include "libboot_recovery.h"


static int forkToFunc(int (*fun)(const char *), const char* path){
	pid_t pid = fork();
	if(pid == 0){
		exit(fun(path));
    }
    else if(pid > 0){
        int status;
        waitpid(pid , &status, 0);
        if(!WIFEXITED(status) || WEXITSTATUS(status) != 0){
            return -1;
        }
        return 0;
    }
    return -1;
}

static int update_Boot0(const char * bootFile){
    return forkToFunc(updateBoot0,bootFile);
}

static int update_Uboot(const char * bootFile){
    return forkToFunc(updateUboot,bootFile);
}

static int update_Toc0(const char * bootFile){
	return forkToFunc(updateToc0,bootFile);
}

static int update_Toc1(const char * bootFile){
	return forkToFunc(updateToc1,bootFile);
}

static int update_Config(const char * bootFile){
 	return forkToFunc(updateBoardConfig,bootFile);
}

int getLibbootVersion(){
	return get_libboot_version();
}

int getLibbootInfo(struct libboot_info * info)
{
	if(info==NULL)
	{
		printf("libboot_info error\n");
		return -1;
	}
	info->flashType = getFlashType();
	if(info->flashType<0)
	{
		printf("get flash type error\n");
		return -1;
	}
	info->secureMode = check_soc_is_secure();
	if(info->secureMode)
	{
		info->burnBoot0 = update_Toc0;
		info->burnUboot = update_Toc1;
	}
	else
	{
		info->burnBoot0 = update_Boot0;
		info->burnUboot = update_Uboot;
	}
	info->burnConfig = update_Config;
    return 0;
}


