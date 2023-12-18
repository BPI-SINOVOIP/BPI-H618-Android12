/*
Copyright (c) 2020-2030 allwinnertech.com JetCui<cuiyuntao@allwinnertech.com>
*/
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <memory.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <pthread.h>
#include <stdbool.h>
#include <unistd.h>

#include "trans_header.h"
#include "sunxi_display2.h"

#ifdef ANDROID_PLT
#define DISP_CONFIG "/vendor/etc/sunxi_pq.cfg"
#define DISP_FIRMWARE "/vendor/etc/firmware/disp_firmware"
#define DISP_FIRMWARE_DATA "/data/disp_firmware"
#else
#define DISP_CONFIG "/etc/sunxi_pq.cfg"
#define DISP_FIRMWARE "/etc/firmware/disp_firmware"
#endif

struct list trans_list;

pthread_mutex_t notfiy_mutex;
pthread_cond_t notfiy_mCond;
pthread_mutex_t list_mutex; 

int disp_fd = -1;
int disp_firm_fd = -1;
int disp_firm_data_fd = -1;
extern int can_close;
extern int de_version;
extern int ic;
extern int de;

static int ic_de_talbe[2][2] = {
	{1855,201},
	{0,0}
};

extern int trans_to_dexx(int disp_fd, void *data, int cmd);
extern int download_firmware_generic(int disp_fd, int id, int de_d, char* buffer, trans_firm_data_t func);
extern int init_generic(void);
extern char* updata_firm_generic(struct PQ_package *packge, data_to_firm_t func, int* length);

long* trans_firm_data(char *buf, int *need)
{
	int i = 0;
	int num = 1;
	int *nu_re = NULL;
	long *reg = NULL;

	while (buf[i] != '}') {
		if (buf[i] == ',') {
			num++;
		}
#if (PQ_DEBUG_LEVEL>2)
		printf("%c", buf[i]);
		if (i%60==0) {
			printf("\n");
		}
#endif
		i++;
	}
	if (*need > 0 && num < *need) {
		PQ_Printf("a err num%d.",num);
		return NULL;
	}
	nu_re = (int *)malloc(num * sizeof(int)); 
	if (nu_re == NULL) {
		PQ_Printf("line[%d] malloc err.", __LINE__);
		return NULL;
	}
	i = 0;
	num = 0;
	nu_re[num++] = 1;
	while(buf[i] != '}') {
		if (buf[i] == ',') {
			nu_re[num++] = i+1;
		}
		i++;
	}
	reg = (long *)malloc(num * sizeof(long));
	if (reg == NULL) {
		PQ_Printf("line[%d] malloc err.", __LINE__);
		free(nu_re);
		return NULL;
	}
#if (PQ_DEBUG_LEVEL > 1)
		printf("data:\n");
#endif
	i = 0;
	while(i < num) {
		reg[i] = (long)atoi(&buf[(int)nu_re[i]]);
#if (PQ_DEBUG_LEVEL > 1)
		printf("%ld, ", reg[i]);
#endif		
		i++;
	}
#if (PQ_DEBUG_LEVEL > 1)
	printf("\n");
#endif
	free(nu_re);
	*need = num;

	return reg;
}

static int data_to_firm(char *firm, int length, int next, long value)
{
	int i = 0, tmp = next;
	char d[20];

	if (firm == NULL) {
		PQ_Printf("[%d]: give a NULL buf:", __LINE__);
		return 0;
	}
	if (next >= length) {
		PQ_Printf("[%d]:please give us a big buf:", __LINE__);
		return next;

	}
	if (next == 0) {
		firm[next++] = '{';
	}
	d[0] = value%10 + '0';
	while ((value/10)&& i < 20) {
		d[i++] = value%10 + '0';
		value = value/10;
	}
	if (i == 0)
		i++;
	while (i > 0 && (next < length) ) {
		firm[next++] = d[--i];
	}
	if (next >= length){
		PQ_Printf("[%d]:please give us a big buf%d:", __LINE__, i);
		return tmp;
	}
	firm[next++] = ',';

	return next;
}

int download_firmware(int fd)
{
	int ret, off_set = 0, file_off = 0, id = 0, de = 0, i = 0, go_end = 0;
	off_t off_end;
	ssize_t size;
	char *t_buff = NULL;
	int begin_id = 0, next = BEGIN_FANG;
	char find;
	char head[20];

	off_end = lseek(fd, 0, SEEK_END);
	lseek(fd, 0, SEEK_SET);
	if (0 != lseek(fd, 0, SEEK_CUR)) {
		ret = -3;
		close(fd);
		goto err;
	}
	if (read(fd, head, 20) <= 0) {
		PQ_Printf("Can read display firmware.");
		ret = -1;
		close(fd);
		goto err;
	}
	t_buff = strstr(head, "#DE");
	id = atoi(t_buff+3);
	head[19] = '\0';
	if (id != de_version) {
		PQ_Printf("de_version not match[%d]-[%d]:%s@%d.", de_version, id, t_buff, t_buff - head);
		ret = -1;
		close(fd);
		goto err;

	}
	lseek(fd, 0, SEEK_SET);
	if (0 !=lseek(fd, 0, SEEK_CUR)) {
		ret = -3;
		goto err;
	}

	while (file_off < (off_end-2)) {
		id = -1;
		find = '\0';
		next = BEGIN_FANG;
		de = 0;
		off_set = 0;
		/* find a whole id */
		size  = off_end - file_off;
		if (size > 4096)
			size = 1;
remap:
		i = 0;
		begin_id = 0;
		file_off += off_set;
		size -= off_set;
		size += 4096;
		size = ALLIGN(size, 4096);
		if (file_off + size >= off_end) {
			size = off_end - file_off;
			go_end++;
		}
		if (go_end >= 2) {
			break;
		}
		t_buff = (char *)mmap(0, size, PROT_READ, MAP_SHARED,
						fd, file_off);
		if (t_buff == MAP_FAILED) {
			PQ_Printf("map disp_firmware offset %d of size %ld err.", file_off, off_end);
			return -1;
		}
find_str:
		switch (next) {
			case BEGIN_FANG:
				find = '[';
			break;
			case END_FANG:
				find = ']';
			break;
			case BEGIN_DA:
				find = '{';
			break;
			case END_DA:
				find = '}';
			break;
			default:
				PQ_Printf("%s not a canonical write", DISP_FIRMWARE);
				continue;
		}
		off_set = i;
		while ((t_buff[i]) != find) {
			i++;
			if (i == size) {
				munmap(t_buff, size);
				goto remap;
			}
		}
		off_set = i;
		if (find == '[') {
			i++;
			while (t_buff[i++] < '0');
			if (t_buff[i-1] > '9') {
				PQ_Printf("give us a err cfg[%c].", t_buff[i-1]);
				munmap(t_buff, size);
				goto remap;
			}
			id = atoi(&t_buff[i-1]);
			i--;
			while(i < size) {
				if (t_buff[i] == ',') {
					while (t_buff[i++] < '0');
					if (t_buff[i-1] > '9') {
						PQ_Printf("give us a err ,[%c].", t_buff[i-1]);
						munmap(t_buff, size);
						goto remap;
					}
					de = atoi(&t_buff[i-1]);
				}
				if(t_buff[i++] == ']')
					break;
			}
			if (i >= size) {
				PQ_Printf("give us a err [%c].", t_buff[i-1]);
				munmap(t_buff, size);
				goto remap;
			}
			next = BEGIN_DA;
			goto find_str;
		}
		if (find == '{') {
			begin_id = i;
			next = END_DA;
			goto find_str;
		}
		if (find == '}') {			
			download_firmware_generic(disp_fd, id, de, &t_buff[begin_id], trans_firm_data);
		}
		next = BEGIN_FANG;
		if (size - i > 10) {
			goto find_str;
		}
		file_off += i;
		munmap(t_buff, size);
	}

	PQ_Printf("config display firmware completed...");
	return 0;
err:
	PQ_Printf("config display firmware err %d.", ret);
	return ret;

}

static void xchang_data(char* buff, int length, int f_id)
{
	int off_set = 0, file_off = 0, id = 0, de_need = 0, i = 0, j = 0, tmp =0;
	off_t off_end, find_off, cpy_off, begin_id = 0;
	ssize_t size, find_size = 0;
	char *t_buff = NULL, *tt_buff = NULL;
	int next = BEGIN_FANG;
	char find;

	off_end = lseek(disp_firm_data_fd, 0, SEEK_END);
	lseek(disp_firm_data_fd, 0, SEEK_SET);
	if (0 != lseek(disp_firm_data_fd, 0, SEEK_CUR)) {
		return;
	}
	while ((file_off < off_end-1) && (find_size == 0)) {
		id = -1;
		find = '[';
		next = BEGIN_FANG;
		de_need = 0;
		off_set = 0;
		/* find a whole id */
		size  = off_end - file_off;
		if (size > 4096)
			size = 1;
remap:
		i = 0;
		begin_id = 0;
		file_off += off_set;
		size -= off_set;
		size += 4096;
		size = ALLIGN(size, 4096);
		if (file_off + size >= off_end) {
			size = off_end - file_off;
		}

		t_buff = (char *)mmap(0, size, PROT_READ, MAP_SHARED,
						disp_firm_data_fd, file_off);
		if (t_buff == MAP_FAILED) {
			PQ_Printf("map disp_firmware offset %d of size %ld err.", file_off, off_end);
			return;
		}
find_str:
		switch (next) {
			case BEGIN_FANG:
				find = '[';
			break;
			case END_FANG:
				find = ']';
			break;
			case BEGIN_DA:
				find = '{';
			break;
			case END_DA:
				find = '}';
			break;
			default:
				PQ_Printf("%s not a canonical write", DISP_FIRMWARE);
				continue;
		}
		off_set = i;
		while ((t_buff[i]) != find) {
			i++;
			if (i == size) {
				munmap(t_buff, size);
				goto remap;
			}
		}
		off_set = i;
		if (find == '[') {
			find_off = off_set + file_off;
			i++;
			while (t_buff[i++] < '0');
			if (t_buff[i-1] > '9') {
				PQ_Printf("give us a err cfg[%c].", t_buff[i-1]);
				munmap(t_buff, size);
				goto remap;
			}
			id = atoi(&t_buff[i-1]);
			if (id != f_id) {
				goto find_str;
			}
			i--;
			while (i < size) {
				if (t_buff[i] == ',') {
					while (t_buff[i++] < '0');
					if (t_buff[i-1] > '9') {
						PQ_Printf("give us a err ,[%c].", t_buff[i-1]);
						munmap(t_buff, size);
						goto remap;
					}
					de_need = atoi(&t_buff[i-1]);
				}
				if(t_buff[i++] == ']') {
					break;
				}
			}
			if (de_need != de) {
				goto find_str;
			}
			if (i >= size) {
				PQ_Printf("%d:map little.", __LINE__);
				munmap(t_buff, size);
				goto remap;
			}
			next = BEGIN_DA;
			goto find_str;
		}
		if (find == '{') {
			begin_id = i;
			next = END_DA;
			goto find_str;
		}
		if (find == '}') {			
			find_size = i - begin_id;
			if (i < size-1) {
				if(t_buff[i+1] != '[') {
					t_buff[i+1] = 0xD;
				}
			}
		}
		begin_id += file_off;
		munmap(t_buff, size);
	}
	if (find_size != 0 && length) {
		if (find_size + find_off + 5 >= off_end || find_size == length) {
			find_off = lseek(disp_firm_data_fd, begin_id, SEEK_SET);
			if (find_off != begin_id) {
				PQ_Printf("%d:lseek err %ld.", __LINE__, find_off);
				return;
			}
			tmp = 1;
			if (find_size == length) {
				length -= 1;
				tmp = 2;
			}
		} else {
			tt_buff = (char *)malloc(4096);
			if (tt_buff == NULL) {
				PQ_Printf("%d:malloc err.", __LINE__);
				return;
			}
			size = 4096;
			begin_id = lseek(disp_firm_data_fd, find_off, SEEK_SET);
			if (find_off != begin_id) {
				PQ_Printf("%d:lseek err.", __LINE__);
				return;
			}
			cpy_off = file_off + find_size;
			while (cpy_off < off_end-1) {
				if (size > off_end - file_off) {
					size = off_end - file_off;
				}
				t_buff = (char *)mmap(0, size, PROT_READ, MAP_SHARED,
						disp_firm_data_fd, cpy_off);
				memcpy(tt_buff, t_buff, size);
				munmap(t_buff, size);
				cpy_off += size;
				write(disp_firm_data_fd, tt_buff, size);
			}
		}
	} else {
		find_off = lseek(disp_firm_data_fd, 0, SEEK_END);
		if (find_off != off_end) {
			PQ_Printf("%d:lseek err.", __LINE__);
			return;
		}
	}
	if (tmp != 1) {
		char dat[20];
		char head[20] = {'[','0',};
		i = 0, j = 0, tmp = f_id;
		while (f_id) {
			dat[i++] = f_id%10;
			f_id = f_id/10;
		}
		while (i--) {
			head[1+j] = dat[i]+'0';
			j++;
		}
		head[1+j] = ',';
		head[2+j] = '0' + de;
		head[3+j] = ']';
		head[4+j] = 0xD;
		head[5+j] = '\0';
		write(disp_firm_data_fd, head, 4 + j);
	}
	write(disp_firm_data_fd, buff, length);
	if (length-1 < find_size) {
		ftruncate(disp_firm_data_fd, size);
	}
	PQ_Printf("update firmware completed...");
}

int save_cfg_firmware(struct trans_data* data)
{
	int length = 0;
	char *buf;
	struct PQ_package *PQ_head = (struct PQ_package *)data->data;
	struct trans_header *id_head = (struct trans_header *)PQ_head->data;
	if (disp_firm_data_fd < 0) {
		disp_firm_data_fd = open(DISP_FIRMWARE_DATA, O_RDWR|O_CREAT, 0664);
		if (disp_firm_data_fd < 0) {
			PQ_Printf("Can not creat %s.", DISP_FIRMWARE_DATA);
			return -1;
		}
	}
	buf = updata_firm_generic(PQ_head, data_to_firm, &length);
	if (buf) {
		buf[length-1] = '}';
		buf[length] = 0xD;
		xchang_data(buf, length+1, id_head->id);
		free(buf);
	}
	return 0;
}

void send_to_translate(struct trans_data* data)
{
#if PQ_DEBUG_LEVEL
	PQ_Printf("receive %d.", data->count);
#endif
	init_list(&data->alist);
	pthread_mutex_lock(&list_mutex);
	add_list_tail(&trans_list, &data->alist);
	pthread_mutex_unlock(&list_mutex);
	pthread_cond_signal(&notfiy_mCond);
}

static inline void trans_send(struct trans_data* data, int read, int reply)
{
	struct PQ_package *PQ_head =  NULL;
	PQ_head = (struct PQ_package *)data->data;
	struct trans_reply *replay_d = NULL;
#if PQ_DEBUG_LEVEL
	PQ_Printf("send %d cmd:%d.", data->count, PQ_head->cmd);
#endif
	if ((read && reply != 0) || (!read)) {
		replay_d = (struct trans_reply *)PQ_head->data;
		replay_d->id = 0;
		replay_d->ret = reply;
		PQ_head->size =  sizeof(struct PQ_package) + sizeof(struct trans_reply);
	}
	send(data->client_fd, PQ_head, PQ_head->size, 0);
	free(data);

}

void* translate_thread(void *arg)
{
	int ret;
	struct trans_data* data;
	struct list *alist = NULL;
	struct PQ_package *PQ_head =  NULL;
	PQ_head = (struct PQ_package *)arg;

	PQ_Printf("Sunxi PQ entry tanslate thread.");

	while(1) {
		if (list_empty(&trans_list)) {
			can_close = 1;
			pthread_cond_wait(&notfiy_mCond, &notfiy_mutex);
		}
		can_close = 0;

		pthread_mutex_lock(&list_mutex);
		alist = trans_list.next;
		del_list(alist);
		pthread_mutex_unlock(&list_mutex);

		data = (struct trans_data*) alist;
		PQ_head = (struct PQ_package *)data->data;

		switch (PQ_head->cmd) {
		case 0:
			trans_send(data, 1, 0);
		break;
		case 1:
			/* read */
			ret = trans_to_dexx(disp_fd, PQ_head->data, READ_CMD);
			trans_send(data, 1, ret);
		break;
		case 3:
			/* save */
			ret = trans_to_dexx(disp_fd, PQ_head->data, WRITE_CMD);
			if (!ret) {
				 ret = save_cfg_firmware(data);
			}
			trans_send(data, 0, ret);
		break;
		case 2:
			/* set */
			ret = trans_to_dexx(disp_fd, PQ_head->data, WRITE_CMD);
			trans_send(data, 0, ret);
		break;
		case 4:
			/* upload cfg */
			ret = open(DISP_CONFIG, O_RDONLY);
			/*TODO*/
			trans_send(data, 1, ret);
		break;
		default:
			PQ_Printf("give us a wronge cmd:%d ...",PQ_head->cmd);
			trans_send(data, 0, -1);
		}
	}
}

static void get_ic_de(void)
{
	int ret = -1, i = 0;
	off_t off_end;
	char *t_buff = NULL, *buf = NULL, ne[10];
	int id_fgd = -1;

	id_fgd = open("/sys/class/sunxi_info/sys_info", O_RDONLY);
	if (id_fgd < 0) {
		PQ_Printf("open /sys/class/sunxi_info/sys_info err %d.", id_fgd);
		return ;
	}else {
		off_end = lseek(id_fgd, 0, SEEK_END);
		lseek(id_fgd, 0, SEEK_SET);
		if (0 != lseek(id_fgd, 0, SEEK_CUR)) {
			PQ_Printf("leek /sys/class/sunxi_info/sys_info err.");
			close(id_fgd);
			return;
		}
	}
	t_buff = malloc(sizeof(char)* off_end);
	if (t_buff == NULL) {
		PQ_Printf("map /sys/class/sunxi_info/sys_info err.");
		close(id_fgd);
		return ;
	}
	if (read(id_fgd, t_buff , off_end) <= 0) {
		goto ret ;
	}

	buf = strstr(t_buff, "sunxi_batchno");
	if (buf == NULL) {
		PQ_Printf("sunxi can find sunxi_batchno.");
		return;
	}
	buf = strstr(buf, "0x");
	if (buf == NULL) {
		PQ_Printf("sunxi can find 0x.");
		goto ret;
	}
	memcpy(ne, buf+2, 4);
	ne[4] = '\0';	
	ret = atoi(ne);
	i = 0;
	ic = ret;
	while (ic_de_talbe[i][0] != 0){
		if (ic_de_talbe[i][0] == ic){
			de_version = ic_de_talbe[i][1];
			break;
		}
		i++;
	}
ret:
	PQ_Printf("ic_id=%d, de_version=%d %s", ic, de_version, buf);
	free(t_buff);
	close(id_fgd);
}

int PQ_translate_init(void)
{
	pthread_t pthread_handle;	

	pthread_cond_init(&notfiy_mCond, NULL);
	pthread_mutex_init(&notfiy_mutex, NULL);
	pthread_mutex_init(&list_mutex, NULL);

	get_ic_de();
	init_generic();
	disp_fd = open("/dev/disp", O_RDWR);
	if (disp_fd < 0) {
		PQ_Printf("open display err %d\n", disp_fd);
		return -1;
	}
	init_list(&trans_list);

	/* download  the firmware  to  display */
	disp_firm_fd = open(DISP_FIRMWARE, O_RDWR);
	if (disp_firm_fd < 0) {
		disp_firm_fd = open(DISP_FIRMWARE, O_RDONLY);
		PQ_Printf("maybe %s, fd:%d @O_RDONLY.", DISP_FIRMWARE, disp_firm_fd);
	}
	if (disp_firm_fd > 0) {
		download_firmware(disp_firm_fd);
	}
	disp_firm_data_fd = open(DISP_FIRMWARE_DATA, O_RDWR);
	if (disp_firm_data_fd > 0) {
		download_firmware(disp_firm_data_fd);
	}
	if (disp_firm_fd < 0) {
		PQ_Printf("There is not the %s, creat it.", DISP_FIRMWARE);
		disp_firm_data_fd = open(DISP_FIRMWARE_DATA, O_RDWR|O_CREAT, 0664);
		if (disp_firm_data_fd < 0) {
			PQ_Printf("Can not creat %s.", DISP_FIRMWARE);
		} else {
			if (de_version > 0) {
				int i = 0, j = 0, tmp = de_version;
				char dat[10];
				char head[10] = {'#','D','E',};
				while (tmp) {
					dat[i++] = tmp%10;
					tmp = tmp/10;
				}
				while (i--) {
					head[3+j] = dat[i]+'0';
					j++;
				}
				head[3+j] = 0xD;
				write(disp_firm_data_fd, head, 3+j);
			}
		}
	}
	pthread_create(&pthread_handle, NULL, translate_thread, NULL);

	return 0;
}
