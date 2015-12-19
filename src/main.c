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

#include <fcntl.h>
#include <fts.h>
#include <limits.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <unistd.h>

#include <alpm.h>

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof(arr[0]))

static const char *const ANSI_BOLD      = "\e[1m";
static const char *const ANSI_RED       = "\e[31m";
static const char *const ANSI_GREEN     = "\e[32m";
static const char *const ANSI_YELLOW    = "\e[33m";
static const char *const ANSI_BLUE      = "\e[34m";
static const char *const ANSI_BOLD_BLUE = "\e[1;34m";
static const char *const ANSI_RESET     = "\e[m";

struct unix_file {
	char *data;
	size_t size;
};

static const char TEMPLATE_HEADER_DATA[] = "//! TEMPLATE\n";
static const size_t TEMPLATE_HEADER_SIZE = ARRAY_SIZE(TEMPLATE_HEADER_DATA) - 1;

int unix_file_open(struct unix_file *unix_file, const char *path)
{
	if (unix_file == NULL)
		return 1;

	int fd = open(path, O_RDONLY);
	if (fd == -1)
		return 1;

	struct stat stat;
	if (fstat(fd, &stat) == -1) {
		close(fd);
		return 1;
	}
	size_t size = stat.st_size;

	char *data = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
	if (data == MAP_FAILED) {
		close(fd);
		return 1;
	}
	close(fd);

	unix_file->data = data;
	unix_file->size = size;
	return 0;
}

int unix_file_close(struct unix_file *unix_file)
{
	if (unix_file == NULL || unix_file->data == NULL)
		return 1;

	munmap(unix_file->data, unix_file->size);

	unix_file->data = NULL;
	unix_file->size = 0;
	return 0;
}

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
			return;
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
bool is_package_installed(alpm_handle_t *alpm_handle, const char *name)
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

struct template_key_value {
	const char *key_data;
	size_t key_size;
	const char *value_data;
	size_t value_size;
};

static
bool template_is_valid(const char *data,
                       size_t size)
{
	if (size <= TEMPLATE_HEADER_SIZE)
		return false;
	if (strncmp(data, TEMPLATE_HEADER_DATA, TEMPLATE_HEADER_SIZE) != 0)
		return false;
	return true;
}

static
bool are_buffers_same(const void *x_data, size_t x_size,
                      const void *y_data, size_t y_size)
{
	if (x_size != y_size)
		return false;
	return memcmp(x_data, y_data, x_size) == 0;
}

static
int basic_process(const char *path,
                  const char *data,
                  size_t size)
{
	char home_path[PATH_MAX];
	const char *home_dir = getenv("HOME");
	strcpy(home_path, home_dir);
	strcat(home_path, "/.");
	strcat(home_path, path);

	int ret;
	struct unix_file dest_file;
	ret = unix_file_open(&dest_file, home_path);
	if (ret == 0) {
		if (are_buffers_same(data, size,
		                     dest_file.data, dest_file.size)) {
			printf("%s'%s' match%s\n",
				ANSI_GREEN, home_path, ANSI_RESET);
		}
		else {
			printf("%s'%s' doesn't match%s\n",
				ANSI_YELLOW, home_path, ANSI_RESET);
		}
		ret = unix_file_close(&dest_file);
		return ret;
	}

	return 0;
}

static
int template_process(const char *path,
                     const char *data,
                     size_t size,
                     struct template_key_value *template_store,
                     size_t template_size)
{
	return 0;
}

static
int check_user_base(const char *base_directory,
                    struct unix_file *template_file,
                    alpm_handle_t *alpm_handle)
{
	bool template_file_valid = template_file != NULL;
	struct template_key_value template_store[128];
	size_t template_size = 0;

	if (template_file_valid) {
		uint8_t state = 0;
		const char *tmp;
		for (size_t i = 0; i < template_file->size; ++i) {
			const char *current = template_file->data + i;
			char c = *current;
			if (state == 0) {
				template_store[template_size].key_data = current;
				tmp = current;
				state = 1;
			}
			else if (state == 1 && c == '=') {
				template_store[template_size].key_size =
					current - tmp;
				state = 2;
			}
			else if (state == 2) {
				template_store[template_size].value_data =
					current;
				tmp = current;
				state = 3;
			}
			else if (state == 3 && c == '\n') {
				template_store[template_size].value_size =
					current - tmp;
				++template_size;
				state = 0;
			}
		}
		if (state != 0) {
			/* Parse error */
			return 2;
		}
	}

	const char *const paths[2] = {base_directory, NULL};
	FTS *fts = fts_open((char * const*)(paths), FTS_PHYSICAL, NULL);
	if (fts == NULL)
		return 1;

	FTSENT *ftsent;
	uint32_t depth = 0;
	size_t base_directory_length = strlen(base_directory) + 1;
	size_t ignore_length = 0;
	int ret;
	while ((ftsent = fts_read(fts))) {
		if (strcmp(ftsent->fts_path, "/etc/ImageMagick-6") == 0) {
			fts_set(fts, ftsent, FTS_SKIP);
		}

		const char* path = ftsent->fts_path;
		struct unix_file configuration_file;
		switch (ftsent->fts_info) {
		case FTS_D:
			++depth;
			if (depth == 2) {
				const char* name =
					path + base_directory_length;
				if (!is_package_installed(alpm_handle, name)) {
					printf("%s'%s' not installed %s\n",
						ANSI_RED, name, ANSI_RESET);
					fts_set(fts, ftsent, FTS_SKIP);
				}
				else {
					ignore_length = strlen(path) + 1;
				}
			}
			break;
		case FTS_DP:
			--depth;
			break;
		case FTS_F:
			if (depth <= 1)
				break;

			ret = unix_file_open(&configuration_file, path);
			if (ret != 0) {
				printf("%s'%s' not used%s\n",
					ANSI_YELLOW,
					path + ignore_length,
					ANSI_RESET);
				ret = 0;
				break;
			}

			if (template_is_valid(configuration_file.data,
			                      configuration_file.size)) {
				ret = template_process(
					path + ignore_length,
					configuration_file.data
						+ TEMPLATE_HEADER_SIZE,
					configuration_file.size,
					template_store, template_size);
			}
			else {
				ret = basic_process(
					path + ignore_length,
					configuration_file.data,
					configuration_file.size);
			}
			printf("%s'%s' up to date%s\n",
				ANSI_GREEN,
				path + ignore_length,
				ANSI_RESET);
			unix_file_close(&configuration_file);
			break;
		case FTS_DNR:
		case FTS_ERR:
		case FTS_NS:
			printf("%sFilesystem error%s\n", ANSI_RED, ANSI_RESET);
			return EXIT_FAILURE;
		}

		if (ret != 0)
			break;
	}

	fts_close(fts);

	return ret;
}

static
int check_user(const char *check_directory,
               const char *hostname,
               alpm_handle_t *alpm_handle)
{
	char user_directory[PATH_MAX];
	strcpy(user_directory, check_directory);
	if (user_directory[strlen(user_directory) - 1] != '/')
		strncat(user_directory, "/", PATH_MAX);
	strncat(user_directory, "user", PATH_MAX);
	if (!is_directory(check_directory)) {
		printf("%sCheck requires a 'user' directory%s\n",
			ANSI_RED, ANSI_RESET);
		return 1;
	}

	char host_directory[PATH_MAX];
	strcpy(host_directory, user_directory);
	strncat(host_directory, "/", PATH_MAX);
	strncat(host_directory, hostname, PATH_MAX);
	if (!is_directory(host_directory)) {
		printf("%sCheck requires a 'user/%s' directory%s\n",
			ANSI_RED, hostname, ANSI_RESET);
		return 1;
	}

	char base_directory[PATH_MAX];
	strcpy(base_directory, user_directory);
	strncat(base_directory, "/base", PATH_MAX);
	bool base_directory_valid = is_directory(base_directory);
	if (base_directory_valid) {
		printf("%sCheck found 'user/base' directory%s\n",
			ANSI_BLUE, ANSI_RESET);
	}

	struct unix_file template_file;
	bool template_file_valid;
	{
		char template_path[PATH_MAX];
		strcpy(template_path, host_directory);
		strncat(template_path, "/TEMPLATE", PATH_MAX);
		int ret;
		ret = unix_file_open(&template_file, template_path);
		template_file_valid = ret == 0;
	}

	if (base_directory_valid) {
		if (template_file_valid)
			check_user_base(
				base_directory, &template_file, alpm_handle);
		else
			check_user_base(
				base_directory, NULL, alpm_handle);
	}

	if (template_file_valid)
		unix_file_close(&template_file);

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
