cmake_minimum_required(VERSION 3.0)
project(multihome C)
include(CheckSymbolExists)
include(CheckCSourceCompiles)

set(CMAKE_C_STANDARD 99)
set(DATA_DIR ${CMAKE_INSTALL_PREFIX}/share/${PROJECT_NAME})
set(MULTIHOME_SCRIPTS_DIR ${CMAKE_INSTALL_PREFIX}/share/${PROJECT_NAME}/init)

include_directories("${CMAKE_CURRENT_BINARY_DIR}")

check_symbol_exists(basename "libgen.h" HAVE_BASENAME)
check_symbol_exists(dirname "libgen.h" HAVE_DIRNAME)
check_symbol_exists(getpwuid "pwd.h" HAVE_GETPWUID)
check_symbol_exists(geteuid "unistd.h" HAVE_GETEUID)
check_symbol_exists(strstr "string.h" HAVE_STRSTR)
check_symbol_exists(sprintf "string.h" HAVE_SPRINTF)
check_symbol_exists(PATH_MAX "limits.h" HAVE_PATH_MAX)

check_c_source_compiles(
    "
    #include <argp.h>
    int main(int argc, char *argv[]) {
        struct argp_option option[1];
    }
    "
    HAVE_ARGP
)

find_program(MULTIHOME_RSYNC_BIN
        NAMES rsync
        REQUIRED)

configure_file("config.h.in" "config.h" @ONLY)

add_executable(multihome
        multihome.c
        tests.c)

install(TARGETS multihome
        RUNTIME DESTINATION bin)

install(DIRECTORY init
        DESTINATION ${DATA_DIR})
