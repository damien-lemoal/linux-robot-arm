/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2020 Damien Le Moal.
 */
#ifndef _SERVO_H_
#define _SERVO_H_

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

#include <pca9685.h>

#define MAX_SERVOS	6

struct servo {
	struct pca9685 *pca;
	int ch;
	int pwm_min;
	int pwm_max;
	int min_pos;
	int max_pos;
	int init_pos;
	int pos;
};

struct serv_vector {
	int id;
	int pos; /* Absolute target pos */
	int step; /* Increment per step (default 1) */
	int deg; /* Relative target pos */
	struct servo *s;
};

void serv_init(int id, struct pca9685 *pca, int ch,
	       int init_pos, int min_pos, int max_pos);
void serv_move_init(int speed);

int serv_move_abs(int id, int pos, int speed);
int serv_move_rel(int id, int deg, int speed);

void serv_move_vector_abs(struct serv_vector *v, int nvec, int speed);
void serv_move_vector_rel(struct serv_vector *v, int nvec, int speed);

void serv_move_to_init(int speed);

#endif /* _SERVO_H_ */
