#include "multihome.h"


/**
 * Globals
 */
struct {
    char path_new[PATH_MAX];
    char path_old[PATH_MAX];
    char path_topdir[PATH_MAX];
    char path_root[PATH_MAX];
    char marker[PATH_MAX];
    char entry_point[PATH_MAX];
    char config_dir[PATH_MAX];
    char config_transfer[PATH_MAX];
    char config_skeleton[PATH_MAX];
    char config_init[PATH_MAX];
} multihome;

/**
 * Generic function to free an array of pointers
 * @param arr an array
 * @param nelem if nelem is 0 free until NULL. >0 free until nelem
 */
void free_array(void **arr, size_t nelem) {
    if (nelem) {
        for (size_t i = 0; i < nelem; i++) {
            free(arr[i]);
        }
    } else {
        for (size_t i = 0; arr[i] != NULL; i++) {
            free(arr[i]);
        }
    }
    free(arr);
}

/**
 * Return the count of a substring in a string
 * @param s Input string
 * @param sub Input substring
 * @return count
 */
ssize_t count_substrings(const char *s, char *sub) {
    char *str;
    char *str_orig;
    size_t str_length;
    size_t sub_length;
    size_t result;

    str = strdup(s);
    if (str == NULL) {
        return -1;
    }

    str_orig = str;
    str_length = strlen(str);
    sub_length = strlen(sub);
    result = 0;

    for (size_t i = 0; i < str_length; i++) {
        char *ptr;
        ptr = strstr(str, sub);

        if (ptr) {
            result++;
        } else {
            break;
        }

        if (i < str_length - sub_length) {
            str = ptr + sub_length;
        }
    }

    free(str_orig);
    return result;
}

/**
 * Split a string using a substring
 * @param sptr Input string
 * @param delim Substring to split on
 * @param num_alloc Address to store count of allocated records
 * @return NULL terminated array of strings
 */
char **split(const char *sptr, char *delim, size_t *num_alloc) {
    char *s;
    char *s_orig;
    char **result;
    char *token;

    token = NULL;
    result = NULL;
    s = strdup(sptr);
    s_orig = s;
    if (s == NULL) {
        return NULL;
    }

    *num_alloc = count_substrings(s, delim);
    if (*num_alloc < 0) {
        goto split_die_2;
    }

    *num_alloc += 2;
    result = calloc(*num_alloc, sizeof(char *));

    if (result == NULL) {
        goto split_die_1;
    }

    for (size_t i = 0; (token = strsep(&s, delim)) != NULL; i++) {
        result[i] = strdup(token);
        if (result[i] == NULL) {
            break;
        }
    }

split_die_1:
    free(s_orig);
split_die_2:
    return result;
}

/**
 * Create directories if they do not exist
 * @param path Filesystem path
 * @return int (0=success, -1=error (errno set))
 */
int mkdirs(char *path) {
    char **parts;
    char tmp[PATH_MAX];
    size_t parts_length;
    memset(tmp, '\0', sizeof(tmp));

    parts = split(path, "/", &parts_length);

    for (size_t i = 0; parts[i] != NULL; i++) {
        if (i == 0 && strlen(parts[i]) == 0) {
            continue;
        }
        strcat(tmp, parts[i]);
        if (tmp[strlen(tmp) - 1] != '/') {
            strcat(tmp, "/");
        }

        if (access(tmp, F_OK) == 0) {
            continue;
        }

        if (mkdir(tmp, (mode_t) 0755) < 0) {
            perror("mkdir");
            return -1;
        }
    }

    free_array((void **)parts, parts_length);
    return 0;
}

/**
 * Execute a shell program
 * @param args (char *[]){"/path/to/program", "arg1", "arg2, ..., NULL};
 * @return exit code of program
 */
int shell(char *args[]){
    pid_t pid;
    pid_t status;

    status = 0;
    errno = 0;

    pid = fork();
    if (pid == -1) {
        fprintf(stderr, "fork failed\n");
        exit(1);
    } else if (pid == 0) {
        int retval;
        retval = execv(args[0], &args[0]);
        exit(retval);
    } else {
        if (waitpid(pid, &status, WUNTRACED) > 0) {
            if (WIFEXITED(status) && WEXITSTATUS(status)) {
                if (WEXITSTATUS(status) == 127) {
                    fprintf(stderr, "execvp failed\n");
                    exit(1);
                }
            } else if (WIFSIGNALED(status))  {
                fprintf(stderr, "signal received: %d\n", WIFSIGNALED(status));
            }
        } else {
            fprintf(stderr, "waitpid() failed\n");
        }
    }
    return WEXITSTATUS(status);
}

/**
 * Copy files using rsync
 * @param source file or directory
 * @param dest file or directory
 * @return rsync exit code
 */
int copy(char *source, char *dest) {
    if (source == NULL || dest == NULL) {
        fprintf(stderr, "copy failed. source and destination may not be NULL\n");
        exit(1);
    }

    return shell((char *[]){RSYNC_BIN, RSYNC_ARGS, source, dest, NULL});
}

/**
 * Create or update the modified time on a file
 * @param filename path to file
 * @return 0=success, -1=error (errno set)
 */
int touch(char *filename) {
    FILE *fp;
    fp = fopen(filename, "w");
    fflush(fp);
    if (fp == NULL) {
        return -1;
    }
    fclose(fp);
    return 0;
}

/**
 * Generate multihome initialization script
 */
void write_init_script() {
    const char *script_block = "#\n# This script was generated on %s\n#\n\n"
        "MULTIHOME=%s\n"
        "if [ -x $MULTIHOME ]; then\n"
        "    HOME_OLD=$HOME\n"
        "    # Redeclare HOME\n"
        "    HOME=$($MULTIHOME)\n"
        "    if [ \"$HOME\" != \"$HOME_OLD\" ]; then\n"
        "        cd $HOME\n"
        "    fi\n"
        "fi\n";
    char buf[PATH_MAX];
    char date[100];
    struct tm *tm;
    time_t now;
    FILE *fp;

    // Determine the absolute path of this program
    if (realpath(multihome.entry_point, buf) < 0) {
        perror(multihome.entry_point);
        exit(errno);
    }

    // Open init script for writing
    fp = fopen(multihome.config_init, "w+");
    if (fp == NULL) {
        perror(multihome.config_init);
        exit(errno);
    }

    // Generate header timestamp
    time(&now);
    tm = localtime(&now);
    sprintf(date, "%02d-%02d-%d @ %02d:%02d:%02d",
            tm->tm_mon + 1, tm->tm_mday, tm->tm_year + 1900,
            tm->tm_hour, tm->tm_min, tm->tm_sec);

    // Write init script
    fprintf(fp, script_block, date, buf);
    fclose(fp);
}

/**
 * Link or copy files from /home/username to /home/username/home_local/nodename
 */
void user_transfer() {
    FILE *fp;
    char rec[PATH_MAX];
    size_t lineno;

    memset(rec, '\0', PATH_MAX);

    fp = fopen(multihome.config_transfer, "r");
    if (fp == NULL) {
        // doesn't exist or isn't readable. non-fatal.
        return;
    }

    // FORMAT:
    // TYPE WHERE
    //
    // TYPE:
    // L = SYMBOLIC LINK
    // H = HARD LINK
    // T = TRANSFER (file, directory, etc)
    //
    // EXAMPLE:
    // L .Xauthority
    // L .ssh
    // H token.asc
    // T special_dotfiles/

    lineno = 0;
    while (fgets(rec, PATH_MAX - 1, fp) != NULL) {
        char *recptr;
        char source[PATH_MAX];
        char dest[PATH_MAX];

        recptr = rec;

        // Ignore: comments and inline comments
        char *comment;
        if (*recptr == '#') {
            continue;
        } else if ((comment = strstr(recptr, "#")) != NULL) {
            comment--;
            for (; comment != NULL && isblank(*comment) && comment > recptr; comment--) {
                *comment = '\0';
            }
        }

        // Ignore: bad lines without enough information
        if (strlen(rec) < 3) {
            fprintf(stderr, "%s:%zu: Invalid format: %s\n", multihome.config_transfer, lineno, rec);
            continue;
        }

        recptr = &rec[2];

        if (*recptr == '/') {
            fprintf(stderr, "%s:%zu: Removing leading '/' from: %s\n", multihome.config_transfer, lineno, recptr);
            memmove(recptr, recptr + 1, strlen(recptr) + 1);
        }

        if (recptr[strlen(recptr) - 1] == '\n') {
            recptr[strlen(recptr) - 1] = '\0';
        }

        // construct data source path
        sprintf(source, "%s/%s", multihome.path_old, recptr);

        // construct data destination path
        char *tmp;
        tmp = strdup(source);
        sprintf(dest, "%s/%s", multihome.path_new, basename(tmp));
        free(tmp);

        switch (rec[0]) {
            case 'L':
                if (symlink(source, dest) < 0) {
                    fprintf(stderr, "symlink: %s: %s -> %s\n", strerror(errno), source, dest);
                }
                break;
            case 'H':
                if (link(source, dest) < 0) {
                    fprintf(stderr, "hardlink: %s: %s -> %s\n", strerror(errno), source, dest);
                }
                break;
            case 'T':
                if (copy(source, dest) != 0) {
                    fprintf(stderr, "transfer: %s: %s -> %s\n", strerror(errno), source, dest);
                }
                break;
            default:
                fprintf(stderr, "%s:%zu: Invalid type: %c\n", multihome.config_transfer, lineno, rec[0]);
                break;
        }
    }
    fclose(fp);
}

void test_split() {
    puts("split()");
    char **result;
    size_t result_alloc;

    result = split("one two three", " ", &result_alloc);
    assert(strcmp(result[0], "one") == 0 && strcmp(result[1], "two") == 0 && strcmp(result[2], "three") == 0);
    assert(result_alloc != 0);
    free_array((void *)result, result_alloc);
}

void test_count_substrings() {
    puts("count_substrings()");
    size_t result;
    result = count_substrings("one two three", " ");
    assert(result == 2);
}

void test_mkdirs() {
    puts("mkdirs()");
    int result;
    char *input = "this/is/a/test";

    if (access(input, F_OK) == 0) {
        assert(remove("this/is/a/test") == 0);
        assert(remove("this/is/a") == 0);
        assert(remove("this/is") == 0);
        assert(remove("this") == 0);
    }

    result = mkdirs(input);
    assert(result == 0);
    assert(access(input, F_OK) == 0);
}

void test_shell() {
    puts("shell()");
    assert(shell((char *[]){"/bin/echo", "testing", NULL}) == 0);
    assert(shell((char *[]){"/bin/date", NULL}) == 0);
    assert(shell((char *[]){"/bin/unlikelyToExistAnywhere", NULL}) != 0);
}

void test_touch() {
    puts("touch()");
    char *input = "touched_file.txt";

    if (access(input, F_OK) == 0) {
        remove(input);
    }

    assert(touch(input) == 0);
    assert(access(input, F_OK) == 0);
}

void test_main() {
    test_count_substrings();
    test_split();
    test_mkdirs();
    test_shell();
    test_touch();
    exit(0);
}

// begin argp setup
static char doc[] = "Partition a home directory per-host when using a centrally mounted /home";
static char args_doc[] = "";
static struct argp_option options[] = {
    {"script", 's', 0, 0, "Generate runtime script"},
#ifdef ENABLE_TESTING
    {"tests", 't', 0, 0, "Run unit tests"},
#endif
    {"version", 'V', 0, 0, "Show version and exit"},
    {0},
};

struct arguments {
    int script;
    int version;
    int testing;
};

static error_t parse_opt (int key, char *arg, struct argp_state *state) {
    struct arguments *arguments = state->input;
    (void) arg; // arg is not used

    switch (key) {
        case 'V':
            arguments->version = 1;
            break;
        case 's':
            arguments->script = 1;
            break;
        case 't':
            arguments->testing = 1;
            break;
        case ARGP_KEY_ARG:
            if (state->arg_num > 1) {
                argp_usage(state);
            }
            break;
        default:
            return ARGP_ERR_UNKNOWN;
    }
    return 0;
}

static struct argp argp = { options, parse_opt, args_doc, doc };
// end of argp setup

int main(int argc, char *argv[]) {
    uid_t uid;
    struct passwd *user_info;
    struct utsname host_info;

    // Disable line buffering via macro
    DISABLE_BUFFERING

    struct arguments arguments;
    arguments.script = 0;
    arguments.version = 0;
    arguments.testing = 0;
    argp_parse(&argp, argc, argv, 0, 0, &arguments);

    if (arguments.version) {
        puts(VERSION);
        exit(0);
    }

    // Refuse to operate if RSYNC_BIN is not available
    if (access(RSYNC_BIN, F_OK) < 0) {
        fprintf(stderr, "rsync program not found (expecting: %s)\n", RSYNC_ARGS);
        return 1;
    }

#ifdef ENABLE_TESTING
    if (arguments.testing) {
        test_main();
        exit(0);
    }
#endif
    // Get account name for the effective user
    uid = geteuid();
    if ((user_info = getpwuid(uid)) == NULL) {
        perror("getpwuid");
        return errno;
    }

    // Get host information
    if (uname(&host_info) < 0) {
        perror("uname");
        return errno;
    }

    // Determine the user's home directory
    char *path_old;
    path_old = getenv("HOME");

    // Handle legitimate case where HOME is undefined. Use the system's records instead...
    // i.e. The user wiped the environment with `env -i` prior to executing multihome
    if (path_old == NULL) {
        path_old = user_info->pw_dir;
        if (path_old == NULL) {
            fprintf(stderr, "Unable to determine home directory path\n");
            return 1;
        }
    }

    // Populate multihome struct
    strcpy(multihome.entry_point, argv[0]);
    strcpy(multihome.path_old, path_old);
    strcpy(multihome.path_root, MULTIHOME_ROOT);
    sprintf(multihome.config_dir, "%s/.multihome", multihome.path_old);
    sprintf(multihome.config_init, "%s/init", multihome.config_dir);
    sprintf(multihome.config_skeleton, "%s/skel/", multihome.config_dir);
    sprintf(multihome.path_new, "%s/%s/%s", multihome.path_old, multihome.path_root, host_info.nodename);
    sprintf(multihome.path_topdir, "%s/topdir", multihome.path_new);
    sprintf(multihome.marker, "%s/.multihome_controlled", multihome.path_new);

    // Refuse to operate within a controlled home directory
    char already_inside[PATH_MAX];
    sprintf(already_inside, "%s/.multihome_controlled", multihome.path_old);
    if (access(already_inside, F_OK) == 0) {
        fprintf(stderr, "error: multihome cannot be nested.\n");
        return 1;
    }

    // Create new home directory
    if (strcmp(multihome.path_new, multihome.path_old) != 0) {
        if (access(multihome.path_new, F_OK) < 0) {
            fprintf(stderr, "Creating home directory: %s\n", multihome.path_new);
            if (mkdirs(multihome.path_new) < 0) {
                perror(multihome.path_new);
                return errno;
            }
        }
    }

    // Generate symbolic link within the new home directory pointing back to the real account home directory
    if (access(multihome.path_topdir, F_OK) != 0 ) {
        fprintf(stderr, "Creating symlink to original home directory: %s\n", multihome.path_topdir);
        if (symlink(multihome.path_old, multihome.path_topdir) < 0) {
            perror(multihome.path_topdir);
            return errno;
        }
    }

    // Generate directory for user-defined account defaults
    // Files placed here will be copied to the new home directory.
    if (access(multihome.config_skeleton, F_OK) < 0) {
        fprintf(stderr, "Creating user skel directory: %s\n", multihome.config_skeleton);
        if (mkdirs(multihome.config_skeleton) < 0) {
            perror(multihome.config_skeleton);
            return errno;
        }
    }

    if (access(multihome.marker, F_OK) < 0) {
        // Copy system account defaults
        fprintf(stderr, "Injecting account skeleton: %s\n", OS_SKEL_DIR);
        copy(OS_SKEL_DIR, multihome.path_new);

        // Copy user-defined account defaults
        fprintf(stderr, "Injecting user-defined account skeleton: %s\n", multihome.config_skeleton);
        copy(multihome.config_skeleton, multihome.path_new);

        // Transfer or link user-defined files into the new home
        fprintf(stderr, "Parsing transfer configuration, if present\n");
        user_transfer();
    }

    // Leave our mark: "multihome was here"
    if (access(multihome.marker, F_OK) < 0) {
        fprintf(stderr, "Creating marker file: %s\n", multihome.marker);
        touch(multihome.marker);
    }

    if (arguments.script) {
        write_init_script();
    } else {
        printf("%s\n", multihome.path_new);
    }
}

