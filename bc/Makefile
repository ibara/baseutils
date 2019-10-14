# baseutils bc Makefile

CC ?=		cc
CFLAGS ?=	-O2 -pipe
CFLAGS +=	-I../libopenbsd -include openbsd.h

LEX =	../lex/lex

LIBS =	../libopenbsd/libopenbsd.a

PREFIX ?=	/usr/local
MANDIR ?=	/usr/local/share/man

PROG =	bc
OBJS =	bc.o execute.o global.o load.o main.o number.o scan.o \
	storage.o util.o warranty.o

all: ${OBJS}
	${CC} ${LDFLAGS} -o ${PROG} ${OBJS} ${LIBS}

bc.h bc.c: bc.y
	../yacc/yacc -d bc.y
	../mv/mv y.tab.c bc.c
	../mv/mv y.tab.h bc.h

install:
	install -c -s -m 555 ${PROG} ${PREFIX}/bin/${PROG}
	install -c -m 444 ${PROG}.1 ${MANDIR}/man1

clean:
	rm -f ${PROG} ${OBJS} bc.c bc.h scan.c
