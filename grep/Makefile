# baseutils grep Makefile

CC ?=		cc
CFLAGS ?=	-O2 -pipe
CFLAGS +=	-I../libopenbsd -include openbsd.h -I../libz

LIBS =	../libz/libz.a ../libopenbsd/libopenbsd.a

PREFIX ?=	/usr/local
MANDIR ?=	/usr/local/share/man

PROG =	grep
OBJS =	binary.o file.o grep.o mmfile.o queue.o util.o

all: ${OBJS}
	${CC} ${LDFLAGS} -o ${PROG} ${OBJS} ${LIBS}

install:
	install -c -s -m 555 ${PROG} ${PREFIX}/bin/${PROG}
	install -c -s -m 555 ${PROG} ${PREFIX}/bin/egrep
	install -c -s -m 555 ${PROG} ${PREFIX}/bin/fgrep
	install -c -s -m 555 ${PROG} ${PREFIX}/bin/zgrep
	install -c -s -m 555 ${PROG} ${PREFIX}/bin/zegrep
	install -c -s -m 555 ${PROG} ${PREFIX}/bin/zfgrep
	install -c -m 444 ${PROG}.1 ${MANDIR}/man1

clean:
	rm -f ${PROG} ${OBJS}
