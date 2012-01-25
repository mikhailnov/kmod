#include <assert.h>
#include <errno.h>
#include <dlfcn.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "testsuite.h"

static void *nextlib;
static const char *rootpath;
static size_t rootpathlen;

static inline bool need_trap(const char *path)
{
	return path != NULL && path[0] == '/';
}

static const char *trap_path(const char *path, char buf[PATH_MAX * 2])
{
	size_t len;

	if (!need_trap(path))
		return path;

	len = strlen(path);

	if (len + rootpathlen > PATH_MAX * 2) {
		errno = ENAMETOOLONG;
		return NULL;
	}

	memcpy(buf, rootpath, rootpathlen);
	strcpy(buf + rootpathlen, path);
	return buf;
}

static bool get_rootpath(const char *f)
{
	if (rootpath != NULL)
		return true;

	rootpath = getenv(S_TC_ROOTFS);
	if (rootpath == NULL) {
		ERR("TRAP %s(): missing export %s?\n", f, S_TC_ROOTFS);
		errno = ENOENT;
		return false;
	}

	rootpathlen = strlen(rootpath);

	return true;
}

static void *get_libc_func(const char *f)
{
	void *fp;

	if (nextlib == NULL) {
#ifdef RTLD_NEXT
		nextlib = RTLD_NEXT;
#else
		nextlib = dlopen("libc.so.6", RTLD_LAZY);
#endif
	}

	fp = dlsym(nextlib, f);
	assert(fp);

	return fp;
}

TS_EXPORT FILE *fopen(const char *path, const char *mode)
{
	const char *p;
	char buf[PATH_MAX * 2];
	static int (*_fopen)(const char *path, const char *mode);

	if (!get_rootpath(__func__))
		return NULL;

	_fopen = get_libc_func("fopen");

	p = trap_path(path, buf);
	if (p == NULL)
		return NULL;

	return (void *) (long) (*_fopen)(p, mode);
}