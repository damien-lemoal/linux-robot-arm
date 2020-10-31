/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Hacked from https://github.com/Reinbert/pca9685
 * Copyright (c) 2014 Reinhard Sprung
 *
 * Copyright (c) 2020 Damien Le Moal
 */
#ifndef _PCA9685_H_
#define _PCA9685_H_

#include <linux/i2c.h>
#include <linux/i2c-dev.h>

/*
 * Setup registers.
 */
#define PCA9685_MODE1		0x0
#define PCA9685_PRESCALE	0xFE

#define PCA9685_FREQ_MIN	40
#define PCA9685_FREQ_MAX	1000

/*
 * Define first and all channels register offsets.
 * All other channel registers are calculcated from the first channel.
 */
#define PCA9685_CHANNEL0_ON_L		0x6
#define PCA9685_ALL_CHANNELS_ON_L	0xFA
#define PCA9685_ALL_CHANNELS		16

/*
 * Adapter descriptor.
 */
struct pca9685 {
	char *dev;
	int fd;
};

/*
 * Setup a pca9685 at the specific i2c address
 */
int pca9685_open(struct pca9685 *pca, int address, int freq);
void pca9685_close(struct pca9685 *pca);
void pca9685_set_pwm_freq(struct pca9685 *pca, int freq);
void pca9685_reset(struct pca9685 *pca);
void pca9685_set_pwm(struct pca9685 *pca, int pin, int on, int off);
void pca9685_get_pwm(struct pca9685 *pca, int pin, int *on, int *off);
void pca9685_set_full_on(struct pca9685 *pca, int pin, int tf);
void pca9685_set_full_off(struct pca9685 *pca, int pin, int tf);

void pca9685_pwm(struct pca9685 *pca, int pin, int value);

/**
 * Simple full-on and full-off control
 * If value is 0, full-off will be enabled
 * If value is not 0, full-on will be enabled
 */
static inline void pca9685_on(struct pca9685 *pca, int pin, int on)
{
	if (on)
		pca9685_set_full_on(pca, pin, 1);
	else
		pca9685_set_full_off(pca, pin, 1);
}

/**
 * Reads off registers as 16 bit of data
 * To get PWM: mask with 0xFFF
 * To get full-off bit: mask with 0x1000
 * Note: ALL_LED pin will always return 0
 */
static inline int pca9685_get_pwm_off(struct pca9685 *pca, int pin)
{
	int off;

	pca9685_get_pwm(pca, pin, NULL, &off);

	return off;
}

/**
 * Reads on registers as 16 bit of data
 * To get PWM: mask with 0xFFF
 * To get full-on bit: mask with 0x1000
 * Note: ALL_LED pin will always return 0
 */
static inline int pca9685_get_pwm_on(struct pca9685 *pca, int pin)
{
	int on;

	pca9685_get_pwm(pca, pin, &on, NULL);

	return on;
}

#endif
