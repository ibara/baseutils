# baseutils libopenbsd Makefile

CC ?=		cc
CFLAGS ?=	-O2 -pipe
CFLAGS +=	-I.

LIB =	libopenbsd.a
OBJS =	arc4random.o basename.o errc.o fmt_scaled.o fts.o getbsize.o pledge.o \
	pwd.o reallocarray.o recallocarray.o setmode.o strlcat.o strlcpy.o \
	strmode.o strtonum.o verrc.o vis.o vwarnc.o warnc.o

all: ${OBJS}
	${AR} cr ${LIB} ${OBJS}
	ranlib ${LIB}

clean:
	rm -f ${LIB} ${OBJS}
