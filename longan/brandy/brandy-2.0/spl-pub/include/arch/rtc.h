/*
 * (C) Copyright 2018-2020
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * wangwei <wangwei@allwinnertech.com>
 */

#ifndef _SUNXI_RTC_H
#define _SUNXI_RTC_H

void rtc_write_data(int index, u32 val);
u32  rtc_read_data(int index);
void rtc_set_fel_flag(void);
u32  rtc_probe_fel_flag(void);
void rtc_clear_fel_flag(void);
void rtc_set_hash_entry(phys_addr_t entry);
void rtc_clear_data(int index);
void rtc_set_data(int index, u32 val);

#ifdef CFG_BOOT0_WIRTE_RTC_TO_ISP
void rtc_set_isp_flag(void);
void rtc_clear_isp_flag(void);
#endif

#endif /* _SUNXI_RTC_H */
