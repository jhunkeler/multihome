cmake_minimum_required(VERSION 3.0)
project(multihome C)
include(CheckSymbolExists)
include(CheckCSourceCompiles)

set(CMAKE_C_STANDARD 99)

include_directories(.)

check_symbol_exists(basename "libgen.h" HAVE_BASENAME)
check_symbol_exists(dirname "libgen.h" HAVE_DIRNAME)
check_symbol_exists(getpwuid "pwd.h" HAVE_GETPWUID)
check_symbol_exists(geteuid "unistd.h" HAVE_GETEUID)

check_c_source_compiles(
    "
    #include <argp.h>
    int main(int argc, char *argv[]) {
        struct argp_option option[10];
    }
    "
    HAVE_ARGP
)
if (NOT HAVE_ARGP)
    message(FATAL_ERROR "Your toolchain does not provide argp")
endif()

find_program(PROGRAM_RSYNC rsync REQUIRED)

add_executable(multihome
        multihome.c
        multihome.h)

install(TARGETS multihome
        RUNTIME DESTINATION bin)