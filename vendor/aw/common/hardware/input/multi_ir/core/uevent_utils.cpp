
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <poll.h>
#include <pthread.h>

#include <sys/time.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/queue.h>
#include <linux/netlink.h>
#include <utils/Log.h>

static const char * device_match_array[] = {
	"change@/devices/platform/soc/hdmi",
	"change@/devices/soc.0/1ee0000.hdmi",
};

static int fd = -1;

/* Returns !0 on failure, 0 on success */
int uevent_init(int *ufds)
{
    struct sockaddr_nl addr;
    int sz = 64*1024;
    int s;

    memset(&addr, 0, sizeof(addr));
    addr.nl_family = AF_NETLINK;
    addr.nl_pid = getpid();
    addr.nl_groups = 0xffffffff;

    s = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_KOBJECT_UEVENT);
    if(s < 0)
        return 0;

    setsockopt(s, SOL_SOCKET, SO_RCVBUFFORCE, &sz, sizeof(sz));

    if(bind(s, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
        close(s);
        return 0;
    }

    fd = s;
	*ufds = fd;
    return (fd > 0) ? 0 : -1;
}

int uevent_next_event(char* buffer, int buffer_length)
{
    while (1) {
        struct pollfd fds;
        int nr;

        fds.fd = fd;
        fds.events = POLLIN;
        fds.revents = 0;
        nr = poll(&fds, 1, -1);

        if(nr > 0 && (fds.revents & POLLIN)) {
            int count = recv(fd, buffer, buffer_length, 0);
            if (count > 0) {
                return count;
            }
        }
    }
    return 0;
}

/* return interval in sec */
long get_interval(void)
{
	static struct timeval previous;
	struct timeval current, interval;

	gettimeofday(&current, NULL);
	timersub(&current, &previous, &interval);
	previous = current;

	return interval.tv_sec;
}

extern void report_standby_request(void);
char msg[1024] = {0};
int uevent_process_event(void)
{
	int i;
	int count = 0;
	char *substr = NULL;
	int cec_opcode = 0;

	count = recv(fd, msg, 1024, 0);
	if (count<=0) return 0;

	msg[count] = 0;
	for (substr=msg; substr<msg+count; substr+=(strlen(substr)+1)) {
		if (!strcmp(substr, device_match_array[0])
				|| !strcmp(substr, device_match_array[1])) {
			i = 0;
			while (i++<4 && substr<(msg+count))
				substr += (strlen(substr)+1);

			sscanf(substr, "CEC_MSG=%x", &cec_opcode);
			ALOGV("recv: %s, opcode [%02x]\n", substr, cec_opcode);

			if (cec_opcode == 0x36) {
				long interval = get_interval();
				if (interval > 5 || interval < 0) {
					report_standby_request();
					ALOGD("standby request from HDMI-CEC\n");
				}
				ALOGD("receive CEC standy request, interval = %ld sec", interval);
			}
			break;
		}
	}

	return 0;
}

#if 0
int main(int argc, char **argv)
{
	char msg[1024] = {0};
	char *substr = NULL;
	int count, i;
	int cec_opcode = 0;

	if (uevent_init()!=0) {
		fprintf(stderr, "uevent init error\n");
		return -1;
	}

	while (1) {
		count = uevent_next_event(msg, sizeof(msg));
		if (count > 0) {
			msg[count] = 0;
			for (substr=msg; substr<msg+count; substr+=(strlen(substr)+1)) {
				if (strcmp(substr, DEVICE_MATCH_STR)==0) {
					i = 0;
					while (i++<4 && substr<(msg+count))
						substr += (strlen(substr)+1);

					sscanf(substr, "CEC_MSG=%x", &cec_opcode);
					fprintf(stdout, "recv: %s, opcode [%02x]\n", substr, cec_opcode);
					break;
				}
			}
		}
	}

	return 0;
}
#endif
