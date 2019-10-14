/*	$OpenBSD: col.c,v 1.19 2015/10/09 01:37:06 deraadt Exp $	*/
/*	$NetBSD: col.c,v 1.7 1995/09/02 05:48:50 jtc Exp $	*/

/*-
 * Copyright (c) 1990, 1993, 1994
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Michael Rendell of the Memorial University of Newfoundland.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <ctype.h>
#include <err.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>

#define	BS	'\b'		/* backspace */
#define	TAB	'\t'		/* tab */
#define	SPACE	' '		/* space */
#define	NL	'\n'		/* newline */
#define	CR	'\r'		/* carriage return */
#define	ESC	'\033'		/* escape */
#define	SI	'\017'		/* shift in to normal character set */
#define	SO	'\016'		/* shift out to alternate character set */
#define	VT	'\013'		/* vertical tab (aka reverse line feed) */

/* build up at least this many lines before flushing them out */
#define	BUFFER_MARGIN		32

typedef char CSET;

typedef struct char_str {
#define	CS_NORMAL	1
#define	CS_ALTERNATE	2
	size_t		c_column;	/* column character is in */
	CSET		c_set;		/* character set (currently only 2) */
	char		c_char;		/* character in question */
} CHAR;

typedef struct line_str LINE;
struct line_str {
	CHAR	*l_line;		/* characters on the line */
	LINE	*l_prev;		/* previous line */
	LINE	*l_next;		/* next line */
	size_t	l_lsize;		/* allocated sizeof l_line */
	size_t	l_line_len;		/* strlen(l_line) */
	size_t	l_max_col;		/* max column in the line */
	int	l_needs_sort;		/* set if chars went in out of order */
};

void	addto_lineno(int *, int);
LINE   *alloc_line(void);
void	dowarn(int);
void	flush_line(LINE *);
void	flush_lines(int);
void	flush_blanks(void);
void	free_line(LINE *);
void	usage(void);
void   *xreallocarray(void *, size_t, size_t);

CSET	last_set;		/* char_set of last char printed */
LINE   *lines;
int	compress_spaces;	/* if doing space -> tab conversion */
int	fine;			/* if `fine' resolution (half lines) */
int	max_bufd_lines;		/* max # of half lines to keep in memory */
int	nblank_lines;		/* # blanks after last flushed line */
int	no_backspaces;		/* if not to output any backspaces */

#define	PUTC(ch) \
	if (putchar(ch) == EOF) \
		err(1, "stdout");

int
main(int argc, char *argv[])
{
	int ch;
	CHAR *c;
	CSET cur_set;			/* current character set */
	LINE *l;			/* current line */
	int extra_lines;		/* # of lines above first line */
	size_t cur_col;			/* current column */
	int cur_line;			/* line number of current position */
	int max_line;			/* max value of cur_line */
	int this_line;			/* line l points to */
	int nflushd_lines;		/* number of lines that were flushed */
	int adjust, opt, warned;
	const char *errstr;

	if (pledge("stdio", NULL) == -1)
		err(1, "pledge");

	max_bufd_lines = 256;
	compress_spaces = 1;		/* compress spaces into tabs */
	while ((opt = getopt(argc, argv, "bfhl:x")) != -1)
		switch (opt) {
		case 'b':		/* do not output backspaces */
			no_backspaces = 1;
			break;
		case 'f':		/* allow half forward line feeds */
			fine = 1;
			break;
		case 'h':		/* compress spaces into tabs */
			compress_spaces = 1;
			break;
		case 'l':		/* buffered line count */
			max_bufd_lines = strtonum(optarg, 1,
			    (INT_MAX - BUFFER_MARGIN) / 2, &errstr) * 2;
			if (errstr != NULL)
				errx(1, "bad -l argument, %s: %s", errstr, 
					optarg);
			break;
		case 'x':		/* do not compress spaces into tabs */
			compress_spaces = 0;
			break;
		case '?':
		default:
			usage();
		}

	if (optind != argc)
		usage();

	adjust = extra_lines = warned = 0;
	cur_col = 0;
	cur_line = max_line = nflushd_lines = this_line = 0;
	cur_set = last_set = CS_NORMAL;
	lines = l = alloc_line();

	while ((ch = getchar()) != EOF) {
		if (!isgraph(ch)) {
			switch (ch) {
			case BS:		/* can't go back further */
				if (cur_col == 0)
					continue;
				--cur_col;
				continue;
			case CR:
				cur_col = 0;
				continue;
			case ESC:		/* just ignore EOF */
				/*
				 * In the input stream, accept both the
				 * XPG5 sequences ESC-digit and the
				 * traditional BSD sequences ESC-ctrl.
				 */
				switch(getchar()) {
				case '7':  /* reverse line feed */
					/* FALLTHROUGH */
				case '\007':
					addto_lineno(&cur_line, -2);
					break;
				case '8':  /* reverse half-line feed */
					/* FALLTHROUGH */
				case '\010':
					addto_lineno(&cur_line, -1);
					break;
				case '9':  /* forward half-line feed */
					/* FALLTHROUGH */
				case '\011':
					addto_lineno(&cur_line, 1);
					if (cur_line > max_line)
						max_line = cur_line;
				}
				continue;
			case NL:
				addto_lineno(&cur_line, 2);
				if (cur_line > max_line)
					max_line = cur_line;
				cur_col = 0;
				continue;
			case SPACE:
				++cur_col;
				continue;
			case SI:
				cur_set = CS_NORMAL;
				continue;
			case SO:
				cur_set = CS_ALTERNATE;
				continue;
			case TAB:		/* adjust column */
				cur_col |= 7;
				++cur_col;
				continue;
			case VT:
				addto_lineno(&cur_line, -2);
				continue;
			}
			continue;
		}

		/* Must stuff ch in a line - are we at the right one? */
		if (cur_line + adjust != this_line) {
			LINE *lnew;

			/* round up to next line */
			adjust = !fine && (cur_line & 1);

			if (cur_line + adjust < this_line) {
				while (cur_line + adjust < this_line &&
				    l->l_prev != NULL) {
					l = l->l_prev;
					this_line--;
				}
				if (cur_line + adjust < this_line) {
					if (nflushd_lines == 0) {
						/*
						 * Allow backup past first
						 * line if nothing has been
						 * flushed yet.
						 */
						while (cur_line + adjust
						    < this_line) {
							lnew = alloc_line();
							l->l_prev = lnew;
							lnew->l_next = l;
							l = lines = lnew;
							extra_lines++;
							this_line--;
						}
					} else {
						if (!warned++)
							dowarn(cur_line);
						cur_line = this_line - adjust;
					}
				}
			} else {
				/* may need to allocate here */
				while (cur_line + adjust > this_line) {
					if (l->l_next == NULL) {
						l->l_next = alloc_line();
						l->l_next->l_prev = l;
					}
					l = l->l_next;
					this_line++;
				}
			}
			if (this_line > nflushd_lines &&
			    this_line - nflushd_lines >=
			    max_bufd_lines + BUFFER_MARGIN) {
				if (extra_lines) {
					flush_lines(extra_lines);
					extra_lines = 0;
				}
				flush_lines(this_line - nflushd_lines -
				    max_bufd_lines);
				nflushd_lines = this_line - max_bufd_lines;
			}
		}
		/* grow line's buffer? */
		if (l->l_line_len + 1 >= l->l_lsize) {
			size_t need;

			need = l->l_lsize ? l->l_lsize : 45;
			l->l_line = xreallocarray(l->l_line,
			    need, 2 * sizeof(CHAR));
			l->l_lsize = need * 2;
		}
		c = &l->l_line[l->l_line_len++];
		c->c_char = ch;
		c->c_set = cur_set;
		c->c_column = cur_col;
		/*
		 * If things are put in out of order, they will need sorting
		 * when it is flushed.
		 */
		if (cur_col < l->l_max_col)
			l->l_needs_sort = 1;
		else
			l->l_max_col = cur_col;
		cur_col++;
	}
	if (extra_lines)
		flush_lines(extra_lines);

	/* goto the last line that had a character on it */
	for (; l->l_next; l = l->l_next)
		this_line++;
	flush_lines(this_line - nflushd_lines + 1);

	/* make sure we leave things in a sane state */
	if (last_set != CS_NORMAL)
		PUTC(SI);

	/* flush out the last few blank lines */
	if (max_line > this_line)
		nblank_lines = max_line - this_line;
	if (max_line & 1)
		nblank_lines++;
	flush_blanks();
	exit(0);
}

void
flush_lines(int nflush)
{
	LINE *l;

	while (--nflush >= 0) {
		l = lines;
		lines = l->l_next;
		if (l->l_line) {
			flush_blanks();
			flush_line(l);
		}
		if (l->l_line || l->l_next)
			nblank_lines++;
		if (l->l_line)
			(void)free((void *)l->l_line);
		free_line(l);
	}
	if (lines)
		lines->l_prev = NULL;
}

/*
 * Print a number of newline/half newlines.  If fine flag is set, nblank_lines
 * is the number of half line feeds, otherwise it is the number of whole line
 * feeds.
 */
void
flush_blanks(void)
{
	int half, i, nb;

	half = 0;
	nb = nblank_lines;
	if (nb & 1) {
		if (fine)
			half = 1;
		else
			nb++;
	}
	nb /= 2;
	for (i = nb; --i >= 0;)
		PUTC('\n');
	if (half) {
		/*
		 * In the output stream, always generate
		 * escape sequences conforming to XPG5.
		 */
		PUTC(ESC);
		PUTC('9');
		if (!nb)
			PUTC('\r');
	}
	nblank_lines = 0;
}

/*
 * Write a line to stdout taking care of space to tab conversion (-h flag)
 * and character set shifts.
 */
void
flush_line(LINE *l)
{
	CHAR *c, *endc;
	size_t nchars, last_col, this_col;

	last_col = 0;
	nchars = l->l_line_len;

	if (l->l_needs_sort) {
		static CHAR *sorted;
		static size_t count_size, i, sorted_size;
		static int *count, save, tot;

		/*
		 * Do an O(n) sort on l->l_line by column being careful to
		 * preserve the order of characters in the same column.
		 */
		if (l->l_lsize > sorted_size) {
			sorted_size = l->l_lsize;
			sorted = xreallocarray(sorted,
			    sorted_size, sizeof(CHAR));
		}
		if (l->l_max_col >= count_size) {
			count_size = l->l_max_col + 1;
			count = xreallocarray(count,
			    count_size, sizeof(int));
		}
		memset(count, 0, sizeof(*count) * (l->l_max_col + 1));
		for (i = nchars, c = l->l_line; i-- > 0; c++)
			count[c->c_column]++;

		/*
		 * calculate running total (shifted down by 1) to use as
		 * indices into new line.
		 */
		for (tot = 0, i = 0; i <= l->l_max_col; i++) {
			save = count[i];
			count[i] = tot;
			tot += save;
		}

		for (i = nchars, c = l->l_line; i-- > 0; c++)
			sorted[count[c->c_column]++] = *c;
		c = sorted;
	} else
		c = l->l_line;
	while (nchars > 0) {
		this_col = c->c_column;
		endc = c;
		do {
			++endc;
		} while (--nchars > 0 && this_col == endc->c_column);

		/* if -b only print last character */
		if (no_backspaces)
			c = endc - 1;

		if (this_col > last_col) {
			size_t nspace = this_col - last_col;

			if (compress_spaces && nspace > 1) {
				size_t ntabs;

				ntabs = ((last_col % 8) + nspace) / 8;
				if (ntabs) {
					nspace -= (ntabs * 8) - (last_col % 8);
					while (ntabs-- > 0)
						PUTC('\t');
				}
			}
			while (nspace-- > 0)
				PUTC(' ');
			last_col = this_col;
		}
		last_col++;

		for (;;) {
			if (c->c_set != last_set) {
				switch (c->c_set) {
				case CS_NORMAL:
					PUTC(SI);
					break;
				case CS_ALTERNATE:
					PUTC(SO);
				}
				last_set = c->c_set;
			}
			PUTC(c->c_char);
			if (++c >= endc)
				break;
			PUTC('\b');
		}
	}
}

/*
 * Increment or decrement a line number, checking for overflow.
 * Stop one below INT_MAX such that the adjust variable is safe.
 */
void
addto_lineno(int *lno, int offset)
{
	if (offset > 0) {
		if (*lno >= INT_MAX - offset)
			errx(1, "too many lines");
	} else {
		if (*lno < INT_MIN - offset)
			errx(1, "too many reverse line feeds");
	}
	*lno += offset;
}

#define	NALLOC 64

static LINE *line_freelist;

LINE *
alloc_line(void)
{
	LINE *l;
	int i;

	if (!line_freelist) {
		l = xreallocarray(NULL, NALLOC, sizeof(LINE));
		line_freelist = l;
		for (i = 1; i < NALLOC; i++, l++)
			l->l_next = l + 1;
		l->l_next = NULL;
	}
	l = line_freelist;
	line_freelist = l->l_next;

	memset(l, 0, sizeof(LINE));
	return (l);
}

void
free_line(LINE *l)
{

	l->l_next = line_freelist;
	line_freelist = l;
}

void *
xreallocarray(void *p, size_t n, size_t size)
{

	if (!(p = reallocarray(p, n, size)))
		err(1, "realloc failed");
	return (p);
}

void
usage(void)
{
	(void)fprintf(stderr, "usage: col [-bfhx] [-l num]\n");
	exit(1);
}

void
dowarn(int line)
{

	warnx("warning: can't back up %s",
		line < 0 ? "past first line" : "-- line already flushed");
}
