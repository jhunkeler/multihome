//
// Created by jhunk on 8/26/20.
//

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
#define OS_SKEL_DIR "/etc/skel/"  // NOTE: Trailing slash is required
#define RSYNC_ARGS "-aq"

#define DISABLE_BUFFERING \
    setvbuf(stdout, NULL, _IONBF, 0); \
    setvbuf(stderr, NULL, _IONBF, 0);

#endif //MULTIHOME_MULTIHOME_H
