# baseutils rcs Makefile

CC ?=		cc
CFLAGS ?=	-O2 -pipe
CFLAGS +=	-I../libopenbsd -include openbsd.h -I. -D_GNU_SOURCE

LIBS =	../libopenbsd/libopenbsd.a

PREFIX ?=	/usr/local
MANDIR ?=	/usr/local/share/man

PROG =	rcs
OBJS =	ci.o co.o ident.o merge.o rcsclean.o rcsdiff.o rcsmerge.o rcsparse.o \
	rcsprog.o rlog.o rcsutil.o buf.o date.o diff.o diff3.o rcs.o rcsnum.o \
	rcstime.o worklist.o xmalloc.o

all: ${OBJS}
	${CC} ${LDFLAGS} -o ${PROG} ${OBJS} ${LIBS}

date.c: date.y
	../yacc/yacc date.y
	../mv/mv y.tab.c date.c

install:
	install -c -s -m 555 ${PROG} ${PREFIX}/bin/${PROG}
	install -c -s -m 555 ${PROG} ${PREFIX}/bin/ci
	install -c -s -m 555 ${PROG} ${PREFIX}/bin/co
	install -c -s -m 555 ${PROG} ${PREFIX}/bin/ident
	install -c -s -m 555 ${PROG} ${PREFIX}/bin/merge
	install -c -s -m 555 ${PROG} ${PREFIX}/bin/rcsclean
	install -c -s -m 555 ${PROG} ${PREFIX}/bin/rcsdiff
	install -c -s -m 555 ${PROG} ${PREFIX}/bin/rcsmerge
	install -c -s -m 555 ${PROG} ${PREFIX}/bin/rlog
	install -c -m 444 ${PROG}.1 ${MANDIR}/man1
	install -c -m 444 ci.1 ${MANDIR}/man1
	install -c -m 444 co.1 ${MANDIR}/man1
	install -c -m 444 ident.1 ${MANDIR}/man1
	install -c -m 444 merge.1 ${MANDIR}/man1
	install -c -m 444 rcs.1 ${MANDIR}/man1
	install -c -m 444 rcsclean.1 ${MANDIR}/man1
	install -c -m 444 rcsdiff.1 ${MANDIR}/man1
	install -c -m 444 rcsmerge.1 ${MANDIR}/man1
	install -c -m 444 rlog.1 ${MANDIR}/man1
	install -c -m 444 rcsfile.5 ${MANDIR}/man5

clean:
	rm -f ${PROG} ${OBJS} date.c
