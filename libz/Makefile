# baseutils libz Makefile

CC ?=		cc
CFLAGS ?=	-O2 -pipe
CFLAGS +=	-DHAVE_STRERROR -DHAVE_MEMCPY -DHAS_vsnprintf -DHAS_snprintf
CFLAGS +=	-I../libopenbsd -include openbsd.h

LIB =	libz.a
OBJS =	adler32.o compress.o crc32.o deflate.o gzio.o infback.o \
	inffast.o inflate.o inftrees.o trees.o uncompr.o zutil.o

all: ${OBJS}
	${AR} cr ${LIB} ${OBJS}
	ranlib ${LIB}

clean:
	rm -f ${LIB} ${OBJS}
