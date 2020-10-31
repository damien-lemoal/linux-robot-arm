// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2020 Damien Le Moal.
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

#include <servo.h>

/* MG996R */
#define PWM_MIN		80
#define PWM_MAX		580
#define US_PER_DEG	2300

static struct servo servos[MAX_SERVOS];

static inline int serv_deg2pwm(struct servo *s, int pos)
{
	return s->pwm_min + (s->pwm_max - s->pwm_min) * pos / 180;
}

static inline void serv_wait_deg(int speed)
{
	usleep(US_PER_DEG * speed);
}

static inline void serv_move(struct servo *s)
{
	pca9685_pwm(s->pca, s->ch, serv_deg2pwm(s, s->pos));
}

void serv_init(int id, struct pca9685 *pca, int ch,
	       int init_pos, int min_pos, int max_pos)
{
	struct servo *s = &servos[id];

	s->pca = pca;
	s->ch = ch;
	s->pwm_min = PWM_MIN;
	s->pwm_max = PWM_MAX;
	s->min_pos = min_pos;
	s->max_pos = max_pos;
	s->init_pos = init_pos;
	s->pos = init_pos;
	serv_move(s);
}

int serv_move_abs(int id, int pos, int speed)
{
	struct servo *s = &servos[id];
	int deg;

	if (pos < s->min_pos)
		pos = s->min_pos;
	else if (pos > s->max_pos)
		pos = s->max_pos;

	deg = pos - s->pos;

	while (s->pos != pos) {
		if (deg < 0)
			s->pos--;
		else
			s->pos++;
		serv_move(s);
		serv_wait_deg(speed);
	}

	return s->pos;
}

int serv_move_rel(int id, int deg, int speed)
{
	return serv_move_abs(id, servos[id].pos + deg, speed);
}

static void serv_move_vector(struct serv_vector *v, int nvec, int speed)
{
	struct servo *s;
	int i, m = nvec;

	while (m) {
		m = 0;
		for (i = 0; i < nvec; i++) {
			s = v[i].s;
			if (v[i].pos == s->pos)
				continue;
			if (v[i].deg < 0) {
				s->pos -= v[i].step;
				if (s->pos < v[i].pos)
					s->pos = v[i].pos;
			} else {
				s->pos += v[i].step;
				if (s->pos > v[i].pos)
					s->pos = v[i].pos;
			}
			serv_move(s);
			if (s->pos != v[i].pos)
				m++;
		}
		if (m)
			serv_wait_deg(speed);
	}
}

static void serv_init_vector(struct serv_vector *v)
{
	struct servo *s = &servos[v->id];

	v->s = s;
	if (v->pos < s->min_pos)
		v->pos = s->min_pos;
	else if (v->pos > s->max_pos)
		v->pos = s->max_pos;
	v->deg = v->pos - s->pos;
	if (!v->step)
		v->step = 1;
}

void serv_move_vector_abs(struct serv_vector *v, int nvec, int speed)
{
	int i;

	for (i = 0; i < nvec; i++)
		serv_init_vector(&v[i]);

	serv_move_vector(v, nvec, speed);
}

void serv_move_vector_rel(struct serv_vector *v, int nvec, int speed)
{
	int i;

	for (i = 0; i < nvec; i++) {
		v[i].pos = servos[v[i].id].pos + v[i].deg;
		serv_init_vector(&v[i]);
	}

	serv_move_vector(v, nvec, speed);
}

void serv_move_to_init(int speed)
{
	struct serv_vector v[MAX_SERVOS];
	int i;

	memset(v, 0, sizeof(v));

	/* Rotate base back to init */
	serv_move_abs(0, servos[0].init_pos, speed);

	for (i = 1; i < MAX_SERVOS; i++) {
		v[i].id = i;
		v[i].pos = servos[i].init_pos;
	}

	serv_move_vector_abs(&v[1], MAX_SERVOS - 1, speed);
}

