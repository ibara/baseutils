# baseutils compree Makefile

CC ?=		cc
CFLAGS ?=	-O2 -pipe
CFLAGS +=	-I../libopenbsd -include openbsd.h -fcommon

LIBS =	../libz/libz.a ../libopenbsd/libopenbsd.a

PREFIX ?=	/usr/local
MANDIR ?=	/usr/local/share/man

PROG =	compress
OBJS =	main.o zopen.o gzopen.o nullopen.o

all: ${OBJS}
	${CC} ${LDFLAGS} -o ${PROG} ${OBJS} ${LIBS}

install:
	install -c -s -m 555 ${PROG} ${PREFIX}/bin/${PROG}
	install -c -s -m 555 ${PROG} ${PREFIX}/bin/uncompress
	install -c -s -m 555 ${PROG} ${PREFIX}/bin/zcat
	install -c -s -m 555 ${PROG} ${PREFIX}/bin/gzip
	install -c -s -m 555 ${PROG} ${PREFIX}/bin/gunzip
	install -c -s -m 555 ${PROG} ${PREFIX}/bin/gzcat
	install -c -m 555 zdiff ${PREFIX}/bin/zdiff
	install -c -m 555 zdiff ${PREFIX}/bin/zcmp
	install -c -m 555 zmore ${PREFIX}/bin/zmore
	install -c -m 555 zmore ${PREFIX}/bin/zless
	install -c -m 555 zforce ${PREFIX}/bin/zforce
	install -c -m 555 gzexe ${PREFIX}/bin/gzexe
	install -c -m 555 znew ${PREFIX}/bin/znew
	install -c -m 444 ${PROG}.1 gzexe.1 gzip.1 zdiff.1 zforce.1 zmore.1 znew.1 ${MANDIR}/man1

clean:
	rm -f ${PROG} ${OBJS}
