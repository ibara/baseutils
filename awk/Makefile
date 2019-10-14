# baseutils awk Makefile

CC ?=		cc
CFLAGS ?=	-O2 -pipe
CFLAGS +=	-I../libopenbsd -include openbsd.h -D_GNU_SOURCE
CFLAGS +=	-I. -DHAS_ISBLANK -DNDEBUG

LIBS =	../libopenbsd/libopenbsd.a

PREFIX ?=	/usr/local
MANDIR ?=	/usr/local/share/man

PROG =	awk
OBJS =	ytab.o lex.o b.o main.o parse.o proctab.o tran.o lib.o run.o

all: ${OBJS}
	${CC} ${LDFLAGS} -o ${PROG} ${OBJS} ${LIBS}

ytab.c ytab.h: awkgram.y
	../yacc/yacc -d awkgram.y
	../mv/mv y.tab.c ytab.c
	../mv/mv y.tab.h ytab.h

proctab.c: maketab
	./maketab >proctab.c

maketab: ytab.h maketab.c
	${CC} ${CFLAGS} maketab.c -o maketab

install:
	install -c -s -m 555 ${PROG} ${PREFIX}/bin/${PROG}
	install -c -m 444 ${PROG}.1 ${MANDIR}/man1

clean:
	rm -f ${PROG} ${OBJS} proctab.c maketab ytab.c ytab.h
