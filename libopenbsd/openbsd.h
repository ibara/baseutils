/*
 * libopenbsd header file
 */

#include <fcntl.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>

#ifndef __dead
#define __dead
#endif

#ifdef __i386__
#define MACHINE		"i386"
#define MACHINE_ARCH	"i386"
#endif

#ifdef __x86_64__
#define MACHINE		"x86_64"
#define MACHINE_ARCH	"x86_64"
#endif

#ifndef _PATH_DEFTAPE
#define _PATH_DEFTAPE "/dev/rst0"
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

#ifndef st_atimespec
#define st_atimespec	st_atim
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

#undef basename

extern void	 arc4random_buf(char *, size_t);
extern char	*basename(const char *);
extern void	 errc(int, int, const char *, ...);
extern int	 fmt_scaled(long long, char *);
extern char	*getbsize(int *, long *);
extern mode_t	 getmode(const void *, mode_t);
extern int	 gid_from_group(const char *, gid_t *);
extern const char *group_from_gid(gid_t, int);
extern int	 pledge(const char *, const char *);
extern void	*reallocarray(void *, size_t, size_t);
extern int	 scan_scaled(char *, long long *);
extern void	*setmode(const char *);
extern int	 stravis(char **, const char *, int);
extern size_t	 strlcat(char *, const char *, size_t);
extern size_t	 strlcpy(char *, const char *, size_t);
extern void	 strmode(int, char *);
extern long long strtonum(const char *, long long, long long, const char **);
extern int	 strunvis(char *, const char *);
extern int	 strvis(char *, const char *, int);
extern int	 strvisx(char *, const char *, size_t, int);
extern int	 uid_from_user(const char *, uid_t *);
extern const char *user_from_uid(uid_t, int);
extern void	 verrc(int, int, const char *, va_list);
extern char	*vis(char *, int, int, int);
extern void	 vwarnc(int, const char *, va_list);
extern void	 warnc(int, const char *, ...);
