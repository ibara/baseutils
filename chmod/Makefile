# baseutils chmod Makefile

CC ?=		cc
CFLAGS ?=	-O2 -pipe
CFLAGS +=	-I../libopenbsd -include openbsd.h

LIBS =	../libopenbsd/libopenbsd.a

PREFIX ?=	/usr/local
MANDIR ?=	/usr/local/share/man

PROG =	chmod
OBJS =	chmod.o

all: ${OBJS}
	${CC} ${LDFLAGS} -o ${PROG} ${OBJS} ${LIBS}

install:
	install -c -s -m 555 ${PROG} ${PREFIX}/bin/${PROG}
	install -c -s -m 555 ${PROG} ${PREFIX}/bin/chgrp
	install -c -s -m 555 ${PROG} ${PREFIX}/sbin/chown
	install -c -s -m 555 ${PROG} ${PREFIX}/sbin/chflags
	install -c -m 444 ${PROG}.1 ${MANDIR}/man1
	install -c -m 444 chgrp.1 ${MANDIR}/man1
	install -c -m 444 chown.8 ${MANDIR}/man8
	install -c -m 444 chflags.1 ${MANDIR}/man1

clean:
	rm -f ${PROG} ${OBJS}
