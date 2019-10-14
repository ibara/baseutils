/*
 * libopenbsd header file
 */

#include <sys/stat.h>
#include <sys/types.h>

#include <fcntl.h>
#include <fts.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#ifdef __aarch64__
#define MACHINE		"aarch64"
#define MACHINE_ARCH	"aarch64"
#endif

#ifdef __arm__
#define MACHINE		"arm"
#define MACHINE_ARCH	"arm"
#endif

#ifdef __i386__
#define MACHINE		"i386"
#define MACHINE_ARCH	"i386"
#endif

#if defined(__powerpc__) || defined(__ppc__)
#define MACHINE		"powerpc"
#define MACHINE_ARCH	"powerpc"
#endif

#ifdef __x86_64__
#define MACHINE		"x86_64"
#define MACHINE_ARCH	"x86_64"
#endif

#ifndef __dead
#define __dead		__attribute__((__noreturn__))
#endif

#ifndef _PATH_DEFTAPE
#define _PATH_DEFTAPE "/dev/rst0"
#endif

#ifndef _PW_NAME_LEN
#define _PW_NAME_LEN	31
#endif

#ifndef ACCESSPERMS
#define ACCESSPERMS	0000777
#endif

#ifndef ALIGN
#define ALIGN(p) (((unsigned long)(p) + (sizeof(long) - 1)) &~(sizeof(long) - 1))
#endif

#ifndef ALIGNBYTES
#define ALIGNBYTES	(sizeof(long) - 1)
#endif

#ifndef ALLPERMS
#define ALLPERMS	0007777
#endif

#ifndef ARG_MAX
#define ARG_MAX		(256 * 1024)
#endif

#ifndef DEFFILEMODE
#define DEFFILEMODE	0000666
#endif

#ifndef EFTYPE
#define EFTYPE	EPERM
#endif

#ifndef FMT_SCALED_STRSIZE
#define FMT_SCALED_STRSIZE 7	/* minus sign, 4 digits, suffix, null byte */
#endif

#ifndef MAXBSIZE
#define MAXBSIZE 65536
#endif

#ifndef MAXNAMLEN
#define MAXNAMLEN	255
#endif

#ifndef MFSNAMELEN
#define MFSNAMELEN	16
#endif

#ifndef NL_TEXTMAX
#define NL_TEXTMAX	255
#endif

#ifndef REG_BASIC
#define REG_BASIC	0000
#endif

#ifndef REG_NOSPEC
#define REG_NOSPEC	0020
#endif

#ifndef REG_STARTEND
#define REG_STARTEND	00004
#endif

#ifndef S_ISTXT
#define S_ISTXT 0
#endif

#ifndef howmany
#define howmany(x, y)	(((x)+((y)-1))/(y))
#endif

#ifndef major
#define major(x)	(((unsigned)(x) >> 8) & 0xff)
#endif

#ifndef makedev
#define makedev(x, y)	((dev_t)((((x) & 0xff) << 8) | ((y) & 0xff) | (((y) & 0xffff00) << 8)))
#endif

#ifndef minor
#define minor(x)	((unsigned)((x) & 0xff) | (((x) & 0xffff0000) >> 8))
#endif

#ifndef srandom_deterministic
#define srandom_deterministic(x)	srandom(x)
#endif

#ifndef st_atimespec
#define st_atimespec	st_atim
#endif

#ifndef st_ctimespec
#define st_ctimespec	st_ctim
#endif

#ifndef st_mtimespec
#define st_mtimespec	st_mtim
#endif

#ifndef timespecclear
#define timespecclear(tsp)	(tsp)->tv_sec = (tsp)->tv_nsec = 0
#endif

#ifndef timespeccmp
#define timespeccmp(tsp, usp, cmp)					\
	(((tsp)->tv_sec == (usp)->tv_sec) ?				\
	    ((tsp)->tv_nsec cmp (usp)->tv_nsec) :			\
	    ((tsp)->tv_sec cmp (usp)->tv_sec))
#endif

#ifndef timespecisset
#define timespecisset(tsp)	((tsp)->tv_sec || (tsp)->tv_nsec)
#endif

#ifndef timespecsub
#define timespecsub(tsp, usp, vsp)					\
	do {								\
		(vsp)->tv_sec = (tsp)->tv_sec - (usp)->tv_sec;		\
		(vsp)->tv_nsec = (tsp)->tv_nsec - (usp)->tv_nsec;	\
		if ((vsp)->tv_nsec < 0) {				\
			(vsp)->tv_sec--;				\
			(vsp)->tv_nsec += 1000000000L;			\
		}							\
	} while (0)
#endif

extern u_int32_t arc4random(void);
extern void	 arc4random_buf(char *, size_t);
extern u_int32_t arc4random_uniform(u_int32_t);
extern double	 atan(double);
extern double	 atan2(double, double);
extern double	 cos(double);
extern void	 errc(int, int, const char *, ...);
extern double	 exp(double);
extern double	 fabs(double);
extern char	*fgetln(FILE *, size_t *);
extern double	 floor(double);
extern double	 fmod(double, double);
extern int	 fmt_scaled(long long, char *);
extern FTSENT	*fts_children(FTS *, int);
extern int	 fts_close(FTS *);
extern FTS	*fts_open(char * const *, int,
			  int(*)(const FTSENT **, const FTSENT **));
extern FTSENT	*fts_read(FTS *);
extern int	 fts_set(FTS *, FTSENT *, int);
extern char	*getbsize(int *, long *);
extern mode_t	 getmode(const void *, mode_t);
extern int	 getopt(int, char * const *, const char *);
extern int	 gid_from_group(const char *, gid_t *);
extern const char *group_from_gid(gid_t, int);
extern double	 ldexp(double, int);
extern double	 log(double);
extern double	 log10(double);
extern double	 modf(double, double *);
extern char	*openbsd_basename(const char *);
extern char	*openbsd_dirname(const char *);
extern int	 pledge(const char *, const char *);
extern double	 pow(double, double);
extern void	*reallocarray(void *, size_t, size_t);
extern void	*recallocarray(void *, size_t, size_t, size_t);
extern double	 scalbn(double, int);
extern int	 scan_scaled(char *, long long *);
extern void	*setmode(const char *);
extern double	 sin(double);
extern double	 sqrt(double);
extern int	 stravis(char **, const char *, int);
extern size_t	 strlcat(char *, const char *, size_t);
extern size_t	 strlcpy(char *, const char *, size_t);
extern void	 strmode(int, char *);
extern long long strtonum(const char *, long long, long long, const char **);
extern int	 strunvis(char *, const char *);
extern int	 strvis(char *, const char *, int);
extern int	 strvisx(char *, const char *, size_t, int);
extern int	 uid_from_user(const char *, uid_t *);
extern int	 unveil(const char *, const char *);
extern const char *user_from_uid(uid_t, int);
extern void	 verrc(int, int, const char *, va_list);
extern char	*vis(char *, int, int, int);
extern void	 vwarnc(int, const char *, va_list);
extern void	 warnc(int, const char *, ...);

extern int	 opterr;
extern int	 optind;
extern int	 optopt;
extern int	 optreset;
extern char	*optarg;
