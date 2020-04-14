/*
 * Public domain
 */

#include <grp.h>
#include <pwd.h>

#include "openbsd.h"

int
gid_from_group(const char *name, gid_t *gid)
{
	struct group *grp;

	if ((grp = getgrnam(name)) == NULL)
		return (-1);

	*gid = grp->gr_gid;

	return 0;
}

const char *
group_from_gid(gid_t gid, int nogroup)
{
	if (getgrgid(gid) != NULL)
		return (getgrgid(gid)->gr_name);
	return (getgrgid(nogroup)->gr_name);
}

int
uid_from_user(const char *name, uid_t *uid)
{
	struct passwd *pw;

	if ((pw = getpwnam(name)) == NULL)
		return (-1);

	*uid = pw->pw_uid;

	return 0;
}

const char *
user_from_uid(uid_t uid, int nouser)
{
	if (getpwuid(uid) != NULL)
		return (getpwuid(uid)->pw_name);
	return (getpwuid(nouser)->pw_name);
}
