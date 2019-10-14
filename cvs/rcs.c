/*	$OpenBSD: rcs.c,v 1.319 2019/06/28 13:35:00 deraadt Exp $	*/
/*
 * Copyright (c) 2004 Jean-Francois Brousseau <jfb@openbsd.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
 * THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL  DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/stat.h>

#include <ctype.h>
#include <errno.h>
#include <libgen.h>
#include <pwd.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "atomicio.h"
#include "cvs.h"
#include "diff.h"
#include "rcs.h"
#include "rcsparse.h"

#define MINIMUM(a, b)	(((a) < (b)) ? (a) : (b))

#define RCS_KWEXP_SIZE  1024

#define ANNOTATE_NEVER	0
#define ANNOTATE_NOW	1
#define ANNOTATE_LATER	2

/* invalid characters in RCS symbol names */
static const char rcs_sym_invch[] = RCS_SYM_INVALCHAR;

/* comment leaders, depending on the file's suffix */
static const struct rcs_comment {
	const char	*rc_suffix;
	const char	*rc_cstr;
} rcs_comments[] = {
	{ "1",    ".\\\" " },
	{ "2",    ".\\\" " },
	{ "3",    ".\\\" " },
	{ "4",    ".\\\" " },
	{ "5",    ".\\\" " },
	{ "6",    ".\\\" " },
	{ "7",    ".\\\" " },
	{ "8",    ".\\\" " },
	{ "9",    ".\\\" " },
	{ "a",    "-- "    },	/* Ada		 */
	{ "ada",  "-- "    },
	{ "adb",  "-- "    },
	{ "asm",  ";; "    },	/* assembler (MS-DOS) */
	{ "ads",  "-- "    },	/* Ada */
	{ "bat",  ":: "    },	/* batch (MS-DOS) */
	{ "body", "-- "    },	/* Ada */
	{ "c",    " * "    },	/* C */
	{ "c++",  "// "    },	/* C++ */
	{ "cc",   "// "    },
	{ "cpp",  "// "    },
	{ "cxx",  "// "    },
	{ "m",    "// "    },	/* Objective-C */
	{ "cl",   ";;; "   },	/* Common Lisp	 */
	{ "cmd",  ":: "    },	/* command (OS/2) */
	{ "cmf",  "c "     },	/* CM Fortran	 */
	{ "csh",  "# "     },	/* shell	 */
	{ "e",    "# "     },	/* efl		 */
	{ "epsf", "% "     },	/* encapsulated postscript */
	{ "epsi", "% "     },	/* encapsulated postscript */
	{ "el",   "; "     },	/* Emacs Lisp	 */
	{ "f",    "c "     },	/* Fortran	 */
	{ "for",  "c "     },
	{ "h",    " * "    },	/* C-header	 */
	{ "hh",   "// "    },	/* C++ header	 */
	{ "hpp",  "// "    },
	{ "hxx",  "// "    },
	{ "in",   "# "     },	/* for Makefile.in */
	{ "l",    " * "    },	/* lex */
	{ "mac",  ";; "    },	/* macro (DEC-10, MS-DOS, PDP-11, VMS, etc) */
	{ "mak",  "# "     },	/* makefile, e.g. Visual C++ */
	{ "me",   ".\\\" " },	/* me-macros	t/nroff	 */
	{ "ml",   "; "     },	/* mocklisp	 */
	{ "mm",   ".\\\" " },	/* mm-macros	t/nroff	 */
	{ "ms",   ".\\\" " },	/* ms-macros	t/nroff	 */
	{ "man",  ".\\\" " },	/* man-macros	t/nroff	 */
	{ "p",    " * "    },	/* pascal	 */
	{ "pas",  " * "    },
	{ "pl",   "# "     },	/* Perl	(conflict with Prolog) */
	{ "pm",   "# "     },	/* Perl	module */
	{ "ps",   "% "     },	/* postscript */
	{ "psw",  "% "     },	/* postscript wrap */
	{ "pswm", "% "     },	/* postscript wrap */
	{ "r",    "# "     },	/* ratfor	 */
	{ "rc",   " * "    },	/* Microsoft Windows resource file */
	{ "red",  "% "     },	/* psl/rlisp	 */
	{ "sh",   "# "     },	/* shell	 */
	{ "sl",   "% "     },	/* psl		 */
	{ "spec", "-- "    },	/* Ada		 */
	{ "tex",  "% "     },	/* tex		 */
	{ "y",    " * "    },	/* yacc		 */
	{ "ye",   " * "    },	/* yacc-efl	 */
	{ "yr",   " * "    },	/* yacc-ratfor	 */
};

struct rcs_kw rcs_expkw[] =  {
	{ "Author",	RCS_KW_AUTHOR   },
	{ "Date",	RCS_KW_DATE     },
	{ "Header",	RCS_KW_HEADER   },
	{ "Id",		RCS_KW_ID       },
	{ "Locker",	RCS_KW_LOCKER	},
	{ "Log",	RCS_KW_LOG      },
	{ "Name",	RCS_KW_NAME     },
	{ "RCSfile",	RCS_KW_RCSFILE  },
	{ "Revision",	RCS_KW_REVISION },
	{ "Source",	RCS_KW_SOURCE   },
	{ "State",	RCS_KW_STATE    },
	{ "Mdocdate",	RCS_KW_MDOCDATE },
};

#define NB_COMTYPES	(sizeof(rcs_comments)/sizeof(rcs_comments[0]))

static RCSNUM	*rcs_get_revision(const char *, RCSFILE *);
int		rcs_patch_lines(struct rcs_lines *, struct rcs_lines *,
		    struct rcs_line **, struct rcs_delta *);
static void	rcs_freedelta(struct rcs_delta *);
static void	rcs_strprint(const u_char *, size_t, FILE *);

static void	rcs_kwexp_line(char *, struct rcs_delta *, struct rcs_lines *,
		    struct rcs_line *, int mode);

/*
 * Prepare RCSFILE for parsing. The given file descriptor (if any) must be
 * read-only and is closed on rcs_close().
 */
RCSFILE *
rcs_open(const char *path, int fd, int flags, ...)
{
	int mode;
	mode_t fmode;
	RCSFILE *rfp;
	va_list vap;
	struct stat st;
	struct rcs_delta *rdp;
	struct rcs_lock *lkr;

	fmode = S_IRUSR|S_IRGRP|S_IROTH;
	flags &= 0xffff;	/* ditch any internal flags */

	if (flags & RCS_CREATE) {
		va_start(vap, flags);
		mode = va_arg(vap, int);
		va_end(vap);
		fmode = (mode_t)mode;
	} else {
		if (fstat(fd, &st) == -1)
			fatal("rcs_open: %s: fstat: %s", path, strerror(errno));
		fmode = st.st_mode;
	}

	fmode &= ~cvs_umask;

	rfp = xcalloc(1, sizeof(*rfp));

	rfp->rf_path = xstrdup(path);
	rfp->rf_flags = flags | RCS_SLOCK | RCS_SYNCED;
	rfp->rf_mode = fmode;
	if (fd == -1)
		rfp->rf_file = NULL;
	else if ((rfp->rf_file = fdopen(fd, "r")) == NULL)
		fatal("rcs_open: %s: fdopen: %s", path, strerror(errno));
	rfp->rf_dead = 0;

	TAILQ_INIT(&(rfp->rf_delta));
	TAILQ_INIT(&(rfp->rf_access));
	TAILQ_INIT(&(rfp->rf_symbols));
	TAILQ_INIT(&(rfp->rf_locks));

	if (!(rfp->rf_flags & RCS_CREATE)) {
		if (rcsparse_init(rfp))
			fatal("could not parse admin data");
	}

	/* fill in rd_locker */
	TAILQ_FOREACH(lkr, &(rfp->rf_locks), rl_list) {
		if ((rdp = rcs_findrev(rfp, lkr->rl_num)) == NULL) {
			rcs_close(rfp);
			return (NULL);
		}

		rdp->rd_locker = xstrdup(lkr->rl_name);
	}

	return (rfp);
}

/*
 * rcs_close()
 *
 * Close an RCS file handle.
 */
void
rcs_close(RCSFILE *rfp)
{
	struct rcs_delta *rdp;
	struct rcs_access *rap;
	struct rcs_lock *rlp;
	struct rcs_sym *rsp;

	if ((rfp->rf_flags & RCS_WRITE) && !(rfp->rf_flags & RCS_SYNCED))
		rcs_write(rfp);

	while (!TAILQ_EMPTY(&(rfp->rf_delta))) {
		rdp = TAILQ_FIRST(&(rfp->rf_delta));
		TAILQ_REMOVE(&(rfp->rf_delta), rdp, rd_list);
		rcs_freedelta(rdp);
	}

	while (!TAILQ_EMPTY(&(rfp->rf_access))) {
		rap = TAILQ_FIRST(&(rfp->rf_access));
		TAILQ_REMOVE(&(rfp->rf_access), rap, ra_list);
		free(rap->ra_name);
		free(rap);
	}

	while (!TAILQ_EMPTY(&(rfp->rf_symbols))) {
		rsp = TAILQ_FIRST(&(rfp->rf_symbols));
		TAILQ_REMOVE(&(rfp->rf_symbols), rsp, rs_list);
		free(rsp->rs_num);
		free(rsp->rs_name);
		free(rsp);
	}

	while (!TAILQ_EMPTY(&(rfp->rf_locks))) {
		rlp = TAILQ_FIRST(&(rfp->rf_locks));
		TAILQ_REMOVE(&(rfp->rf_locks), rlp, rl_list);
		free(rlp->rl_num);
		free(rlp->rl_name);
		free(rlp);
	}

	free(rfp->rf_head);
	free(rfp->rf_branch);

	if (rfp->rf_file != NULL)
		fclose(rfp->rf_file);
	free(rfp->rf_path);
	free(rfp->rf_comment);
	free(rfp->rf_expand);
	free(rfp->rf_desc);
	if (rfp->rf_pdata != NULL)
		rcsparse_free(rfp);
	free(rfp);
}

/*
 * rcs_write()
 *
 * Write the contents of the RCS file handle <rfp> to disk in the file whose
 * path is in <rf_path>.
 */
void
rcs_write(RCSFILE *rfp)
{
	FILE *fp;
	char   numbuf[CVS_REV_BUFSZ], *fn, tmpdir[PATH_MAX];
	struct rcs_access *ap;
	struct rcs_sym *symp;
	struct rcs_branch *brp;
	struct rcs_delta *rdp;
	struct rcs_lock *lkp;
	size_t len;
	int fd, saved_errno;

	fd = -1;

	if (rfp->rf_flags & RCS_SYNCED)
		return;

	if (cvs_noexec == 1)
		return;

	/* Write operations need the whole file parsed */
	if (rcsparse_deltatexts(rfp, NULL))
		fatal("rcs_write: rcsparse_deltatexts");

	if (strlcpy(tmpdir, rfp->rf_path, sizeof(tmpdir)) >= sizeof(tmpdir))
		fatal("rcs_write: truncation");
	(void)xasprintf(&fn, "%s/rcs.XXXXXXXXXX", dirname(tmpdir));

	if ((fd = mkstemp(fn)) == -1)
		fatal("%s", fn);

	if ((fp = fdopen(fd, "w")) == NULL) {
		saved_errno = errno;
		(void)unlink(fn);
		fatal("fdopen %s: %s", fn, strerror(saved_errno));
	}

	worklist_add(fn, &temp_files);

	if (rfp->rf_head != NULL)
		rcsnum_tostr(rfp->rf_head, numbuf, sizeof(numbuf));
	else
		numbuf[0] = '\0';

	fprintf(fp, "head\t%s;\n", numbuf);

	if (rfp->rf_branch != NULL) {
		rcsnum_tostr(rfp->rf_branch, numbuf, sizeof(numbuf));
		fprintf(fp, "branch\t%s;\n", numbuf);
	}

	fputs("access", fp);
	TAILQ_FOREACH(ap, &(rfp->rf_access), ra_list) {
		fprintf(fp, "\n\t%s", ap->ra_name);
	}
	fputs(";\n", fp);

	fprintf(fp, "symbols");
	TAILQ_FOREACH(symp, &(rfp->rf_symbols), rs_list) {
		if (RCSNUM_ISBRANCH(symp->rs_num))
			rcsnum_addmagic(symp->rs_num);
		rcsnum_tostr(symp->rs_num, numbuf, sizeof(numbuf));
		fprintf(fp, "\n\t%s:%s", symp->rs_name, numbuf);
	}
	fprintf(fp, ";\n");

	fprintf(fp, "locks");
	TAILQ_FOREACH(lkp, &(rfp->rf_locks), rl_list) {
		rcsnum_tostr(lkp->rl_num, numbuf, sizeof(numbuf));
		fprintf(fp, "\n\t%s:%s", lkp->rl_name, numbuf);
	}

	fprintf(fp, ";");

	if (rfp->rf_flags & RCS_SLOCK)
		fprintf(fp, " strict;");
	fputc('\n', fp);

	fputs("comment\t@", fp);
	if (rfp->rf_comment != NULL) {
		rcs_strprint((const u_char *)rfp->rf_comment,
		    strlen(rfp->rf_comment), fp);
		fputs("@;\n", fp);
	} else
		fputs("# @;\n", fp);

	if (rfp->rf_expand != NULL) {
		fputs("expand @", fp);
		rcs_strprint((const u_char *)rfp->rf_expand,
		    strlen(rfp->rf_expand), fp);
		fputs("@;\n", fp);
	}

	fputs("\n\n", fp);

	TAILQ_FOREACH(rdp, &(rfp->rf_delta), rd_list) {
		fprintf(fp, "%s\n", rcsnum_tostr(rdp->rd_num, numbuf,
		    sizeof(numbuf)));
		fprintf(fp, "date\t%d.%02d.%02d.%02d.%02d.%02d;",
		    rdp->rd_date.tm_year + 1900, rdp->rd_date.tm_mon + 1,
		    rdp->rd_date.tm_mday, rdp->rd_date.tm_hour,
		    rdp->rd_date.tm_min, rdp->rd_date.tm_sec);
		fprintf(fp, "\tauthor %s;\tstate %s;\n",
		    rdp->rd_author, rdp->rd_state);
		fputs("branches", fp);
		TAILQ_FOREACH(brp, &(rdp->rd_branches), rb_list) {
			fprintf(fp, "\n\t%s", rcsnum_tostr(brp->rb_num, numbuf,
			    sizeof(numbuf)));
		}
		fputs(";\n", fp);
		fprintf(fp, "next\t%s;\n\n", rcsnum_tostr(rdp->rd_next,
		    numbuf, sizeof(numbuf)));
	}

	fputs("\ndesc\n@", fp);
	if (rfp->rf_desc != NULL && (len = strlen(rfp->rf_desc)) > 0) {
		rcs_strprint((const u_char *)rfp->rf_desc, len, fp);
		if (rfp->rf_desc[len-1] != '\n')
			fputc('\n', fp);
	}
	fputs("@\n", fp);

	/* deltatexts */
	TAILQ_FOREACH(rdp, &(rfp->rf_delta), rd_list) {
		fprintf(fp, "\n\n%s\n", rcsnum_tostr(rdp->rd_num, numbuf,
		    sizeof(numbuf)));
		fputs("log\n@", fp);
		if (rdp->rd_log != NULL) {
			len = strlen(rdp->rd_log);
			rcs_strprint((const u_char *)rdp->rd_log, len, fp);
			if (len == 0 || rdp->rd_log[len-1] != '\n')
				fputc('\n', fp);
		}
		fputs("@\ntext\n@", fp);
		if (rdp->rd_text != NULL)
			rcs_strprint(rdp->rd_text, rdp->rd_tlen, fp);
		fputs("@\n", fp);
	}

	if (fchmod(fd, rfp->rf_mode) == -1) {
		saved_errno = errno;
		(void)unlink(fn);
		fatal("fchmod %s: %s", fn, strerror(saved_errno));
	}

	(void)fclose(fp);

	if (rename(fn, rfp->rf_path) == -1) {
		saved_errno = errno;
		(void)unlink(fn);
		fatal("rename(%s, %s): %s", fn, rfp->rf_path,
		    strerror(saved_errno));
	}

	rfp->rf_flags |= RCS_SYNCED;
	free(fn);
}

/*
 * rcs_head_get()
 *
 * Retrieve the revision number of the head revision for the RCS file <file>.
 */
RCSNUM *
rcs_head_get(RCSFILE *file)
{
	struct rcs_branch *brp;
	struct rcs_delta *rdp;
	RCSNUM *rev, *rootrev;

	if (file->rf_head == NULL)
		return NULL;

	rev = rcsnum_alloc();
	if (file->rf_branch != NULL) {
		/* we have a default branch, use that to calculate the
		 * real HEAD*/
		rootrev = rcsnum_alloc();
		rcsnum_cpy(file->rf_branch, rootrev,
		    file->rf_branch->rn_len - 1);
		if ((rdp = rcs_findrev(file, rootrev)) == NULL)
			fatal("rcs_head_get: could not find root revision");

		/* HEAD should be the last revision on the default branch */
		TAILQ_FOREACH(brp, &(rdp->rd_branches), rb_list) {
			if (rcsnum_cmp(brp->rb_num, file->rf_branch,
			    file->rf_branch->rn_len) == 0)
				break;
		}
		free(rootrev);

		if (brp == NULL)
			fatal("rcs_head_get: could not find first default "
			    "branch revision");

		if ((rdp = rcs_findrev(file, brp->rb_num)) == NULL)
			fatal("rcs_head_get: could not find branch revision");
		while (rdp->rd_next->rn_len != 0)
			if ((rdp = rcs_findrev(file, rdp->rd_next)) == NULL)
				fatal("rcs_head_get: could not find "
				    "next branch revision");

		rcsnum_cpy(rdp->rd_num, rev, 0);
	} else {
		rcsnum_cpy(file->rf_head, rev, 0);
	}

	return (rev);
}

/*
 * rcs_head_set()
 *
 * Set the revision number of the head revision for the RCS file <file> to
 * <rev>, which must reference a valid revision within the file.
 */
int
rcs_head_set(RCSFILE *file, RCSNUM *rev)
{
	if (rcs_findrev(file, rev) == NULL)
		return (-1);

	if (file->rf_head == NULL)
		file->rf_head = rcsnum_alloc();

	rcsnum_cpy(rev, file->rf_head, 0);
	file->rf_flags &= ~RCS_SYNCED;
	return (0);
}

/*
 * rcs_branch_new()
 *
 * Create a new branch out of supplied revision for the RCS file <file>.
 */
RCSNUM *
rcs_branch_new(RCSFILE *file, RCSNUM *rev)
{
	RCSNUM *brev;
	struct rcs_sym *sym;

	if ((brev = rcsnum_new_branch(rev)) == NULL)
		return (NULL);

	for (;;) {
		TAILQ_FOREACH(sym, &(file->rf_symbols), rs_list)
			if (!rcsnum_cmp(sym->rs_num, brev, 0))
				break;

		if (sym == NULL)
			break;

		if (rcsnum_inc(brev) == NULL ||
		    rcsnum_inc(brev) == NULL) {
			free(brev);
			return (NULL);
		}
	}

	return (brev);
}

/*
 * rcs_branch_get()
 *
 * Retrieve the default branch number for the RCS file <file>.
 * Returns the number on success.  If NULL is returned, then there is no
 * default branch for this file.
 */
const RCSNUM *
rcs_branch_get(RCSFILE *file)
{
	return (file->rf_branch);
}

/*
 * rcs_branch_set()
 *
 * Set the default branch for the RCS file <file> to <bnum>.
 * Returns 0 on success, -1 on failure.
 */
int
rcs_branch_set(RCSFILE *file, const RCSNUM *bnum)
{
	if (file->rf_branch == NULL)
		file->rf_branch = rcsnum_alloc();

	rcsnum_cpy(bnum, file->rf_branch, 0);
	file->rf_flags &= ~RCS_SYNCED;
	return (0);
}

/*
 * rcs_access_add()
 *
 * Add the login name <login> to the access list for the RCS file <file>.
 * Returns 0 on success, or -1 on failure.
 */
int
rcs_access_add(RCSFILE *file, const char *login)
{
	struct rcs_access *ap;

	/* first look for duplication */
	TAILQ_FOREACH(ap, &(file->rf_access), ra_list) {
		if (strcmp(ap->ra_name, login) == 0)
			return (-1);
	}

	ap = xmalloc(sizeof(*ap));
	ap->ra_name = xstrdup(login);
	TAILQ_INSERT_TAIL(&(file->rf_access), ap, ra_list);

	/* not synced anymore */
	file->rf_flags &= ~RCS_SYNCED;
	return (0);
}

/*
 * rcs_access_remove()
 *
 * Remove an entry with login name <login> from the access list of the RCS
 * file <file>.
 * Returns 0 on success, or -1 on failure.
 */
int
rcs_access_remove(RCSFILE *file, const char *login)
{
	struct rcs_access *ap;

	TAILQ_FOREACH(ap, &(file->rf_access), ra_list)
		if (strcmp(ap->ra_name, login) == 0)
			break;

	if (ap == NULL)
		return (-1);

	TAILQ_REMOVE(&(file->rf_access), ap, ra_list);
	free(ap->ra_name);
	free(ap);

	/* not synced anymore */
	file->rf_flags &= ~RCS_SYNCED;
	return (0);
}

/*
 * rcs_sym_add()
 *
 * Add a symbol to the list of symbols for the RCS file <rfp>.  The new symbol
 * is named <sym> and is bound to the RCS revision <snum>.
 */
int
rcs_sym_add(RCSFILE *rfp, const char *sym, RCSNUM *snum)
{
	struct rcs_sym *symp;

	if (!rcs_sym_check(sym))
		return (-1);

	/* first look for duplication */
	TAILQ_FOREACH(symp, &(rfp->rf_symbols), rs_list) {
		if (strcmp(symp->rs_name, sym) == 0)
			return (1);
	}

	symp = xmalloc(sizeof(*symp));
	symp->rs_name = xstrdup(sym);
	symp->rs_num = rcsnum_alloc();
	rcsnum_cpy(snum, symp->rs_num, 0);

	TAILQ_INSERT_HEAD(&(rfp->rf_symbols), symp, rs_list);

	/* not synced anymore */
	rfp->rf_flags &= ~RCS_SYNCED;
	return (0);
}

/*
 * rcs_sym_remove()
 *
 * Remove the symbol with name <sym> from the symbol list for the RCS file
 * <file>.  If no such symbol is found, the call fails and returns with an
 * error.
 * Returns 0 on success, or -1 on failure.
 */
int
rcs_sym_remove(RCSFILE *file, const char *sym)
{
	struct rcs_sym *symp;

	if (!rcs_sym_check(sym))
		return (-1);

	TAILQ_FOREACH(symp, &(file->rf_symbols), rs_list)
		if (strcmp(symp->rs_name, sym) == 0)
			break;

	if (symp == NULL)
		return (-1);

	TAILQ_REMOVE(&(file->rf_symbols), symp, rs_list);
	free(symp->rs_name);
	free(symp->rs_num);
	free(symp);

	/* not synced anymore */
	file->rf_flags &= ~RCS_SYNCED;
	return (0);
}

/*
 * rcs_sym_get()
 *
 * Find a specific symbol <sym> entry in the tree of the RCS file <file>.
 *
 * Returns a pointer to the symbol on success, or NULL on failure.
 */
struct rcs_sym *
rcs_sym_get(RCSFILE *file, const char *sym)
{
	struct rcs_sym *symp;

	TAILQ_FOREACH(symp, &(file->rf_symbols), rs_list)
		if (strcmp(symp->rs_name, sym) == 0)
			return (symp);

	return (NULL);
}

/*
 * rcs_sym_getrev()
 *
 * Retrieve the RCS revision number associated with the symbol <sym> for the
 * RCS file <file>.  The returned value is a dynamically-allocated copy and
 * should be freed by the caller once they are done with it.
 * Returns the RCSNUM on success, or NULL on failure.
 */
RCSNUM *
rcs_sym_getrev(RCSFILE *file, const char *sym)
{
	RCSNUM *num;
	struct rcs_sym *symp;

	if (!rcs_sym_check(sym) || file->rf_head == NULL)
		return (NULL);

	if (!strcmp(sym, RCS_HEAD_BRANCH)) {
		num = rcsnum_alloc();
		rcsnum_cpy(file->rf_head, num, 0);
		return (num);
	}

	num = NULL;
	TAILQ_FOREACH(symp, &(file->rf_symbols), rs_list)
		if (strcmp(symp->rs_name, sym) == 0)
			break;

	if (symp != NULL) {
		num = rcsnum_alloc();
		rcsnum_cpy(symp->rs_num, num, 0);
	}

	return (num);
}

/*
 * rcs_sym_check()
 *
 * Check the RCS symbol name <sym> for any unsupported characters.
 * Returns 1 if the tag is correct, 0 if it isn't valid.
 */
int
rcs_sym_check(const char *sym)
{
	int ret;
	const unsigned char *cp;

	ret = 1;
	cp = sym;
	if (!isalpha(*cp++))
		return (0);

	for (; *cp != '\0'; cp++)
		if (!isgraph(*cp) || (strchr(rcs_sym_invch, *cp) != NULL)) {
			ret = 0;
			break;
		}

	return (ret);
}

/*
 * rcs_lock_getmode()
 *
 * Retrieve the locking mode of the RCS file <file>.
 */
int
rcs_lock_getmode(RCSFILE *file)
{
	return (file->rf_flags & RCS_SLOCK) ? RCS_LOCK_STRICT : RCS_LOCK_LOOSE;
}

/*
 * rcs_lock_setmode()
 *
 * Set the locking mode of the RCS file <file> to <mode>, which must either
 * be RCS_LOCK_LOOSE or RCS_LOCK_STRICT.
 * Returns the previous mode on success, or -1 on failure.
 */
int
rcs_lock_setmode(RCSFILE *file, int mode)
{
	int pmode;
	pmode = rcs_lock_getmode(file);

	if (mode == RCS_LOCK_STRICT)
		file->rf_flags |= RCS_SLOCK;
	else if (mode == RCS_LOCK_LOOSE)
		file->rf_flags &= ~RCS_SLOCK;
	else
		fatal("rcs_lock_setmode: invalid mode `%d'", mode);

	file->rf_flags &= ~RCS_SYNCED;
	return (pmode);
}

/*
 * rcs_lock_add()
 *
 * Add an RCS lock for the user <user> on revision <rev>.
 * Returns 0 on success, or -1 on failure.
 */
int
rcs_lock_add(RCSFILE *file, const char *user, RCSNUM *rev)
{
	struct rcs_lock *lkp;

	/* first look for duplication */
	TAILQ_FOREACH(lkp, &(file->rf_locks), rl_list) {
		if (strcmp(lkp->rl_name, user) == 0 &&
		    rcsnum_cmp(rev, lkp->rl_num, 0) == 0)
			return (-1);
	}

	lkp = xmalloc(sizeof(*lkp));
	lkp->rl_name = xstrdup(user);
	lkp->rl_num = rcsnum_alloc();
	rcsnum_cpy(rev, lkp->rl_num, 0);

	TAILQ_INSERT_TAIL(&(file->rf_locks), lkp, rl_list);

	/* not synced anymore */
	file->rf_flags &= ~RCS_SYNCED;
	return (0);
}


/*
 * rcs_lock_remove()
 *
 * Remove the RCS lock on revision <rev>.
 * Returns 0 on success, or -1 on failure.
 */
int
rcs_lock_remove(RCSFILE *file, const char *user, RCSNUM *rev)
{
	struct rcs_lock *lkp;

	TAILQ_FOREACH(lkp, &(file->rf_locks), rl_list) {
		if (strcmp(lkp->rl_name, user) == 0 &&
		    rcsnum_cmp(lkp->rl_num, rev, 0) == 0)
			break;
	}

	if (lkp == NULL)
		return (-1);

	TAILQ_REMOVE(&(file->rf_locks), lkp, rl_list);
	free(lkp->rl_num);
	free(lkp->rl_name);
	free(lkp);

	/* not synced anymore */
	file->rf_flags &= ~RCS_SYNCED;
	return (0);
}

/*
 * rcs_desc_get()
 *
 * Retrieve the description for the RCS file <file>.
 */
const char *
rcs_desc_get(RCSFILE *file)
{
	return (file->rf_desc);
}

/*
 * rcs_desc_set()
 *
 * Set the description for the RCS file <file>.
 */
void
rcs_desc_set(RCSFILE *file, const char *desc)
{
	char *tmp;

	tmp = xstrdup(desc);
	free(file->rf_desc);
	file->rf_desc = tmp;
	file->rf_flags &= ~RCS_SYNCED;
}

/*
 * rcs_comment_lookup()
 *
 * Lookup the assumed comment leader based on a file's suffix.
 * Returns a pointer to the string on success, or NULL on failure.
 */
const char *
rcs_comment_lookup(const char *filename)
{
	int i;
	const char *sp;

	if ((sp = strrchr(filename, '.')) == NULL)
		return (NULL);
	sp++;

	for (i = 0; i < (int)NB_COMTYPES; i++)
		if (strcmp(rcs_comments[i].rc_suffix, sp) == 0)
			return (rcs_comments[i].rc_cstr);
	return (NULL);
}

/*
 * rcs_comment_get()
 *
 * Retrieve the comment leader for the RCS file <file>.
 */
const char *
rcs_comment_get(RCSFILE *file)
{
	return (file->rf_comment);
}

/*
 * rcs_comment_set()
 *
 * Set the comment leader for the RCS file <file>.
 */
void
rcs_comment_set(RCSFILE *file, const char *comment)
{
	char *tmp;

	tmp = xstrdup(comment);
	free(file->rf_comment);
	file->rf_comment = tmp;
	file->rf_flags &= ~RCS_SYNCED;
}

int
rcs_patch_lines(struct rcs_lines *dlines, struct rcs_lines *plines,
    struct rcs_line **alines, struct rcs_delta *rdp)
{
	u_char op;
	char *ep;
	struct rcs_line *lp, *dlp, *ndlp;
	int i, lineno, nbln;
	u_char tmp;

	dlp = TAILQ_FIRST(&(dlines->l_lines));
	lp = TAILQ_FIRST(&(plines->l_lines));

	/* skip first bogus line */
	for (lp = TAILQ_NEXT(lp, l_list); lp != NULL;
	    lp = TAILQ_NEXT(lp, l_list)) {
		if (lp->l_len < 2)
			fatal("line too short, RCS patch seems broken");
		op = *(lp->l_line);
		/* NUL-terminate line buffer for strtol() safety. */
		tmp = lp->l_line[lp->l_len - 1];
		lp->l_line[lp->l_len - 1] = '\0';
		lineno = (int)strtol((char*)(lp->l_line + 1), &ep, 10);
		if (lineno - 1 > dlines->l_nblines || lineno < 0) {
			fatal("invalid line specification in RCS patch");
		}
		ep++;
		nbln = (int)strtol(ep, &ep, 10);
		/* Restore the last byte of the buffer */
		lp->l_line[lp->l_len - 1] = tmp;
		if (nbln < 0)
			fatal("invalid line number specification in RCS patch");

		/* find the appropriate line */
		for (;;) {
			if (dlp == NULL)
				break;
			if (dlp->l_lineno == lineno)
				break;
			if (dlp->l_lineno > lineno) {
				dlp = TAILQ_PREV(dlp, tqh, l_list);
			} else if (dlp->l_lineno < lineno) {
				if (((ndlp = TAILQ_NEXT(dlp, l_list)) == NULL) ||
				    ndlp->l_lineno > lineno)
					break;
				dlp = ndlp;
			}
		}
		if (dlp == NULL)
			fatal("can't find referenced line in RCS patch");

		if (op == 'd') {
			for (i = 0; (i < nbln) && (dlp != NULL); i++) {
				ndlp = TAILQ_NEXT(dlp, l_list);
				TAILQ_REMOVE(&(dlines->l_lines), dlp, l_list);
				if (alines != NULL && dlp->l_line != NULL) {
					dlp->l_delta = rdp;
					alines[dlp->l_lineno_orig - 1] =
						dlp;
				} else
					free(dlp);
				dlp = ndlp;
				/* last line is gone - reset dlp */
				if (dlp == NULL) {
					ndlp = TAILQ_LAST(&(dlines->l_lines),
					    tqh);
					dlp = ndlp;
				}
			}
		} else if (op == 'a') {
			for (i = 0; i < nbln; i++) {
				ndlp = lp;
				lp = TAILQ_NEXT(lp, l_list);
				if (lp == NULL)
					fatal("truncated RCS patch");
				TAILQ_REMOVE(&(plines->l_lines), lp, l_list);
				if (alines != NULL) {
					if (lp->l_needsfree == 1)
						free(lp->l_line);
					lp->l_line = NULL;
					lp->l_needsfree = 0;
				}
				lp->l_delta = rdp;
				TAILQ_INSERT_AFTER(&(dlines->l_lines), dlp,
				    lp, l_list);
				dlp = lp;

				/* we don't want lookup to block on those */
				lp->l_lineno = lineno;

				lp = ndlp;
			}
		} else
			fatal("unknown RCS patch operation `%c'", op);

		/* last line of the patch, done */
		if (lp->l_lineno == plines->l_nblines)
			break;
	}

	/* once we're done patching, rebuild the line numbers */
	lineno = 0;
	TAILQ_FOREACH(lp, &(dlines->l_lines), l_list)
		lp->l_lineno = lineno++;
	dlines->l_nblines = lineno - 1;

	return (0);
}

void
rcs_delta_stats(struct rcs_delta *rdp, int *ladded, int *lremoved)
{
	struct rcs_lines *plines;
	struct rcs_line *lp;
	int added, i, nbln, removed;
	char op, *ep;
	u_char tmp;

	added = removed = 0;

	plines = cvs_splitlines(rdp->rd_text, rdp->rd_tlen);
	lp = TAILQ_FIRST(&(plines->l_lines));

	/* skip first bogus line */
	for (lp = TAILQ_NEXT(lp, l_list); lp != NULL;
	    lp = TAILQ_NEXT(lp, l_list)) {
		if (lp->l_len < 2)
			fatal("line too short, RCS patch seems broken");
		op = *(lp->l_line);
		/* NUL-terminate line buffer for strtol() safety. */
		tmp = lp->l_line[lp->l_len - 1];
		lp->l_line[lp->l_len - 1] = '\0';
		(void)strtol((lp->l_line + 1), &ep, 10);
		ep++;
		nbln = (int)strtol(ep, &ep, 10);
		/* Restore the last byte of the buffer */
		lp->l_line[lp->l_len - 1] = tmp;
		if (nbln < 0)
			fatal("invalid line number specification in RCS patch");

		if (op == 'a') {
			added += nbln;
			for (i = 0; i < nbln; i++) {
				lp = TAILQ_NEXT(lp, l_list);
				if (lp == NULL)
					fatal("truncated RCS patch");
			}
		}
		else if (op == 'd')
			removed += nbln;
		else
			fatal("unknown RCS patch operation '%c'", op);
	}

	cvs_freelines(plines);

	*ladded = added;
	*lremoved = removed;
}

/*
 * rcs_rev_add()
 *
 * Add a revision to the RCS file <rf>.  The new revision's number can be
 * specified in <rev> (which can also be RCS_HEAD_REV, in which case the
 * new revision will have a number equal to the previous head revision plus
 * one).  The <msg> argument specifies the log message for that revision, and
 * <date> specifies the revision's date (a value of -1 is
 * equivalent to using the current time).
 * If <author> is NULL, set the author for this revision to the current user.
 * Returns 0 on success, or -1 on failure.
 */
int
rcs_rev_add(RCSFILE *rf, RCSNUM *rev, const char *msg, time_t date,
    const char *author)
{
	time_t now;
	RCSNUM *root = NULL;
	struct passwd *pw;
	struct rcs_branch *brp, *obrp;
	struct rcs_delta *ordp, *rdp;

	if (rev == RCS_HEAD_REV) {
		if (rf->rf_flags & RCS_CREATE) {
			if ((rev = rcsnum_parse(RCS_HEAD_INIT)) == NULL)
				return (-1);
			free(rf->rf_head);
			rf->rf_head = rev;
		} else if (rf->rf_head == NULL) {
			return (-1);
		} else {
			rev = rcsnum_inc(rf->rf_head);
		}
	} else {
		if ((rdp = rcs_findrev(rf, rev)) != NULL)
			return (-1);
	}

	rdp = xcalloc(1, sizeof(*rdp));

	TAILQ_INIT(&(rdp->rd_branches));

	rdp->rd_num = rcsnum_alloc();
	rcsnum_cpy(rev, rdp->rd_num, 0);

	rdp->rd_next = rcsnum_alloc();

	if (!author && !(author = getlogin())) {
		if (!(pw = getpwuid(getuid())))
			fatal("getpwuid failed");
		author = pw->pw_name;
	}
	rdp->rd_author = xstrdup(author);
	rdp->rd_state = xstrdup(RCS_STATE_EXP);
	rdp->rd_log = xstrdup(msg);

	if (date != (time_t)(-1))
		now = date;
	else
		time(&now);
	gmtime_r(&now, &(rdp->rd_date));

	if (RCSNUM_ISBRANCHREV(rev))
		TAILQ_INSERT_TAIL(&(rf->rf_delta), rdp, rd_list);
	else
		TAILQ_INSERT_HEAD(&(rf->rf_delta), rdp, rd_list);
	rf->rf_ndelta++;

	if (!(rf->rf_flags & RCS_CREATE)) {
		if (RCSNUM_ISBRANCHREV(rev)) {
			if (rev->rn_id[rev->rn_len - 1] == 1) {
				/* a new branch */
				root = rcsnum_branch_root(rev);
				brp = xmalloc(sizeof(*brp));
				brp->rb_num = rcsnum_alloc();
				rcsnum_cpy(rdp->rd_num, brp->rb_num, 0);

				if ((ordp = rcs_findrev(rf, root)) == NULL)
					fatal("root node not found");

				TAILQ_FOREACH(obrp, &(ordp->rd_branches),
				    rb_list) {
					if (!rcsnum_cmp(obrp->rb_num,
					    brp->rb_num,
					    brp->rb_num->rn_len - 1))
						break;
				}

				if (obrp == NULL) {
					TAILQ_INSERT_TAIL(&(ordp->rd_branches),
					    brp, rb_list);
				}
			} else {
				root = rcsnum_alloc();
				rcsnum_cpy(rev, root, 0);
				rcsnum_dec(root);
				if ((ordp = rcs_findrev(rf, root)) == NULL)
					fatal("previous revision not found");
				rcsnum_cpy(rdp->rd_num, ordp->rd_next, 0);
			}
		} else {
			ordp = TAILQ_NEXT(rdp, rd_list);
			rcsnum_cpy(ordp->rd_num, rdp->rd_next, 0);
		}
	}

	free(root);

	/* not synced anymore */
	rf->rf_flags &= ~RCS_SYNCED;

	return (0);
}

/*
 * rcs_rev_remove()
 *
 * Remove the revision whose number is <rev> from the RCS file <rf>.
 */
int
rcs_rev_remove(RCSFILE *rf, RCSNUM *rev)
{
	int fd1, fd2;
	char *path_tmp1, *path_tmp2;
	struct rcs_delta *rdp, *prevrdp, *nextrdp;
	BUF *prevbuf, *newdiff, *newdeltatext;

	if (rev == RCS_HEAD_REV)
		rev = rf->rf_head;

	if (rev == NULL)
		return (-1);

	/* do we actually have that revision? */
	if ((rdp = rcs_findrev(rf, rev)) == NULL)
		return (-1);

	/*
	 * This is confusing, the previous delta is next in the TAILQ list.
	 * the next delta is the previous one in the TAILQ list.
	 *
	 * When the HEAD revision got specified, nextrdp will be NULL.
	 * When the first revision got specified, prevrdp will be NULL.
	 */
	prevrdp = (struct rcs_delta *)TAILQ_NEXT(rdp, rd_list);
	nextrdp = (struct rcs_delta *)TAILQ_PREV(rdp, tqh, rd_list);

	newdeltatext = NULL;
	prevbuf = NULL;
	path_tmp1 = path_tmp2 = NULL;

	if (prevrdp != NULL && nextrdp != NULL) {
		newdiff = buf_alloc(64);

		/* calculate new diff */
		(void)xasprintf(&path_tmp1, "%s/diff1.XXXXXXXXXX", cvs_tmpdir);
		fd1 = rcs_rev_write_stmp(rf, nextrdp->rd_num, path_tmp1, 0);

		(void)xasprintf(&path_tmp2, "%s/diff2.XXXXXXXXXX", cvs_tmpdir);
		fd2 = rcs_rev_write_stmp(rf, prevrdp->rd_num, path_tmp2, 0);

		diff_format = D_RCSDIFF;
		if (diffreg(path_tmp1, path_tmp2,
		    fd1, fd2, newdiff, D_FORCEASCII) == D_ERROR)
			fatal("rcs_diffreg failed");

		close(fd1);
		close(fd2);

		newdeltatext = newdiff;
	} else if (nextrdp == NULL && prevrdp != NULL) {
		newdeltatext = prevbuf;
	}

	if (newdeltatext != NULL) {
		if (rcs_deltatext_set(rf, prevrdp->rd_num, newdeltatext) < 0)
			fatal("error setting new deltatext");
	}

	TAILQ_REMOVE(&(rf->rf_delta), rdp, rd_list);

	/* update pointers */
	if (prevrdp != NULL && nextrdp != NULL) {
		rcsnum_cpy(prevrdp->rd_num, nextrdp->rd_next, 0);
	} else if (prevrdp != NULL) {
		if (rcs_head_set(rf, prevrdp->rd_num) < 0)
			fatal("rcs_head_set failed");
	} else if (nextrdp != NULL) {
		free(nextrdp->rd_next);
		nextrdp->rd_next = rcsnum_alloc();
	} else {
		free(rf->rf_head);
		rf->rf_head = NULL;
	}

	rf->rf_ndelta--;
	rf->rf_flags &= ~RCS_SYNCED;

	rcs_freedelta(rdp);
	free(newdeltatext);
	free(path_tmp1);
	free(path_tmp2);

	return (0);
}

/*
 * rcs_findrev()
 *
 * Find a specific revision's delta entry in the tree of the RCS file <rfp>.
 * The revision number is given in <rev>.
 *
 * Returns a pointer to the delta on success, or NULL on failure.
 */
struct rcs_delta *
rcs_findrev(RCSFILE *rfp, RCSNUM *rev)
{
	int isbrev;
	struct rcs_delta *rdp;

	if (rev == NULL)
		return NULL;

	isbrev = RCSNUM_ISBRANCHREV(rev);

	/*
	 * We need to do more parsing if the last revision in the linked list
	 * is greater than the requested revision.
	 */
	rdp = TAILQ_LAST(&(rfp->rf_delta), rcs_dlist);
	if (rdp == NULL ||
	    (!isbrev && rcsnum_cmp(rdp->rd_num, rev, 0) == -1) ||
	    ((isbrev && rdp->rd_num->rn_len < 4) ||
	    (isbrev && rcsnum_differ(rev, rdp->rd_num)))) {
		if (rcsparse_deltas(rfp, rev))
			fatal("error parsing deltas");
	}

	TAILQ_FOREACH(rdp, &(rfp->rf_delta), rd_list) {
		if (rcsnum_differ(rdp->rd_num, rev))
			continue;
		else
			return (rdp);
	}

	return (NULL);
}

/*
 * rcs_kwexp_set()
 *
 * Set the keyword expansion mode to use on the RCS file <file> to <mode>.
 */
void
rcs_kwexp_set(RCSFILE *file, int mode)
{
	int i;
	char *tmp, buf[8] = "";

	if (RCS_KWEXP_INVAL(mode))
		return;

	i = 0;
	if (mode == RCS_KWEXP_NONE)
		buf[0] = 'b';
	else if (mode == RCS_KWEXP_OLD)
		buf[0] = 'o';
	else {
		if (mode & RCS_KWEXP_NAME)
			buf[i++] = 'k';
		if (mode & RCS_KWEXP_VAL)
			buf[i++] = 'v';
		if (mode & RCS_KWEXP_LKR)
			buf[i++] = 'l';
	}

	tmp = xstrdup(buf);
	free(file->rf_expand);
	file->rf_expand = tmp;
	/* not synced anymore */
	file->rf_flags &= ~RCS_SYNCED;
}

/*
 * rcs_kwexp_get()
 *
 * Retrieve the keyword expansion mode to be used for the RCS file <file>.
 */
int
rcs_kwexp_get(RCSFILE *file)
{
	if (file->rf_expand == NULL)
		return (RCS_KWEXP_DEFAULT);

	return (rcs_kflag_get(file->rf_expand));
}

/*
 * rcs_kflag_get()
 *
 * Get the keyword expansion mode from a set of character flags given in
 * <flags> and return the appropriate flag mask.  In case of an error, the
 * returned mask will have the RCS_KWEXP_ERR bit set to 1.
 */
int
rcs_kflag_get(const char *flags)
{
	int fl;
	size_t len;
	const char *fp;

	if (flags == NULL || !(len = strlen(flags)))
		return (RCS_KWEXP_ERR);

	fl = 0;
	for (fp = flags; *fp != '\0'; fp++) {
		if (*fp == 'k')
			fl |= RCS_KWEXP_NAME;
		else if (*fp == 'v')
			fl |= RCS_KWEXP_VAL;
		else if (*fp == 'l')
			fl |= RCS_KWEXP_LKR;
		else if (*fp == 'o') {
			if (len != 1)
				fl |= RCS_KWEXP_ERR;
			fl |= RCS_KWEXP_OLD;
		} else if (*fp == 'b') {
			if (len != 1)
				fl |= RCS_KWEXP_ERR;
			fl |= RCS_KWEXP_NONE;
		} else	/* unknown letter */
			fl |= RCS_KWEXP_ERR;
	}

	return (fl);
}

/*
 * rcs_freedelta()
 *
 * Free the contents of a delta structure.
 */
static void
rcs_freedelta(struct rcs_delta *rdp)
{
	struct rcs_branch *rb;

	free(rdp->rd_num);
	free(rdp->rd_next);
	free(rdp->rd_author);
	free(rdp->rd_locker);
	free(rdp->rd_state);
	free(rdp->rd_log);
	free(rdp->rd_text);

	while ((rb = TAILQ_FIRST(&(rdp->rd_branches))) != NULL) {
		TAILQ_REMOVE(&(rdp->rd_branches), rb, rb_list);
		free(rb->rb_num);
		free(rb);
	}

	free(rdp);
}

/*
 * rcs_strprint()
 *
 * Output an RCS string <str> of size <slen> to the stream <stream>.  Any
 * '@' characters are escaped.  Otherwise, the string can contain arbitrary
 * binary data.
 */
static void
rcs_strprint(const u_char *str, size_t slen, FILE *stream)
{
	const u_char *ap, *ep, *sp;

	if (slen == 0)
		return;

	ep = str + slen - 1;

	for (sp = str; sp <= ep;)  {
		ap = memchr(sp, '@', ep - sp);
		if (ap == NULL)
			ap = ep;
		(void)fwrite(sp, sizeof(u_char), ap - sp + 1, stream);

		if (*ap == '@')
			putc('@', stream);
		sp = ap + 1;
	}
}

/*
 * rcs_deltatext_set()
 *
 * Set deltatext for <rev> in RCS file <rfp> to <dtext>
 * Returns -1 on error, 0 on success.
 */
int
rcs_deltatext_set(RCSFILE *rfp, RCSNUM *rev, BUF *bp)
{
	size_t len;
	u_char *dtext;
	struct rcs_delta *rdp;

	/* Write operations require full parsing */
	if (rcsparse_deltatexts(rfp, NULL))
		return (-1);

	if ((rdp = rcs_findrev(rfp, rev)) == NULL)
		return (-1);

	free(rdp->rd_text);

	len = buf_len(bp);
	dtext = buf_release(bp);
	bp = NULL;

	if (len != 0) {
		rdp->rd_text = xmalloc(len);
		rdp->rd_tlen = len;
		(void)memcpy(rdp->rd_text, dtext, len);
	} else {
		rdp->rd_text = NULL;
		rdp->rd_tlen = 0;
	}

	free(dtext);
	return (0);
}

/*
 * rcs_rev_setlog()
 *
 * Sets the log message of revision <rev> to <logtext>.
 */
int
rcs_rev_setlog(RCSFILE *rfp, RCSNUM *rev, const char *logtext)
{
	struct rcs_delta *rdp;

	if ((rdp = rcs_findrev(rfp, rev)) == NULL)
		return (-1);

	free(rdp->rd_log);

	rdp->rd_log = xstrdup(logtext);
	rfp->rf_flags &= ~RCS_SYNCED;
	return (0);
}
/*
 * rcs_rev_getdate()
 *
 * Get the date corresponding to a given revision.
 * Returns the date on success, -1 on failure.
 */
time_t
rcs_rev_getdate(RCSFILE *rfp, RCSNUM *rev)
{
	struct rcs_delta *rdp;

	if ((rdp = rcs_findrev(rfp, rev)) == NULL)
		return (-1);

	return (timegm(&rdp->rd_date));
}

/*
 * rcs_state_set()
 *
 * Sets the state of revision <rev> to <state>
 * NOTE: default state is 'Exp'. States may not contain spaces.
 *
 * Returns -1 on failure, 0 on success.
 */
int
rcs_state_set(RCSFILE *rfp, RCSNUM *rev, const char *state)
{
	struct rcs_delta *rdp;

	if ((rdp = rcs_findrev(rfp, rev)) == NULL)
		return (-1);

	free(rdp->rd_state);

	rdp->rd_state = xstrdup(state);

	rfp->rf_flags &= ~RCS_SYNCED;

	return (0);
}

/*
 * rcs_state_check()
 *
 * Check if string <state> is valid.
 *
 * Returns 0 if the string is valid, -1 otherwise.
 */
int
rcs_state_check(const char *state)
{
	if (strcmp(state, RCS_STATE_DEAD) && strcmp(state, RCS_STATE_EXP))
		return (-1);

	return (0);
}

/*
 * rcs_state_get()
 *
 * Get the state for a given revision of a specified RCSFILE.
 *
 * Returns NULL on failure.
 */
const char *
rcs_state_get(RCSFILE *rfp, RCSNUM *rev)
{
	struct rcs_delta *rdp;

	if ((rdp = rcs_findrev(rfp, rev)) == NULL)
		return (NULL);

	return (rdp->rd_state);
}

/* rcs_get_revision() */
static RCSNUM *
rcs_get_revision(const char *revstr, RCSFILE *rfp)
{
	RCSNUM *rev, *brev, *frev;
	struct rcs_branch *brp;
	struct rcs_delta *rdp;
	size_t i;

	rdp = NULL;

	if (!strcmp(revstr, RCS_HEAD_BRANCH)) {
		if (rfp->rf_head == NULL)
			return (NULL);

		frev = rcsnum_alloc();
		rcsnum_cpy(rfp->rf_head, frev, 0);
		return (frev);
	}

	/* Possibly we could be passed a version number */
	if ((rev = rcsnum_parse(revstr)) != NULL) {
		/* Do not return if it is not in RCS file */
		if ((rdp = rcs_findrev(rfp, rev)) != NULL)
			return (rev);
	} else {
		/* More likely we will be passed a symbol */
		rev = rcs_sym_getrev(rfp, revstr);
	}

	if (rev == NULL)
		return (NULL);

	/*
	 * If it was not a branch, thats ok the symbolic
	 * name refered to a revision, so return the resolved
	 * revision for the given name. */
	if (!RCSNUM_ISBRANCH(rev)) {
		/* Sanity check: The first two elements of any
		 * revision (be it on a branch or on trunk) cannot
		 * be greater than HEAD.
		 *
		 * XXX: To avoid comparing to uninitialized memory,
		 * the minimum of both revision lengths is taken
		 * instead of just 2.
		 */
		if (rfp->rf_head == NULL || rcsnum_cmp(rev, rfp->rf_head,
		    MINIMUM(rfp->rf_head->rn_len, rev->rn_len)) < 0) {
			free(rev);
			return (NULL);
		}
		return (rev);
	}

	brev = rcsnum_alloc();
	rcsnum_cpy(rev, brev, rev->rn_len - 1);

	if ((rdp = rcs_findrev(rfp, brev)) == NULL)
		fatal("rcs_get_revision: tag `%s' does not exist", revstr);
	free(brev);

	TAILQ_FOREACH(brp, &(rdp->rd_branches), rb_list) {
		for (i = 0; i < rev->rn_len; i++)
			if (brp->rb_num->rn_id[i] != rev->rn_id[i])
				break;
		if (i != rev->rn_len)
			continue;
		break;
	}

	free(rev);
	frev = rcsnum_alloc();
	if (brp == NULL) {
		rcsnum_cpy(rdp->rd_num, frev, 0);
		return (frev);
	} else {
		/* Fetch the delta with the correct branch num */
		if ((rdp = rcs_findrev(rfp, brp->rb_num)) == NULL)
			fatal("rcs_get_revision: could not fetch branch "
			    "delta");
		rcsnum_cpy(rdp->rd_num, frev, 0);
		return (frev);
	}
}

/*
 * rcs_rev_getlines()
 *
 * Get the entire contents of revision <frev> from the RCSFILE <rfp> and
 * return it as a pointer to a struct rcs_lines.
 */
struct rcs_lines *
rcs_rev_getlines(RCSFILE *rfp, RCSNUM *frev, struct rcs_line ***alines)
{
	size_t plen;
	int annotate, done, i, nextroot;
	RCSNUM *tnum, *bnum;
	struct rcs_branch *brp;
	struct rcs_delta *hrdp, *prdp, *rdp, *trdp;
	u_char *patch;
	struct rcs_line *line, *nline;
	struct rcs_lines *dlines, *plines;

	hrdp = prdp = rdp = trdp = NULL;

	if (rfp->rf_head == NULL ||
	    (hrdp = rcs_findrev(rfp, rfp->rf_head)) == NULL)
		fatal("rcs_rev_getlines: no HEAD revision");

	tnum = frev;
	if (rcsparse_deltatexts(rfp, hrdp->rd_num))
		fatal("rcs_rev_getlines: rcsparse_deltatexts");

	/* revision on branch, get the branch root */
	nextroot = 2;
	bnum = rcsnum_alloc();
	if (RCSNUM_ISBRANCHREV(tnum))
		rcsnum_cpy(tnum, bnum, nextroot);
	else
		rcsnum_cpy(tnum, bnum, tnum->rn_len);

	if (alines != NULL) {
		/* start with annotate first at requested revision */
		annotate = ANNOTATE_LATER;
		*alines = NULL;
	} else
		annotate = ANNOTATE_NEVER;

	dlines = cvs_splitlines(hrdp->rd_text, hrdp->rd_tlen);

	done = 0;

	rdp = hrdp;
	if (!rcsnum_differ(rdp->rd_num, bnum)) {
		if (annotate == ANNOTATE_LATER) {
			/* found requested revision for annotate */
			i = 0;
			TAILQ_FOREACH(line, &(dlines->l_lines), l_list) {
				line->l_lineno_orig = line->l_lineno;
				i++;
			}

			*alines = xcalloc(i + 1, sizeof(struct rcs_line *));
			(*alines)[i] = NULL;
			annotate = ANNOTATE_NOW;

			/* annotate down to 1.1 from where we are */
			free(bnum);
			bnum = rcsnum_parse("1.1");
			if (!rcsnum_differ(rdp->rd_num, bnum)) {
				goto next;
			}
		} else
			goto next;
	}

	prdp = hrdp;
	if ((rdp = rcs_findrev(rfp, hrdp->rd_next)) == NULL)
		goto done;

again:
	while (rdp != NULL) {
		if (rdp->rd_next->rn_len != 0) {
			trdp = rcs_findrev(rfp, rdp->rd_next);
			if (trdp == NULL)
				fatal("failed to grab next revision");
		}

		if (rdp->rd_tlen == 0) {
			if (rcsparse_deltatexts(rfp, rdp->rd_num))
				fatal("rcs_rev_getlines: rcsparse_deltatexts");
			if (rdp->rd_tlen == 0) {
				if (!rcsnum_differ(rdp->rd_num, bnum))
					break;
				rdp = trdp;
				continue;
			}
		}

		plen = rdp->rd_tlen;
		patch = rdp->rd_text;
		plines = cvs_splitlines(patch, plen);
		if (annotate == ANNOTATE_NOW)
			rcs_patch_lines(dlines, plines, *alines, prdp);
		else
			rcs_patch_lines(dlines, plines, NULL, NULL);
		cvs_freelines(plines);

		if (!rcsnum_differ(rdp->rd_num, bnum)) {
			if (annotate != ANNOTATE_LATER)
				break;

			/* found requested revision for annotate */
			i = 0;
			TAILQ_FOREACH(line, &(dlines->l_lines), l_list) {
				line->l_lineno_orig = line->l_lineno;
				i++;
			}

			*alines = xcalloc(i + 1, sizeof(struct rcs_line *));
			(*alines)[i] = NULL;
			annotate = ANNOTATE_NOW;

			/* annotate down to 1.1 from where we are */
			free(bnum);
			bnum = rcsnum_parse("1.1");

			if (!rcsnum_differ(rdp->rd_num, bnum))
				break;
		}

		prdp = rdp;
		rdp = trdp;
	}

next:
	if (rdp == NULL || !rcsnum_differ(rdp->rd_num, frev))
		done = 1;

	if (RCSNUM_ISBRANCHREV(frev) && done != 1) {
		nextroot += 2;
		rcsnum_cpy(frev, bnum, nextroot);

		TAILQ_FOREACH(brp, &(rdp->rd_branches), rb_list) {
			for (i = 0; i < nextroot - 1; i++)
				if (brp->rb_num->rn_id[i] != bnum->rn_id[i])
					break;
			if (i == nextroot - 1)
				break;
		}

		if (brp == NULL) {
			if (annotate != ANNOTATE_NEVER) {
				free(*alines);
				*alines = NULL;
				cvs_freelines(dlines);
				free(bnum);
				return (NULL);
			}
			fatal("expected branch not found on branch list");
		}

		if ((rdp = rcs_findrev(rfp, brp->rb_num)) == NULL)
			fatal("rcs_rev_getlines: failed to get delta for target rev");

		goto again;
	}
done:
	/* put remaining lines into annotate buffer */
	if (annotate == ANNOTATE_NOW) {
		for (line = TAILQ_FIRST(&(dlines->l_lines));
		    line != NULL; line = nline) {
			nline = TAILQ_NEXT(line, l_list);
			TAILQ_REMOVE(&(dlines->l_lines), line, l_list);
			if (line->l_line == NULL) {
				free(line);
				continue;
			}

			line->l_delta = rdp;
			(*alines)[line->l_lineno_orig - 1] = line;
		}

		cvs_freelines(dlines);
		dlines = NULL;
	}

	if (bnum != tnum)
		free(bnum);

	return (dlines);
}

void
rcs_annotate_getlines(RCSFILE *rfp, RCSNUM *frev, struct rcs_line ***alines)
{
	size_t plen;
	int i, nextroot;
	RCSNUM *bnum;
	struct rcs_branch *brp;
	struct rcs_delta *rdp, *trdp;
	u_char *patch;
	struct rcs_line *line;
	struct rcs_lines *dlines, *plines;

	rdp = trdp = NULL;

	if (!RCSNUM_ISBRANCHREV(frev))
		fatal("rcs_annotate_getlines: branch revision expected");

	/* revision on branch, get the branch root */
	nextroot = 2;
	bnum = rcsnum_alloc();
	rcsnum_cpy(frev, bnum, nextroot);

	/*
	 * Going from HEAD to 1.1 enables the use of an array, which is
	 * much faster. Unfortunately this is not possible with branch
	 * revisions, so copy over our alines (array) into dlines (tailq).
	 */
	dlines = xcalloc(1, sizeof(*dlines));
	TAILQ_INIT(&(dlines->l_lines));
	line = xcalloc(1, sizeof(*line));
	TAILQ_INSERT_TAIL(&(dlines->l_lines), line, l_list);

	for (i = 0; (*alines)[i] != NULL; i++) {
		line = (*alines)[i];
		line->l_lineno = i + 1;
		TAILQ_INSERT_TAIL(&(dlines->l_lines), line, l_list);
	}

	rdp = rcs_findrev(rfp, bnum);
	if (rdp == NULL)
		fatal("failed to grab branch root revision");

	do {
		nextroot += 2;
		rcsnum_cpy(frev, bnum, nextroot);

		TAILQ_FOREACH(brp, &(rdp->rd_branches), rb_list) {
			for (i = 0; i < nextroot - 1; i++)
				if (brp->rb_num->rn_id[i] != bnum->rn_id[i])
					break;
			if (i == nextroot - 1)
				break;
		}

		if (brp == NULL)
			fatal("expected branch not found on branch list");

		if ((rdp = rcs_findrev(rfp, brp->rb_num)) == NULL)
			fatal("failed to get delta for target rev");

		for (;;) {
			if (rdp->rd_next->rn_len != 0) {
				trdp = rcs_findrev(rfp, rdp->rd_next);
				if (trdp == NULL)
					fatal("failed to grab next revision");
			}

			if (rdp->rd_tlen == 0) {
				if (rcsparse_deltatexts(rfp, rdp->rd_num))
					fatal("rcs_annotate_getlines: "
					    "rcsparse_deltatexts");
				if (rdp->rd_tlen == 0) {
					if (!rcsnum_differ(rdp->rd_num, bnum))
						break;
					rdp = trdp;
					continue;
				}
			}

			plen = rdp->rd_tlen;
			patch = rdp->rd_text;
			plines = cvs_splitlines(patch, plen);
			rcs_patch_lines(dlines, plines, NULL, rdp);
			cvs_freelines(plines);

			if (!rcsnum_differ(rdp->rd_num, bnum))
				break;

			rdp = trdp;
		}
	} while (rcsnum_differ(rdp->rd_num, frev));

	if (bnum != frev)
		free(bnum);

	/*
	 * All lines have been parsed, now they must be copied over
	 * into alines (array) again.
	 */
	free(*alines);

	i = 0;
	TAILQ_FOREACH(line, &(dlines->l_lines), l_list) {
		if (line->l_line != NULL)
			i++;
	}
	*alines = xcalloc(i + 1, sizeof(struct rcs_line *));
	(*alines)[i] = NULL;

	i = 0;
	TAILQ_FOREACH(line, &(dlines->l_lines), l_list) {
		if (line->l_line != NULL)
			(*alines)[i++] = line;
	}
}

/*
 * rcs_rev_getbuf()
 *
 * XXX: This is really really slow and should be avoided if at all possible!
 *
 * Get the entire contents of revision <rev> from the RCSFILE <rfp> and
 * return it as a BUF pointer.
 */
BUF *
rcs_rev_getbuf(RCSFILE *rfp, RCSNUM *rev, int mode)
{
	int expmode, expand;
	struct rcs_delta *rdp;
	struct rcs_lines *lines;
	struct rcs_line *lp, *nlp;
	BUF *bp;

	rdp = NULL;
	expmode = RCS_KWEXP_NONE;
	expand = 0;
	lines = rcs_rev_getlines(rfp, rev, NULL);
	bp = buf_alloc(1024 * 16);

	if (!(mode & RCS_KWEXP_NONE)) {
		expmode = rcs_kwexp_get(rfp);

		if (!(expmode & RCS_KWEXP_NONE)) {
			if ((rdp = rcs_findrev(rfp, rev)) == NULL) {
				char version[RCSNUM_MAXSTR];

				rcsnum_tostr(rev, version, sizeof(version));
				fatal("could not find desired version %s in %s",
				    version, rfp->rf_path);
			}

			expand = 1;
		}
	}

	for (lp = TAILQ_FIRST(&lines->l_lines); lp != NULL;) {
		nlp = TAILQ_NEXT(lp, l_list);

		if (lp->l_line == NULL) {
			lp = nlp;
			continue;
		}

		if (expand)
			rcs_kwexp_line(rfp->rf_path, rdp, lines, lp, expmode);

		do {
			buf_append(bp, lp->l_line, lp->l_len);
		} while ((lp = TAILQ_NEXT(lp, l_list)) != nlp);
	}

	cvs_freelines(lines);

	return (bp);
}

/*
 * rcs_rev_write_fd()
 *
 * Write the entire contents of revision <frev> from the rcsfile <rfp> to
 * file descriptor <fd>.
 */
void
rcs_rev_write_fd(RCSFILE *rfp, RCSNUM *rev, int _fd, int mode)
{
	int fd;
	FILE *fp;
	size_t ret;
	int expmode, expand;
	struct rcs_delta *rdp;
	struct rcs_lines *lines;
	struct rcs_line *lp, *nlp;
	extern int print_stdout;

	rdp = NULL;
	expmode = RCS_KWEXP_NONE;
	expand = 0;
	lines = rcs_rev_getlines(rfp, rev, NULL);

	if (!(mode & RCS_KWEXP_NONE)) {
		expmode = rcs_kwexp_get(rfp);

		if (!(expmode & RCS_KWEXP_NONE)) {
			if ((rdp = rcs_findrev(rfp, rev)) == NULL)
				fatal("could not fetch revision");
			expand = 1;
		}
	}

	fd = dup(_fd);
	if (fd == -1)
		fatal("rcs_rev_write_fd: dup: %s", strerror(errno));

	if ((fp = fdopen(fd, "w")) == NULL)
		fatal("rcs_rev_write_fd: fdopen: %s", strerror(errno));

	for (lp = TAILQ_FIRST(&lines->l_lines); lp != NULL;) {
		nlp = TAILQ_NEXT(lp, l_list);

		if (lp->l_line == NULL) {
			lp = nlp;
			continue;
		}

		if (expand)
			rcs_kwexp_line(rfp->rf_path, rdp, lines, lp, expmode);

		do {
			/*
			 * Solely for the checkout and update -p options.
			 */
			if (cvs_server_active == 1 &&
			    (cvs_cmdop == CVS_OP_CHECKOUT ||
			    cvs_cmdop == CVS_OP_UPDATE) && print_stdout == 1) {
				ret = fwrite("M ", 1, 2, fp);
				if (ret != 2)
					fatal("rcs_rev_write_fd: %s",
					    strerror(errno));
			}

			ret = fwrite(lp->l_line, 1, lp->l_len, fp);
			if (ret != lp->l_len)
				fatal("rcs_rev_write_fd: %s", strerror(errno));
		} while ((lp = TAILQ_NEXT(lp, l_list)) != nlp);
	}

	cvs_freelines(lines);
	(void)fclose(fp);
}

/*
 * rcs_rev_write_stmp()
 *
 * Write the contents of the rev <rev> to a temporary file whose path is
 * specified using <template> (see mkstemp(3)). NB. This function will modify
 * <template>, as per mkstemp.
 */
int
rcs_rev_write_stmp(RCSFILE *rfp,  RCSNUM *rev, char *template, int mode)
{
	int fd;

	if ((fd = mkstemp(template)) == -1)
		fatal("mkstemp: `%s': %s", template, strerror(errno));

	worklist_add(template, &temp_files);
	rcs_rev_write_fd(rfp, rev, fd, mode);

	if (lseek(fd, 0, SEEK_SET) == -1)
		fatal("rcs_rev_write_stmp: lseek: %s", strerror(errno));

	return (fd);
}

static void
rcs_kwexp_line(char *rcsfile, struct rcs_delta *rdp, struct rcs_lines *lines,
    struct rcs_line *line, int mode)
{
	BUF *tmpbuf;
	int kwtype;
	u_int j, found;
	const u_char *c, *start, *fin, *end;
	char *kwstr;
	char expbuf[256], buf[256];
	size_t clen, kwlen, len, tlen;

	kwtype = 0;
	kwstr = NULL;

	if (mode & RCS_KWEXP_OLD)
		return;

	len = line->l_len;
	if (len == 0)
		return;

	c = line->l_line;
	found = 0;
	/* Final character in buffer. */
	fin = c + len - 1;

	/*
	 * Keyword formats:
	 * $Keyword$
	 * $Keyword: value$
	 */
	for (; c < fin; c++) {
		if (*c != '$')
			continue;

		/* remember start of this possible keyword */
		start = c;

		/* first following character has to be alphanumeric */
		c++;
		if (!isalpha(*c)) {
			c = start;
			continue;
		}

		/* Number of characters between c and fin, inclusive. */
		clen = fin - c + 1;

		/* look for any matching keywords */
		found = 0;
		for (j = 0; j < RCS_NKWORDS; j++) {
			kwlen = strlen(rcs_expkw[j].kw_str);
			/*
			 * kwlen must be less than clen since clen
			 * includes either a terminating `$' or a `:'.
			 */
			if (kwlen < clen &&
			    memcmp(c, rcs_expkw[j].kw_str, kwlen) == 0 &&
			    (c[kwlen] == '$' || c[kwlen] == ':')) {
				found = 1;
				kwstr = rcs_expkw[j].kw_str;
				kwtype = rcs_expkw[j].kw_type;
				c += kwlen;
				break;
			}
		}

		if (found == 0 && cvs_tagname != NULL) {
			kwlen = strlen(cvs_tagname);
			if (kwlen < clen &&
			    memcmp(c, cvs_tagname, kwlen) == 0 &&
			    (c[kwlen] == '$' || c[kwlen] == ':')) {
				found = 1;
				kwstr = cvs_tagname;
				kwtype = RCS_KW_ID;
				c += kwlen;
			}
		}

		/* unknown keyword, continue looking */
		if (found == 0) {
			c = start;
			continue;
		}

		/*
		 * if the next character was ':' we need to look for
		 * an '$' before the end of the line to be sure it is
		 * in fact a keyword.
		 */
		if (*c == ':') {
			for (; c <= fin; ++c) {
				if (*c == '$' || *c == '\n')
					break;
			}

			if (*c != '$') {
				c = start;
				continue;
			}
		}
		end = c + 1;

		/* start constructing the expansion */
		expbuf[0] = '\0';

		if (mode & RCS_KWEXP_NAME) {
			if (strlcat(expbuf, "$", sizeof(expbuf)) >=
			    sizeof(expbuf) || strlcat(expbuf, kwstr,
			    sizeof(expbuf)) >= sizeof(expbuf))
				fatal("rcs_kwexp_line: truncated");
			if ((mode & RCS_KWEXP_VAL) &&
			    strlcat(expbuf, ": ", sizeof(expbuf)) >=
			    sizeof(expbuf))
				fatal("rcs_kwexp_line: truncated");
		}

		/*
		 * order matters because of RCS_KW_ID and
		 * RCS_KW_HEADER here
		 */
		if (mode & RCS_KWEXP_VAL) {
			if (kwtype & RCS_KW_RCSFILE) {
				if (!(kwtype & RCS_KW_FULLPATH))
					(void)strlcat(expbuf, basename(rcsfile),
					    sizeof(expbuf));
				else
					(void)strlcat(expbuf, rcsfile,
					    sizeof(expbuf));
				if (strlcat(expbuf, " ", sizeof(expbuf)) >=
				    sizeof(expbuf))
					fatal("rcs_kwexp_line: truncated");
			}

			if (kwtype & RCS_KW_REVISION) {
				rcsnum_tostr(rdp->rd_num, buf, sizeof(buf));
				if (strlcat(buf, " ", sizeof(buf)) >=
				    sizeof(buf) || strlcat(expbuf, buf,
				    sizeof(expbuf)) >= sizeof(buf))
					fatal("rcs_kwexp_line: truncated");
			}

			if (kwtype & RCS_KW_DATE) {
				if (strftime(buf, sizeof(buf),
				    "%Y/%m/%d %H:%M:%S ",
				    &rdp->rd_date) == 0)
					fatal("rcs_kwexp_line: strftime "
					    "failure");
				if (strlcat(expbuf, buf, sizeof(expbuf)) >=
				    sizeof(expbuf))
					fatal("rcs_kwexp_line: string "
					    "truncated");
			}

			if (kwtype & RCS_KW_MDOCDATE) {
				/*
				 * Do not prepend ' ' for a single
				 * digit, %e would do so and there is
				 * no better format for strftime().
				 */
				if (strftime(buf, sizeof(buf),
				    (rdp->rd_date.tm_mday < 10) ?
				        "%B%e %Y " : "%B %e %Y ",
				    &rdp->rd_date) == 0)
					fatal("rcs_kwexp_line: strftime "
					    "failure");
				if (strlcat(expbuf, buf, sizeof(expbuf)) >=
				    sizeof(expbuf))
					fatal("rcs_kwexp_line: string "
					    "truncated");
			}

			if (kwtype & RCS_KW_AUTHOR) {
				if (strlcat(expbuf, rdp->rd_author,
				    sizeof(expbuf)) >= sizeof(expbuf) ||
				    strlcat(expbuf, " ", sizeof(expbuf)) >=
				    sizeof(expbuf))
					fatal("rcs_kwexp_line: string "
					    "truncated");
			}

			if (kwtype & RCS_KW_STATE) {
				if (strlcat(expbuf, rdp->rd_state,
				    sizeof(expbuf)) >= sizeof(expbuf) ||
				    strlcat(expbuf, " ", sizeof(expbuf)) >=
				    sizeof(expbuf))
					fatal("rcs_kwexp_line: string "
					    "truncated");
			}

			/* order does not matter anymore below */
			if (kwtype & RCS_KW_LOG) {
				char linebuf[256];
				struct rcs_line *cur, *lp;
				char *logp, *l_line, *prefix, *q, *sprefix;
				size_t i;

				/* Log line */
				if (!(kwtype & RCS_KW_FULLPATH))
					(void)strlcat(expbuf,
					    basename(rcsfile), sizeof(expbuf));
				else
					(void)strlcat(expbuf, rcsfile,
					    sizeof(expbuf));

				if (strlcat(expbuf, " ", sizeof(expbuf)) >=
				    sizeof(expbuf))
					fatal("rcs_kwexp_line: string "
					    "truncated");

				cur = line;

				/* copy rdp->rd_log for strsep */
				logp = xstrdup(rdp->rd_log);

				/* copy our prefix for later processing */
				prefix = xmalloc(start - line->l_line + 1);
				memcpy(prefix, line->l_line,
				    start - line->l_line);
				prefix[start - line->l_line] = '\0';

				/* copy also prefix without trailing blanks. */
				sprefix = xstrdup(prefix);
				for (i = strlen(sprefix); i > 0 &&
				    sprefix[i - 1] == ' '; i--)
					sprefix[i - 1] = '\0';

				/* new line: revision + date + author */
				linebuf[0] = '\0';
				if (strlcat(linebuf, "Revision ",
				    sizeof(linebuf)) >= sizeof(linebuf))
					fatal("rcs_kwexp_line: truncated");
				rcsnum_tostr(rdp->rd_num, buf, sizeof(buf));
				if (strlcat(linebuf, buf, sizeof(linebuf))
				    >= sizeof(buf))
					fatal("rcs_kwexp_line: truncated");
				if (strftime(buf, sizeof(buf),
				    "  %Y/%m/%d %H:%M:%S  ",
				    &rdp->rd_date) == 0)
					fatal("rcs_kwexp_line: strftime "
					    "failure");
				if (strlcat(linebuf, buf, sizeof(linebuf))
				    >= sizeof(linebuf))
					fatal("rcs_kwexp_line: string "
					    "truncated");
				if (strlcat(linebuf, rdp->rd_author,
				    sizeof(linebuf)) >= sizeof(linebuf))
					fatal("rcs_kwexp_line: string "
					    "truncated");

				lp = xcalloc(1, sizeof(*lp));
				xasprintf((char **)&(lp->l_line), "%s%s\n",
				    prefix, linebuf);
				lp->l_len = strlen(lp->l_line);
				TAILQ_INSERT_AFTER(&(lines->l_lines), cur, lp,
				    l_list);
				cur = lp;

				/* Log message */
				q = logp;
				while ((l_line = strsep(&q, "\n")) != NULL &&
				    q != NULL) {
					lp = xcalloc(1, sizeof(*lp));

					if (l_line[0] == '\0') {
						xasprintf((char **)&(lp->l_line),
						    "%s\n", sprefix);
					} else {
						xasprintf((char **)&(lp->l_line),
						    "%s%s\n", prefix, l_line);
					}

					lp->l_len = strlen(lp->l_line);
					TAILQ_INSERT_AFTER(&(lines->l_lines),
					    cur, lp, l_list);
					cur = lp;
				}
				free(logp);

				/*
				 * This is just another hairy mess, but it must
				 * be done: All characters behind Log will be
				 * written in a new line next to log messages.
				 * But that's not enough, we have to strip all
				 * trailing whitespaces of our prefix.
				 */
				lp = xcalloc(1, sizeof(*lp));
				xasprintf((char **)&lp->l_line, "%s%s",
				    sprefix, end);
				lp->l_len = strlen(lp->l_line);
				TAILQ_INSERT_AFTER(&(lines->l_lines), cur, lp,
				    l_list);
				cur = lp;

				end = line->l_line + line->l_len - 1;

				free(prefix);
				free(sprefix);

			}

			if (kwtype & RCS_KW_SOURCE) {
				if (strlcat(expbuf, rcsfile, sizeof(expbuf)) >=
				    sizeof(expbuf) || strlcat(expbuf, " ",
				    sizeof(expbuf)) >= sizeof(expbuf))
					fatal("rcs_kwexp_line: string "
					    "truncated");
			}

			if (kwtype & RCS_KW_NAME)
				if (strlcat(expbuf, " ", sizeof(expbuf)) >=
				    sizeof(expbuf))
					fatal("rcs_kwexp_line: string "
					    "truncated");

			if (kwtype & RCS_KW_LOCKER)
				if (strlcat(expbuf, " ", sizeof(expbuf)) >=
				    sizeof(expbuf))
					fatal("rcs_kwexp_line: string "
					    "truncated");
		}

		/* end the expansion */
		if (mode & RCS_KWEXP_NAME)
			if (strlcat(expbuf, "$",
			    sizeof(expbuf)) >= sizeof(expbuf))
				fatal("rcs_kwexp_line: truncated");

		/* Concatenate everything together. */
		tmpbuf = buf_alloc(len + strlen(expbuf));
		/* Append everything before keyword. */
		buf_append(tmpbuf, line->l_line,
		    start - line->l_line);
		/* Append keyword. */
		buf_puts(tmpbuf, expbuf);
		/* Point c to end of keyword. */
		tlen = buf_len(tmpbuf) - 1;
		/* Append everything after keyword. */
		buf_append(tmpbuf, end,
		    line->l_line + line->l_len - end);
		c = buf_get(tmpbuf) + tlen;
		/* Point fin to end of data. */
		fin = buf_get(tmpbuf) + buf_len(tmpbuf) - 1;
		/* Recalculate new length. */
		len = buf_len(tmpbuf);

		/* tmpbuf is now ready, convert to string */
		if (line->l_needsfree)
			free(line->l_line);
		line->l_len = len;
		line->l_line = buf_release(tmpbuf);
		line->l_needsfree = 1;
	}
}

/* rcs_translate_tag() */
RCSNUM *
rcs_translate_tag(const char *revstr, RCSFILE *rfp)
{
	int follow;
	time_t deltatime;
	char branch[CVS_REV_BUFSZ];
	RCSNUM *brev, *frev, *rev;
	struct rcs_delta *rdp, *trdp;
	time_t cdate;

	brev = frev = NULL;

	if (revstr == NULL) {
		if (rfp->rf_branch != NULL) {
			rcsnum_tostr(rfp->rf_branch, branch, sizeof(branch));
			revstr = branch;
		} else {
			revstr = RCS_HEAD_BRANCH;
		}
	}

	if ((rev = rcs_get_revision(revstr, rfp)) == NULL)
		return (NULL);

	if ((rdp = rcs_findrev(rfp, rev)) == NULL)
		return (NULL);

	/* let's see if we must follow a branch */
	if (!strcmp(revstr, RCS_HEAD_BRANCH))
		follow = 1;
	else {
		frev = rcs_sym_getrev(rfp, revstr);
		if (frev == NULL)
			frev = rcsnum_parse(revstr);

		brev = rcsnum_alloc();
		rcsnum_cpy(rev, brev, rev->rn_len - 1);

		if (frev != NULL && RCSNUM_ISBRANCH(frev) &&
		    !rcsnum_cmp(frev, brev, 0)) {
			follow = 1;
		} else
			follow = 0;

		free(brev);
	}

	if (cvs_specified_date != -1)
		cdate = cvs_specified_date;
	else
		cdate = cvs_directory_date;

	if (cdate == -1) {
		free(frev);

		/* XXX */
		if (rev->rn_len < 4 || !follow) {
			return (rev);
		}

		/* Find the latest delta on that branch */
		free(rev);
		for (;;) {
			if (rdp->rd_next->rn_len == 0)
				break;
			if ((rdp = rcs_findrev(rfp, rdp->rd_next)) == NULL)
				fatal("rcs_translate_tag: could not fetch "
				    "branch delta");
		}

		rev = rcsnum_alloc();
		rcsnum_cpy(rdp->rd_num, rev, 0);
		return (rev);
	}

	if (frev != NULL) {
		brev = rcsnum_revtobr(frev);
		brev->rn_len = rev->rn_len - 1;
		free(frev);
	}

	free(rev);

	do {
		deltatime = timegm(&(rdp->rd_date));

		if (RCSNUM_ISBRANCHREV(rdp->rd_num)) {
			if (deltatime > cdate) {
				trdp = TAILQ_PREV(rdp, rcs_dlist, rd_list);
				if (trdp == NULL)
					trdp = rdp;

				if (trdp->rd_num->rn_len != rdp->rd_num->rn_len)
					return (NULL);

				rev = rcsnum_alloc();
				rcsnum_cpy(trdp->rd_num, rev, 0);
				return (rev);
			}

			if (rdp->rd_next->rn_len == 0) {
				rev = rcsnum_alloc();
				rcsnum_cpy(rdp->rd_num, rev, 0);
				return (rev);
			}
		} else {
			if (deltatime < cdate) {
				rev = rcsnum_alloc();
				rcsnum_cpy(rdp->rd_num, rev, 0);
				return (rev);
			}
		}

		if (follow && rdp->rd_next->rn_len != 0) {
			if (brev != NULL && !rcsnum_cmp(brev, rdp->rd_num, 0))
				break;

			trdp = rcs_findrev(rfp, rdp->rd_next);
			if (trdp == NULL)
				fatal("failed to grab next revision");
			rdp = trdp;
		} else
			follow = 0;
	} while (follow);

	return (NULL);
}
