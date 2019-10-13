# baseutils pax Makefile

CC ?=		cc
CFLAGS ?=	-O2 -pipe
CFLAGS +=	-I../libopenbsd -include openbsd.h

LIBS =	../libopenbsd/libopenbsd.a

PREFIX ?=	/usr/local
MANDIR ?=	/usr/local/share/man

PROG =	pax
OBJS =	ar_io.o ar_subs.o buf_subs.o cpio.o file_subs.o ftree.o \
	gen_subs.o getoldopt.o options.o pat_rep.o pax.o sel_subs.o \
	tables.o tar.o tty_subs.o

all: ${OBJS}
	${CC} ${LDFLAGS} -o ${PROG} ${OBJS} ${LIBS}

install:
	install -c -s -m 555 ${PROG} ${PREFIX}/bin/${PROG}
	install -c -s -m 555 ${PROG} ${PREFIX}/bin/tar
	install -c -s -m 555 ${PROG} ${PREFIX}/bin/cpio
	install -c -m 444 ${PROG}.1 ${MANDIR}/man1
	install -c -m 444 tar.1 ${MANDIR}/man1
	install -c -m 444 cpio.1 ${MANDIR}/man1

clean:
	rm -f ${PROG} ${OBJS}
