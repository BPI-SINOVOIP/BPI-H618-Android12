/*
 * (C) Copyright 2014-2019
 * allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * caiyongheng <caiyongheng@allwinnertech.com>
 *
 * some simple description for this code
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#include <common.h>
#include <errno.h>
#include <asm/io.h>
#include <mapmem.h>
#include <fdtdec.h>
#include <fdt_support.h>
#include <sys_config.h>
#include <asm/global_data.h>
#include <asm/gpio.h>
#include <asm/arch/dma.h>

#include "sun55iw3-codec.h"
#include "sunxi_rw_func.h"

#if 0
#define codec_dbg(fmt, arg...)	printf("[%s:%d] " fmt "\n", __FUNCTION__, __LINE__, ##arg)
#define sunxi_boot_codec_debug
#else
#define codec_dbg(fmt, arg...)
#endif

DECLARE_GLOBAL_DATA_PTR;

struct sunxi_codec {
	void __iomem *addr_clkbase;
	void __iomem *addr_clkdspbase;
	void __iomem *addr_dbase;
	void __iomem *addr_abase;
	user_gpio_set_t spk_cfg;
	u32 lineout_vol;
	u32 spk_gpio;
	u32 pa_ctl_level;
};

static struct sunxi_codec g_codec;

#if 0
static int sunxi_codec_read(struct sunxi_codec *codec, u32 reg)
{
	if (reg >= SUNXI_PR_CFG) {
		/* Analog part */
		reg = reg - SUNXI_PR_CFG;
		return read_prcm_wvalue(reg, codec->addr_abase);
	} else {
		return codec_rdreg(codec->addr_dbase + reg);
	}
	return 0;
}
#endif

static int sunxi_codec_write(struct sunxi_codec *codec, u32 reg, u32 val)
{
	codec_wrreg(codec->addr_dbase + reg, val);

	return 0;

}

static int sunxi_codec_update_bits(struct sunxi_codec *codec, u32 reg, u32 mask,
				   u32 val)
{
	codec_wrreg_bits(codec->addr_dbase + reg, mask, val);

	return 0;
}

static const struct sample_rate sample_rate_conv[] = {
	{44100, 0},
	{48000, 0},
	{8000, 5},
	{32000, 1},
	{22050, 2},
	{24000, 2},
	{16000, 3},
	{11025, 4},
	{12000, 4},
	{192000, 6},
	{96000, 7},
};

/*
 * sample bits
 * sample rate
 * channels
 *
 */

int sunxi_codec_hw_params(u32 sb, u32 sr, u32 ch)
{
	int i;
	struct sunxi_codec *codec = &g_codec;

	/*
	 * Audio codec
	 * SUNXI_DAC_FIFO_CTL		0xF0
	 * SUNXI_DAC_DPC		0x00
	 *
	 */
	/* set playback sample resolution, only little endian*/
	switch (sb) {
	case 16:
		sunxi_codec_update_bits(codec, SUNXI_DAC_FIFO_CTL,
					   0x3 << DAC_FIFO_MODE, 0x3 << DAC_FIFO_MODE);
		sunxi_codec_update_bits(codec, SUNXI_DAC_FIFO_CTL,
					   0x1 << TX_SAMPLE_BITS, 0x0 << TX_SAMPLE_BITS);
		break;
	case 24:
	case 32:
		sunxi_codec_update_bits(codec, SUNXI_DAC_FIFO_CTL,
				   0x3 << DAC_FIFO_MODE, 0x0 << DAC_FIFO_MODE);
		sunxi_codec_update_bits(codec, SUNXI_DAC_FIFO_CTL,
				   0x1 << TX_SAMPLE_BITS, 0x1 << TX_SAMPLE_BITS);
		break;
	default:
		return -1;
	}
	/* set playback sample rate */
	for (i = 0; i < ARRAY_SIZE(sample_rate_conv); i++) {
		if (sample_rate_conv[i].samplerate == sr) {
			sunxi_codec_update_bits(codec, SUNXI_DAC_FIFO_CTL, 0x7 << DAC_FS,
					   (sample_rate_conv[i].rate_bit << DAC_FS));
		}
	}
	/* set playback channels */
	if (ch == 1) {
		sunxi_codec_update_bits(codec, SUNXI_DAC_FIFO_CTL,
					(1 << DAC_MONO_EN), (1 << DAC_MONO_EN));
	} else if (ch == 2) {
		sunxi_codec_update_bits(codec, SUNXI_DAC_FIFO_CTL,
					(1 << DAC_MONO_EN), (0 << DAC_MONO_EN));
	} else {
		printf("channel:%u is not support!\n", ch);
	}

	return 0;
}

static int sunxi_codec_init(struct sunxi_codec *codec)
{
	void __iomem *cfg_reg;
	volatile u32 tmp_val = 0;

	/*
	 * CCMU register    24576000
	 * BASE				0x0200 1000
	 * PLL_PERI0_CTRL_REG       0x20
	 * PLL_AUDIO_CTRL_REG		0x78
	 * PLL_AUDIO_PAT0_CTRL_REG	0x178
	 * PLL_AUDIO_PAT1_CTRL_REG	0x17C
	 * PLL_AUDIO_BIAS_REG		0x378
	 * DSP_CLK_REG		        0xC70
	 */

	cfg_reg = codec->addr_clkbase + 0x20;
	tmp_val = 0xF8216310;
	writel(tmp_val, cfg_reg);

	cfg_reg = codec->addr_clkbase + 0x78;
	tmp_val = 0xE8145500; //0xF9015900;
	writel(tmp_val, cfg_reg);
	/*printf("reg:0x%08x, before: 0x%x, after:0x%x\n", cfg_reg, val, tmp_val);*/

	cfg_reg = codec->addr_clkbase + 0x178;
	tmp_val = 0xA000A234;
	writel(tmp_val, cfg_reg);

	cfg_reg = codec->addr_clkbase + 0x17C;
	tmp_val = 0x0;
	writel(tmp_val, cfg_reg);

	cfg_reg = codec->addr_clkbase + 0x378;
	tmp_val = 0x00030000;
	writel(tmp_val, cfg_reg);

	cfg_reg = codec->addr_clkbase + 0xC70;
	tmp_val = 0x83000001;
	writel(tmp_val, cfg_reg);

	codec_dbg("0x02001020 %x 0x02001078 %x 0x02001178 %x 0x02001C70 %x\n", \
				readl(codec->addr_clkbase + 0x20), readl(codec->addr_clkbase + 0x78), \
				readl(codec->addr_clkbase + 0x178), readl(codec->addr_clkbase + 0xC70));

	/*
	 * DSP CCMU register  22579200
	 * BASE				0x0710 2000
	 * PLL_AUDIO_CTRL_REG		0x0C
	 * PLL_AUDIO_PAT0_CTRL_REG	0x10
	 * PLL_AUDIO_PAT1_CTRL_REG	0x14
	 * PLL_AUDIO_BIAS_REG		0x18
	 * AUD_CLK_REG		        0x1C
	 * DSP_CLK_REG				0x20
	 * AUDIO_CODEC_DAC_CLK_REG	0x58
	 * AUDIO_CODEC_BGR_REG	    0x60
	 */
	cfg_reg = codec->addr_clkdspbase + 0x0C;
	tmp_val = 0xF8417F00;
	writel(tmp_val, cfg_reg);
	/*printf("reg:0x%08x, before: 0x%x, after:0x%x\n", cfg_reg, val, tmp_val);*/
	cfg_reg = codec->addr_clkdspbase + 0x10;
	tmp_val = 0x0;
	writel(tmp_val, cfg_reg);

	cfg_reg = codec->addr_clkdspbase + 0x14;
	tmp_val = 0x0;
	writel(tmp_val, cfg_reg);

	cfg_reg = codec->addr_clkdspbase + 0x18;
	tmp_val = 0x00030000;
	writel(tmp_val, cfg_reg);

	cfg_reg = codec->addr_clkdspbase + 0x1C;
	tmp_val = 0x0;
	writel(tmp_val, cfg_reg);

	cfg_reg = codec->addr_clkdspbase + 0x20;
	tmp_val = 0x83000003;
	writel(tmp_val, cfg_reg);

	cfg_reg = codec->addr_clkdspbase + 0x60;
	tmp_val = 0x00010000;
	writel(tmp_val, cfg_reg);
	tmp_val = 0x00010001;
	writel(tmp_val, cfg_reg);

	cfg_reg = codec->addr_clkdspbase + 0x58;
	tmp_val = 0x80000003; //0x80000163;  //0x8200001b;
	writel(tmp_val, cfg_reg);

	codec_dbg("0x0710200C %x 0x07102018 %x 0x07102058 %x 0x07102060 %x\n", \
				readl(codec->addr_clkdspbase + 0x0C), readl(codec->addr_clkdspbase + 0x18), \
				readl(codec->addr_clkdspbase + 0x58), readl(codec->addr_clkdspbase + 0x60));


	/*
	 * Audio codec
	 * SUNXI_DAC_DAP_CTL		0xF0
	 * SUNXI_DAC_DPC		    0x00
	 *
	 */
	/* Enable DAC DAP */
	sunxi_codec_update_bits(codec, SUNXI_DAC_DAP_CTL, 0x1 << DDAP_EN, 0x1 << DDAP_EN);

	/* cpvcc set 1.2V, analog power for headphone charge pump */
	sunxi_codec_update_bits(codec, SUNXI_DAC_AN_REG, 0x1 << CPLDO_EN, 0x1 << CPLDO_EN);
	sunxi_codec_update_bits(codec, SUNXI_DAC_AN_REG, 0x3 << CPLDO_VOLTAGE, 0x3 << CPLDO_VOLTAGE);
	/* Open VRP to remove noise */
	sunxi_codec_update_bits(codec, SUNXI_POWER_AN_CTL, 0x1 << VRP_LDO_EN, 0x1 << VRP_LDO_EN);

	/* set digital volume 0x0, 0*-1.16 = 0dB */
	sunxi_codec_update_bits(codec, SUNXI_DAC_DPC, (0x3F << DVOL), (0x0 << DVOL));
	/* set digital L/R volume */
	sunxi_codec_update_bits(codec, SUNXI_DAC_VOL_CTL, 0xFF << DAC_VOL_L, (0xC0 << DAC_VOL_L));
	sunxi_codec_update_bits(codec, SUNXI_DAC_VOL_CTL, 0xFF << DAC_VOL_R, (0xC0 << DAC_VOL_R));
	/* set LINEOUT volume, such as 0x19, -(31-0x19)*1.5 = -9dB */
	sunxi_codec_update_bits(codec, SUNXI_DAC_AN_REG, 0x1F << LINEOUT_GAIN, (codec->lineout_vol << LINEOUT_GAIN));
	/* set HPOUT volume 0-7 */
	sunxi_codec_update_bits(codec, SUNXI_DAC_AN_REG, 0x7 << HEADPHONE_GAIN, 0x7 << HEADPHONE_GAIN);

	return 0;
}

#if 1
fdt_addr_t new_fdtdec_get_addr_size_fixed(const void *blob, int node,
		const char *prop_name, int index, int na,
		int ns, fdt_size_t *sizep,
		bool translate)
{
	const fdt32_t *prop, *prop_end;
	const fdt32_t *prop_addr, *prop_size, *prop_after_size;
	int len;
	fdt_addr_t addr;

	prop = fdt_getprop(blob, node, prop_name, &len);
	if (!prop) {
		debug("(not found)\n");
		return FDT_ADDR_T_NONE;
	}
	prop_end = prop + (len / sizeof(*prop));

	prop_addr = prop + (index * (na + ns));
	prop_size = prop_addr + na;
	prop_after_size = prop_size + ns;
	if (prop_after_size > prop_end) {
		debug("(not enough data: expected >= %d cells, got %d cells)\n",
		      (u32)(prop_after_size - prop), ((u32)(prop_end - prop)));
		return FDT_ADDR_T_NONE;
	}

#if CONFIG_IS_ENABLED(OF_TRANSLATE)
	if (translate)
		addr = fdt_translate_address(blob, node, prop_addr);
	else
#endif
		addr = fdtdec_get_number(prop_addr, na);

	if (sizep) {
		*sizep = fdtdec_get_number(prop_size, ns);
		debug("addr=%08llx, size=%llx\n", (unsigned long long)addr,
		      (unsigned long long)*sizep);
	} else {
		debug("addr=%08llx\n", (unsigned long long)addr);
	}

	return addr;
}
#else
#define new_fdtdec_get_addr_size_fixed fdtdec_get_addr_size_fixed
#endif

static int get_sunxi_codec_values(struct sunxi_codec *codec, const void *blob)
{
	int node, ret;
	fdt_addr_t addr = 0;
	fdt_size_t size = 0;

	/* Get ccu clock (parent clock) */
	node = fdt_node_offset_by_compatible(blob, 0, "allwinner,clk-init");
	if (node < 0) {
		printf("unable to find allwinner,ccu node in device tree.\n");
		return node;
	}
	addr = new_fdtdec_get_addr_size_fixed(blob, node, "reg", 0, 2, 2, &size, false);
	codec->addr_clkbase = map_sysmem(addr, size);

	/* Get dsp ccu clock */
	addr = new_fdtdec_get_addr_size_fixed(blob, node, "reg", 3, 2, 2, &size, false);
	codec->addr_clkdspbase = map_sysmem(addr, size);

	codec_dbg("clkbase:0x%p, clkdspbase:0x%p\n", codec->addr_clkbase, codec->addr_clkdspbase);

	node = fdt_node_offset_by_compatible(blob, 0, "allwinner,sunxi-internal-codec");
	if (node < 0) {
		printf("unable to find allwinner,sunxi-snd-codec node in device tree.\n");
		return node;
	}
	if (!fdtdec_get_is_enabled(blob, node)) {
		printf("sunxi-snd-codec disabled in device tree.\n");
		return -1;
	}

	/*regs = fdtdec_get_addr();*/
	addr = new_fdtdec_get_addr_size_fixed(blob, node, "reg", 0, 2, 2, &size, false);
	codec->addr_dbase = map_sysmem(addr, size);

	addr = new_fdtdec_get_addr_size_fixed(blob, node, "reg", 1, 2, 2, &size, false);
	codec->addr_abase = map_sysmem(addr, size);

	codec_dbg("dbase:0x%p, abase:0x%p\n", codec->addr_dbase, codec->addr_abase);

	ret = fdt_getprop_u32(blob, node, "lineout_vol", &codec->lineout_vol);
	if (ret < 0) {
		printf("lineout_gain not set. default 0x1a\n");
		codec->lineout_vol = 0x1a;
	}
	ret = fdt_getprop_u32(blob, node, "pa_level", &codec->pa_ctl_level);
	if (ret < 0) {
		printf("pa_ctl_level not set. default 1\n");
		codec->pa_ctl_level = 1;
	}

	ret = fdt_get_one_gpio_by_offset(node, "gpio-spk", &codec->spk_cfg);
	if (ret < 0 || codec->spk_cfg.port == 0) {
		printf("parser gpio-spk failed, ret:%d, port:%u\n", ret, codec->spk_cfg.port);
		return -1;
	}
	codec->spk_gpio = (codec->spk_cfg.port - 1) * 32 + codec->spk_cfg.port_num;

	gpio_request(codec->spk_gpio, NULL);
#if 1
	codec_dbg("pa_ctl_level:%u, port:%u, num:%u, gpio:%u\n",
	       codec->pa_ctl_level, codec->spk_cfg.port,
	       codec->spk_cfg.port_num, codec->spk_gpio);
#endif

	return 0;
}

int sunxi_codec_playback_prepare(void)
{
	struct sunxi_codec *codec = &g_codec;

	/* RMC Enable */

	sunxi_codec_update_bits(codec, SUNXI_RAMP, 0x1 << 1, 0x1 << 1);


	/* FIFO flush, clear pending, clear sample counter */
	sunxi_codec_update_bits(codec, SUNXI_DAC_FIFO_CTL,
			   0x1 << DAC_FIFO_FLUSH, 0x1 << DAC_FIFO_FLUSH);
	sunxi_codec_write(codec, SUNXI_DAC_FIFO_STA,
			 0x1 << DAC_TXE_INT | 0x1 << DAC_TXU_INT | 0x1 << DAC_TXO_INT);
	sunxi_codec_write(codec, SUNXI_DAC_CNT, 0x0);

	/* enable FIFO empty DRQ */
	sunxi_codec_update_bits(codec, SUNXI_DAC_FIFO_CTL,
				(1 << DAC_DRQ_EN), (1 << DAC_DRQ_EN));

	/* Enable DAC analog left/Right channel */
	sunxi_codec_update_bits(codec, SUNXI_DAC_AN_REG,
				(0x1 << DACL_EN), (0x1 << DACL_EN));
	sunxi_codec_update_bits(codec, SUNXI_DAC_AN_REG,
				(0x1 << DACR_EN), (0x1 << DACR_EN));

	/* enable DAC digital part */
	sunxi_codec_update_bits(codec, SUNXI_DAC_DPC,
				(0x1 << DAC_DIG_EN), (0x1 << DAC_DIG_EN));

	/* enable left and right LINEOUT */
	sunxi_codec_update_bits(codec, SUNXI_DAC_AN_REG, 0x1 << LMUTE, 0x1 << LMUTE);
	sunxi_codec_update_bits(codec, SUNXI_DAC_AN_REG,
			   (0x1 << LINEOUTL_EN), (0x1 << LINEOUTL_EN));

	sunxi_codec_update_bits(codec, SUNXI_DAC_AN_REG, 0x1 << RMUTE, 0x1 << RMUTE);
	sunxi_codec_update_bits(codec, SUNXI_DAC_AN_REG,
			   (0x1 << LINEOUTR_EN), (0x1 << LINEOUTR_EN));

	/* enable HPOUT */
	sunxi_codec_update_bits(codec, SUNXI_HP_AN_CTL, 0x1 << HPPA_EN, 0x1 << HPPA_EN);

	/* increase delay time, if Pop occured */
	mdelay(10);
	gpio_direction_output(codec->spk_gpio, codec->pa_ctl_level);

	//sunxi_codec_write(codec, SUNXI_DAC_DEBUG, 0xB00);

	codec_dbg("0x07110000 [%x]  0x07110004 [%x] 0x07110010 [%x]  0x07110014 [%x] \n \
			    0x07110024 [%x]  0x07110028 [%x] 0x07110310 [%x] 0x07110320 [%x]  0x07110324 [%x] \n", \
			readl(codec->addr_dbase), readl(codec->addr_dbase + 0x04), readl(codec->addr_dbase + 0x10), readl(codec->addr_dbase + 0x14),\
			readl(codec->addr_dbase + 0x24), readl(codec->addr_dbase + 0x28), readl(codec->addr_dbase + 0x310), readl(codec->addr_dbase + 0x320), readl(codec->addr_dbase + 0x324));

	return 0;
}


int sunxi_codec_playback_debug(void)
{
#ifdef sunxi_boot_codec_debug
	struct sunxi_codec *codec = &g_codec;

	struct sunxi_gpio_reg *const gpio_reg = (struct sunxi_gpio_reg *)SUNXI_PIO_BASE;
	struct sunxi_gpio *pio = &gpio_reg->gpio_bank[7];
#endif

	codec_dbg("gpio7 [%p] cfg0 [%p] dat[%p] drv0[%p] pull0[%p]\n \
			0x2000150 [%x] 0x2000160 [%x] 0x2000164 [%x] 0x2000174 [%x] \n\n", \
			&gpio_reg->gpio_bank[7], &pio->cfg[0], &pio->dat, &pio->drv[0], &pio->pull[0], \
			readl(&pio->cfg[0]), readl(&pio->dat), readl(&pio->drv[0]), readl(&pio->pull[0]));

	int i = 0;

	for (i = 0; i < 5; i++) {
		/* ccu */
		codec_dbg("0x02001020 [%x] 0x02001078 [%x] 0x02001178 [%x] \n \
					0x02001378 [%x] 0x02001C70 [%x] \n\n", \
			readl(codec->addr_clkbase + 0x20), readl(codec->addr_clkbase + 0x78), readl(codec->addr_clkbase + 0x178), \
			readl(codec->addr_clkbase + 0x378), readl(codec->addr_clkbase + 0xC70));

		/* dsp ccu */

		codec_dbg("0x0710200C [%x] 0x07102020 [%x] 0x07102058 [%x] \n \
					0x07102060 [%x] 0x07102104 [%x] 0x0710211C [%x] \n\n", \
			readl(codec->addr_clkdspbase + 0x0C), readl(codec->addr_clkdspbase + 0x20), readl(codec->addr_clkdspbase + 0x58), \
			readl(codec->addr_clkdspbase + 0x60), readl(codec->addr_clkdspbase + 0x104), readl(codec->addr_clkdspbase + 0x11C));

		/* audiocodec */

		codec_dbg("0x07110000 [%x]  0x07110010 [%x]  0x07110014 [%x] 0x07110024 [%x]  0x0711031C [%x]\n",  \
			readl(codec->addr_dbase), readl(codec->addr_dbase + 0x10), readl(codec->addr_dbase + 0x14), readl(codec->addr_dbase + 0x24), \
			readl(codec->addr_dbase + 0x31C));

		mdelay(200);

	}
	return 0;
}

int sunxi_codec_probe(void)
{

	int ret;
	struct sunxi_codec *codec = &g_codec;
	const void *blob = gd->fdt_blob;

	codec_dbg("blob %p\n", blob);

	if (get_sunxi_codec_values(codec, blob) < 0) {
		debug("FDT Codec values failed\n");
		return -1;
	}

	ret = sunxi_codec_init(codec);

	return ret;
}

void sunxi_codec_fill_txfifo(u32 *data)
{
	struct sunxi_codec *codec = &g_codec;

	sunxi_codec_write(codec, SUNXI_DAC_TXDATA, *data);
}

int sunxi_codec_playback_start(ulong handle, u32 *srcBuf, u32 cnt)
{
	int ret = 0;
	struct sunxi_codec *codec = &g_codec;

	flush_cache((unsigned long)srcBuf, cnt);

#if 1
	codec_dbg("start dma: from 0x%p to 0x%p  total 0x%x(%u)byte\n",
	       srcBuf, codec->addr_dbase + SUNXI_DAC_TXDATA, cnt, cnt);
#endif
	/*ret = sunxi_dma_start(handle, (uint)srcBuf, codec->addr_dbase + SUNXI_DAC_TXDATA);*/
	ret = sunxi_dma_start(handle, (uint)srcBuf,
			      (uint)(codec->addr_dbase + SUNXI_DAC_TXDATA), cnt);

	return ret;
}
