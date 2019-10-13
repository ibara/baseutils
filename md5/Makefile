# baseutils md5 Makefile

CC ?=		cc
CFLAGS ?=	-O2 -pipe
CFLAGS +=	-I../libopenbsd -include openbsd.h -I../md5

LIBS =	../libopenbsd/libopenbsd.a

PREFIX ?=	/usr/local
MANDIR ?=	/usr/local/share/man

PROG =	md5
OBJS =	crc.o md5.o

all: ${OBJS}
	${CC} ${LDFLAGS} -o ${PROG} ${OBJS} ${LIBS}

install:
	install -c -s -m 555 ${PROG} ${PREFIX}/bin/${PROG}
	install -c -s -m 555 ${PROG} ${PREFIX}/bin/sha1
	install -c -s -m 555 ${PROG} ${PREFIX}/bin/sha256
	install -c -s -m 555 ${PROG} ${PREFIX}/bin/sha512
	install -c -s -m 555 ${PROG} ${PREFIX}/bin/cksum
	install -c -m 444 ${PROG}.1 ${MANDIR}/man1
	install -c -m 444 cksum.1 ${MANDIR}/man1

clean:
	rm -f ${PROG} ${OBJS}
