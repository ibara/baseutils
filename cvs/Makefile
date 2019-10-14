# baseutils cvs Makefile

CC ?=		cc
CFLAGS ?=	-O2 -pipe
CFLAGS +=	-I../libopenbsd -include openbsd.h -I. -D_GNU_SOURCE

LIBS =	../libopenbsd/libopenbsd.a

PREFIX ?=	/usr/local
MANDIR ?=	/usr/local/share/man

PROG =	cvs
OBJS =	cvs.o add.o admin.o annotate.o atomicio.o commit.o config.o \
	checkout.o client.o buf.o cmd.o date.o diff.o diff3.o \
	diff_internals.o edit.o entries.o fatal.o file.o getlog.o hash.o \
	hash_func.o history.o log.o logmsg.o modules.o import.o init.o  \
	release.o remove.o repository.o rcs.o rcsnum.o rcsparse.o remote.o \
	root.o server.o status.o tag.o trigger.o worklist.o util.o update.o \
	version.o watch.o xmalloc.o

all: ${OBJS}
	${CC} ${LDFLAGS} -o ${PROG} ${OBJS} ${LIBS}

date.c: date.y
	../yacc/yacc date.y
	../mv/mv y.tab.c date.c

install:
	install -c -s -m 555 ${PROG} ${PREFIX}/bin/opencvs
	install -c -m 444 ${PROG}.1 ${MANDIR}/man1
	install -c -m 444 cvs.5 ${MANDIR}/man5
	install -c -m 444 cvsintro.7 ${MANDIR}/man7

clean:
	rm -f ${PROG} ${OBJS} date.c
