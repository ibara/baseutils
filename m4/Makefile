# baseutils cat Makefile

CC ?=		cc
CFLAGS ?=	-O2 -pipe
CFLAGS +=	-I../libopenbsd -include openbsd.h -DEXTENDED -I.

YACC =	../yacc/yacc

LIBS =	../libopenbsd/libopenbsd.a

PREFIX ?=	/usr/local
MANDIR ?=	/usr/local/share/man

PROG =	m4
OBJS =	eval.o expr.o look.o main.o misc.o gnum4.o parser.o trace.o tokenizer.o

all: ${OBJS}
	${CC} ${LDFLAGS} -o ${PROG} ${OBJS} ${LIBS}

parser.h parser.c: parser.y
	../yacc/yacc -d parser.y
	../mv/mv y.tab.c parser.c
	../mv/mv y.tab.h parser.h

install:
	install -c -s -m 555 ${PROG} ${PREFIX}/bin/${PROG}
	install -c -m 444 ${PROG}.1 ${MANDIR}/man1

clean:
	rm -f ${PROG} ${OBJS} parser.c parser.h
