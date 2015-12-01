/*
 * Copyright 2015 Jonathan Eyolfson
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <string.h>
#include <stdbool.h>
#include <stdio.h>

#include <fts.h>
#include <limits.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <alpm.h>

static const char * const ANSI_BOLD      = "\e[1m";
static const char * const ANSI_RED       = "\e[31m";
static const char * const ANSI_BLUE      = "\e[34m";
static const char * const ANSI_BOLD_BLUE = "\e[1;34m";
static const char * const ANSI_RESET     = "\e[m";

void check_ownership()
{
	const char *const paths[3] = {"/etc", "/usr", NULL};
	FTS *fts = fts_open((char * const*)(paths), FTS_PHYSICAL, NULL);

	FTSENT *ftsent;
	while ((ftsent = fts_read(fts))) {
		if (strcmp(ftsent->fts_path, "/etc/ImageMagick-6") == 0) {
			fts_set(fts, ftsent, FTS_SKIP);
		}

		switch (ftsent->fts_info) {
		case FTS_D:
			printf("PRE ");
			break;
		case FTS_DP:
			printf("POST ");
			break;
		case FTS_DNR:
		case FTS_ERR:
		case FTS_NS:
			printf("%sFilesystem error%s\n", ANSI_RED, ANSI_RESET);
			return EXIT_FAILURE;
		}
		printf("%s\n", ftsent->fts_path);
	}

	fts_close(fts);
}

static
bool is_directory(const char *const dir)
{
	struct stat buf;
	if (stat(dir, &buf) == 0 && S_ISDIR(buf.st_mode))
		return true;
	return false;
}

static
bool is_package_installed(alpm_handle_t *alpm_handle, char *name)
{
	alpm_db_t *alpm_db = alpm_get_localdb(alpm_handle);
	alpm_list_t *i;
	for(i = alpm_db_get_pkgcache(alpm_db); i; i = alpm_list_next(i)) {
		alpm_pkg_t *pkg = (alpm_pkg_t *)(i->data);
		if (strcmp(name, alpm_pkg_get_name(pkg)) == 0) {
			return true;
		}
	}
	return false;
}

typedef enum { SYSTEM, USER } check_mode_t;

static
int check_system(const char *check_directory,
                 const char *hostname,
                 alpm_handle_t *alpm_handle)
{
	printf("xorg-server ");
	if (!is_package_installed(alpm_handle, "xorg-server")) {
		printf("not ");
	}
	printf("installed\n");
	return 0;
}

static
int check_user(const char *check_directory,
               const char *hostname,
               alpm_handle_t *alpm_handle)
{
	char user_directory[PATH_MAX];
	strcpy(user_directory, check_directory);
	strcat(user_directory, "/user");

	if (!is_directory(check_directory)) {
		printf("%sCheck requires a 'user' directory%s\n",
			ANSI_RED, ANSI_RESET);
		return 1;
	}

	char host_directory[PATH_MAX];
	strcpy(host_directory, user_directory);
	strcat(host_directory, "/");
	strcat(host_directory, hostname);
	if (!is_directory(host_directory)) {
		printf("%sCheck requires a 'user/%s' directory%s\n",
			ANSI_RED, hostname, ANSI_RESET);
		return 1;
	}

	return 0;
}

int main(int argc, const char * * argv)
{
	printf("%sConfiguration Check%s %s0.1.0-development%s\n",
		ANSI_BOLD_BLUE, ANSI_RESET, ANSI_BOLD, ANSI_RESET);

	if (argc < 2) {
		printf("%sCheck requires an argument%s\n",
			ANSI_RED, ANSI_RESET);
		return 1;
	}
	const char *const check_directory = argv[1];
	if (!is_directory(check_directory)) {
		printf("%sCheck requires a directory%s\n",
			ANSI_RED, ANSI_RESET);
		return 1;
	}

	check_mode_t check_mode;
	if (getuid() == 0)
		check_mode = SYSTEM;
	else
		check_mode = USER;

	char hostname[64];
	if (gethostname(hostname, 64) != 0) {
		return 1;
	}

	alpm_errno_t alpm_errno = 0;
	alpm_handle_t *alpm_handle = alpm_initialize(
		"/", "/var/lib/pacman", &alpm_errno);
	if (alpm_errno) {
		printf("%s[ERROR] pacman:%s %s\n",
			ANSI_RED, ANSI_RESET, alpm_strerror(alpm_errno));
		return 1;
	}

	int ret;
	if (check_mode == SYSTEM) {
		ret = check_system(check_directory, hostname, alpm_handle);
	}
	else if (check_mode == USER) {
		ret = check_user(check_directory, hostname, alpm_handle);
	}

	if (alpm_release(alpm_handle) != 0) {
		return 1;
	}

	return ret;
}
