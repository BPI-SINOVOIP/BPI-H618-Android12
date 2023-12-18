/*
 * Copyright (C) 2019 Allwinner.
 * weidonghui <weidonghui@allwinnertech.com>
 *
 * SUNXI AXP  Driver
 *
 * SPDX-License-Identifier: GPL-2.0+
 */

#include <common.h>
#include <sunxi_power/bmu_axp81X.h>
#include <sunxi_power/axp.h>
#include <asm/arch/pmic_bus.h>

int bmu_axp81X_get_poweron_source(void);
int runtime_tick(void);

static int bmu_axp81X_probe(void)
{
	u8 pmu_chip_id;
	if (pmic_bus_init(AXP81X_DEVICE_ADDR, AXP81X_RUNTIME_ADDR)) {
		tick_printf("%s pmic_bus_init fail\n", __func__);
		return -1;
	}
	if (pmic_bus_read(AXP81X_RUNTIME_ADDR, AXP81X_VERSION, &pmu_chip_id)) {
		tick_printf("%s pmic_bus_read fail\n", __func__);
		return -1;
	}
	pmu_chip_id &= 0XCF;
	if (pmu_chip_id == AXP81X_CHIP_ID) {
		/*pmu type AXP803*/
		bmu_axp81X_get_poweron_source();
		tick_printf("PMU: AXP803\n");
		return 0;
	}
	return -1;

}

int bmu_axp81X_get_battery_probe(void)
{
	u8 reg_value;
	static int probe_count;

	if (!probe_count) {
		probe_count++;
		int old_time = runtime_tick();
		while ((runtime_tick() - old_time) < 2000) {
			if (pmic_bus_read(AXP81X_RUNTIME_ADDR, AXP81X_MODE_CHGSTATUS, &reg_value))
				return -1;
			if ((reg_value & (1<<4)) && (reg_value & (1<<5)))
				return 1;
		}
	}

	if (pmic_bus_read(AXP81X_RUNTIME_ADDR, AXP81X_MODE_CHGSTATUS, &reg_value))
		return -1;
	/*bit4 determines whether bit5 is valid*/
	/*bit5 determines whether bat is exist*/
	if ((reg_value & (1<<4)) && (reg_value & (1<<5)))
		return 1;

	return -1;
}

int bmu_axp81X_reset_capacity(void)
{
	u8 reg_value;
	if (pmic_bus_write(AXP81X_RUNTIME_ADDR, AXP81X_BAT_MAX_CAP1, 0x00))
		return -1;

	if (pmic_bus_write(AXP81X_RUNTIME_ADDR, AXP81X_BAT_MAX_CAP0, 0x00))
		return -1;

	if (pmic_bus_read(AXP81X_RUNTIME_ADDR, AXP81X_FUEL_GAUGE_CTL, &reg_value))
		return -1;

	reg_value &= 0x3F;

	if (pmic_bus_write(AXP81X_RUNTIME_ADDR, AXP81X_FUEL_GAUGE_CTL, reg_value))
		return -1;

	return 1;
}


int bmu_axp81X_get_poweron_source(void)
{
	static uchar reg_value;
	if (!reg_value) {
		if (pmic_bus_read(AXP81X_RUNTIME_ADDR, AXP81X_OTG_STATUS, &reg_value)) {
			return -1;
		}

		if (pmic_bus_write(AXP81X_RUNTIME_ADDR, AXP81X_OTG_STATUS, 0xff)) {
			return -1;
		}
	}

	if (reg_value & (1 << 0)) {
		return AXP_BOOT_SOURCE_BUTTON;
	} else if (reg_value & (1 << 1)) {
		if (bmu_axp81X_get_battery_probe() > -1)
			return AXP_BOOT_SOURCE_CHARGER;
		else
			return AXP_BOOT_SOURCE_VBUS_USB;
	} else if (reg_value & (1 << 2)) {
		return AXP_BOOT_SOURCE_BATTERY;
	}
	return -1;
}

int bmu_axp81X_get_axp_bus_exist(void)
{
	u8 reg_value;
	if (pmic_bus_read(AXP81X_RUNTIME_ADDR, AXP81X_STATUS, &reg_value)) {
		return -1;
	}
	if (reg_value & 0x10) {		//vbus exist
		return AXP_VBUS_EXIST;
	}
	if (reg_value & 0x40) {		//dc in exist
		return AXP_DCIN_EXIST;
	}
	return 0;

}

int bmu_axp81X_get_battery_vol(void)
{
	u8  reg_value_h, reg_value_l;
	int bat_vol, tmp_value;

	if (pmic_bus_read(AXP81X_RUNTIME_ADDR, AXP81X_BAT_AVERVOL_H8, &reg_value_h)) {
		return -1;
	}
	if (pmic_bus_read(AXP81X_RUNTIME_ADDR, AXP81X_BAT_AVERVOL_L4, &reg_value_l)) {
	    return -1;
	}
	tmp_value = (reg_value_h << 4) | reg_value_l;
	bat_vol = tmp_value * 1100 / 1000;

	return bat_vol;
}

int bmu_axp81X_get_battery_capacity(void)
{
	u8 reg_value;
	int old_time = runtime_tick();
	while ((runtime_tick() - old_time) < 2000) {
		if (pmic_bus_read(AXP81X_RUNTIME_ADDR, AXP81X_BAT_PERCEN_CAL,
				  &reg_value)) {
			return -1;
		}
		if (reg_value) {
			return reg_value&0x7f;
		}
	}
	return -1;
}


#if 0
int bmu_axp81X_set_coulombmeter_onoff(int onoff)
{
	u8 reg_value;

	if (pmic_bus_read(AXP81X_RUNTIME_ADDR, AXP81X_FUEL_GAUGE_CTL,
			  &reg_value)) {
		return -1;
	}
	if (!onoff)
		reg_value &= ~(0x01 << 3);
	else
		reg_value |= (0x01 << 3);

	if (pmic_bus_write(AXP81X_RUNTIME_ADDR, AXP81X_FUEL_GAUGE_CTL,
			   reg_value)) {
		return -1;
	}
	return 0;
}

int bmu_axp81X_set_vbus_current_limit(int current)
{
	u8 reg_value;
	if (pmic_bus_read(AXP81X_RUNTIME_ADDR, AXP81X_VBUS_CUR_SET, &reg_value)) {
		return -1;
	}
	reg_value &= 0xf8;

	if (current >= 2000) {
		/*limit to 2000mA */
		reg_value |= 0x05;
	} else if (current >= 1500) {
		/* limit to 1500mA */
		reg_value |= 0x04;
	} else if (current >= 1000) {
		/* limit to 1000mA */
		reg_value |= 0x03;
	} else if (current >= 900) {
		/*limit to 900mA */
		reg_value |= 0x02;
	} else if (current >= 500) {
		/*limit to 500mA */
		reg_value |= 0x01;
	} else if (current >= 100) {
		/*limit to 100mA */
		reg_value |= 0x0;
	} else
		reg_value |= 0x01;

	tick_printf("Input current:%d mA\n", current);
	if (pmic_bus_write(AXP81X_RUNTIME_ADDR, AXP81X_VBUS_CUR_SET, reg_value)) {
		return -1;
	}
	return 0;
}

int bmu_axp81X_get_vbus_current_limit(void)
{
	uchar reg_value;
	if (pmic_bus_read(AXP81X_RUNTIME_ADDR, AXP81X_VBUS_CUR_SET, &reg_value)) {
		return -1;
	}
	reg_value &= 0x07;
	if (reg_value == 0x05) {
		printf("limit to 2000mA \n");
		return 2000;
	} else if (reg_value == 0x04) {
		printf("limit to 1500mA \n");
		return 1500;
	} else if (reg_value == 0x03) {
		printf("limit to 1000mA \n");
		return 1000;
	} else if (reg_value == 0x02) {
		printf("limit to 900mA \n");
		return 900;
	} else if (reg_value == 0x01) {
		printf("limit to 500mA \n");
		return 500;
	} else if (reg_value == 0x00) {
		printf("limit to 100mA \n");
		return 100;
	} else {
		printf("do not limit current \n");
		return 0;
	}
}
int bmu_axp81X_set_charge_current_limit(int current)
{
	u8 reg_value;
	int step;

	if (pmic_bus_read(AXP81X_RUNTIME_ADDR, AXP81X_CHARGE1, &reg_value)) {
		return -1;
	}
	reg_value &= ~0x1f;
	if (current > 2000) {
		current = 2000;
	}
	if (current <= 200)
		step = current / 25;
	else
		step = (current / 100) + 6;

	reg_value |= (step & 0x1f);
	if (pmic_bus_write(AXP81X_RUNTIME_ADDR, AXP81X_CHARGE1, reg_value)) {
		return -1;
	}

	return 0;
}
#endif

unsigned char bmu_axp81X_get_reg_value(unsigned char reg_addr)
{
	u8 reg_value;
	if (pmic_bus_read(AXP81X_RUNTIME_ADDR, reg_addr, &reg_value)) {
		return -1;
	}
	return reg_value;
}

unsigned char bmu_axp81X_set_reg_value(unsigned char reg_addr, unsigned char reg_value)
{
	unsigned char reg;
	if (pmic_bus_write(AXP81X_RUNTIME_ADDR, reg_addr, reg_value)) {
		return -1;
	}
	if (pmic_bus_read(AXP81X_RUNTIME_ADDR, reg_addr, &reg)) {
		return -1;
	}
	return reg;
}

int bmu_axp81X_set_ntc_cur(int ntc_cur)
{
	unsigned char reg_value;
	if (pmic_bus_read(AXP81X_RUNTIME_ADDR, AXP81X_ADC_SPEED_TS, &reg_value)) {
			return -1;
	}
	reg_value &= 0xCF;

	if (ntc_cur < 40)
		reg_value |= 0x00;
	else if (ntc_cur < 60)
		reg_value |= 0x10;
	else if (ntc_cur < 80)
		reg_value |= 0x20;
	else
		reg_value |= 0x30;

	if (pmic_bus_write(AXP81X_RUNTIME_ADDR, AXP81X_ADC_SPEED_TS, reg_value)) {
			return -1;
	}

	return reg_value;
}

int bmu_axp81X_set_ntc_onff(int onoff, int ntc_cur)
{
	unsigned char reg_value;
	if (!onoff) {
		if (pmic_bus_read(AXP81X_RUNTIME_ADDR, AXP81X_ADC_EN, &reg_value))
			return -1;

		reg_value &= ~(1 << 0);
		if (pmic_bus_write(AXP81X_RUNTIME_ADDR, AXP81X_ADC_EN, reg_value))
			return -1;

		if (pmic_bus_read(AXP81X_RUNTIME_ADDR, AXP81X_ADC_SPEED_TS, &reg_value))
			return -1;

		reg_value |= (1 << 2);
		reg_value &= ~(3 << 0);
		if (pmic_bus_write(AXP81X_RUNTIME_ADDR, AXP81X_ADC_SPEED_TS, reg_value))
			return -1;
	} else {
		if (pmic_bus_read(AXP81X_RUNTIME_ADDR, AXP81X_ADC_EN, &reg_value))
			return -1;

		reg_value |= (1 << 0);
		if (pmic_bus_write(AXP81X_RUNTIME_ADDR, AXP81X_ADC_EN, reg_value))
			return -1;

		if (pmic_bus_read(AXP81X_RUNTIME_ADDR, AXP81X_ADC_SPEED_TS, &reg_value))
			return -1;

		reg_value &= ~(1 << 2);
		reg_value |= (3 << 0);
		if (pmic_bus_write(AXP81X_RUNTIME_ADDR, AXP81X_ADC_SPEED_TS, reg_value))
			return -1;

		bmu_axp81X_set_ntc_cur(ntc_cur);
	}
	return 0;
}

static int axp_vts_to_temp(int data, int param[16])
{
	int temp;

	if (data < param[15])
		return 800;
	else if (data <= param[14]) {
		temp = 700 + (param[14]-data)*100/
		(param[14]-param[15]);
	} else if (data <= param[13]) {
		temp = 600 + (param[13]-data)*100/
		(param[13]-param[14]);
	} else if (data <= param[12]) {
		temp = 550 + (param[12]-data)*50/
		(param[12]-param[13]);
	} else if (data <= param[11]) {
		temp = 500 + (param[11]-data)*50/
		(param[11]-param[12]);
	} else if (data <= param[10]) {
		temp = 450 + (param[10]-data)*50/
		(param[10]-param[11]);
	} else if (data <= param[9]) {
		temp = 400 + (param[9]-data)*50/
		(param[9]-param[10]);
	} else if (data <= param[8]) {
		temp = 300 + (param[8]-data)*100/
		(param[8]-param[9]);
	} else if (data <= param[7]) {
		temp = 200 + (param[7]-data)*100/
		(param[7]-param[8]);
	} else if (data <= param[6]) {
		temp = 100 + (param[6]-data)*100/
		(param[6]-param[7]);
	} else if (data <= param[5]) {
		temp = 50 + (param[5]-data)*50/
		(param[5]-param[6]);
	} else if (data <= param[4]) {
		temp = 0 + (param[4]-data)*50/
		(param[4]-param[5]);
	} else if (data <= param[3]) {
		temp = -50 + (param[3]-data)*50/
		(param[3] - param[4]);
	} else if (data <= param[2]) {
		temp = -100 + (param[2]-data)*50/
		(param[2] - param[3]);
	} else if (data <= param[1]) {
		temp = -150 + (param[1]-data)*50/
		(param[1] - param[2]);
	} else if (data <= param[0]) {
		temp = -250 + (param[0]-data)*100/
		(param[0] - param[1]);
	} else
		temp = -250;
	return temp;
}

int bmu_axp81X_get_ntc_temp(int param[16])
{
	unsigned char reg_value[2];
	int temp, tmp;

	if (pmic_bus_read(AXP81X_RUNTIME_ADDR, AXP803_VTS_RES_H, &reg_value[0])) {
			return -1;
		}
	if (pmic_bus_read(AXP81X_RUNTIME_ADDR, AXP803_VTS_RES_L, &reg_value[1])) {
			return -1;
	}

	temp = (((reg_value[0] & GENMASK(4, 0)) << 0x04) | ((reg_value[1]) & 0x0F));
	tmp = temp * 800 / 1000;
	temp = axp_vts_to_temp(tmp, (int *)param);

	return temp;
}

U_BOOT_AXP_BMU_INIT(bmu_axp81X) = {
	.bmu_name		  = "bmu_axp81X",
	.probe			  = bmu_axp81X_probe,
	/*set_power_off		  = bmu_axp81X_set_power_off,*/
	.get_poweron_source       = bmu_axp81X_get_poweron_source,
	.get_axp_bus_exist	= bmu_axp81X_get_axp_bus_exist,
	/*.set_coulombmeter_onoff   = bmu_axp81X_set_coulombmeter_onoff,*/
	.get_battery_vol	  = bmu_axp81X_get_battery_vol,
	.get_battery_probe	= bmu_axp81X_get_battery_probe,
	.get_battery_capacity     = bmu_axp81X_get_battery_capacity,
	/*.set_vbus_current_limit   = bmu_axp81X_set_vbus_current_limit,
	.get_vbus_current_limit   = bmu_axp81X_get_vbus_current_limit,
	.set_charge_current_limit = bmu_axp81X_set_charge_current_limit,*/
	.get_reg_value	   = bmu_axp81X_get_reg_value,
	.set_reg_value	   = bmu_axp81X_set_reg_value,
	.reset_capacity	   = bmu_axp81X_reset_capacity,
	.set_ntc_onoff     = bmu_axp81X_set_ntc_onff,
	.get_ntc_temp      = bmu_axp81X_get_ntc_temp,
};

