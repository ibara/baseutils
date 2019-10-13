# baseutils libopenbsd Makefile

CC ?=		cc
CFLAGS ?=	-O2 -pipe

LIB =	libopenbsd.a
OBJS =	arc4random.o basename.o errc.o fmt_scaled.o getbsize.o pledge.o pwd.o \
	reallocarray.o setmode.o strlcat.o strlcpy.o strmode.o strtonum.o \
	verrc.o vis.o vwarnc.o warnc.o

all: ${OBJS}
	${AR} cr ${LIB} ${OBJS}
	ranlib ${LIB}

clean:
	rm -f ${LIB} ${OBJS}
