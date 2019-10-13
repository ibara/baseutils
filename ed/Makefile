# baseutils ed Makefile

CC ?=		cc
CFLAGS ?=	-O2 -pipe
CFLAGS +=	-I../libopenbsd -include openbsd.h

LIBS =	../libopenbsd/libopenbsd.a

PREFIX ?=	/usr/local
MANDIR ?=	/usr/local/share/man

PROG =	ed
OBJS =	buf.o glbl.o io.o main.o re.o sub.o undo.o

all: ${OBJS}
	${CC} ${LDFLAGS} -o ${PROG} ${OBJS} ${LIBS}

install:
	install -c -s -m 555 ${PROG} ${PREFIX}/bin/${PROG}
	install -c -m 444 ${PROG}.1 ${MANDIR}/man1

clean:
	rm -f ${PROG} ${OBJS}
