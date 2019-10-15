# baseutils bc Makefile

CC ?=		cc
CFLAGS ?=	-O2 -pipe
CFLAGS +=	-I../libopenbsd -include openbsd.h -I.
CFLAGS +=	-DBC_ENABLED -DDC_ENABLED -DVERSION=2.1.3
CFLAGS +=	-DEXECPREFIX= -DMAINEXEC=bc -DNDEBUG
CFLAGS +=	-DBC_NUM_KARATSUBA_LEN=64 -DBC_ENABLE_NLS=0
CFLAGS +=	-DBC_ENABLE_PROMPT=1 -DBC_ENABLE_EXTRA_MATH=1
CFLAGS +=	-DBC_ENABLE_SIGNALS=1 -DBC_ENABLE_HISTORY=1

LIBS =	../libopenbsd/libopenbsd.a

PREFIX ?=	/usr/local
MANDIR ?=	/usr/local/share/man

PROG =	bc
OBJS =	args.o bc.o bc_help.o bc_lex.o bc_parse.o data.o dc.o \
	dc_help.o dc_lex.o dc_parse.o history.o lang.o lex.o \
	lib.o lib2.o main.o num.o parse.o program.o read.o \
	vector.o vm.o

all: strgen bc_help.c dc_help.c lib.c lib2.c ${OBJS}
	${CC} ${LDFLAGS} -o ${PROG} ${OBJS} ${LIBS}

strgen:
	${CC} ${CFLAGS} ${LDFLAGS} -o strgen strgen.c
	./strgen bc_help.txt bc_help.c bc_help bc.h "" BC_ENABLED
	./strgen dc_help.txt dc_help.c dc_help dc.h "" DC_ENABLED
	./strgen lib.bc lib.c bc_lib bc.h bc_lib_name BC_ENABLED 1
	./strgen lib2.bc lib2.c bc_lib2 bc.h bc_lib2_name \
		"BC_ENABLED && BC_ENABLE_EXTRA_MATH" 1

install:
	install -c -s -m 555 ${PROG} ${PREFIX}/bin/${PROG}
	install -c -s -m 555 ${PROG} ${PREFIX}/bin/dc
	install -c -m 444 ${PROG}.1 ${MANDIR}/man1
	install -c -m 444 dc.1 ${MANDIR}/man1

clean:
	rm -f ${PROG} ${OBJS} strgen bc_help.c dc_help.c lib.c lib2.c
