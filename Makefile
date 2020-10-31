#
# Copyright (c) 2020 Damien Le Moal.
#
TARGET   = demo

CCDEFS   = -D_GNU_SOURCE
# CCDEFS   += -DPCA_DEBUG
CCINC    = -I.
CCWARN   = -Wall -Wstrict-prototypes

CC = $(CROSS_COMPILE)gcc

ifeq ("$(CROSS_COMPILE)","")
CCDEFS   += -DHAVE_LIBNCURSES
CCOPTS   = -O2
LDFLAGS  = -O2 -lcurses
else
CCOPTS   = -O2 -fPIC
LDFLAGS  = -O2 -static -Wl,-elf2flt=-r
endif

CCFLAGS  = ${CCDEFS} ${CCINC} ${CCWARN} ${CCOPTS}

.SUFFIXES: .c

SRCS = $(shell ls *.c)
HDRS = $(shell ls *.h)
OBJS = $(SRCS:.c=.o)

all: ${TARGET}

${TARGET}: ${OBJS}
	@echo "Building $@"
	@${CC} -o $@ ${OBJS} ${LDFLAGS}

.c.o: ${HDRS}
	@echo "  CC $<"
	@${CC} ${CCFLAGS} -o $@ -c $<

clean:
	@-rm -f ${TARGET} *.o *.gdb

distclean: clean

