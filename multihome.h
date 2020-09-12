#ifndef MULTIHOME_MULTIHOME_H
#define MULTIHOME_MULTIHOME_H

#ifdef ENABLE_TESTING
#include <assert.h>
#endif

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <pwd.h>
#include <sys/utsname.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <libgen.h>
#include <wait.h>
#include <argp.h>
#include <time.h>
#include "config.h"

#define VERSION "0.0.1"
#define MULTIHOME_ROOT "home_local"
#define MULTIHOME_TOPDIR "topdir"
#define MULTIHOME_CFGDIR ".multihome"
#define MULTIHOME_CFG_TRANSFER "transfer"
#define MULTIHOME_CFG_INIT "init"
#define MULTIHOME_CFG_SKEL "skel/"  // NOTE: Trailing slash is required
#define MULTIHOME_MARKER ".multihome_controlled"
#define OS_SKEL_DIR "/etc/skel/"    // NOTE: Trailing slash is required
#define RSYNC_ARGS "-aq"
#define COPY_NORMAL 0
#define COPY_UPDATE 1

#define DISABLE_BUFFERING \
    setvbuf(stdout, NULL, _IONBF, 0); \
    setvbuf(stderr, NULL, _IONBF, 0);

void free_array(void **arr, size_t nelem);
ssize_t count_substrings(const char *s, char *sub);
char **split(const char *sptr, char *delim, size_t *num_alloc);
char *find_program(const char *_name);
int shell(char *args[]);
int mkdirs(char *path);
int copy(char *source, char *dest, int mode);
int touch(char *filename);
char *get_timestamp(char **result);
void write_init_script();
void user_transfer(int copy_mode);
char *strip_domainname(char *hostname);

#endif //MULTIHOME_MULTIHOME_H
