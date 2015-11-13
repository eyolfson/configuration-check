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
#include <stdio.h>

#include <fts.h>
#include <sys/types.h>
#include <unistd.h>

#include <alpm.h>

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

int main(int argc, const char * * argv)
{
	printf("%sConfiguration Check 0.1.0-development%s\n", ANSI_BOLD_BLUE,
		ANSI_RESET);

	if (getuid() != 0) {
		printf("%sMust be run as root%s\n", ANSI_RED, ANSI_RESET);
		return 1;
	}

	char hostname[64];
	if (gethostname(hostname, 64) != 0) {
		return 1;
	}
	printf("HOSTNAME: %s\n", hostname);

	alpm_errno_t alpm_errno;
	alpm_handle_t *alpm_handle = alpm_initialize("/", "/var/lib/pacman",
		&alpm_errno);
	alpm_db_t *alpm_db = alpm_get_localdb(alpm_handle);

	alpm_list_t *i;
	for(i = alpm_db_get_pkgcache(alpm_db); i; i = alpm_list_next(i)) {
		alpm_pkg_t *pkg = (alpm_pkg_t *)(i->data);
		printf("PACKAGE: %s\n", alpm_pkg_get_name(pkg));
	}

	if (alpm_release(alpm_handle) != 0) {
		return 1;
	}

	return 0;
}
