# baseutils libopenbsd Makefile

CC ?=		cc
CFLAGS ?=	-O2 -pipe
CFLAGS +=	-I.

LIB =	libopenbsd.a
OBJS =	arc4random.o basename.o dirname.o e_atan2.o e_exp.o e_fmod.o e_log.o e_log10.o e_pow.o e_rem_pio2.o e_sqrt.o errc.o fgetln.o \
	fmt_scaled.o fts.o getbsize.o getopt_long.o k_cos.o k_rem_pio2.o k_sin.o ldexp.o modf.o ohash.o pledge.o pwd.o \
	reallocarray.o recallocarray.o s_atan.o s_cos.o s_fabs.o s_floor.o s_scalbn.o s_sin.o setmode.o strlcat.o strlcpy.o \
	strmode.o strtonum.o unveil.o verrc.o vis.o vwarnc.o warnc.o

all: ${OBJS}
	${AR} cr ${LIB} ${OBJS}
	ranlib ${LIB}

clean:
	rm -f ${LIB} ${OBJS}
