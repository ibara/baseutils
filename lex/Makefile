# baseutils cat Makefile

CC ?=		cc
CFLAGS ?=	-O2 -pipe
CFLAGS +=	-I../libopenbsd -include openbsd.h -I. -DHAVE_CONFIG_H

LIBS =	../libopenbsd/libopenbsd.a

PREFIX ?=	/usr/local
MANDIR ?=	/usr/local/share/man

PROG =	lex
OBJS =	buf.o ccl.o dfa.o ecs.o filter.o gen.o main.o misc.o \
	nfa.o options.o parse.o regex.o scan.o scanflags.o \
	scanopt.o skel.o sym.o tables.o tables_shared.o \
	tblcmp.o yylex.o

all: ${OBJS}
	${CC} ${LDFLAGS} -o ${PROG} ${OBJS} ${LIBS}

parse.h parse.c: parse.y
	../yacc/yacc -d parse.y
	../mv/mv y.tab.c parse.c
	../mv/mv y.tab.h parse.h

install:
	install -d -m 755 ${PREFIX}/include
	install -c -m 444 FlexLexer.h ${PREFIX}/include
	install -c -s -m 555 ${PROG} ${PREFIX}/bin/${PROG}
	install -c -s -m 555 ${PROG} ${PREFIX}/bin/flex
	install -c -s -m 555 ${PROG} ${PREFIX}/bin/flex++
	install -c -m 444 flex.1 ${MANDIR}/man1

clean:
	rm -f ${PROG} ${OBJS} parse.c parse.h
