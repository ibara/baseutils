# baseutils indent Makefile

CC ?=		cc
CFLAGS ?=	-O2 -pipe
CFLAGS +=	-I../libopenbsd -include openbsd.h -fcommon

LIBS =	../libopenbsd/libopenbsd.a

PREFIX ?=	/usr/local
MANDIR ?=	/usr/local/share/man

PROG =	indent
OBJS =	indent.o io.o lexi.o parse.o pr_comment.o args.o

all: ${OBJS}
	${CC} ${LDFLAGS} -o ${PROG} ${OBJS} ${LIBS}

install:
	install -c -s -m 555 ${PROG} ${PREFIX}/bin/${PROG}
	install -c -m 444 ${PROG}.1 ${MANDIR}/man1

clean:
	rm -f ${PROG} ${OBJS}
