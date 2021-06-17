#include "multihome.h"

#ifdef ENABLE_TESTING
#include "tests.h"
#endif


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
    char config_host_group[PATH_MAX];
    char config_transfer[PATH_MAX];
    char config_skeleton[PATH_MAX];
    char scripts_dir[PATH_MAX];
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
    ssize_t result;

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
 * Using $PATH, return the location of _name
 *
 * If _name starts with "./" return the absolute path to the file
 *
 * This function returns local storage
 *
 * @param _name program name
 * @return path, or NULL if not found
 */
char *find_program(const char *_name) {
    static char buf[PATH_MAX];
    char *pathvar;
    char **parts;
    size_t parts_count;
    int found;

    pathvar = getenv("PATH");
    if (pathvar == NULL) {
        return NULL;
    }

    parts = split(pathvar, ":", &parts_count);
    if (parts == NULL) {
        return NULL;
    }

    memset(buf, '\0', sizeof(buf));
    found = 0;

    // Return the absolute path of _name when:
    // 1) Path starts with "./" (absolute)
    // 2) Path starts with "/" (absolute)
    // 3) Path contains "/" (relative)
    //
    // Obviously strstr will do this job all by itself, however it's like watching slow scan TV.
    if (strncmp(_name, "./", 2) == 0 || strncmp(_name, "/", 1) == 0 || strstr(_name, "/") != NULL) {
        if (access(_name, F_OK) == 0) {
            found = 1;
            realpath(_name, buf);
        }
    } else {
        for (int i = 0; parts[i] != NULL; i++) {
            char tmp[PATH_MAX];
            memset(tmp, '\0', sizeof(tmp));
            strcat(tmp, parts[i]);
            strcat(tmp, "/");
            strcat(tmp, _name);
            if (access(tmp, F_OK) == 0) {
                found = 1;
                realpath(tmp, buf);
                break;
            }
        }
    }

    free_array((void**)parts, parts_count);
    return found ? buf : NULL;
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
            strcat(tmp, "/");
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
int shell(char *args[]) {
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
 * @param mode 0=direct copy, 1=update only
 * @return rsync exit code
 */
int copy(char *source, char *dest, int mode) {
    if (source == NULL || dest == NULL) {
        fprintf(stderr, "copy failed. source and destination may not be NULL\n");
        exit(1);
    }

    char args[255];
    memset(args, '\0', sizeof(args));
    strcat(args, RSYNC_ARGS);
    if (mode == COPY_UPDATE) {
        strcat(args, "u");
    }

    return shell((char *[]){MULTIHOME_RSYNC_BIN, args, source, dest, NULL});
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
 * Get date and time as a string
 *
 * @return pointer to buffer containing date and time (do not free())
 */
char *get_timestamp() {
    static char result[100];
    struct tm *tm;
    time_t now;

    // Get current time
    time(&now);

    // Convert current time to tm struct
    tm = localtime(&now);

    // Write result to buffer
    memset(result, '\0', sizeof(result));
    sprintf(result, "%02d-%02d-%d @ %02d:%02d:%02d",
            tm->tm_mon + 1, tm->tm_mday, tm->tm_year + 1900,
            tm->tm_hour, tm->tm_min, tm->tm_sec);

    return result;
}

/**
 * Generate multihome initialization scripts
 */
void write_init_script() {
    char buf[BUFSIZ];
    DIR *d;
    struct dirent *rec;
    char *date;

    memset(buf, '\0', sizeof(buf));

    d = opendir(multihome.scripts_dir);
    if (!d) {
        perror(multihome.scripts_dir);
        exit(1);
    }
    while ((rec = readdir(d)) != NULL) {
        if (rec->d_type == DT_REG && strstr(rec->d_name, "init.")) {
            FILE *fp_input;
            FILE *fp_output;
            char *script_name;
            char path_input[PATH_MAX];
            char path_output[PATH_MAX];

            script_name = basename(rec->d_name);
            sprintf(path_input, "%s/%s", multihome.scripts_dir, script_name);
            fp_input = fopen(path_input, "r");
            if (!fp_input) {
                perror(rec->d_name);
                exit(1);
            }
            memset(buf, '\0', sizeof(buf));
            fread(buf, sizeof(char), sizeof(buf) - 1, fp_input);
            fclose(fp_input);

            sprintf(path_output, "%s/%s", multihome.config_dir, script_name);
            fp_output = fopen(path_output, "w+");
            if (!fp_output) {
                perror(path_output);
                exit(1);
            }
            date = get_timestamp();
            fprintf(fp_output, "# Version: %s\n", VERSION);
            fprintf(fp_output, "# Generated: %s\n\n", date);
            fprintf(fp_output, buf, multihome.entry_point);
            fclose(fp_output);
        }
    }
    closedir(d);
}

/**
 * End string on first occurrence of LF or whitespace
 * @param str
 * @return non-zero if LF not found
 */
static int strip(char **str) {
    char *orig;
    char *tmp;
    char *result;

    orig = (*str);
    tmp = NULL;
    result = strchr((*str), '\n');
    if (result) {
        *result = '\0';
    }

    tmp = (*str);
    while(tmp) {
        if (isblank(*tmp)) {
            tmp++;
            continue;
        }
        break;
    }
    size_t size;
    size_t len;
    size = tmp - (*str);
    len = strlen(tmp);

    memmove(orig, tmp, len);
    len = strlen(orig) - size;
    *((*str) + len) = '\0';

    result = strrchr((*str), ' ');
    if (result) {
        *result = '\0';
    }

    return 0;
}

/**
 * Read and apply transformations defined by the host_group configuration file
 *
 * FORMAT:
 *     # Comment
 *     HOST_PATTERN = COMMON_HOME
 *     HOST_PATTERN=COMMON_HOME  # Inline comment
 *
 * EXAMPLE:
 * # To map all hosts starting with "example" to one home directory
 *     example.* = example
 * # To map only hosts example1 and example2 to home directory "special_boxes"
 *     example[1-2]+ = special_boxes
 * # Then map the remaining hosts to the "example" home directory
 *     example.* = example
 *
 * @param hostname
 * @return 0=not found, 1=found
 */
int user_host_group(char **hostname) {
    int found;
    FILE *fp;
    char *recptr;
    char rec[PATH_MAX];
    char home_tmp[PATH_MAX];
    int status;

    fp = fopen(multihome.config_host_group, "r");
    if (!fp) {
        perror(multihome.config_host_group);
        exit(1);
    }

    // Preserve hostname
    strcpy(home_tmp, (*hostname));

    found = 0;
    for (size_t i = 0; (fgets(rec, PATH_MAX - 1, fp) != NULL) && !found; i++) {
        size_t alloc;
        regex_t compiled;
        regmatch_t match[100];
        int num_matches;
        char **data;
        char *comment;

        recptr = rec;

        // Ignore empty lines
        if (strlen(recptr) == 0 || *recptr == '\n') {
            continue;
        }

        // Ignore comments and inline comments
        if (*recptr == '#') {
            continue;
        } else if ((comment = strstr(recptr, "#")) != NULL) {
            comment--;
            for (; comment != NULL && isblank(*comment) && comment > recptr; comment--) {
                *comment = '\0';
            }
        }

        // Report and skip invalid records
        if (strchr(recptr, '=') == NULL) {
            fprintf(stderr, "%s:%zu:syntax error, missing '=' operator\n", multihome.config_host_group, i);
            continue;
        }

        data = split(recptr, "=", &alloc);
        if (data == NULL) {
            continue;
        }

        // Strip blank characters from data
        for (size_t item = 0; alloc > 1 && item < alloc - 1; item++) {
            strip(&data[item]);
        }

        // Initialize regex patttern
        if (regcomp(&compiled, data[0], 0) != 0) {
            // handle compilation failure
            fprintf(stderr, "%s:%zu:unable to compile regex pattern '%s'\n", multihome.config_host_group, i, data[0]);
            free_array((void **) data, alloc);
            continue;
        }

        // Check whether the regex pattern matches
        num_matches = sizeof(match) / sizeof(regmatch_t);
        status = regexec(&compiled, (*hostname), num_matches, match, REG_EXTENDED);
        if (status == REG_NOMATCH) {
            // Ignore unmatched records
        } else if (status > 0) {
            // handle fatal error
            char errbuf[BUFSIZ];
            regerror(status, &compiled, errbuf, BUFSIZ);
            fprintf(stderr, "%s:%zu:regex %s\n", multihome.config_host_group, i, errbuf);
        } else {
            // Replace preserved hostname with the requested name, and stop
            strcpy(home_tmp, data[1]);
            found = 1;
        }

        regfree(&compiled);
        free_array((void **)data, alloc);
    }
    fclose(fp);

    // Replace the hostname
    strcpy((*hostname), home_tmp);
    return found;
}

/**
 * Link or copy files from /home/username to /home/username/home_local/nodename
 */
void user_transfer(int copy_mode) {
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
        char *field_type;
        char *field_where;
        char source[PATH_MAX];
        char dest[PATH_MAX];

        // Set pointer to string
        recptr = rec;

        // Set pointer to TYPE field (beginning of string)
        field_type = recptr;

        // Ignore empty lines
        if (strlen(recptr) == 0 || *recptr == '\n') {
            continue;
        }

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

        // Error checking is done
        // Set pointer to WHERE field
        field_where = &rec[2];

        if (*field_where == '/') {
            fprintf(stderr, "%s:%zu: Removing leading '/' from: %s\n", multihome.config_transfer, lineno, field_where);
            memmove(field_where, field_where + 1, strlen(field_where) + 1);
        }

        if (field_where[strlen(field_where) - 1] == '\n') {
            field_where[strlen(field_where) - 1] = '\0';
        }

        // construct data source path
        sprintf(source, "%s/%s", multihome.path_old, field_where);

        // construct data destination path
        char *tmp;
        tmp = strdup(source);
        sprintf(dest, "%s/%s", multihome.path_new, basename(tmp));
        free(tmp);

        // Perform task based on TYPE field
        switch (*field_type) {
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
                if (copy(source, dest, copy_mode) != 0) {
                    fprintf(stderr, "transfer: %s: %s -> %s\n", strerror(errno), source, dest);
                }
                break;
            default:
                fprintf(stderr, "%s:%zu: Invalid type: '%c'\n", multihome.config_transfer, lineno, *field_type);
                break;
        }
    }
    fclose(fp);
}

/**
 * Retrieve hostname from FQDN
 * @param hostname
 * @return short hostname
 */
char *strip_domainname(char *hostname) {
    char *ptr;

    ptr = strchr(hostname, '.');
    if (ptr != NULL) {
        *ptr = '\0';
    }

    return hostname;
}


// begin argp setup
static char doc[] = "Partition a home directory per-host when using a centrally mounted /home";
static char args_doc[] = "";
static struct argp_option options[] = {
    {"script", 's', 0, 0, "Generate runtime script"},
#ifdef ENABLE_TESTING
    {"tests", 't', 0, 0, "Run unit tests"},
#endif
    {"update", 'u', 0, 0, "Synchronize user skeleton and transfer configuration"},
    {"version", 'V', 0, 0, "Show version and exit"},
    {0},
};

struct arguments {
    int script;
#ifdef ENABLE_TESTING
    int testing;
#endif
    int update;
    int version;
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
#ifdef ENABLE_TESTING
        case 't':
            arguments->testing = 1;
            break;
#endif
        case 'u':
            arguments->update = 1;
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

/**
 * Is rsync available?
 * @return 0=not found, 1 found
 */
int rsync_exists() {
    if (access(MULTIHOME_RSYNC_BIN, F_OK) < 0) {
        return 1;
    }
    return 0;
}

static struct argp argp = { options, parse_opt, args_doc, doc };
// end of argp setup

int main(int argc, char *argv[]) {
    int copy_mode;
    uid_t uid;
    struct passwd *user_info;
    struct utsname host_info;

    // Disable line buffering via macro
    DISABLE_BUFFERING

    struct arguments arguments;
    arguments.script = 0;
#ifdef ENABLE_TESTING
    arguments.testing = 0;
#endif
    arguments.update = 0;
    arguments.version = 0;
    argp_parse(&argp, argc, argv, 0, 0, &arguments);

    if (arguments.version) {
        puts(VERSION);
        exit(0);
    }

    // Refuse to operate if rsync is not available
    if (rsync_exists() != 0) {
        fprintf(stderr, "rsync program not found (expecting: %s)\n", MULTIHOME_RSYNC_BIN);
        return 1;
    }

#ifdef ENABLE_TESTING
    if (arguments.testing) {
        test_main();
        exit(0);
    }
#endif

    // Get effective user account information
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
    if (!arguments.update) {
        path_old = getenv("HOME");
    } else { // Update mode requires the original home path
        path_old = getenv("HOME_OLD");
    }

    // Handle legitimate case where HOME (or HOME_OLD) is undefined. Use the system's records instead...
    // i.e. The user wiped the environment with `env -i` prior to executing multihome
    if (path_old == NULL) {
        path_old = user_info->pw_dir;
        if (path_old == NULL) {
            fprintf(stderr, "Unable to determine home directory path\n");
            return 1;
        }
    }

    // Populate multihome struct
    char *entry_point;
    entry_point = find_program(argv[0]);
    strcpy(multihome.entry_point, entry_point);
    strcpy(multihome.path_old, path_old);
    strcpy(multihome.path_root, MULTIHOME_ROOT);
    strcpy(multihome.scripts_dir, MULTIHOME_SCRIPTS_DIR);
    sprintf(multihome.config_dir, "%s/%s", multihome.path_old, MULTIHOME_CFGDIR);
    sprintf(multihome.config_transfer, "%s/%s", multihome.config_dir, MULTIHOME_CFG_TRANSFER);
    sprintf(multihome.config_skeleton, "%s/%s", multihome.config_dir, MULTIHOME_CFG_SKEL);
    sprintf(multihome.config_host_group, "%s/%s", multihome.config_dir, MULTIHOME_CFG_HOST_GROUP);

    // Generate configuration directory
    if (access(multihome.config_dir, F_OK) < 0) {
        fprintf(stderr, "Creating configuration directory: %s\n", multihome.config_dir);
        if (mkdirs(multihome.config_dir) < 0) {
            perror(multihome.config_dir);
            return errno;
        }
    }

    // Generate a blank host group configuration
    if (access(multihome.config_host_group, F_OK) < 0) {
        fprintf(stderr, "Creating host group configuration: %s\n", multihome.config_host_group);
        if (touch(multihome.config_host_group) < 0) {
            perror(multihome.config_host_group);
            return errno;
        }
    }

    // The short hostname is used to establish the name for the new home directory
    char *nodename;
    nodename = strip_domainname(host_info.nodename);

    // When this host belongs to a host group, modify the hostname once more
    user_host_group(&nodename);

    sprintf(multihome.path_new, "%s/%s/%s", multihome.path_old, multihome.path_root, nodename);
    sprintf(multihome.path_topdir, "%s/%s", multihome.path_new, MULTIHOME_TOPDIR);
    sprintf(multihome.marker, "%s/%s", multihome.path_new, MULTIHOME_MARKER);

    copy_mode = arguments.update; // 0 = normal copy, 1 = update files

    // Refuse to operate within a controlled home directory
    char already_inside[PATH_MAX];
    sprintf(already_inside, "%s/%s", multihome.path_old, MULTIHOME_MARKER);
    if (arguments.update == 0 && access(already_inside, F_OK) == 0) {
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

    // Generate a blank transfer configuration
    if (access(multihome.config_transfer, F_OK) < 0) {
        fprintf(stderr, "Creating transfer configuration: %s\n", multihome.config_transfer);
        if (touch(multihome.config_transfer) < 0) {
            perror(multihome.config_transfer);
            return errno;
        }
    }

    // NOTE: update mode skips the home directory marker check
    if (arguments.update || access(multihome.marker, F_OK) < 0) {
        // Copy system account defaults
        fprintf(stderr, "Pulling account skeleton: %s\n", OS_SKEL_DIR);
        copy(OS_SKEL_DIR, multihome.path_new, copy_mode);

        // Copy user-defined account defaults
        fprintf(stderr, "Pulling user-defined account skeleton: %s\n", multihome.config_skeleton);
        copy(multihome.config_skeleton, multihome.path_new, copy_mode);

        // Transfer or link user-defined files into the new home
        user_transfer(copy_mode);
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
