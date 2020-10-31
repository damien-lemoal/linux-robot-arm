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

#ifdef HAVE_LIBNCURSES
#include <curses.h>
#endif

#include <servo.h>

/*
 * Servos:
 * 0: base, + is left (view from back).
 * 1: shoulder: + is down
 * 2: elbow: + is up
 * 3: wrist: + is down
 * 4: rotation: + is right
 * 5: hand: + is close
 */

static void interactive(void)
{
#ifdef HAVE_LIBNCURSES
	int s = 0, d, k;

	initscr();
	raw();
	keypad(stdscr, TRUE);
	noecho();
	mvprintw(0, 0, "[Interactive mode (q to quit)]\n");

	while (1) {

		k = getch();
		if (k == 0xffffffff)
			continue;

		switch (k) {
		case 0x30:
		case 0x31:
		case 0x32:
		case 0x33:
		case 0x34:
		case 0x35:
			s = k - 0x30;
			move(1, 0);
			clrtoeol();
			mvprintw(1, 0, "Servo %d selected\n", s);
			continue;
		case 0x103:
			d = 1;
			break;
		case 0x102:
			d = -1;
			break;
		case 'q':
			return;
		default:
			continue;
		}

		d = serv_move_rel(s, d, 1);
		move(1, 0);
		clrtoeol();
		mvprintw(1, 0, "Servo %d: %d deg\n", s, d);
	}

	endwin();
#else
	printf("Interactive mode not supported\n");
#endif
}

static void run_demo(void)
{
	struct serv_vector v[MAX_SERVOS];

	/* Raise arm */
	memset(v, 0, sizeof(v));
	v[0].id = 1;
	v[0].pos = 60;

	v[1].id = 2;
	v[1].pos = 100;

	v[2].id = 3;
	v[2].pos = 90;

	serv_move_vector_abs(v, 3, 10);
	usleep(100000);

	/* Turn */
	serv_move_abs(0, 20, 10);
	usleep(100000);

	/* Lower arm while turning and opening grip */
	memset(v, 0, sizeof(v));
	v[0].id = 1;
	v[0].pos = 87;

	v[1].id = 2;
	v[1].pos = 120;

	v[2].id = 3;
	v[2].pos = 78;

	v[3].id = 4;
	v[3].pos = 150;
	v[3].step = 5;

	v[4].id = 5;
	v[4].pos = 75;

	serv_move_vector_abs(v, 5, 12);
	usleep(100000);

	/* Grab */
	serv_move_abs(5, 110, 5);
	usleep(250000);

	/* Raise arm while turning */
	memset(v, 0, sizeof(v));
	v[0].id = 0;
	v[0].pos = 90;

	v[1].id = 1;
	v[1].pos = 50;

	v[2].id = 2;
	v[2].pos = 95;

	v[3].id = 3;
	v[3].pos = 80;

	v[4].id = 4;
	v[4].pos = 84;
	v[4].step = 2;

	serv_move_vector_abs(v, 5, 10);
	usleep(3000000);

	/* Turn to release */
	serv_move_abs(0, 20, 10);
	usleep(100000);

	/* Lower arm */
	memset(v, 0, sizeof(v));
	v[0].id = 1;
	v[0].pos = 87;

	v[1].id = 2;
	v[1].pos = 120;

	v[2].id = 3;
	v[2].pos = 78;

	v[3].id = 4;
	v[3].pos = 150;
	v[3].step = 5;

	serv_move_vector_abs(v, 4, 12);
	usleep(100000);

	/* Release */
	serv_move_abs(5, 75, 5);
	usleep(250000);

	/* Raise arm */
	memset(v, 0, sizeof(v));
	v[0].id = 1;
	v[0].pos = 60;

	v[1].id = 2;
	v[1].pos = 100;

	serv_move_vector_abs(v, 2, 10);
	usleep(100000);

	/* Turn to init */
	memset(v, 0, sizeof(v));
	v[0].id = 0;
	v[0].pos = 80;

	v[1].id = 4;
	v[1].pos = 87;
	v[1].step = 3;
	serv_move_vector_abs(v, 2, 10);

	/* Go back to init */
	serv_move_to_init(10);
}

int main(int argc, char **argv)
{
	struct pca9685 pca;
	unsigned int i2c_addr = 0x40;
	int i, ret;
	char c[2];

	if (argc < 2) {
usage:
		fprintf(stderr, "Usage: %s [options] <i2c dev>\n", argv[0]);
		fprintf(stderr, "Options:\n");
		fprintf(stderr, "    -h: help\n");
		fprintf(stderr, "    -l: list i2c adapters\n");
		fprintf(stderr, "    -a <addr (HEX)>: i2c slave address\n");
		return 1;
	}

	for (i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "-h")) {
			goto usage;
		} else if (!strcmp(argv[i], "-l")) {
			system("i2cdetect -l");
			return 0;
		} else if (!strcmp(argv[i], "-a")) {
			i++;
			if (i == argc - 1)
				goto usage;
			i2c_addr = strtol(argv[i], NULL, 16);
		} else {
			break;
		}
	}

	if (i == argc)
		goto usage;

	/* Connect controller */
	pca.dev = argv[i];
	pca.fd = -1;

	printf("Connecting i2c servo controller at 0x%x, 50 Hz on %s...\n",
	       i2c_addr, pca.dev);

	ret = pca9685_open(&pca, i2c_addr, 50);
	if (ret)
		return 1;

	printf("Resetting controller...\n");
	pca9685_reset(&pca);
	usleep(100000);

	/* Set initial positions and min/max positions */
	serv_init(0, &pca, 0, 84, 5, 155);
	serv_init(1, &pca, 1, 76, 40, 100);
	serv_init(2, &pca, 2, 130, 57, 135);
	serv_init(3, &pca, 3, 110, 24, 120);
	serv_init(4, &pca, 4, 87, 22, 162);
	serv_init(5, &pca, 5, 100, 70, 115);

	printf("Moving to initial position...\n");
	serv_move_to_init(10);
	usleep(500000);

	while (1) {

		printf("\r                            ");
		printf("\rCommand ('h' for help): ");
		fflush(stdin);

		memset(c, 0, sizeof(c));
		if (!fgets(c, 2, stdin))
			break;

		switch (c[0]) {
		case 'h':
			printf("\r\nCommands:\n");
			printf("    h: Help\n");
			printf("    i: Interactive mode. Type servo number\n"
			       "       (0 to 5), then arrow up/down to move\n");
			printf("    r: Reset to initial position\n");
			printf("    d: Run demonstration\n");
			printf("    q: Quit\n");
			break;
		case 'i':
			printf("\r\nInteractive mode...\n");
			interactive();
			break;
		case 'r':
			serv_move_to_init(10);
			printf("\r\nReset position...\n");
			break;
		case 'd':
			printf("\r\nRunning demo...\n");
			run_demo();
			break;
		case 'q':
			printf("\r\nQuit\r\n");
			goto out;
		default:
			break;
		}
	}

out:
	serv_move_to_init(10);
	pca9685_close(&pca);

	return 0;
}
