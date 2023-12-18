#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <pthread.h>
#include <linux/if_ether.h>
#include <arpa/inet.h>
#include "cutils/misc.h"
#include <unistd.h>
#include <sys/time.h>
#include <time.h>
#include <netutils/ifc.h>

#define MODULE_PATH "/system/vendor/modules/pktgen.ko"
#define IFNAME "eth0"
static int numTotalPkts = 800;
static int pktSize = 1200;
static int txDelay = 5000000;

static char *html_title = "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\"\n \
\"http://www.w3.org/TR/html4/loose.dtd\">\n \
<html>\n \
<body>\n \
<meta charset=\"utf-8\">\n \
<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\" />\n \
<h1 align=\"center\">以太网测试</h1>\n";

static char *html_result = "<p>测试结果（小于70秒成功）:<font color=red>%s</font></p> \n \
<p>总共发送包:%d</p> \n \
<p>总共接收包:%d</p> \n \
<p>平均包大小:%d字节</p> \n \
<p>接收消耗时间:%d秒</p> \n ";

static char *html_end = "</body> \n \
</html> \n \
";



extern int init_module(void *, unsigned long, const char *);
static int insmod(const char *filename, const char *args)
{
    void *module;
    unsigned int size;
    int ret;

    module = load_file(filename, &size);
    if (!module)
        return -1;
    ret = init_module(module, size, args);
    free(module);
    return ret;
}

static int postReport(int result,int numPkts,int numTotalPkts,int size,int costtime){
	char result_page[256];
	sprintf(result_page,html_result,result>0?"成功":"失败",numTotalPkts,numPkts,size,costtime);
	FILE *fp = fopen("/sdcard/eth_result.html","w");
        if(fp<0){
		return -1;
	}
	fwrite(html_title,strlen(html_title) + 1,1,fp);
	fwrite(result_page,strlen(result_page) + 1,1,fp);
	fwrite(html_end,strlen(html_end) + 1,1,fp);
	fclose(fp);
	//system("/system/bin/sh -c \"am start -n com.android.htmlviewer/.HTMLViewerActivity -d file:///sdcard/eth_result.html\"");
	return 0;

}



static int doRecv()
{
    int sockfd = -1;
    int numPkts = 0;
    char buf[1500] = {0};
    int size = 0;
    struct timeval stv,etv;
    if((sockfd = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL))) < 0) {
        printf("creat socket failed!\n");
        return -1;
    }
    if(setsockopt(sockfd, SOL_SOCKET, SO_BINDTODEVICE, IFNAME, strlen(IFNAME)) < 0) {
        printf("bind socket to device %s failed\n", IFNAME);
        close(sockfd);
        return -1;
    }
    gettimeofday(&stv, NULL);
    while(numPkts < numTotalPkts) {
        size = recvfrom(sockfd, buf, 1500, 0, NULL, NULL);
        if(size > 0) {
            numPkts++;
            printf("recv sockNum:%d sockSize:%d\n",numPkts, size);
        }
    }
    gettimeofday(&etv, NULL);
    close(sockfd);
    int costtime = etv.tv_sec - stv.tv_sec;
    int result = 0;
    if(costtime<70){
	   result = 1;
    }
    postReport(result,numPkts,numTotalPkts,size,costtime);
    printf("result:%s \n",result>0?"success":"failed" );
    printf("totalPackets:%d \n",numTotalPkts);
    printf("recvPackets:%d \n",numPkts);
    printf("avragePktSize:%d \n",size);
    printf("costTime:%d \n",time);
    
    return numPkts;
}

static void* recv_thread()
{
    return((void*)doRecv());
}

int main(int argc, char** agrv)
{
    int ret = 0;
    pthread_t fd;
    FILE* fp = NULL;
    char* msg = NULL;
    char module_arg[10] = {0};
    
    if(insmod(MODULE_PATH, module_arg) < 0) {
        printf("insmod pktgen.ko failed!\n");
        //return -1;
    }
    if(pthread_create(&fd, NULL, recv_thread, NULL) < 0) {
        printf("create recv_thread failed\n");
        return -1;
    }
    fp = fopen("/proc/net/pktgen/kpktgend_0", "w");
    if(fp != NULL) {
        asprintf(&msg, "%s %s", "add_device", IFNAME);
        fwrite(msg, strlen(msg) + 1, 1, fp);
        free(msg);
        fclose(fp);
    } else {
        printf("fopen /proc/net/pktgen/kpktgend_0 failed\n");
        return -1;
    }
    fp=fopen("/proc/net/pktgen/eth0", "w");
    if(fp != NULL) {
        asprintf(&msg, "%s %d", "count", numTotalPkts);
        fwrite(msg, strlen(msg) + 1, 1, fp);
        free(msg);
        fclose(fp);
    } else {
        printf("fopen /proc/net/pktgen/eth0 failed\n");
        return -1;
    }
    fp=fopen("/proc/net/pktgen/eth0", "w");
    if(fp != NULL) {
        asprintf(&msg, "%s %d", "pkt_size", pktSize);
        fwrite(msg, strlen(msg) + 1, 1, fp);
        free(msg);
        fclose(fp);
    } else {
        printf("fopen /proc/net/pktgen/eth0 failed!\n");
        return -1;
    }
    fp=fopen("/proc/net/pktgen/eth0", "w");
    if(fp != NULL) {
        unsigned char hwaddr[6];
        ifc_init();
        memset(hwaddr, 0, sizeof(hwaddr));
        if (ifc_get_hwaddr(IFNAME, (void *) hwaddr)) {
            asprintf(&msg, "%s %s", "dst_mac", "00:00:00:00:00:00");
        } else {
            asprintf(&msg, "%s %.2x:%.2x:%.2x:%.2x:%.2x:%.2x", "dst_mac",
                        hwaddr[0], hwaddr[1], hwaddr[2], hwaddr[3], hwaddr[4], hwaddr[5]);
        }
        fwrite(msg, strlen(msg) + 1, 1, fp);
        ifc_close();
        free(msg);
        fclose(fp);
    } else {
        printf("fopen /proc/net/pktgen/eth0 failed!!\n");
        return -1;
    }
    fp=fopen("/proc/net/pktgen/eth0", "w");
    if(fp != NULL) {
        asprintf(&msg, "%s %d", "delay", txDelay);
        fwrite(msg, strlen(msg) + 1, 1, fp);
        free(msg);
        fclose(fp);
    } else {
        printf("fopen /proc/net/pktgen/eth0 failed!!!\n");
        return -1;
    } 
    usleep(10000);
    fp=fopen("/proc/net/pktgen/pgctrl", "w");
    if(fp != NULL) {
        fwrite("start", sizeof("start"), 1, fp);
        fclose(fp);
    } else {
        printf("fopen /proc/net/pktgen/pgctrl failed\n");
        return -1;
    }
    pthread_join(fd, NULL);
    return 0;
}
