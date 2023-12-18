/*
 * \file        platform.h
 * \Descriptions:
 *      1. board difference
 * Copyright (c) 2012 Allwinner Technology. All Rights Reserved.
 *
 */
#ifndef SUN50IW10_H
#define SUN50IW10_H

#include <sys/utsname.h>
#include <stdio.h>
#include <linux/version.h>
#include <stdlib.h>
#include <string.h>


/* get Linux version */
int get_version() {

	struct utsname uts;
	int major, minor;
	int ret;

	if (uname(&uts) < 0) {
		printf("cannot get uname infomation!\n");
		return -1;
	}

	sscanf(uts.release, "%d.%d.", &major, &minor);

	if ((major > 5) || ((major == 5) && (minor > 4)))
		return 1;
	else
		return 0;

}


/*
* Control name.
* Before and including linux-5.4，our platform uses old audio driver.
* In new driver, some control name has changes.
*/
// #define DAC_SWAP		"DAC Swap"
// #define MIC1_GAIN_VOLUME	"MIC1 gain volume"
// #define MIC2_GAIN_VOLUME	"MIC2 gain volume"
// ??
// #define HPOUT_SWITCH		"HPOUT Switch"
// #define SPK_SWITCH		"SPK Switch"
// #define HPOUT_VOLUME		"HPOUT Volume"
// #define MIC1_SWITCH		"MIC1 Switch"
// #define MIC2_SWITCH		"MIC2 Switch"
// #define MAX_VOLUME		7
// #else
// #define HPOUT_SWITCH		"Headphone Switch"
// #define SPK_SWITCH		"HpSpeaker Switch"
// #define HPOUT_VOLUME		"Headphone Volume"
// #define MIC1_SWITCH		"ADCL Input MIC1 Boost Switch"
// #define MIC2_SWITCH		"ADCR Input MIC2 Boost Switch"
// #define MAX_VOLUME		0
// #endif

/*
* board's difference
*/

/* P25 */
#define CHANNEL		1
#define RATE		48000
#define BITS		16


#endif
