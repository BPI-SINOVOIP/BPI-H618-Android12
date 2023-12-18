/************************************************************************************
 *
 *  Filename:      bdt_unisoc.c
 *
 *  Description:   bluetooth Test application
 *
 ***********************************************************************************/

#include <stdio.h>
#include <dlfcn.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/prctl.h>
#include <sys/capability.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>

#include <semaphore.h>
#include <pthread.h>

#include "vendor_suite.h"
#include "bdt_unisoc.h"

/************************************************************************************
**  Constants & Macros
************************************************************************************/

#define PID_FILE "/data/.bdt_pid"

#ifndef MAX
#define MAX(x, y) ((x) > (y) ? (x) : (y))
#endif

#define CASE_RETURN_STR(const) case const: return #const;

/** Used to remove warnings about unused parameters */
#define UNUSED(x) ((void)(x))
#define is_cmd(str) ((strlen(str) == strlen(cmd)) && strncmp((const char *)&cmd, str, strlen(str)) == 0)

#define  BTD(format, ...)                          \
    do{                                                 \
        printf(format "\n", ## __VA_ARGS__);    \
    }while(0)
/************************************************************************************
**  Static variables
************************************************************************************/
static void *lib_handle;
static bt_vendor_suite_interface_t *lib_interface;
static sem_t semt_wait;
static pthread_mutex_t mutex_lock;

static unsigned char main_done = 0;
static unsigned char bt_enabled = 0;
static bt_status_t status;

/************************************************************************************
**  Static functions
************************************************************************************/
static void process_cmd(char *p, unsigned char is_job);
static void bdt_log(const char *fmt_str, ...);

/************************************************************************************
**  Shutdown helper functions
************************************************************************************/

static void bdt_shutdown(void)
{
    bdt_log("shutdown bdroid test app\n");
    main_done = 1;
}

/*****************************************************************************
**   Logger API
*****************************************************************************/

void bdt_log(const char *fmt_str, ...)
{
    static char buffer[1024];
    va_list ap;

    va_start(ap, fmt_str);
    vsnprintf(buffer, 1024, fmt_str, ap);
    va_end(ap);

    fprintf(stdout, "%s\n", buffer);
}


/*******************************************************************************
 ** Misc helper functions
 *******************************************************************************/
static const char* dump_bt_status(bt_status_t status)
{
    switch(status)
    {
        CASE_RETURN_STR(BT_STATUS_SUCCESS)
        CASE_RETURN_STR(BT_STATUS_FAIL)
        CASE_RETURN_STR(BT_STATUS_NOT_READY)
        CASE_RETURN_STR(BT_STATUS_NOMEM)
        CASE_RETURN_STR(BT_STATUS_BUSY)
        CASE_RETURN_STR(BT_STATUS_UNSUPPORTED)

        default:
            return "unknown status code";
    }
}

void skip_blanks(char **p)
{
  while (**p == ' ')
    (*p)++;
}

int get_signed_int(char **p, int DefaultValue)
{
  int    Value = 0;
  unsigned char   UseDefault;
  unsigned char  NegativeNum = 0;

  UseDefault = 1;
  skip_blanks(p);

  if ( (**p) == '-')
    {
      NegativeNum = 1;
      (*p)++;
    }
  while ( ((**p)<= '9' && (**p)>= '0') )
    {
      Value = Value * 10 + (**p) - '0';
      UseDefault = 0;
      (*p)++;
    }

  if (UseDefault)
    return DefaultValue;
  else
    return ((NegativeNum == 0)? Value : -Value);
}

void get_str(char **p, char *Buffer)
{
  skip_blanks(p);

  while (**p != 0 && **p != ' ')
    {
      *Buffer = **p;
      (*p)++;
      Buffer++;
    }

  *Buffer = 0;
}

uint32_t get_hex(char **p, int DefaultValue)
{
  uint32_t Value = 0;
  unsigned char   UseDefault;

  UseDefault = 1;
  skip_blanks(p);

  while ( ((**p)<= '9' && (**p)>= '0') ||
          ((**p)<= 'f' && (**p)>= 'a') ||
          ((**p)<= 'F' && (**p)>= 'A') )
    {
      if (**p >= 'a')
        Value = Value * 16 + (**p) - 'a' + 10;
      else if (**p >= 'A')
        Value = Value * 16 + (**p) - 'A' + 10;
      else
        Value = Value * 16 + (**p) - '0';
      UseDefault = 0;
      (*p)++;
    }

  if (UseDefault)
    return DefaultValue;
  else
    return Value;
}

void get_bdaddr(const char *str, bt_bdaddr_t *bd) {
    char *d = ((char *)bd), *endp;
    int i;
    for(i = 0; i < 6; i++) {
        *d++ = strtol(str, &endp, 16);
        if (*endp != ':' && i != 5) {
            memset(bd, 0, sizeof(bt_bdaddr_t));
            return;
        }
        str = endp + 1;
    }
}

typedef void (t_console_cmd_handler) (char *p);

typedef struct {
    const char *name;
    t_console_cmd_handler *handler;
    const char *help;
    unsigned char is_job;
} t_cmd;

const t_cmd console_cmd_list[];

static void cmdjob_handler(void *param)
{
    char *job_cmd = (char*)param;

    bdt_log("cmdjob starting (%s)", job_cmd);

    process_cmd(job_cmd, 1);

    bdt_log("cmdjob terminating");

    free(job_cmd);
}

static int create_cmdjob(char *cmd)
{
    pthread_t thread_id;
    char *job_cmd;

    job_cmd = malloc(strlen(cmd)+1); /* freed in job handler */
    strcpy(job_cmd, cmd);

    if (pthread_create(&thread_id, NULL,
                       (void*)cmdjob_handler, (void*)job_cmd)!=0)
      perror("pthread_create");

    return 0;
}

static void bdt_cleanup(void)
{
    //BTD();
    if (lib_handle)
        dlclose(lib_handle);
    lib_handle = NULL;
    lib_interface = NULL;
}

static int bdt_init(void)
{
    bdt_log("INIT BT ");
    if(lib_handle != NULL){
        bdt_log("libbt-vendor handle should be null");
	}

	lib_handle = dlopen("/vendor/lib/libbt-sprd_suite.so", RTLD_NOW);
	if (!lib_handle) {
        bdt_log("%s unable to open libbt-sprd_suite.so: %s", __func__, dlerror());
        goto error;
    }

	lib_interface = (bt_vendor_suite_interface_t *)dlsym(lib_handle, "BT_VENDOR_SUITE_INTERFACE");
	if(!lib_interface){
	    goto error;
	}

	sem_init(&semt_wait, 0, 0);
    pthread_mutex_init(&mutex_lock, NULL);

	return 0;

error:;
    bdt_cleanup();
	return -1;
}

void check_return_status(bt_status_t status)
{
    if (status != BT_STATUS_SUCCESS)
    {
        bdt_log("HAL REQUEST FAILED status : %d (%s)", status, dump_bt_status(status));
    }
    else
    {
        bdt_log("HAL REQUEST SUCCESS");
    }
}

static void handle_nonsig_rx_data(hci_cmd_complete_t *p)
{
    uint8_t result,rssi;
    uint32_t pkt_cnt,pkt_err_cnt;
    uint32_t bit_cnt, bit_err_cnt;
    uint8_t *buf;

    bdt_log("%s", __FUNCTION__);
    bdt_log("opcode = 0x%X", p->opcode);
    bdt_log("param_len = 0x%X", p->param_len);

    if (p->param_len != 18) {
       status =  -1;
       return;
    }

    buf = p->p_param_buf;
    result = *buf;
    for(int i = 0;i<18;i++){
      bdt_log("buf[%d]=0x%X",i,buf[i]);
    }
    rssi = *(buf + 1);
    pkt_cnt = *(uint32_t *)(buf + 2);
    pkt_err_cnt = *(uint32_t *)(buf + 6);
    bit_cnt = *(uint32_t *)(buf + 10);
    bit_err_cnt = *(uint32_t *)(buf + 14);

	double per = 0;
     double ber = 0;

     if(pkt_cnt!= 0){
        per = (pkt_err_cnt * 100) / pkt_cnt;
     }
     if(bit_cnt!= 0){
        ber = (bit_err_cnt * 100) / bit_cnt;
     }

    bdt_log("ret:0x%X,rssi:0x%X,pkt_cnt:0x%X, pkt_err_cnt:0x%X,bit_cnt:0x%X,bit_err_cnt:0x%X",result, rssi, pkt_cnt, pkt_err_cnt,bit_cnt,bit_err_cnt);
    bdt_log("rssi:%d, per:%.2f%%, ber:%.2f%%",(int8_t)rssi, per, ber);
}

/*******************************************************************************
**
** Function         dut_vsc_cback
**
** Description     Callback invoked on completion of vendor specific command
**
** Returns          None
**
*******************************************************************************/
static void dut_vsc_cback(hci_cmd_complete_t *p )
{
    if(p->opcode == NONSIG_RX_GETDATA || p->opcode == NONSIG_LE_RX_GETDATA){
       handle_nonsig_rx_data(p);
	}
}

void bdt_enable(void)
{
    bdt_log("ENABLE BT");
	if (bt_enabled) {
        bdt_log("Bluetooth is already enabled");
        return;
    }

	pthread_mutex_lock(&mutex_lock);

    status = lib_interface->enable();

	bt_enabled = 1;
	pthread_mutex_unlock(&mutex_lock);

    check_return_status(status);
}

void bdt_disable(void)
{
    bdt_log("DISABLE BT");
    if (!bt_enabled) {
        bdt_log("Bluetooth is already disabled");
        return;
    }

	pthread_mutex_lock(&mutex_lock);
    lib_interface->disable();
    bt_enabled = 0;
	pthread_mutex_unlock(&mutex_lock);

	//bdt_cleanup();
	//callback_cleanup();

    check_return_status(status);
}

static int dut_mode_configure(uint8_t mode)
{
    bdt_log("dut_mode_configure mode=%d",mode);
	if(mode)
	    lib_interface->dut_mode_enable();
	else
	    lib_interface->dut_mode_disable();

	return BT_STATUS_SUCCESS;
}

static int set_nonsig_tx_testmode(uint16_t enable, uint16_t le, uint16_t pattern, uint16_t channel, uint16_t pac_type,uint16_t pac_len,uint16_t power_type,uint16_t power_value,uint16_t pac_cnt)
{
    uint16_t opcode;
    bdt_log("set_nonsig_tx_testmode");

    /* sanity check */
    /*if (interface_ready() == FALSE)
        return BT_STATUS_NOT_READY;*/

	bdt_log("enable  : %x",enable);
	bdt_log("le      : %x",le);

	bdt_log("pattern : %x",pattern);
	bdt_log("channel : %x",channel);
	bdt_log("pac_type: %x",pac_type);
	bdt_log("pac_len : %x",pac_len);
	bdt_log("power_type   : %x",power_type);
	bdt_log("power_value  : %x",power_value);
	bdt_log("pac_cnt      : %x",pac_cnt);

    if(enable){
        opcode = le ? NONSIG_LE_TX_ENABLE : NONSIG_TX_ENABLE;
    }else{
        opcode = le ? NONSIG_LE_TX_DISABLE : NONSIG_TX_DISABLE;
    }

    if(enable){
        uint8_t buf[11];
        memset(buf,0x0,sizeof(buf));

        buf[0] = (uint8_t)pattern;
        buf[1] = (uint8_t)channel;
        buf[2] = (uint8_t)(pac_type & 0x00FF);
        buf[3] = (uint8_t)((pac_type & 0xFF00) >> 8);
        buf[4] = (uint8_t)(pac_len & 0x00FF);
        buf[5] = (uint8_t)((pac_len & 0xFF00) >> 8);
        buf[6] = (uint8_t)power_type;
        buf[7] = (uint8_t)(power_value & 0x00FF);
        buf[8] = (uint8_t)((power_value & 0xFF00) >> 8);
        buf[9] = (uint8_t)(pac_cnt & 0x00FF);
        buf[10] = (uint8_t)((pac_cnt & 0xFF00) >> 8);

        bdt_log("send hci cmd, opcode = 0x%x",opcode);
        lib_interface->hci_cmd_send(opcode, sizeof(buf), buf, dut_vsc_cback);
    }else{/* disable */
        bdt_log("send hci cmd, opcode = 0x%X",opcode);
        lib_interface->hci_cmd_send(opcode, 0, NULL, dut_vsc_cback);
    }

    return BT_STATUS_SUCCESS;

}

int set_nonsig_rx_testmode(uint16_t enable, uint16_t le, uint16_t pattern, uint16_t channel,
					uint16_t pac_type,uint16_t rx_gain, bt_bdaddr_t addr)
{
    uint16_t opcode;
    bdt_log("set_nosig_rx_testmode");

    /* sanity check */
    /*if (interface_ready() == FALSE)
        return BT_STATUS_NOT_READY;*/


    bdt_log("enable  : %x",enable);
    bdt_log("le      : %x",le);

    bdt_log("pattern : %d",pattern);
    bdt_log("channel : %d",channel);
    bdt_log("pac_type: %d",pac_type);
    bdt_log("rx_gain : %d",rx_gain);
    bdt_log("addr    : %02x:%02x:%02x:%02x:%02x:%02x",
        addr.address[0],addr.address[1],addr.address[2],
        addr.address[3],addr.address[4],addr.address[5]);

    if(enable){
        opcode = le ? NONSIG_LE_RX_ENABLE : NONSIG_RX_ENABLE;
    }else{
        opcode = le ? NONSIG_LE_RX_DISABLE : NONSIG_RX_DISABLE;
    }

    if(enable){
        uint8_t buf[11];
        memset(buf,0x0,sizeof(buf));

        buf[0] = (uint8_t)pattern;
        buf[1] = (uint8_t)channel;
        buf[2] = (uint8_t)(pac_type & 0x00FF);
        buf[3] = (uint8_t)((pac_type & 0xFF00) >> 8);
        buf[4] = (uint8_t)rx_gain;
        buf[5] = addr.address[5];
        buf[6] = addr.address[4];
        buf[7] = addr.address[3];
        buf[8] = addr.address[2];
        buf[9] = addr.address[1];
        buf[10] = addr.address[0];
        lib_interface->hci_cmd_send(opcode, sizeof(buf), buf, dut_vsc_cback);
    }else{
        lib_interface->hci_cmd_send(opcode, 0, NULL, dut_vsc_cback);
    }

    return BT_STATUS_SUCCESS;
}

static int le_enhanced_transmitter_cmd(uint8_t channel, uint8_t length, uint8_t payload, uint8_t phy)
{
    uint8_t buf[4];

    bdt_log("%s channel: 0x%02x, length: 0x%02x, payload: 0x%02x, phy: 0x%02x", __func__, channel, length, payload, phy);
    buf[0] = channel;
    buf[1] = length;
    buf[2] = payload;
    buf[3] = phy;
    lib_interface->hci_cmd_send(HCI_BLE_ENHANCED_TRANSMITTER_TEST, sizeof(buf), buf, dut_vsc_cback);
    return BT_STATUS_SUCCESS;
}

static int le_enhanced_receiver_cmd(uint8_t channel, uint8_t phy, uint8_t modulation_index)
{
      uint8_t buf[3];

      bdt_log("%s channel: 0x%02x, phy: 0x%02x, modulation_index: 0x%02x", __func__, channel, phy, modulation_index);
      buf[0] = channel;
      buf[1] = phy;
      buf[2] = modulation_index;
      lib_interface->hci_cmd_send(HCI_BLE_ENHANCED_RECEIVER_TEST, sizeof(buf), buf, dut_vsc_cback);
      return 0;
}

static int get_nonsig_rx_data(uint16_t le)
{
    bdt_log("get_nonsig_rx_data LE=%d",le);
    uint16_t opcode;
    opcode = le ? NONSIG_LE_RX_GETDATA : NONSIG_RX_GETDATA;
    lib_interface->hci_cmd_send(opcode, 0, NULL, dut_vsc_cback);
    return BT_STATUS_SUCCESS;
}

static int le_test_end_cmd(void)
{
    bdt_log("%s", __func__);
    lib_interface->hci_cmd_send(HCI_BLE_TEST_END, 0, NULL, dut_vsc_cback);
    return BT_STATUS_SUCCESS;
}

void bdt_dut_mode_configure(char *p)
{
    int32_t mode = -1;

    bdt_log("BT DUT MODE CONFIGURE");
    if (!bt_enabled) {
        bdt_log("Bluetooth is not already enabled");
        return;
    }
    mode = get_signed_int(&p, mode);
    if ((mode != 0) && (mode != 1)) {
        bdt_log("Please specify mode: 1 to enter, 0 to exit");
        return;
    }
    status = dut_mode_configure(mode);

    check_return_status(status);
}

void bdt_set_nonsig_tx(char *p)
{
    int i;
    unsigned int buf[9];

    bdt_log("bdt_set_nonsig_tx");
    if (!bt_enabled) {
        bdt_log("Bluetooth is not already enabled");
        return;
    }

    bdt_log("%s\n",p);

    for(i=0 ; i<9 ; i++)
		buf[i] = get_hex(&p,-1);

    for(i=0 ; i<9 ; i++)
		bdt_log("buf[%d] = 0x%x",i,buf[i]);

    status = set_nonsig_tx_testmode(buf[0],buf[1],buf[2],buf[3],buf[4],buf[5],buf[6],buf[7],buf[8]);

    check_return_status(status);
}

void bdt_set_nonsig_rx(char *p)
{
    int i;
    unsigned int buf[9];
    bt_bdaddr_t addr;

    bdt_log("bdt_set_nonsig_tx");
    if (!bt_enabled) {
        bdt_log("Bluetooth is not already enabled");
        return;
    }

    BTD("%s\n",p);
    for(i=0 ; i<6 ; i++)
        buf[i] = get_hex(&p,-1);

    for(i=0 ; i<6 ; i++)
        BTD("buf[%d] = 0x%x",i,buf[i]);

    BTD("%s\n",p);
    get_bdaddr(p,&addr);

    BTD("mac[0] = %x",addr.address[0]);
    BTD("mac[1] = %x",addr.address[1]);
    BTD("mac[2] = %x",addr.address[2]);
    BTD("mac[3] = %x",addr.address[3]);
    BTD("mac[4] = %x",addr.address[4]);
    BTD("mac[5] = %x",addr.address[5]);

    status = set_nonsig_rx_testmode(buf[0],buf[1],buf[2],buf[3],buf[4],buf[5],addr);

    check_return_status(status);
}

void bdt_le_enhanced_transmitter_test(char *p)
{
    int i;
    unsigned int buf[9];

    bdt_log("bdt_le_enhanced_transmitter_test");
    if (!bt_enabled) {
        bdt_log("Bluetooth is not already enabled");
        return;
    }
    BTD("%s\n",p);
    for(i=0 ; i<4 ; i++)
        buf[i] = get_hex(&p,-1);

    for(i=0 ; i<4 ; i++)
        BTD("buf[%d] = 0x%x",i,buf[i]);


    status = le_enhanced_transmitter_cmd(buf[0],buf[1],buf[2],buf[3]);

    check_return_status(status);
}

void bdt_le_enhanced_receiver_test(char *p)
{
    int i;
    unsigned int buf[9];

	bdt_log("bdt_le_enhanced_receiver_test");
    if (!bt_enabled) {
        bdt_log("Bluetooth is not already enabled");
        return;
    }
    BTD("%s\n",p);
    for(i=0 ; i<3 ; i++)
        buf[i] = get_hex(&p,-1);

    for(i=0 ; i<3 ; i++)
	    BTD("buf[%d] = 0x%x",i,buf[i]);

	BTD("%s\n",p);

	status = le_enhanced_receiver_cmd(buf[0],buf[1],buf[2]);

    check_return_status(status);
}

void bdt_get_nonsig_rx_data(char *p)
{
    int32_t le = -1;

    le = get_signed_int(&p, le);
    bdt_log("bdt_get_nonsig_rx_data, LE = %d",le);

    if(le != 0 && le != 1) {
        bdt_log("invalid parameter");
        return;
    }

    if (!bt_enabled) {
        bdt_log("Bluetooth is not already enabled");
        return;
    }

    status = get_nonsig_rx_data(le);

    check_return_status(status);
}

void bdt_le_test_end_cmd(void)
{
     if (!bt_enabled) {
        bdt_log("Bluetooth is not already enabled");
        return;
    }
	status = le_test_end_cmd();

	check_return_status(status);
}

/*******************************************************************************
 ** Console commands
 *******************************************************************************/

void do_help(char *p)
{
    UNUSED(p);
    int i = 0;
    char line[128];
    int pos = 0;

    while (console_cmd_list[i].name != NULL)
    {
        pos = sprintf(line, "%s", (char*)console_cmd_list[i].name);
        bdt_log("%s %s\n", (char*)line, (char*)console_cmd_list[i].help);
        i++;
    }
}

void do_quit(char *p)
{
    UNUSED(p);
    bdt_shutdown();
}

/*******************************************************************
 *
 *  BT TEST  CONSOLE COMMANDS
 *
 *  Parses argument lists and passes to API test function
 *
*/
void do_enable(char *p)
{
    UNUSED(p);
    bdt_enable();
}

void do_disable(char *p)
{
    UNUSED(p);
    bdt_disable();
}

void do_dut_mode_configure(char *p)
{
    bdt_dut_mode_configure(p);
}

void do_set_nonsig_tx(char *p)
{
	bdt_set_nonsig_tx(p);
}

void do_set_nonsig_rx(char *p)
{
    bdt_set_nonsig_rx(p);
}

void do_le_enhanced_transmitter_test(char *p)
{
    bdt_le_enhanced_transmitter_test(p);
}

void do_le_enhanced_receiver_test(char *p)
{
    bdt_le_enhanced_receiver_test(p);
}

void do_get_nonsig_rx_data(char *p)
{
    bdt_get_nonsig_rx_data(p);
}

void do_le_test_end_cmd(char *p)
{
    UNUSED(p);
    bdt_le_test_end_cmd();
}

const t_cmd console_cmd_list[] =
{
     /*
     * INTERNAL
     */

    { "help", do_help, "lists all available console commands", 0 },
	{ "quit", do_quit, "", 0},

	/*
     * API CONSOLE COMMANDS
     */
	{ "enable", do_enable, ":: enables bluetooth", 0 },
	{ "disable", do_disable, ":: disables bluetooth", 0 },
	{ "dut_mode_configure", do_dut_mode_configure, ":: do_dut_mode_configure<mode> DUT mode - 1 to enter,0 to exit", 0 },
	{ "set_nonsig_tx", do_set_nonsig_tx, ":: send vendor specific command to start/stop nonsig tx test \n\t \
                      usage - set_nonsig_tx <enable><LE><pattern><channel><pac_type><pac_len><power_type><power_value><pac_cnt>", 0 },
	{ "set_nonsig_rx", do_set_nonsig_rx, ":: send vendor specific command to enter/stop set_nonsig_rx mode \n\t \
                      usage - set_nonsig_rx <enable><LE><pattern><channel><pac_type><rx_gain><bdaddr>", 0 },
	{ "le_enhanced_transmitter_test", do_le_enhanced_transmitter_test, ":: send vendor specific command to start le nonsig tx test \n\t \
                      usage - le_enhanced_transmitter_test <channel><pac_len><pattern><LE_PHY>", 0 },
	{ "le_enhanced_receiver_test", do_le_enhanced_receiver_test, ":: send vendor specific command to start le nonsig rx test \n\t \
                      usage - le_enhanced_receiver_test <channel><LE_PHY><Mod_Index> - 1 to Stable, -0 to Standard", 0 },
	{ "get_nonsig_rx_data", do_get_nonsig_rx_data, ":: do_get_nonsig_rx_data<LE> - 1 to le, 0 to classic", 0 },
    { "le_test_end_cmd", do_le_test_end_cmd, ":: ble test end cmd", 0 },
	    /* last entry */
    {NULL, NULL, "", 0},
};

/*
 * Main console command handler
*/

static void process_cmd(char *p, unsigned char is_job)
{
    char cmd[64];
    int i = 0;
    char *p_saved = p;

    get_str(&p, cmd);

    /* table commands */
    while (console_cmd_list[i].name != NULL)
    {
        if (is_cmd(console_cmd_list[i].name))
        {
            if (!is_job && console_cmd_list[i].is_job)
                create_cmdjob(p_saved);
            else
            {
                console_cmd_list[i].handler(p);
            }
            return;
        }
        i++;
    }
    bdt_log("%s : unknown command\n", p_saved);
    do_help(NULL);
}

int main (int argc, char * argv[])
{
    UNUSED(argc);
	UNUSED(argv);

	bdt_log("\n:::::::::::::::::::::::::::::::::::::::::::::::::::");
    bdt_log(":: bdt_unisoc test app starting");

	if (bdt_init() < 0){
		bdt_log("bt_init failed");
		exit(0);
	}

	while(!main_done)
    {
        char line[128];
		/* command prompt */
	    printf( ">" );
        fflush(stdout);

		fgets (line, 128, stdin);

		if(line[0] != '\0')
		{
		   /* remove linefeed */
            line[strlen(line)-1] = 0;

            process_cmd(line, 0);
            memset(line, '\0', 128);
		}
    }

	bdt_cleanup();

	bdt_log(":: Bluedroid test app terminating");
    return 0;
}
