# baseutils yacc Makefile

CC ?=		cc
CFLAGS ?=	-O2 -pipe
CFLAGS +=	-I../libopenbsd -include openbsd.h -D_GNU_SOURCE

LIBS =	../libopenbsd/libopenbsd.a

PREFIX ?=	/usr/local
MANDIR ?=	/usr/local/share/man

PROG =	yacc
OBJS =	closure.o error.o lalr.o lr0.o main.o mkpar.o output.o reader.o \
	skeleton.o symtab.o verbose.o warshall.o

all: ${OBJS}
	${CC} ${LDFLAGS} -o ${PROG} ${OBJS} ${LIBS}

install:
	install -c -s -m 555 ${PROG} ${PREFIX}/bin/${PROG}
	install -c -m 555 yyfix.sh ${PREFIX}/bin/yyfix
	install -c -m 444 ${PROG}.1 ${MANDIR}/man1
	install -c -m 444 yyfix.1 ${MANDIR}/man1

clean:
	rm -f ${PROG} ${OBJS}
