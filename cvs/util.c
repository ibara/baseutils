/*	$OpenBSD: util.c,v 1.162 2019/06/28 13:35:00 deraadt Exp $	*/
/*
 * Copyright (c) 2004 Jean-Francois Brousseau <jfb@openbsd.org>
 * Copyright (c) 2005, 2006 Joris Vink <joris@openbsd.org>
 * Copyright (c) 2005, 2006 Xavier Santolaria <xsa@openbsd.org>
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
#include <sys/types.h>
#include <sys/wait.h>

#include <atomicio.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <paths.h>
#include <unistd.h>

#include "cvs.h"
#include "remote.h"
#include "hash.h"

extern int print_stdout;
extern int build_dirs;
extern int disable_fast_checkout;

/* letter -> mode type map */
static const int cvs_modetypes[26] = {
	-1, -1, -1, -1, -1, -1,  1, -1, -1, -1, -1, -1, -1,
	-1,  2, -1, -1, -1, -1, -1,  0, -1, -1, -1, -1, -1,
};

/* letter -> mode map */
static const mode_t cvs_modes[3][26] = {
	{
		0,  0,       0,       0,       0,  0,  0,    /* a - g */
		0,  0,       0,       0,       0,  0,  0,    /* h - m */
		0,  0,       0,       S_IRUSR, 0,  0,  0,    /* n - u */
		0,  S_IWUSR, S_IXUSR, 0,       0             /* v - z */
	},
	{
		0,  0,       0,       0,       0,  0,  0,    /* a - g */
		0,  0,       0,       0,       0,  0,  0,    /* h - m */
		0,  0,       0,       S_IRGRP, 0,  0,  0,    /* n - u */
		0,  S_IWGRP, S_IXGRP, 0,       0             /* v - z */
	},
	{
		0,  0,       0,       0,       0,  0,  0,    /* a - g */
		0,  0,       0,       0,       0,  0,  0,    /* h - m */
		0,  0,       0,       S_IROTH, 0,  0,  0,    /* n - u */
		0,  S_IWOTH, S_IXOTH, 0,       0             /* v - z */
	}
};


/* octal -> string */
static const char *cvs_modestr[8] = {
	"", "x", "w", "wx", "r", "rx", "rw", "rwx"
};

/*
 * cvs_strtomode()
 *
 * Read the contents of the string <str> and generate a permission mode from
 * the contents of <str>, which is assumed to have the mode format of CVS.
 * The CVS protocol specification states that any modes or mode types that are
 * not recognized should be silently ignored.  This function does not return
 * an error in such cases, but will issue warnings.
 */
void
cvs_strtomode(const char *str, mode_t *mode)
{
	char type;
	size_t l;
	mode_t m;
	char buf[32], ms[4], *sp, *ep;

	m = 0;
	l = strlcpy(buf, str, sizeof(buf));
	if (l >= sizeof(buf))
		fatal("cvs_strtomode: string truncation");

	sp = buf;
	ep = sp;

	for (sp = buf; ep != NULL; sp = ep + 1) {
		ep = strchr(sp, ',');
		if (ep != NULL)
			*ep = '\0';

		memset(ms, 0, sizeof ms);
		if (sscanf(sp, "%c=%3s", &type, ms) != 2 &&
			sscanf(sp, "%c=", &type) != 1) {
			fatal("failed to scan mode string `%s'", sp);
		}

		if (type <= 'a' || type >= 'z' ||
		    cvs_modetypes[type - 'a'] == -1) {
			cvs_log(LP_ERR,
			    "invalid mode type `%c'"
			    " (`u', `g' or `o' expected), ignoring", type);
			continue;
		}

		/* make type contain the actual mode index */
		type = cvs_modetypes[type - 'a'];

		for (sp = ms; *sp != '\0'; sp++) {
			if (*sp <= 'a' || *sp >= 'z' ||
			    cvs_modes[(int)type][*sp - 'a'] == 0) {
				fatal("invalid permission bit `%c'", *sp);
			} else
				m |= cvs_modes[(int)type][*sp - 'a'];
		}
	}

	*mode = m;
}

/*
 * cvs_modetostr()
 *
 * Generate a CVS-format string to represent the permissions mask on a file
 * from the mode <mode> and store the result in <buf>, which can accept up to
 * <len> bytes (including the terminating NUL byte).  The result is guaranteed
 * to be NUL-terminated.
 */
void
cvs_modetostr(mode_t mode, char *buf, size_t len)
{
	char tmp[16], *bp;
	mode_t um, gm, om;

	um = (mode & S_IRWXU) >> 6;
	gm = (mode & S_IRWXG) >> 3;
	om = mode & S_IRWXO;

	bp = buf;
	*bp = '\0';

	if (um) {
		if (strlcpy(tmp, "u=", sizeof(tmp)) >= sizeof(tmp) ||
		    strlcat(tmp, cvs_modestr[um], sizeof(tmp)) >= sizeof(tmp))
			fatal("cvs_modetostr: overflow for user mode");

		if (strlcat(buf, tmp, len) >= len)
			fatal("cvs_modetostr: string truncation");
	}

	if (gm) {
		if (um) {
			if (strlcat(buf, ",", len) >= len)
				fatal("cvs_modetostr: string truncation");
		}

		if (strlcpy(tmp, "g=", sizeof(tmp)) >= sizeof(tmp) ||
		    strlcat(tmp, cvs_modestr[gm], sizeof(tmp)) >= sizeof(tmp))
			fatal("cvs_modetostr: overflow for group mode");

		if (strlcat(buf, tmp, len) >= len)
			fatal("cvs_modetostr: string truncation");
	}

	if (om) {
		if (um || gm) {
			if (strlcat(buf, ",", len) >= len)
				fatal("cvs_modetostr: string truncation");
		}

		if (strlcpy(tmp, "o=", sizeof(tmp)) >= sizeof(tmp) ||
		    strlcat(tmp, cvs_modestr[gm], sizeof(tmp)) >= sizeof(tmp))
			fatal("cvs_modetostr: overflow for others mode");

		if (strlcat(buf, tmp, len) >= len)
			fatal("cvs_modetostr: string truncation");
	}
}

/*
 * cvs_getargv()
 *
 * Parse a line contained in <line> and generate an argument vector by
 * splitting the line on spaces and tabs.  The resulting vector is stored in
 * <argv>, which can accept up to <argvlen> entries.
 * Returns the number of arguments in the vector, or -1 if an error occurred.
 */
int
cvs_getargv(const char *line, char **argv, int argvlen)
{
	u_int i;
	int argc, error;
	char *linebuf, *lp, *cp;

	linebuf = xstrdup(line);

	memset(argv, 0, argvlen * sizeof(char *));
	argc = 0;

	/* build the argument vector */
	error = 0;
	for (lp = linebuf; lp != NULL;) {
		cp = strsep(&lp, " \t");
		if (cp == NULL)
			break;
		else if (*cp == '\0')
			continue;

		if (argc == argvlen) {
			error++;
			break;
		}

		argv[argc] = xstrdup(cp);
		argc++;
	}

	if (error != 0) {
		/* ditch the argument vector */
		for (i = 0; i < (u_int)argc; i++)
			free(argv[i]);
		argc = -1;
	}

	free(linebuf);
	return (argc);
}

/*
 * cvs_makeargv()
 *
 * Allocate an argument vector large enough to accommodate for all the
 * arguments found in <line> and return it.
 */
char **
cvs_makeargv(const char *line, int *argc)
{
	int i, ret;
	char *argv[1024], **copy;

	ret = cvs_getargv(line, argv, 1024);
	if (ret == -1)
		return (NULL);

	copy = xcalloc(ret + 1, sizeof(char *));

	for (i = 0; i < ret; i++)
		copy[i] = argv[i];
	copy[ret] = NULL;

	*argc = ret;
	return (copy);
}

/*
 * cvs_freeargv()
 *
 * Free an argument vector previously generated by cvs_getargv().
 */
void
cvs_freeargv(char **argv, int argc)
{
	int i;

	for (i = 0; i < argc; i++)
		free(argv[i]);
}

/*
 * cvs_chdir()
 *
 * Change to directory <path>.
 * If <rm> is equal to `1', <path> is removed if chdir() fails so we
 * do not have temporary directories leftovers.
 * Returns 0 on success.
 */
int
cvs_chdir(const char *path, int rm)
{
	if (chdir(path) == -1) {
		if (rm == 1)
			cvs_unlink(path);
		fatal("cvs_chdir: `%s': %s", path, strerror(errno));
	}

	return (0);
}

/*
 * cvs_rename()
 * Change the name of a file.
 * rename() wrapper with an error message.
 * Returns 0 on success.
 */
int
cvs_rename(const char *from, const char *to)
{
	if (cvs_server_active == 0)
		cvs_log(LP_TRACE, "cvs_rename(%s,%s)", from, to);

	if (cvs_noexec == 1)
		return (0);

	if (rename(from, to) == -1)
		fatal("cvs_rename: `%s'->`%s': %s", from, to, strerror(errno));

	return (0);
}

/*
 * cvs_unlink()
 *
 * Removes the link named by <path>.
 * unlink() wrapper with an error message.
 * Returns 0 on success, or -1 on failure.
 */
int
cvs_unlink(const char *path)
{
	if (cvs_server_active == 0)
		cvs_log(LP_TRACE, "cvs_unlink(%s)", path);

	if (cvs_noexec == 1 && disable_fast_checkout != 0)
		return (0);

	if (unlink(path) == -1 && errno != ENOENT) {
		cvs_log(LP_ERRNO, "%s", path);
		return (-1);
	}

	return (0);
}

/*
 * cvs_rmdir()
 *
 * Remove a directory tree from disk.
 * Returns 0 on success, or -1 on failure.
 */
int
cvs_rmdir(const char *path)
{
	int type, ret = -1;
	DIR *dirp;
	struct dirent *ent;
	struct stat st;
	char fpath[PATH_MAX];

	if (cvs_server_active == 0)
		cvs_log(LP_TRACE, "cvs_rmdir(%s)", path);

	if (cvs_noexec == 1 && disable_fast_checkout != 0)
		return (0);

	if ((dirp = opendir(path)) == NULL) {
		cvs_log(LP_ERR, "failed to open '%s'", path);
		return (-1);
	}

	while ((ent = readdir(dirp)) != NULL) {
		if (!strcmp(ent->d_name, ".") ||
		    !strcmp(ent->d_name, ".."))
			continue;

		(void)xsnprintf(fpath, sizeof(fpath), "%s/%s",
		    path, ent->d_name);

		if (ent->d_type == DT_UNKNOWN) {
			if (lstat(fpath, &st) == -1)
				fatal("'%s': %s", fpath, strerror(errno));

			switch (st.st_mode & S_IFMT) {
			case S_IFDIR:
				type = CVS_DIR;
				break;
			case S_IFREG:
				type = CVS_FILE;
				break;
			default:
				fatal("'%s': Unknown file type in copy",
				    fpath);
			}
		} else {
			switch (ent->d_type) {
			case DT_DIR:
				type = CVS_DIR;
				break;
			case DT_REG:
				type = CVS_FILE;
				break;
			default:
				fatal("'%s': Unknown file type in copy",
				    fpath);
			}
		}
		switch (type) {
		case CVS_DIR:
			if (cvs_rmdir(fpath) == -1)
				goto done;
			break;
		case CVS_FILE:
			if (cvs_unlink(fpath) == -1 && errno != ENOENT)
				goto done;
			break;
		default:
			fatal("type %d unknown, shouldn't happen", type);
		}
	}


	if (rmdir(path) == -1 && errno != ENOENT) {
		cvs_log(LP_ERRNO, "%s", path);
		goto done;
	}

	ret = 0;
done:
	closedir(dirp);
	return (ret);
}

void
cvs_get_repository_path(const char *dir, char *dst, size_t len)
{
	char buf[PATH_MAX];

	cvs_get_repository_name(dir, buf, sizeof(buf));
	(void)xsnprintf(dst, len, "%s/%s", current_cvsroot->cr_dir, buf);
	cvs_validate_directory(dst);
}

void
cvs_get_repository_name(const char *dir, char *dst, size_t len)
{
	FILE *fp;
	char fpath[PATH_MAX];

	dst[0] = '\0';

	if (!(cmdp->cmd_flags & CVS_USE_WDIR)) {
		if (strlcpy(dst, dir, len) >= len)
			fatal("cvs_get_repository_name: truncation");
		return;
	}

	switch (cvs_cmdop) {
	case CVS_OP_EXPORT:
		if (strcmp(dir, "."))
			if (strlcpy(dst, dir, len) >= len)
				fatal("cvs_get_repository_name: truncation");
		break;
	case CVS_OP_IMPORT:
		if (strlcpy(dst, import_repository, len) >= len)
			fatal("cvs_get_repository_name: truncation");
		if (strlcat(dst, "/", len) >= len)
			fatal("cvs_get_repository_name: truncation");

		if (strcmp(dir, "."))
			if (strlcat(dst, dir, len) >= len)
				fatal("cvs_get_repository_name: truncation");
		break;
	default:
		(void)xsnprintf(fpath, sizeof(fpath), "%s/%s",
		    dir, CVS_PATH_REPOSITORY);
		if ((fp = fopen(fpath, "r")) != NULL) {
			if ((fgets(dst, len, fp)) == NULL)
				fatal("%s: bad repository file", fpath);
			dst[strcspn(dst, "\n")] = '\0';
			(void)fclose(fp);
		} else if (cvs_cmdop != CVS_OP_CHECKOUT)
			fatal("%s is missing", fpath);
		break;
	}
}

void
cvs_mkadmin(const char *path, const char *root, const char *repo,
    char *tag, char *date)
{
	FILE *fp;
	int fd;
	char buf[PATH_MAX];
	struct hash_data *hdata, hd;

	hdata = hash_table_find(&created_cvs_directories, path, strlen(path));
	if (hdata != NULL)
		return;

	hd.h_key = xstrdup(path);
	hd.h_data = NULL;
	hash_table_enter(&created_cvs_directories, &hd);

	if (cvs_server_active == 0)
		cvs_log(LP_TRACE, "cvs_mkadmin(%s, %s, %s, %s, %s)",
		    path, root, repo, (tag != NULL) ? tag : "",
		    (date != NULL) ? date : "");

	(void)xsnprintf(buf, sizeof(buf), "%s/%s", path, CVS_PATH_CVSDIR);

	if (mkdir(buf, 0755) == -1 && errno != EEXIST)
		fatal("cvs_mkadmin: %s: %s", buf, strerror(errno));

	if (cvs_cmdop == CVS_OP_CHECKOUT || cvs_cmdop == CVS_OP_ADD ||
	    (cvs_cmdop == CVS_OP_UPDATE && build_dirs == 1)) {
		(void)xsnprintf(buf, sizeof(buf), "%s/%s",
		    path, CVS_PATH_ROOTSPEC);

		if ((fp = fopen(buf, "w")) == NULL)
			fatal("cvs_mkadmin: %s: %s", buf, strerror(errno));

		fprintf(fp, "%s\n", root);
		(void)fclose(fp);
	}

	(void)xsnprintf(buf, sizeof(buf), "%s/%s", path, CVS_PATH_REPOSITORY);

	if ((fp = fopen(buf, "w")) == NULL)
		fatal("cvs_mkadmin: %s: %s", buf, strerror(errno));

	fprintf(fp, "%s\n", repo);
	(void)fclose(fp);

	cvs_write_tagfile(path, tag, date);

	(void)xsnprintf(buf, sizeof(buf), "%s/%s", path, CVS_PATH_ENTRIES);

	if ((fd = open(buf, O_WRONLY|O_CREAT|O_EXCL, 0666 & ~cvs_umask))
	    == -1) {
		if (errno == EEXIST)
			return;
		fatal("cvs_mkadmin: %s: %s", buf, strerror(errno));
	}

	if (atomicio(vwrite, fd, "D\n", 2) != 2)
		fatal("cvs_mkadmin: %s", strerror(errno));
	close(fd);
}

void
cvs_mkpath(const char *path, char *tag)
{
	CVSENTRIES *ent;
	FILE *fp;
	size_t len;
	struct hash_data *hdata, hd;
	char *entry, *sp, *dp, *dir, *p, rpath[PATH_MAX], repo[PATH_MAX];

	hdata = hash_table_find(&created_directories, path, strlen(path));
	if (hdata != NULL)
		return;

	hd.h_key = xstrdup(path);
	hd.h_data = NULL;
	hash_table_enter(&created_directories, &hd);

	if (cvsroot_is_remote() || cvs_server_active == 1)
		cvs_validate_directory(path);

	dir = xstrdup(path);

	STRIP_SLASH(dir);

	if (cvs_server_active == 0)
		cvs_log(LP_TRACE, "cvs_mkpath(%s)", dir);

	repo[0] = '\0';
	rpath[0] = '\0';

	if ((cvs_cmdop != CVS_OP_CHECKOUT) && (cvs_cmdop != CVS_OP_EXPORT)) {
		if ((fp = fopen(CVS_PATH_REPOSITORY, "r")) != NULL) {
			if ((fgets(repo, sizeof(repo), fp)) == NULL)
				fatal("cvs_mkpath: bad repository file");
			repo[strcspn(repo, "\n")] = '\0';
			(void)fclose(fp);
		}
	}

	for (sp = dir; sp != NULL; sp = dp) {
		dp = strchr(sp, '/');
		if (dp != NULL)
			*(dp++) = '\0';

		if (sp == dir && module_repo_root != NULL) {
			len = strlcpy(repo, module_repo_root, sizeof(repo));
			if (len >= (int)sizeof(repo))
				fatal("cvs_mkpath: overflow");
		} else if (strcmp(sp, ".")) {
			if (repo[0] != '\0') {
				len = strlcat(repo, "/", sizeof(repo));
				if (len >= (int)sizeof(repo))
					fatal("cvs_mkpath: overflow");
			}

			len = strlcat(repo, sp, sizeof(repo));
			if (len >= (int)sizeof(repo))
				fatal("cvs_mkpath: overflow");
		}

		if (rpath[0] != '\0') {
			len = strlcat(rpath, "/", sizeof(rpath));
			if (len >= (int)sizeof(rpath))
				fatal("cvs_mkpath: overflow");
		}

		len = strlcat(rpath, sp, sizeof(rpath));
		if (len >= (int)sizeof(rpath))
			fatal("cvs_mkpath: overflow");

		if (mkdir(rpath, 0755) == -1 && errno != EEXIST)
			fatal("cvs_mkpath: %s: %s", rpath, strerror(errno));

		if (cvs_cmdop == CVS_OP_EXPORT && !cvs_server_active)
			continue;

		cvs_mkadmin(rpath, current_cvsroot->cr_str, repo,
		    tag, NULL);

		if (dp != NULL) {
			if ((p = strchr(dp, '/')) != NULL)
				*p = '\0';

			entry = xmalloc(CVS_ENT_MAXLINELEN);
			cvs_ent_line_str(dp, NULL, NULL, NULL, NULL, 1, 0,
			    entry, CVS_ENT_MAXLINELEN);

			ent = cvs_ent_open(rpath);
			cvs_ent_add(ent, entry);
			free(entry);

			if (p != NULL)
				*p = '/';
		}
	}

	free(dir);
}

void
cvs_mkdir(const char *path, mode_t mode)
{
	size_t len;
	char *sp, *dp, *dir, rpath[PATH_MAX];

	if (cvsroot_is_remote() || cvs_server_active == 1)
		cvs_validate_directory(path);

	dir = xstrdup(path);

	STRIP_SLASH(dir);

	if (cvs_server_active == 0)
		cvs_log(LP_TRACE, "cvs_mkdir(%s)", dir);

	rpath[0] = '\0';

	for (sp = dir; sp != NULL; sp = dp) {
		dp = strchr(sp, '/');
		if (dp != NULL)
			*(dp++) = '\0';

		len = strlcat(rpath, "/", sizeof(rpath));
		if (len >= (int)sizeof(rpath))
			fatal("cvs_mkdir: overflow");

		len = strlcat(rpath, sp, sizeof(rpath));
		if (len >= (int)sizeof(rpath))
			fatal("cvs_mkdir: overflow");
		if (1 == len)
			continue;

		if (mkdir(rpath, mode) == -1 && errno != EEXIST)
			fatal("cvs_mkdir: %s: %s", rpath, strerror(errno));
	}

	free(dir);
}

/*
 * Split the contents of a file into a list of lines.
 */
struct rcs_lines *
cvs_splitlines(u_char *data, size_t len)
{
	u_char *p, *c;
	size_t i, tlen;
	struct rcs_lines *lines;
	struct rcs_line *lp;

	lines = xcalloc(1, sizeof(*lines));
	TAILQ_INIT(&(lines->l_lines));

	lp = xcalloc(1, sizeof(*lp));
	TAILQ_INSERT_TAIL(&(lines->l_lines), lp, l_list);

	p = c = data;
	for (i = 0; i < len; i++) {
		if (*p == '\n' || (i == len - 1)) {
			tlen = p - c + 1;
			lp = xcalloc(1, sizeof(*lp));
			lp->l_line = c;
			lp->l_len = tlen;
			lp->l_lineno = ++(lines->l_nblines);
			TAILQ_INSERT_TAIL(&(lines->l_lines), lp, l_list);
			c = p + 1;
		}
		p++;
	}

	return (lines);
}

void
cvs_freelines(struct rcs_lines *lines)
{
	struct rcs_line *lp;

	while ((lp = TAILQ_FIRST(&(lines->l_lines))) != NULL) {
		TAILQ_REMOVE(&(lines->l_lines), lp, l_list);
		if (lp->l_needsfree == 1)
			free(lp->l_line);
		free(lp);
	}

	free(lines);
}

/*
 * cvs_strsplit()
 *
 * Split a string <str> of <sep>-separated values and allocate
 * an argument vector for the values found.
 */
struct cvs_argvector *
cvs_strsplit(char *str, const char *sep)
{
	struct cvs_argvector *av;
	size_t i = 0;
	char *cp, *p;

	cp = xstrdup(str);
	av = xmalloc(sizeof(*av));
	av->str = cp;
	av->argv = xmalloc(sizeof(*(av->argv)));

	while ((p = strsep(&cp, sep)) != NULL) {
		av->argv[i++] = p;
		av->argv = xreallocarray(av->argv,
		    i + 1, sizeof(*(av->argv)));
	}
	av->argv[i] = NULL;

	return (av);
}

/*
 * cvs_argv_destroy()
 *
 * Free an argument vector previously allocated by cvs_strsplit().
 */
void
cvs_argv_destroy(struct cvs_argvector *av)
{
	free(av->str);
	free(av->argv);
	free(av);
}

u_int
cvs_revision_select(RCSFILE *file, char *range)
{
	int i;
	u_int nrev;
	char *lstr, *rstr;
	struct rcs_delta *rdp;
	struct cvs_argvector *revargv, *revrange;
	RCSNUM *lnum, *rnum;

	nrev = 0;
	lnum = rnum = NULL;

	revargv = cvs_strsplit(range, ",");
	for (i = 0; revargv->argv[i] != NULL; i++) {
		revrange = cvs_strsplit(revargv->argv[i], ":");
		if (revrange->argv[0] == NULL)
			fatal("invalid revision range: %s", revargv->argv[i]);
		else if (revrange->argv[1] == NULL)
			lstr = rstr = revrange->argv[0];
		else {
			if (revrange->argv[2] != NULL)
				fatal("invalid revision range: %s",
				    revargv->argv[i]);

			lstr = revrange->argv[0];
			rstr = revrange->argv[1];

			if (strcmp(lstr, "") == 0)
				lstr = NULL;
			if (strcmp(rstr, "") == 0)
				rstr = NULL;
		}

		if (lstr == NULL)
			lstr = RCS_HEAD_INIT;

		if ((lnum = rcs_translate_tag(lstr, file)) == NULL)
			fatal("cvs_revision_select: could not translate tag `%s'", lstr);

		if (rstr != NULL) {
			if ((rnum = rcs_translate_tag(rstr, file)) == NULL)
				fatal("cvs_revision_select: could not translate tag `%s'", rstr);
		} else {
			rnum = rcsnum_alloc();
			rcsnum_cpy(file->rf_head, rnum, 0);
		}

		cvs_argv_destroy(revrange);

		TAILQ_FOREACH(rdp, &(file->rf_delta), rd_list) {
			if (rcsnum_cmp(rdp->rd_num, lnum, 0) <= 0 &&
			    rcsnum_cmp(rdp->rd_num, rnum, 0) >= 0 &&
			    !(rdp->rd_flags & RCS_RD_SELECT)) {
				rdp->rd_flags |= RCS_RD_SELECT;
				nrev++;
			}
		}

		free(lnum);
		free(rnum);
	}

	cvs_argv_destroy(revargv);

	return (nrev);
}

int
cvs_yesno(void)
{
	int c, ret;

	ret = 0;

	fflush(stderr);
	fflush(stdout);

	if ((c = getchar()) != 'y' && c != 'Y')
		ret = -1;
	else
		while (c != EOF && c != '\n')
			c = getchar();

	return (ret);
}

/*
 * cvs_exec()
 *
 * Execute <prog> and send <in> to the STDIN if not NULL.
 * If <needwait> == 1, return the result of <prog>, 
 * else, 0 or -1 if an error occur.
 */
int
cvs_exec(char *prog, char *in, int needwait)
{
	pid_t pid;
	size_t size;
	int fds[2], st;
	char *argp[4] = { "sh", "-c", prog, NULL };

	if (in != NULL && pipe(fds) == -1) {
		cvs_log(LP_ERR, "cvs_exec: pipe failed");
		return (-1);
	}

	if ((pid = fork()) == -1) {
		cvs_log(LP_ERR, "cvs_exec: fork failed");
		return (-1);
	} else if (pid == 0) {
		if (in != NULL) {
			close(fds[1]);
			dup2(fds[0], STDIN_FILENO);
		}

		setenv("CVSROOT", current_cvsroot->cr_dir, 1);
		execv(_PATH_BSHELL, argp);
		cvs_log(LP_ERR, "cvs_exec: failed to run '%s'", prog);
		_exit(127);
	}

	if (in != NULL) {
		close(fds[0]);
		size = strlen(in);
		if (atomicio(vwrite, fds[1], in, size) != size)
			cvs_log(LP_ERR, "cvs_exec: failed to write on STDIN");
		close(fds[1]);
	}

	if (needwait == 1) {
		while (waitpid(pid, &st, 0) == -1)
			;
		if (!WIFEXITED(st)) {
			errno = EINTR;
			return (-1);
		}
		return (WEXITSTATUS(st));
	}

	return (0);
}
