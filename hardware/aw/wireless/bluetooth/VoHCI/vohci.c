#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <cutils/sockets.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <getopt.h>
#include <tinyalsa/asoundlib.h>
#include <pthread.h>
#include <inttypes.h>

#define LOG_TAG "VoHCI"
#include <log/log.h>

#define DBG_E         ALOGE
#define DBG_W         ALOGW
#define DBG_I         ALOGI
#define DBG_D         ALOGD
#define DBG_V         ALOGV

#define VOHCI_VERSION "v1.0.0"

#define ARRAY_SIZE(x) sizeof(x) / sizeof(x[0])

#define SCO_CTRL_PATH "/data/misc/bluedroid/.sco_ctrl"
#define SCO_DATA_PATH "/data/misc/bluedroid/.sco_data"

typedef enum {
    SCO_CTRL_CMD_NONE,
    SCO_CTRL_CMD_CHECK_READY,
    SCO_CTRL_CMD_OUT_START,
    SCO_CTRL_CMD_IN_START,
    SCO_CTRL_CMD_OUT_STOP,
    SCO_CTRL_CMD_IN_STOP,
    SCO_CTRL_CMD_SUSPEND,
    SCO_CTRL_GET_AUDIO_CONFIG,
    SCO_CTRL_CMD_OFFLOAD_START,
    SCO_CTRL_CMD_CLOSE,
} tSCO_CTRL_CMD;

struct pcmcfg_t {
    char    *name;
    int      card;
    int      device;
    uint32_t channels;
    uint32_t bits;
    uint32_t rate;
    uint32_t period_size;
    uint32_t period_count;
};

struct codec_para_t {
    uint8_t  input_format;
    uint8_t  input_channels;
    uint8_t  input_bits;
    uint16_t input_rate;
    uint8_t  output_format;
    uint8_t  output_channels;
    uint8_t  output_bits;
    uint16_t output_rate;
} __attribute__ ((packed));

/* control block */
struct vohci_t {
    struct pcmcfg_t pcmcfg;
    struct codec_para_t codec_para;
    int ctrl_fd;
    int data_fd;
    pthread_t thread_ctrl_id;
    pthread_t thread_recv_id;
    pthread_t thread_send_id;
    pthread_cond_t send_cond;
    pthread_mutex_t mutex;
    bool ctrl_thread_running;
    bool data_thread_running;
    bool debug;
};

static struct vohci_t vohci = {
    .pcmcfg = {
        .name     = "VoHCILoopback",
        .card     = 1,
        .device   = 0,
        .bits     = 16,
        .channels = 1,
        .rate     = 8000,
        .period_size  = 1024,
        .period_count = 1,
    },
    .debug = false,
};

static void *vohci_ctrl_thread(void *arg);
static void *vohci_recv_thread(void *arg);
static void *vohci_send_thread(void *arg);

static const char *sco_cmd_to_name(tSCO_CTRL_CMD cmd)
{
    switch (cmd) {
        case SCO_CTRL_CMD_NONE: return "SCO_CTRL_CMD_NONE";
        case SCO_CTRL_CMD_CHECK_READY: return "SCO_CTRL_CMD_NONE";
        case SCO_CTRL_CMD_OUT_START: return "SCO_CTRL_CMD_OUT_START";
        case SCO_CTRL_CMD_IN_START: return "SCO_CTRL_CMD_IN_START";
        case SCO_CTRL_CMD_OUT_STOP: return "SCO_CTRL_CMD_OUT_STOP";
        case SCO_CTRL_CMD_IN_STOP: return "SCO_CTRL_CMD_IN_STOP";
        case SCO_CTRL_CMD_SUSPEND: return "SCO_CTRL_CMD_SUSPEND";
        case SCO_CTRL_GET_AUDIO_CONFIG: return "SCO_CTRL_GET_AUDIO_CONFIG";
        case SCO_CTRL_CMD_OFFLOAD_START: return "SCO_CTRL_CMD_OFFLOAD_START";
        case SCO_CTRL_CMD_CLOSE: return "SCO_CTRL_CMD_CLOSE";
        default: return "UNKNOWN CMD";
    }
    return "NULL";
}

static uint64_t time_gettimeofday_us(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000000ULL + (uint64_t)tv.tv_usec;
}

static void timer_handle(int signo)
{
    pthread_mutex_lock(&vohci.mutex);
    pthread_cond_signal(&vohci.send_cond);
    pthread_mutex_unlock(&vohci.mutex);
}

static int vohci_open(void)
{
    pthread_attr_t thread_attr;
    pthread_attr_init(&thread_attr);
    pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_JOINABLE);

    vohci.ctrl_thread_running = true;
    if (pthread_create(&vohci.thread_ctrl_id, &thread_attr, vohci_ctrl_thread, NULL) != 0) {
        DBG_E("pthread_create : %s", strerror(errno));
        return -1;
    }

    DBG_D("%s done", __func__);
    return 0;
}

static void vohci_close(void)
{
    int result;

    vohci.ctrl_thread_running = false;
    vohci.data_thread_running = false;

    pthread_join(vohci.thread_ctrl_id, NULL);

    DBG_D("%s done", __func__);
}

static int pcm_check_param(struct pcm_params *params, uint32_t param, uint32_t value,
                 char *param_name, char *param_unit)
{
    uint32_t min;
    uint32_t max;
    int is_within_bounds = 1;

    min = pcm_params_get_min(params, param);
    if (value < min) {
        DBG_E("%s is %u%s, device only supports >= %u%s\n", param_name, value,
                param_unit, min, param_unit);
        is_within_bounds = 0;
    }

    max = pcm_params_get_max(params, param);
    if (value > max) {
        DBG_E("%s is %u%s, device only supports <= %u%s\n", param_name, value,
                param_unit, max, param_unit);
        is_within_bounds = 0;
    }

    return is_within_bounds;
}

static int pcm_is_playable(uint32_t card, uint32_t device, uint32_t channels,
                        uint32_t rate, uint32_t bits, uint32_t period_size,
                        uint32_t period_count)
{
    struct pcm_params *params;
    int can_play;

    params = pcm_params_get(card, device, PCM_OUT);
    if (params == NULL) {
        DBG_E("Unable to open PCM device %u.\n", device);
        return 0;
    }

    can_play = pcm_check_param(params, PCM_PARAM_RATE, rate, "Sample rate", "Hz");
    can_play &= pcm_check_param(params, PCM_PARAM_CHANNELS, channels, "Sample", " channels");
    can_play &= pcm_check_param(params, PCM_PARAM_SAMPLE_BITS, bits, "Bitrate", " bits");
    can_play &= pcm_check_param(params, PCM_PARAM_PERIOD_SIZE, period_size, "Period size", " frames");
    can_play &= pcm_check_param(params, PCM_PARAM_PERIODS, period_count, "Period count", " periods");

    pcm_params_free(params);

    return can_play;
}

static struct pcm *vohci_play_init(uint32_t card, uint32_t device, uint32_t channels,
                 uint32_t rate, uint32_t bits, uint32_t period_size,
                 uint32_t period_count)
{
    struct pcm_config config;
    struct pcm *pcm;
    char *buffer;
    uint32_t size, read_sz;
    int num_read;

    memset(&config, 0, sizeof(config));
    config.channels = channels;
    config.rate = rate;
    config.period_size = period_size;
    config.period_count = period_count;
    if (bits == 32)
        config.format = PCM_FORMAT_S32_LE;
    else if (bits == 24)
        config.format = PCM_FORMAT_S24_3LE;
    else if (bits == 16)
        config.format = PCM_FORMAT_S16_LE;
    config.start_threshold = 0;
    config.stop_threshold = 0;
    config.silence_threshold = 0;

    if (!pcm_is_playable(card, device, channels, rate, bits, period_size, period_count)) {
        return NULL;
    }

    pcm = pcm_open(card, device, PCM_OUT, &config);
    if (!pcm || !pcm_is_ready(pcm)) {
        DBG_E("Unable to open PCM device %u (%s)", device, pcm_get_error(pcm));
        return NULL;
    }

    return pcm;
}

static struct pcm *vohci_capture_init(uint32_t card, uint32_t device,
                            uint32_t channels, uint32_t rate,
                            uint32_t bits, uint32_t period_size,
                            uint32_t period_count)
{
    struct pcm_config config;
    struct pcm *pcm;
    char *buffer;
    uint32_t size;
    uint32_t bytes_read = 0;
    enum pcm_format format;

    switch (bits) {
    case 32:
        format = PCM_FORMAT_S32_LE;
        break;
    case 24:
        format = PCM_FORMAT_S24_LE;
        break;
    case 16:
        format = PCM_FORMAT_S16_LE;
        break;
    default:
        DBG_E("%u bits is not supported.\n", bits);
        return NULL;
    }

    memset(&config, 0, sizeof(config));
    config.channels = channels;          // -c
    config.rate = rate;                  // -r
    config.period_size = period_size;    // -p
    config.period_count = period_count;  // -n
    config.format = format;              // -b bits -> format
    config.start_threshold = 0;
    config.stop_threshold = 0;
    config.silence_threshold = 0;

    pcm = pcm_open(card, device, PCM_IN, &config);
    if (!pcm || !pcm_is_ready(pcm)) {
        DBG_E("Unable to open PCM device (%s)", pcm_get_error(pcm));
        return NULL;
    }

    return pcm;
}

static void *vohci_ctrl_thread(void *arg)
{
    (void)arg;
    int len, ret;
    uint8_t cmd;
    struct timeval timeout = { .tv_sec = 0, .tv_usec = 500000}; //500ms

    pthread_attr_t thread_attr;
    pthread_attr_init(&thread_attr);
    pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_JOINABLE);

    while (1) {
        vohci.ctrl_fd = socket(AF_LOCAL, SOCK_STREAM, 0);
        if (vohci.ctrl_fd < 0) {
            DBG_V("ctrl socket open fail");
            goto done;
        }

        ret = socket_local_client_connect(vohci.ctrl_fd, SCO_CTRL_PATH, ANDROID_SOCKET_NAMESPACE_ABSTRACT, 0);
        if (ret < 0) {
            DBG_V("ctrl socket connect fail");
            goto err_ctrl_connect;
        }

        DBG_D("sco ctrl: socket connect success");

        setsockopt(vohci.ctrl_fd, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(struct timeval));
        setsockopt(vohci.ctrl_fd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(struct timeval));

        cmd = SCO_CTRL_CMD_IN_START;
        len = send(vohci.ctrl_fd, &cmd, 1, 0);
        DBG_D("send cmd: %s, len: %d", sco_cmd_to_name(cmd), len);
        if (len <= 0)
            goto err_ctrl_connect;

        cmd = SCO_CTRL_CMD_OUT_START;
        len = send(vohci.ctrl_fd, &cmd, 1, 0);
        DBG_D("send cmd: %s, len: %d", sco_cmd_to_name(cmd), len);
        if (len <= 0)
            goto err_ctrl_connect;

        cmd = SCO_CTRL_GET_AUDIO_CONFIG;
        len = send(vohci.ctrl_fd, &cmd, 1, 0);
        DBG_D("send cmd: %s, len: %d", sco_cmd_to_name(cmd), len);
        if (len <= 0)
            goto err_ctrl_connect;

        len = recv(vohci.ctrl_fd, &vohci.codec_para, sizeof(struct codec_para_t), 0);
        DBG_D("recv cmd: %s, len: %d", sco_cmd_to_name(cmd), len);
        if (len <= 0)
            goto err_ctrl_connect;

        DBG_D("Codec settings(in/out): format = %d/%d, channels = %d/%d, bits = %d/%d, rate = %d/%d",
                vohci.codec_para.input_format,   vohci.codec_para.output_format,
                vohci.codec_para.input_channels, vohci.codec_para.output_channels,
                vohci.codec_para.input_bits,     vohci.codec_para.output_bits,
                vohci.codec_para.input_rate,     vohci.codec_para.output_rate);

        vohci.data_fd = socket(AF_LOCAL, SOCK_STREAM, 0);
        if (vohci.data_fd < 0) {
            DBG_V("data socket open fail");
            goto err_ctrl_connect;
        }

        setsockopt(vohci.data_fd, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(struct timeval));
        setsockopt(vohci.data_fd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(struct timeval));

        ret = socket_local_client_connect(vohci.data_fd, SCO_DATA_PATH, ANDROID_SOCKET_NAMESPACE_ABSTRACT, 0);
        if (ret < 0) {
            DBG_V("data socket connect fail");
            goto err_data_connect;
        }

        vohci.data_thread_running = true;

        if (pthread_create(&vohci.thread_recv_id, &thread_attr, vohci_recv_thread, NULL) != 0) {
            DBG_E("pthread_create : %s", strerror(errno));
            goto err_data_connect;
        }

        if (pthread_create(&vohci.thread_send_id, &thread_attr, vohci_send_thread, NULL) != 0) {
            DBG_E("pthread_create : %s", strerror(errno));
            goto err_recv_thread;
        }

        pthread_join(vohci.thread_recv_id, NULL);
        pthread_join(vohci.thread_send_id, NULL);

        vohci.data_thread_running = false;
        close(vohci.data_fd);

        cmd = SCO_CTRL_CMD_IN_STOP;
        len = send(vohci.ctrl_fd, &cmd, 1, 0);
        DBG_D("send cmd: %s, len: %d", sco_cmd_to_name(cmd), len);

        cmd = SCO_CTRL_CMD_OUT_STOP;
        len = send(vohci.ctrl_fd, &cmd, 1, 0);
        DBG_D("send cmd: %s, len: %d", sco_cmd_to_name(cmd), len);

        DBG_D("sco ctrl: waiting next connection...");

        goto err_ctrl_connect;

err_recv_thread:
        ret = pthread_join(vohci.thread_recv_id, NULL);

err_data_connect:
        close(vohci.data_fd);
        vohci.data_thread_running = false;

err_ctrl_connect:
        close(vohci.ctrl_fd);

done:
        if (vohci.ctrl_thread_running == false)
            break;

        usleep(500000);
    }

    DBG_D("%s exit", __func__);
    return NULL;
}

static void *vohci_recv_thread(void *arg)
{
    (void)arg;
    int bytes, needs;
    uint64_t total = 0;
    int package_size = 120, len;

    char buffer[1024];
    int16_t *resample = (int16_t *)&buffer[0];
    int n = 0;

    uint64_t ts0, ts1;

    struct pcm *pcm = vohci_play_init(vohci.pcmcfg.card, vohci.pcmcfg.device,
                                      vohci.pcmcfg.channels, vohci.pcmcfg.rate,
                                      vohci.pcmcfg.bits, vohci.pcmcfg.period_size, vohci.pcmcfg.period_count);

    if (pcm == NULL) {
        DBG_E("pcm play device init fail");
        goto done;
    }

    if (vohci.codec_para.input_rate == 16000)
        package_size = 240;

    while (vohci.data_thread_running) {
        len = recv(vohci.data_fd, buffer, package_size, 0);
        if (len <= 0) {
            goto pipe_broken;
        }
        total += len;
        if (len > 0) {
            if (vohci.codec_para.input_rate == 16000) { // 16k to 8k, resample
                len /= 2;
                for (n = 0; n < len / 2; n++) {
                    resample[n] = (resample[n * 2] + resample[n * 2 + 1]) / 2;
                }
            }
            pcm_write(pcm, buffer, len);

            ts1 = time_gettimeofday_us();
            ts0 = ts0 ? ts0 : ts1;
            if (total % (package_size * 100) == 0) {
                DBG_D("tid: %5d, recv data len: %7" PRIu64 ", time: %6" PRIu64 " ms, delta: %4" PRIu64 " ms(expect 750 ms)", gettid(), total, total * 750 / (package_size * 100), (ts1 - ts0) / 1000);
                ts0 = ts1;
            }
        }
    }
pipe_broken:
    pcm_close(pcm);

done:
    DBG_D("%s exit", __func__);
    return NULL;
}

static void *vohci_send_thread(void *arg)
{
    (void)arg;
    int bytes, needs;
    uint64_t total = 0;
    int package_size = 120, datasize = INT32_MAX, len = 120;

    char buffer[1024];
    int16_t *resample = (int16_t *)&buffer[0];
    int n = 0;

    uint64_t ts0 = 0, ts1;

    struct sigaction tact;
    struct itimerval value;

    struct pcm *pcm = vohci_capture_init(vohci.pcmcfg.card, vohci.pcmcfg.device,
                                         vohci.pcmcfg.channels, vohci.pcmcfg.rate,
                                         vohci.pcmcfg.bits, vohci.pcmcfg.period_size, vohci.pcmcfg.period_count);

    if (pcm == NULL) {
        DBG_E("pcm capture device init fail");
        goto done;
    }

    pthread_cond_init(&vohci.send_cond, NULL);
    pthread_mutex_init(&vohci.mutex, NULL);

    tact.sa_handler = timer_handle;
    tact.sa_flags = 0;
    sigemptyset(&tact.sa_mask);
    sigaction(SIGALRM, &tact, NULL);

    // 120 / (8k * 2) = 0.0075s = 7500us
    value.it_value.tv_sec = 0;
    value.it_value.tv_usec = 7500;
    value.it_interval = value.it_value;
    setitimer(ITIMER_REAL, &value, NULL);

    if (vohci.codec_para.output_rate == 16000)
        package_size = 240;

    while (vohci.data_thread_running) {
        pthread_mutex_lock(&vohci.mutex);
        pthread_cond_wait(&vohci.send_cond, &vohci.mutex);
        pthread_mutex_unlock(&vohci.mutex);

        pcm_read(pcm, buffer, len);

        if (vohci.codec_para.output_rate == 16000) {// 8k to 16k, resample
            for (n = package_size / 2 - 1; n >= 0; n--) {
                resample[n * 2 + 0] = resample[n];
                resample[n * 2 + 1] = resample[n];
            }
        }

        bytes = 0;
        needs = package_size;
        while (needs > 0 && total < datasize) {
            bytes  = send(vohci.data_fd, buffer + bytes, needs, 0);
            if (bytes <= 0) {
                goto pipe_broken;
            }
            needs -= bytes;
            total += bytes;

            ts1 = time_gettimeofday_us();
            ts0 = ts0 ? ts0 : ts1;
            if (total % (package_size * 100) == 0) {
                DBG_D("tid: %5d, send data len: %7" PRIu64 ", time: %6" PRIu64 " ms, delta: %4" PRIu64 " ms(expect 750 ms)", gettid(), total, total * 750 / (package_size * 100), (ts1 - ts0) / 1000);
                ts0 = time_gettimeofday_us();
            }
        }
    }
pipe_broken:
    pcm_close(pcm);
    value.it_value.tv_sec = 0;
    value.it_value.tv_usec = 0;
    value.it_interval = value.it_value;
    setitimer(ITIMER_REAL, &value, NULL);
    usleep(100000); //wait next mutex & cond done
    pthread_mutex_destroy(&vohci.mutex);
    pthread_cond_destroy(&vohci.send_cond);

done:
    DBG_D("%s exit", __func__);
    return NULL;
}

static void handle_termination(int signo)
{
    DBG_D("Reviced signal [%d], cleanup...", signo);
    vohci_close();
    DBG_D("Termination~");
    _exit(0);
}

struct option_t {
    const struct option long_options;
    const char *help;
};

static const struct option_t option_list[] = {
    {{"card",         1, NULL, 'D'}, "sound card name, default Loopback"},
    {{"device",       1, NULL, 'd'}, "device in card, default 0"},
    {{"period_size",  1, NULL, 'p'}, "period_size, default 1024"},
    {{"period_count", 1, NULL, 'n'}, "period_count, default 4"},
    {{"channels",     1, NULL, 'c'}, "channels, default 1"},
    {{"bits",         1, NULL, 'b'}, "pcm bits, default 16"},
    {{"rate",         1, NULL, 'r'}, "pcm rate, default 8000"},
};

int show_help(const char *name)
{
    int i;
    DBG_D("Usage: %s [|D|d|p|n|c|b|r|h]", name);
    DBG_D("");

    for (i = 0; i < ARRAY_SIZE(option_list); i++)
        DBG_D("\t-%c, --%-10s %s",
                    option_list[i].long_options.val,
                    option_list[i].long_options.name,
                    option_list[i].help);

    DBG_D("");
    return 0;
}

int load_config(int argc, char **argv)
{
    int ret, opt, i;
    struct option long_options[ARRAY_SIZE(option_list)];
    char path[64];
    char buf[16];
    int fd, sz, cardnum = -1;
    const char *basepath = "/sys/class/sound"; // or "/proc/asound"
    DIR *d;
    struct dirent *de;

    for (i = 0; i < ARRAY_SIZE(option_list); i++) {
        long_options[i] = option_list[i].long_options;
    }

    while ((opt = getopt_long(argc, argv, "D:d:p:n:c:b:r:h", long_options, NULL)) != -1) {
        switch(opt) {
            case 'D':
                vohci.pcmcfg.name = optarg;
                break;
            case 'd':
                vohci.pcmcfg.device = strtoul(optarg, 0, 0);
                break;
            case 'p':
                vohci.pcmcfg.period_size = strtoul(optarg, 0, 0);
                break;
            case 'n':
                vohci.pcmcfg.period_count = strtoul(optarg, 0, 0);
                break;
            case 'c':
                vohci.pcmcfg.channels = strtoul(optarg, 0, 0);
                break;
            case 'b':
                vohci.pcmcfg.bits = strtoul(optarg, 0, 0);
                break;
            case 'r':
                vohci.pcmcfg.rate = strtoul(optarg, 0, 0);
                break;
            default:
                show_help(argv[0]);
                exit(0);
        }
    }

    if (!(d = opendir(basepath)))
        return -1;

    while ((de = readdir(d))) {
        if (strstr(de->d_name, "card")) {
            snprintf(path, sizeof(path), "%s/%s/id", basepath, de->d_name);
            fd = open(path, O_RDONLY);
            if (fd < 0)
                continue;

            sz = read(fd, &buf, sizeof(buf));
            close(fd);

            if (sz <= 0)
                continue;

            for (i = sz - 1; i > 0; i--) {
                if (buf[i] == '\r' || buf[i] == '\n') {
                    buf[i] = 0;
                    sz--;
                }
            }

            if (sz == strlen(vohci.pcmcfg.name) && memcmp(buf, vohci.pcmcfg.name, sz) == 0) {
                cardnum = strtoul(de->d_name + 4, 0, 0);
                break;
            }
        }
    }
    closedir(d);

    if (cardnum == -1) {
       DBG_E("Cannot get card id for name: %s", vohci.pcmcfg.name);
       return -1;
    }

    vohci.pcmcfg.card = cardnum;
    DBG_D("PCM name: %s, card: %d, device: %d, channels: %d, rate: %d, bits: %d, period_size: %d, period_count: %d",
           vohci.pcmcfg.name, vohci.pcmcfg.card, vohci.pcmcfg.device, vohci.pcmcfg.channels,
           vohci.pcmcfg.rate, vohci.pcmcfg.bits, vohci.pcmcfg.period_size, vohci.pcmcfg.period_count);

    return 0;
}

int main(int argc, char **argv)
{
    DBG_D("VoHCI service version: %s", VOHCI_VERSION);
    if (load_config(argc, argv) < 0)
        return -1;

    signal(SIGINT, handle_termination);
    signal(SIGABRT, SIG_IGN);
    signal(SIGPIPE, SIG_IGN);

    if (vohci_open() < 0)
        return -1;

    while (1) {
        usleep(INT32_MAX);
    }

    return 0;
}
