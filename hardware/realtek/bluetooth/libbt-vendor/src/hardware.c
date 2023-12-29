/******************************************************************************
 *
 *  Copyright (C) 2009-2018 Realtek Corporation
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at:
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 ******************************************************************************/

/******************************************************************************
 *
 *  Filename:      hardware.c
 *
 *  Description:   Contains controller-specific functions, like
 *                      firmware patch download
 *                      low power mode operations
 *
 ******************************************************************************/

#define LOG_TAG "bt_hwcfg"
#define RTKBT_RELEASE_NAME "20230424_BT_ANDROID_12.0"

#include <utils/Log.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <time.h>
#include <errno.h>
#include <fcntl.h>
#include <dirent.h>
#include <ctype.h>
#include <cutils/properties.h>
#include <stdlib.h>
#include "bt_hci_bdroid.h"
#include "bt_vendor_rtk.h"
#include "userial.h"
#include "userial_vendor.h"
#include "upio.h"
#include <unistd.h>
#include <endian.h>
#include <byteswap.h>
#include <unistd.h>

#include "bt_vendor_lib.h"
#include "hardware.h"

/******************************************************************************
**  Constants &  Macros
******************************************************************************/

/******************************************************************************
**  Externs
******************************************************************************/

//void hw_config_cback(void *p_evt_buf);
//void hw_usb_config_cback(void *p_evt_buf);

extern uint8_t vnd_local_bd_addr[BD_ADDR_LEN];
extern bool rtkbt_auto_restart;

/******************************************************************************
**  Static variables
******************************************************************************/
//signature: realtech
static const uint8_t RTK_EPATCH_SIGNATURE[8]={0x52,0x65,0x61,0x6C,0x74,0x65,0x63,0x68};
//signature: rtbtcore
static const uint8_t RTK_EPATCH_SIGNATURE_V2[8]={0x52,0x54,0x42,0x54,0x43,0x6F,0x72,0x65};

/* fw Section opcode */
enum {
  FW_PATCH_SNIPPET = 1,
  FW_DUMMY_HEADER,
  FW_SECURITY_SIGNATURE,
  FW_OTA_FLAG
};


bt_hw_cfg_cb_t hw_cfg_cb;

/*
static bt_lpm_param_t lpm_param =
{
    LPM_SLEEP_MODE,
    LPM_IDLE_THRESHOLD,
    LPM_HC_IDLE_THRESHOLD,
    LPM_BT_WAKE_POLARITY,
    LPM_HOST_WAKE_POLARITY,
    LPM_ALLOW_HOST_SLEEP_DURING_SCO,
    LPM_COMBINE_SLEEP_MODE_AND_LPM,
    LPM_ENABLE_UART_TXD_TRI_STATE,*/
    //0,  /* not applicable */
   // 0,  /* not applicable */
   // 0,  /* not applicable */
    /*LPM_PULSED_HOST_WAKE
};*/

int getmacaddr(unsigned char * addr)
{
    int i = 0;
    char data[256], *str;
    int addr_fd;

    char property[100] = {0};
    if (property_get("persist.vendor.rtkbt.bdaddr_path", property, "none")) {
        if(strcmp(property, "none") == 0) {
			ALOGE("%s,persist.vendor.rtkbt.bdaddr_path = none", __func__);
            return -1;
        }
        else if(strcmp(property, "default") == 0) {
          memcpy(addr, vnd_local_bd_addr, BD_ADDR_LEN);
          return 0;

        }
        else if ((addr_fd = open(property, O_RDONLY)) != -1)
        {
            memset(data, 0, sizeof(data));
            int ret = read(addr_fd, data, 17);
            if(ret < 17) {
                ALOGE("%s, read length = %d", __func__, ret);
                close(addr_fd);
                return -1;
            }
            for (i = 0,str = data; i < 6; i++) {
                addr[5-i] = (unsigned char)strtoul(str, &str, 16);
                str++;
            }
            close(addr_fd);
            return 0;
        }
    }
	ALOGE("%s,return -1", __func__);
    return -1;
}

int rtk_get_bt_firmware(uint8_t** fw_buf, char* fw_short_name)
{
    char filename[PATH_MAX] = {0};
    struct stat st;
    int fd = -1;
    size_t fwsize = 0;
    size_t buf_size = 0;

    sprintf(filename, FIRMWARE_DIRECTORY, fw_short_name);
    ALOGI("BT fw file: %s", filename);

    if (stat(filename, &st) < 0)
    {
        ALOGE("Can't access firmware, errno:%d", errno);
        return -1;
    }

    fwsize = st.st_size;
    buf_size = fwsize;

    if ((fd = open(filename, O_RDONLY)) < 0)
    {
        ALOGE("Can't open firmware, errno:%d", errno);
        return -1;
    }

    if (!(*fw_buf = malloc(buf_size)))
    {
        ALOGE("Can't alloc memory for fw&config, errno:%d", errno);
        if (fd >= 0)
        close(fd);
        return -1;
    }

    if (read(fd, *fw_buf, fwsize) < (ssize_t) fwsize)
    {
        free(*fw_buf);
        *fw_buf = NULL;
        if (fd >= 0)
        close(fd);
        return -1;
    }

    if (fd >= 0)
        close(fd);

    ALOGI("Load FW OK");
    return buf_size;
}

uint8_t rtk_get_fw_parsing_rule(uint8_t *p_buf)
{
    uint8_t opcode;
    uint8_t len;
    uint8_t data = 1;

    do {
        opcode = *p_buf;
        len = *(p_buf - 1);
        if (opcode == 0x01)
        {
            if (len == 1)
            {
                data = *(p_buf - 2);
                ALOGI("rtk_get_fw_parsing_rule: opcode %d, len %d, data %d", opcode, len, data);
                break;
            }
            else
            {
                ALOGW("rtk_get_fw_parsing_rule: invalid len %d", len);
            }
        }
        p_buf -= len + 2;
    } while (*p_buf != 0xFF);

    return data;
}

uint8_t rtk_check_epatch_signature(bt_hw_cfg_cb_t* cfg_cb, uint8_t parsing_rule)
{
    if(parsing_rule == 1)
    {
        ALOGI("using legacy parsing rule(V1) ");
        if(cfg_cb->lmp_subversion == LMPSUBVERSION_8723a)
        {
            if(memcmp(cfg_cb->fw_buf, RTK_EPATCH_SIGNATURE, 8) == 0)
            {
                ALOGE("8723as check signature error!");
                cfg_cb->dl_fw_flag = 0;
                return -1;
            }
            else
            {
                cfg_cb->total_len = cfg_cb->fw_len + cfg_cb->config_len;
                if (!(cfg_cb->total_buf = malloc(cfg_cb->total_len)))
                {
                    ALOGE("can't alloc memory for fw&config, errno:%d", errno);
                    cfg_cb->dl_fw_flag = 0;
                    return -1;
                }
                else
                {
                    ALOGI("8723as, fw copy direct");
                    memcpy(cfg_cb->total_buf, cfg_cb->fw_buf, cfg_cb->fw_len);
                    memcpy(cfg_cb->total_buf+cfg_cb->fw_len, cfg_cb->config_buf, cfg_cb->config_len);
                    //cfg_cb->lmp_sub_current = *(uint16_t *)(cfg_cb->total_buf + cfg_cb->total_len - cfg_cb->config_len - 4);
                    cfg_cb->dl_fw_flag = 1;
                    return -1;
                }
            }
        }

        if (memcmp(cfg_cb->fw_buf, RTK_EPATCH_SIGNATURE, 8))
        {
            ALOGE("check signature error");
            cfg_cb->dl_fw_flag = 0;
            return -1;
        }

    }else if(parsing_rule == 2){
        ALOGI("using new parsing rule(V2) ");
        if (memcmp(cfg_cb->fw_buf, RTK_EPATCH_SIGNATURE_V2, 8))
        {
            ALOGE("check signature error");
            cfg_cb->dl_fw_flag = 0;
            return -1;
        }

    }else {
        ALOGE(" error parsing rule ");
        return -1;
    }

    return 0;
}

uint8_t rtk_get_fw_project_id(uint8_t *p_buf)
{
    uint8_t opcode;
    uint8_t len;
    uint8_t data = 0;

    do {
        opcode = *p_buf;
        len = *(p_buf - 1);
        if (opcode == 0x00)
        {
            if (len == 1)
            {
                data = *(p_buf - 2);
                ALOGI("bt_hw_parse_project_id: opcode %d, len %d, data %d", opcode, len, data);
                break;
            }
            else
            {
                ALOGW("bt_hw_parse_project_id: invalid len %d", len);
            }
        }
        p_buf -= len + 2;
    } while (*p_buf != 0xFF);

    return data;
}

uint8_t get_heartbeat_from_hardware()
{
    return hw_cfg_cb.heartbeat;
}

struct rtk_epatch_entry *rtk_get_patch_entry(bt_hw_cfg_cb_t *cfg_cb)
{
    uint16_t i;
    struct rtk_epatch *patch;
    struct rtk_epatch_entry *entry;
    uint8_t *p;
    uint16_t chip_id;

    patch = (struct rtk_epatch *)cfg_cb->fw_buf;
    entry = (struct rtk_epatch_entry *)malloc(sizeof(*entry));
    if(!entry)
    {
        ALOGE("rtk_get_patch_entry: failed to allocate mem for patch entry");
        return NULL;
    }

    patch->number_of_patch = le16_to_cpu(patch->number_of_patch);

    ALOGI("rtk_get_patch_entry: fw_ver 0x%08x, patch_num %d",
                le32_to_cpu(patch->fw_version), patch->number_of_patch);

    for (i = 0; i < patch->number_of_patch; i++)
    {
        p = cfg_cb->fw_buf + 14 + 2*i;
        STREAM_TO_UINT16(chip_id, p);
        if (chip_id == cfg_cb->eversion + 1)
        {
            entry->chip_id = chip_id;
            p = cfg_cb->fw_buf + 14 + 2*patch->number_of_patch + 2*i;
            STREAM_TO_UINT16(entry->patch_length, p);
            p = cfg_cb->fw_buf + 14 + 4*patch->number_of_patch + 4*i;
            STREAM_TO_UINT32(entry->patch_offset, p);
            ALOGI("rtk_get_patch_entry: chip_id %d, patch_len 0x%x, patch_offset 0x%x",
                    entry->chip_id, entry->patch_length, entry->patch_offset);
            break;
        }
    }

    if (i == patch->number_of_patch)
    {
        ALOGE("rtk_get_patch_entry: failed to get etnry");
        free(entry);
        entry = NULL;
    }

    return entry;
}

uint16_t rtk_get_v1_final_fw(bt_hw_cfg_cb_t* cfg_cb)
{
    uint16_t fw_patch_len = 0;
    struct rtk_epatch_entry* entry = NULL;
    struct rtk_epatch *patch = (struct rtk_epatch *)cfg_cb->fw_buf;
    entry = rtk_get_patch_entry(cfg_cb);
    if (entry)
    {
        cfg_cb->total_len = entry->patch_length + cfg_cb->config_len;
    }
    else
    {
        cfg_cb->dl_fw_flag = 0;
        return fw_patch_len;
    }

    ALOGI("total_len = 0x%x", cfg_cb->total_len);

    if (!(cfg_cb->total_buf = malloc(cfg_cb->total_len)))
    {
        ALOGE("Can't alloc memory for multi fw&config, errno:%d", errno);
        cfg_cb->dl_fw_flag = 0;
    }
    else
    {
        memcpy(cfg_cb->total_buf, cfg_cb->fw_buf + entry->patch_offset, entry->patch_length);
        memcpy(cfg_cb->total_buf + entry->patch_length - 4, &patch->fw_version, 4);
        memcpy(&entry->svn_version, cfg_cb->total_buf + entry->patch_length - 8, 4);
        memcpy(&entry->coex_version, cfg_cb->total_buf + entry->patch_length - 12, 4);
        fw_patch_len = entry->patch_length;

        ALOGI("BTCOEX:20%06d-%04x svn_version:%u lmp_subversion:0x%x hci_version:0x%x hci_revision:0x%x chip_type:%d Cut:%d libbt-vendor version:%s, patch->fw_version = %x\n",
        ((entry->coex_version >> 16) & 0x7ff) + ((entry->coex_version >> 27) * 10000),
        (entry->coex_version & 0xffff), entry->svn_version, cfg_cb->lmp_subversion, cfg_cb->hci_version, cfg_cb->hci_revision, cfg_cb->chip_type, cfg_cb->eversion+1, RTK_VERSION, patch->fw_version);
    }

    if(cfg_cb->lmp_subversion == LMPSUBVERSION_8723a)
    {
        cfg_cb->lmp_sub_current = 0;
    }
    else
    {
        cfg_cb->lmp_sub_current = (uint16_t)patch->fw_version;
    }

    if(entry)
    {
        free(entry);
    }

    return fw_patch_len;
}

uint8_t rtk_insert_fw_patch_fragment_to_linklist(struct rtk_epatch_fragment *fragment,
        struct rtk_epatch_fragment_linklist **header)
{
    struct rtk_epatch_fragment_linklist *p ;
    struct rtk_epatch_fragment_linklist *q ;
    struct rtk_epatch_fragment_linklist *tmp;
    tmp = (struct rtk_epatch_fragment_linklist *)malloc(sizeof(struct rtk_epatch_fragment_linklist));
    //ALOGI("rtk_insert_fw_patch_fragment_to_linklist ");
    if(!tmp)
    {
        ALOGE("rtk_insert_fw_patch_fragment_to_linklist: failed to allocate mem for patch entry");
        return -1;
    }

    tmp->fragment = fragment;
    tmp->next = NULL;
    if(*header == NULL){
        *header = tmp;
        return 0;
    }

    if(tmp->fragment->priority < (*header)->fragment->priority){
        tmp->next = *header;
        *header = tmp;
        return 0;
    }
    p = *header;
    q = p ->next;
    while(p){
         if(q) {
             if(tmp->fragment->priority < q->fragment->priority){
                p->next = tmp;
                tmp->next = q;
                break;
             }
             p = q;
             q = p ->next;
         }else{
             p->next = tmp;
             break;
         }
    }

    return 0;
}

uint32_t rtk_get_fw_patch_link_list(bt_hw_cfg_cb_t* cfg_cb,
                  struct rtk_epatch_fragment_linklist **linklist, uint16_t chip_id)
{
    uint8_t res;
    uint16_t i,j;
    struct rtk_epatch_v2 * patch;
    struct rtk_epatch_section * section;
    struct rtk_epatch_fragment * fragment;
    struct rtk_epatch_fragment_linklist * link_header = NULL;
    struct rtk_epatch_fragment_linklist * tmp;

    uint8_t *p, *q, *data;
    uint32_t fw_patch_len = 0;
    bool to_add = 0;
    uint32_t sec_sig_cnt = 0;
    uint8_t key_id = cfg_cb->keyid;

    patch = (struct rtk_epatch_v2 *)cfg_cb->fw_buf;

    patch->number_of_section = le32_to_cpu(patch->number_of_section);

    ALOGI("rtk_get_fw_patch_link_list: fw_ver 0x%08x,  fw_ver_sub 0x%08x, number_of_section %d",
        le32_to_cpu(patch->fw_version), le32_to_cpu(patch->fw_version_sub), patch->number_of_section);

    p = cfg_cb->fw_buf + 20;
   //Traversal section
    for(i = 0; i < patch->number_of_section; i++)
    {
        section = (struct rtk_epatch_section *) p;
        section->opcode = le32_to_cpu(section->opcode);
        section->length = le32_to_cpu(section->length);
        //q point to rtk_epatch_fragment
        q = p + 12;
        // p point to next rtk_epatch_section
        p = p + 8 + section->length;
        if(section->length == 0)
            continue;

        section->number_of_fragment = le16_to_cpu(section->number_of_fragment);
        ALOGI("rtk_get_fw_patch_link_list: opcode: %d,  length:%d, number_of_fragment: %d",
           section->opcode, section->length, section->number_of_fragment);

        //Traversal patch fragment
        for(j = 0; j < section->number_of_fragment; j++){
            if(section->opcode != FW_OTA_FLAG){
                fragment = (struct rtk_epatch_fragment *) q;
                fragment->length = le32_to_cpu(fragment->length);
                ALOGI("rtk_get_fw_patch_link_list: chip_id: %d,  priority:%d, length:  0x%x",
                   fragment->chip_id, fragment->priority, fragment->length);
            }
            switch(section->opcode){
               case FW_PATCH_SNIPPET:
                    if(fragment->chip_id == chip_id)
                        to_add = true;
                    q = q + 8 + fragment->length;
                    break;
               case FW_DUMMY_HEADER:
                    if((fragment->chip_id == chip_id) && (key_id == 0x00 || key_id == 0xff))
                        to_add = true;
                    q = q + 8 + fragment->length;
                    break;
               case FW_SECURITY_SIGNATURE:
                    if((fragment->chip_id == chip_id) && (fragment->key_id == key_id) ){
                        to_add = true;
                        sec_sig_cnt++;
                    }
                    q = q + 8 + fragment->length;
                    break;
               case FW_OTA_FLAG:
                    q = q + sizeof(struct rtk_epatch_ota);
                    break;
               default:
                    break;
            }
            if(to_add){
                res = rtk_insert_fw_patch_fragment_to_linklist(fragment, &link_header);
                if(res)
                   goto free_linklist;
                to_add = false;
                fw_patch_len += fragment->length;
                {
                    data = fragment->data;
                    ALOGI("fragment->data  %02x %02x %02x %02x %02x %02x %02x %02x",
                       *(data), *(data+1), *(data+2), *(data+3), *(data+4), *(data+5), *(data+6), *(data+7));
                }
            }
      }
    }

    if( (key_id != 0x00) && (key_id != 0xff) && (sec_sig_cnt == 0)){
       p = cfg_cb->fw_buf + 20;
       for(i = 0; i < patch->number_of_section; i++)
       {
           section = (struct rtk_epatch_section *) p;
           section->opcode = le32_to_cpu(section->opcode);
           section->length = le32_to_cpu(section->length);

           q = p + 12;
           p = p + 8 + section->length;
           if(section->length == 0)
               continue;

           section->number_of_fragment = le16_to_cpu(section->number_of_fragment);
           ALOGI("rtk_get_fw_patch_link_list 2: opcode: %d,  length:%d, number_of_fragment: %d",
              section->opcode, section->length, section->number_of_fragment);
           //Traversal patch fragment
           if(section->opcode == FW_DUMMY_HEADER){
               for(j = 0; j < section->number_of_fragment; j++){
                   fragment = (struct rtk_epatch_fragment *) q;
                   fragment->length = le32_to_cpu(fragment->length);
                   ALOGI("rtk_get_fw_patch_link_list: chip_id: %d, priority:%d, length:  0x%x",
                       fragment->chip_id, fragment->priority, fragment->length);

                   if(fragment->chip_id == chip_id){
                       res = rtk_insert_fw_patch_fragment_to_linklist(fragment, &link_header);
                   if(res)
                       goto free_linklist;
                   to_add = false;
                   fw_patch_len += fragment->length;
                   {
                       data = fragment->data;
                       ALOGI("fragment->data  %02x %02x %02x %02x %02x %02x %02x %02x",
                          *(data), *(data+1), *(data+2), *(data+3), *(data+4), *(data+5), *(data+6), *(data+7));
                   }
            }
            q = q + 8 + fragment->length;
           }
        }
    }
  }

    *linklist = link_header;
    return fw_patch_len;
free_linklist:
      while(link_header){
          tmp = link_header;
          link_header = link_header->next;
          tmp->fragment = NULL;
          tmp->next = NULL;
          free(tmp);
      }

    return -1;
}

uint32_t rtk_get_v2_final_fw(bt_hw_cfg_cb_t* cfg_cb)
{
    uint8_t *p, *data;
    uint32_t fw_patch_len = 0;
    uint32_t fw_version, svn_version, coex_version;
    uint16_t chip_id = cfg_cb->eversion + 1;
    struct rtk_epatch_fragment_linklist *fw_patch_link = NULL;
    struct rtk_epatch_fragment_linklist * tmp;

    fw_patch_len = rtk_get_fw_patch_link_list(cfg_cb, &fw_patch_link, chip_id);
    if(fw_patch_len > 0)
    {
        cfg_cb->total_len = fw_patch_len + cfg_cb->config_len;
    }
    else
    {
        cfg_cb->dl_fw_flag = 0;
        return fw_patch_len;
    }

    ALOGI("fw_patch_len = 0x%x, total_len = 0x%x", fw_patch_len, cfg_cb->total_len);

    if (!(cfg_cb->total_buf = malloc(cfg_cb->total_len)))
    {
        ALOGE("Can't alloc memory for multi fw&config, errno:%d", errno);
        cfg_cb->dl_fw_flag = 0;
        fw_patch_len = 0;
    }
    else
    {
        p = cfg_cb->total_buf;
        tmp = fw_patch_link;
        //fw_patch_len = 0;
        ALOGI("copy patch fragment to total_buf");
        while(tmp)
        {
            if(tmp->fragment)
            {
                ALOGI("fragment->priority= %d, fragment->length = 0x%x",
                               tmp->fragment->priority, tmp->fragment->length);

                memcpy(p, tmp->fragment->data, tmp->fragment->length);
                p += fw_patch_link->fragment->length;
                //fw_patch_len += fw_patch_link->fragment->length;

                {
                    data = tmp->fragment->data;
                    ALOGI("fragment->data  %02x %02x %02x %02x %02x %02x %02x %02x",
                       *(data), *(data+1), *(data+2), *(data+3), *(data+4), *(data+5), *(data+6), *(data+7));
                }
            }
            tmp = tmp->next;
        }
    }
/*
    {
        FILE *fp;
        int i;
        uint8_t *ch = cfg_cb->total_buf;
        if((fp=fopen("/data/vendor/bluetooth/fw_patch.txt","w+"))==NULL)
            printf("file cannot open \n");

        for(i=0; i < fw_patch_len; i++){
            if(i%16 == 15)
                fprintf(fp,"%02x\n", *ch);
            else
                fprintf(fp,"%02x ", *ch);

            ch++;
        }
        fclose(fp);
    }
*/
    memcpy(&fw_version, cfg_cb->total_buf + fw_patch_len - 4, 4);
    memcpy(&svn_version, cfg_cb->total_buf + fw_patch_len - 8, 4);
    memcpy(&coex_version, cfg_cb->total_buf + fw_patch_len - 12, 4);
    cfg_cb->lmp_sub_current = (uint16_t)fw_version;

    ALOGI("BTCOEX:20%06d-%04x svn_version:%u lmp_subversion:0x%x hci_version:0x%x hci_revision:0x%x chip_type:%d Cut:%d libbt-vendor version:%s, patch->fw_version = %x\n",
    ((coex_version >> 16) & 0x7ff) + ((coex_version >> 27) * 10000),
    (coex_version & 0xffff), svn_version, cfg_cb->lmp_subversion, cfg_cb->hci_version, cfg_cb->hci_revision, cfg_cb->chip_type, cfg_cb->eversion+1, RTK_VERSION, fw_version);

    while(fw_patch_link){
        tmp = fw_patch_link;
        fw_patch_link = fw_patch_link->next;
        free(tmp);
      }

    return fw_patch_len;
}

/******************************************************************************
**   LPM Static Functions
******************************************************************************/

/*******************************************************************************
**
** Function         hw_lpm_ctrl_cback
**
** Description      Callback function for lpm enable/disable rquest
**
** Returns          None
**
*******************************************************************************/
void hw_lpm_ctrl_cback(void *p_mem)
{
    HC_BT_HDR *p_evt_buf = (HC_BT_HDR *) p_mem;
    bt_vendor_op_result_t status = BT_VND_OP_RESULT_FAIL;

    if (*((uint8_t *)(p_evt_buf + 1) + HCI_EVT_CMD_CMPL_STATUS_OFFSET) == 0)
    {
        status = BT_VND_OP_RESULT_SUCCESS;
    }

    if (bt_vendor_cbacks)
    {
        bt_vendor_cbacks->lpm_cb(status);
        bt_vendor_cbacks->dealloc(p_evt_buf);
    }
}


#if (HW_END_WITH_HCI_RESET == TRUE)
/******************************************************************************
*
**
** Function         hw_epilog_cback
**
** Description      Callback function for Command Complete Events from HCI
**                  commands sent in epilog process.
**
** Returns          None
**
*******************************************************************************/
void hw_epilog_cback(void *p_mem)
{
    HC_BT_HDR   *p_evt_buf = (HC_BT_HDR *) p_mem;
    uint8_t     *p, status;
    uint16_t    opcode;

    status = *((uint8_t *)(p_evt_buf + 1) + HCI_EVT_CMD_CMPL_STATUS_OFFSET);
    p = (uint8_t *)(p_evt_buf + 1) + HCI_EVT_CMD_CMPL_OPCODE_OFFSET;
    STREAM_TO_UINT16(opcode,p);

    BTVNDDBG("%s Opcode:0x%04X Status: %d", __FUNCTION__, opcode, status);

    if (bt_vendor_cbacks)
    {
        /* Must free the RX event buffer */
        bt_vendor_cbacks->dealloc(p_evt_buf);

        /* Once epilog process is done, must call epilog_cb callback
           to notify caller */
        bt_vendor_cbacks->epilog_cb(BT_VND_OP_RESULT_SUCCESS);
    }
}

/******************************************************************************
*
**
** Function         hw_epilog_process
**
** Description      Sample implementation of epilog process
**
** Returns          None
**
*******************************************************************************/
void hw_epilog_process(void)
{
    HC_BT_HDR  *p_buf = NULL;
    uint8_t     *p;

    BTVNDDBG("hw_epilog_process");

    /* Sending a HCI_RESET */
    if (bt_vendor_cbacks)
    {
        /* Must allocate command buffer via HC's alloc API */
        p_buf = (HC_BT_HDR *) bt_vendor_cbacks->alloc(BT_HC_HDR_SIZE + \
                                                       HCI_CMD_PREAMBLE_SIZE);
    }

    if (p_buf)
    {
        p_buf->event = MSG_STACK_TO_HC_HCI_CMD;
        p_buf->offset = 0;
        p_buf->layer_specific = 0;
        p_buf->len = HCI_CMD_PREAMBLE_SIZE;

        p = (uint8_t *) (p_buf + 1);
        UINT16_TO_STREAM(p, HCI_RESET);
        *p = 0; /* parameter length */

        /* Send command via HC's xmit_cb API */
        bt_vendor_cbacks->xmit_cb(HCI_RESET, p_buf, hw_epilog_cback);
    }
    else
    {
        if (bt_vendor_cbacks)
        {
            ALOGE("vendor lib epilog process aborted [no buffer]");
            bt_vendor_cbacks->epilog_cb(BT_VND_OP_RESULT_FAIL);
        }
    }
}
#endif // (HW_END_WITH_HCI_RESET == TRUE)
