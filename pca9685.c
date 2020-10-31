// SPDX-License-Identifier: GPL-2.0
/*
 * Hacked from https://github.com/Reinbert/pca9685
 * Copyright (c) 2014 Reinhard Sprung
 *
 * Copyright (c) 2020 Damien Le Moal.
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/ioctl.h>
#include <linux/types.h>

#include "pca9685.h"

#ifdef PCA_DEBUG
#define pca_dbg(fmt, ...)	printf(fmt, ##__VA_ARGS__)
#else
#define pca_dbg(fmt, ...)	{}
#endif

/**
 * Get channel register
 */
static __u8 pca9685_channel_reg(int ch)
{
	if (ch >= PCA9685_ALL_CHANNELS)
		return PCA9685_ALL_CHANNELS_ON_L;

	return PCA9685_CHANNEL0_ON_L + ch * 4;
}

static int pca9685_access(int fd, int read_write, __u8 reg,
			  int size, union i2c_smbus_data *data)
{
	struct i2c_smbus_ioctl_data args;
	int ret;

	args.read_write = read_write;
	args.command = reg;
	args.size = size;
	args.data = data;

	ret = ioctl(fd, I2C_SMBUS, &args);
	if (ret < 0) {
		fprintf(stderr, "I2C_SMBUS %s ioctl failed %d (%s)\n",
			read_write ? "read" : "write",
			errno, strerror(errno));
		return -1;
	}

	return 0;
}

static int pca9685_write8(struct pca9685 *pca, __u8 reg, __u8 val)
{
	union i2c_smbus_data data = { .byte = val };

	pca_dbg("pca9685_write8 reg 0x%02x, val 0x%02x\n",
		(unsigned int) reg,
		(unsigned int) val);

	return pca9685_access(pca->fd, I2C_SMBUS_WRITE, reg, 2, &data);
}

static int pca9685_read8(struct pca9685 *pca, __u8 reg)
{
	union i2c_smbus_data data;
	int ret;

	pca_dbg("pca9685_read8 reg 0x%02x\n",
		(unsigned int) reg);

	ret = pca9685_access(pca->fd, I2C_SMBUS_READ, reg, 2, &data);
	if (ret < 0)
		return ret;

	return data.byte & 0x00ff;
}

static int pca9685_write16(struct pca9685 *pca, __u8 reg, __u16 val)
{
	union i2c_smbus_data data = { .word = val };

	pca_dbg("pca9685_write16 reg 0x%02x, val 0x%04x\n",
		(unsigned int) reg,
		(unsigned int) val);

	return pca9685_access(pca->fd, I2C_SMBUS_WRITE, reg, 3, &data);
}

static int pca9685_read16(struct pca9685 *pca, __u8 reg)
{
	union i2c_smbus_data data;
	int ret;

	pca_dbg("pca9685_read16 reg 0x%02x\n",
		(unsigned int) reg);

	ret = pca9685_access(pca->fd, I2C_SMBUS_READ, reg, 2, &data);
	if (ret < 0)
		return ret;

	return data.word & 0x00ffff;
}

int pca9685_open(struct pca9685 *pca, int address, int freq)
{
	int fd, mode;

	pca_dbg("Opening device %s\n", pca->dev);
	fd = open(pca->dev, O_RDWR);
	if (fd < 0) {
		fprintf(stderr, "Open i2c adapter %s failed %d (%s)\n",
			pca->dev, errno, strerror(errno));
		return -1;
	}

	/* Set 7-bits address */
	if (ioctl(fd, I2C_TENBIT, 0) < 0) {
		fprintf(stderr, "Set 7-bits address failed %d (%s)\n",
			errno, strerror(errno));
		goto err;
	}

	/* Connect pca */
	pca_dbg("Set slave address 0x%x\n", address);
	if (ioctl(fd, I2C_SLAVE, address) < 0) {
		fprintf(stderr, "Connect to slave at 0x%x failed %d (%s)\n",
			address, errno, strerror(errno));
		goto err;
	}

	pca->fd = fd;

	/* Setup the chip: enable auto-increment of registers */
	mode = pca9685_read8(pca, PCA9685_MODE1);
	if (pca9685_write8(pca, PCA9685_MODE1, (mode | 0x20) & 0x7F))
		goto err;

	/*
	 * Set frequency of PWM signals. Also ends sleep mode and starts
	 * PWM output.
	 */
	pca9685_set_pwm_freq(pca, freq);

	return 0;

err:
	close(fd);
	return -1;
}

void pca9685_close(struct pca9685 *pca)
{
	if (pca->fd > 0)
		close(pca->fd);
	pca->fd = -1;
}

/**
 * Sets the frequency of PWM signals.
 * Frequency will be capped to range [40..1000] Hertz. Try 50 for servos.
 */
void pca9685_set_pwm_freq(struct pca9685 *pca, int freq)
{
	int prescale, sleep, wake, mode;

	if (!freq)
		return;

	if (freq > PCA9685_FREQ_MAX)
		freq = PCA9685_FREQ_MAX;
	else if (freq < PCA9685_FREQ_MIN)
		freq = PCA9685_FREQ_MIN;

	/*
	 * To set pwm frequency we have to set the prescale register.
	 * The formula is:
	 *    prescale = round(osc_clock / (4096 * frequency))) - 1
	 * where osc_clock = 25 MHz
	 * Further info here:
	 * http://www.nxp.com/documents/data_sheet/PCA9685.pdf Page 24
	 */
	prescale = (int)(25000000.0f / (4096 * freq) - 0.5f);

	mode = pca9685_read8(pca, PCA9685_MODE1);
	mode &= 0x7F;
	sleep = mode | 0x10;
	wake = mode & 0xEF;

	/* Go to sleep, set prescale and wake up again */
	pca9685_write8(pca, PCA9685_MODE1, sleep);
	pca9685_write8(pca, PCA9685_PRESCALE, prescale);
	pca9685_write8(pca, PCA9685_MODE1, wake);

	/*
	 * wait a millisecond for the oscillator to finish stabilizing
	 * and restart PWM.
	 */
	usleep(1000);
	pca9685_write8(pca, PCA9685_MODE1, wake | 0x80);
}

/**
 * Set all channels back to default values (full off)
 */
void pca9685_reset(struct pca9685 *pca)
{
	pca9685_write16(pca, PCA9685_ALL_CHANNELS_ON_L, 0x0000);
	pca9685_write16(pca, PCA9685_ALL_CHANNELS_ON_L + 2, 0x1000);
}

/**
 * Write on and off ticks manually to a channel
 * (Deactivates any full-on and full-off)
 */
void pca9685_set_pwm(struct pca9685 *pca, int ch, int on, int off)
{
	int reg = pca9685_channel_reg(ch);

	/*
	 * Write to on and off registers and mask the 12 lowest bits of data
	 * to overwrite full-on and off.
	 */
	pca9685_write16(pca, reg, on & 0x0FFF);
	pca9685_write16(pca, reg + 2, off & 0x0FFF);
}

/**
 * Reads both on and off registers as 16 bit of data
 * To get PWM: mask each value with 0x0FFF
 * To get full-on or off bit: mask with 0x1000
 */
void pca9685_get_pwm(struct pca9685 *pca, int ch, int *on, int *off)
{
	int reg = pca9685_channel_reg(ch);

	if (on)
		*on = pca9685_read16(pca, reg);
	if (off)
		*off = pca9685_read16(pca, reg + 2);
}

/**
 * Enable/disable full-on
 * tf = true: full-on
 * tf = false: according to PWM
 */
void pca9685_set_full_on(struct pca9685 *pca, int ch, int tf)
{
	int reg = pca9685_channel_reg(ch) + 1; // LEDX_ON_H
	int state = pca9685_read8(pca, reg);

	if (tf)
		state |= 0x10;
	else
		state &= 0xEF;

	pca9685_write8(pca, reg, state);

	/* Set full-off to 0 as it has priority over full-on */
	if (tf)
		pca9685_set_full_off(pca, ch, 0);
}

/**
 * Enable/disable full-off
 * tf = true: full-off
 * tf = false: according to PWM or full-on
 */
void pca9685_set_full_off(struct pca9685 *pca, int ch, int tf)
{
	int reg = pca9685_channel_reg(ch) + 3; // LEDX_OFF_H
	int state = pca9685_read8(pca, reg);

	if (tf)
		state |= 0x10;
	else
		state &= 0xEF;

	pca9685_write8(pca, reg, state);
}

/**
 * Simple PWM control which sets on-tick to 0 and off-tick to value.
 * If value is <= 0, full-off will be enabled
 * If value is >= 4096, full-on will be enabled
 * Every value in between enables PWM output
 */
void pca9685_pwm(struct pca9685 *pca, int ch, int len)
{
	if (len >= 4096) {
		pca9685_set_full_on(pca, ch, 1);
		return;
	}

	if (len <= 0) {
		pca9685_set_full_off(pca, ch, 1);
		return;
	}

	pca9685_set_pwm(pca, ch, 0, len);
}
